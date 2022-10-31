/*
    neat - simple graphics engine
    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

#include <X11/Xlib.h>
#include <GL/glx.h>

#include <NoCopy.hh>
#include <interface.hh>

using namespace neat;

class X11Window : private NoCopy {
    NeatWindowData settings_;
    Display* display_;
    Window window_;
    XVisualInfo* visual_;
    GLXContext context_;
    std::atomic_bool stop_;
    bool pressed_;
    int x_;
    int y_;

  public:
    X11Window() : stop_(false) {
        if (XInitThreads() == 0) {
            throw std::runtime_error("could not init threads");
        }
        display_ = XOpenDisplay(nullptr);
        if (display_ == nullptr) {
            throw std::runtime_error("could not open display");
        }
        auto screenId = DefaultScreen(display_);
        GLint glxAttribs[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 24,
            GLX_STENCIL_SIZE, 8, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8, GLX_SAMPLE_BUFFERS, 0, GLX_SAMPLES, 0, None};
        visual_ = glXChooseVisual(display_, screenId, glxAttribs);
        if (visual_ == nullptr) {
            XCloseDisplay(display_);
            throw std::runtime_error("could not create correct visual window");
        }
        queryData(&settings_);
        XSetWindowAttributes wa;
        wa.border_pixel = BlackPixel(display_, screenId);
        wa.background_pixel = WhitePixel(display_, screenId);
        wa.override_redirect = True;
        auto root = RootWindow(display_, screenId);
        wa.colormap =
            XCreateColormap(display_, root, visual_->visual, AllocNone);
        wa.event_mask = PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
        window_ = XCreateWindow(display_, root, 0, 0, settings_.width,
            settings_.height, 0, visual_->depth, InputOutput, visual_->visual,
            CWBackPixel | CWColormap | CWEventMask, &wa);
        context_ = glXCreateContext(display_, visual_, nullptr, GL_TRUE);
        glXMakeCurrent(display_, window_, context_);
        init(settings_.width, settings_.height);
    }

    ~X11Window() {
        glXDestroyContext(display_, context_);
        XFree(visual_);
        XWindowAttributes wa;
        XGetWindowAttributes(display_, window_, &wa);
        XFreeColormap(display_, wa.colormap);
        XDestroyWindow(display_, window_);
        XCloseDisplay(display_);
    }

    int loop() {
        XClearWindow(display_, window_);
        XMapRaised(display_, window_);
        std::thread eventThread([this]() {
            XEvent ev;
            auto fd = ConnectionNumber(display_);
            fd_set in_fds;
            struct timeval tv;
            while (!stop_) {
                FD_ZERO(&in_fds);
                FD_SET(fd, &in_fds);
                tv.tv_usec = 10000;
                tv.tv_sec = 0;

                select(fd + 1, &in_fds, nullptr, nullptr, &tv);
                while (XPending(display_)) {
                    XNextEvent(display_, &ev);
                    if (ev.type == MotionNotify) {
                        x_ = ev.xmotion.x;
                        y_ = ev.xmotion.y;
                        if (pressed_) {
                            if (action(Actions::Move, x_ / settings_.dpi,
                                    y_ / settings_.dpi) == 1) {
                                stop();
                            }
                        }
                    } else if (ev.type == ButtonPress ||
                               ev.type == ButtonRelease) {
                        pressed_ = (ev.type == ButtonPress);
                        if (action(pressed_ ? Actions::TouchDown
                                            : Actions::TouchUp,
                                x_, y_) == 1) {
                            stop();
                        }
                    }
                }
            }
        });
        while (!stop_) {
            ::draw(std::chrono::system_clock::now().time_since_epoch() /
                   std::chrono::milliseconds(1));

            glXSwapBuffers(display_, window_);
        }
        eventThread.join();
        return 0;
    }

    void stop() {
        stop_ = true;
    }
};

X11Window wnd;

void sigHandler([[maybe_unused]] int sig) {
    wnd.stop();
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    signal(SIGINT, sigHandler);
    return wnd.loop();
}
