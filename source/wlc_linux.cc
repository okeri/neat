// Copyright 2018 Keri Oleg

#include <signal.h>
#include <linux/input.h>
#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

#include <cstring>
#include <stdexcept>

#include <Actions.hh>
#include <NoCopy.hh>
#include <platform.hh>

using namespace neat;

// declarations of linux interface
void init(int width, int heigth);
int action(Actions act, int x, int y);
void draw(uint64_t);

class WaylandWindow : private NoCopy {
    wl_display* display_;
    wl_compositor* compositor_;
    wl_surface* surface_;
    wl_shell* shell_;
    wl_shell_surface* shell_surface_;
    wl_registry* registry_;
    wl_egl_window* window_;
    wl_callback* callback_;
    wl_cursor_theme* cursor_theme_;
    wl_cursor* default_cursor_;
    wl_surface* cursor_surface_;
    wl_pointer* pointer_;
    wl_seat* seat_;

    EGLSurface egl_surface_;
    EGLDisplay dpy_;
    EGLContext ctx_;
    EGLConfig conf_;
    bool stop_;
    bool pressed_;
    int x_;
    int y_;

  public:
    WaylandWindow() : stop_(false) {
        static const wl_registry_listener registry_listener = {
            [](void* data, wl_registry* registry, uint32_t id,
                const char* interface, uint32_t) {
                WaylandWindow* window = reinterpret_cast<WaylandWindow*>(data);
                if (strcmp(interface, "wl_compositor") == 0) {
                    window->compositor_ =
                        reinterpret_cast<wl_compositor*>(wl_registry_bind(
                            registry, id, &wl_compositor_interface, 1));
                } else if (strcmp(interface, "wl_shell") == 0) {
                    window->shell_ = reinterpret_cast<wl_shell*>(
                        wl_registry_bind(registry, id, &wl_shell_interface, 1));
                } else if (strcmp(interface, "wl_seat") == 0) {
                    static const wl_seat_listener seat_listener = {
                        [](void* data, wl_seat* seat, unsigned caps) {
                            static const wl_pointer_listener pointer_listener =
                                {[](void* data, wl_pointer* pointer,
                                     uint32_t serial, wl_surface*, wl_fixed_t,
                                     wl_fixed_t) {
                                     WaylandWindow* window =
                                         reinterpret_cast<WaylandWindow*>(data);
                                     wl_cursor_image* image =
                                         window->default_cursor_->images[0];
                                     wl_buffer* buffer =
                                         wl_cursor_image_get_buffer(image);
                                     wl_pointer_set_cursor(pointer, serial,
                                         window->cursor_surface_,
                                         image->hotspot_x, image->hotspot_y);
                                     wl_surface_attach(
                                         window->cursor_surface_, buffer, 0, 0);
                                     wl_surface_damage(window->cursor_surface_,
                                         0, 0, image->width, image->height);
                                     wl_surface_commit(window->cursor_surface_);
                                 },
                                    [](void*, wl_pointer*, uint32_t,
                                        wl_surface*) {},
                                    [](void* data, wl_pointer*, uint32_t,
                                        wl_fixed_t sx, wl_fixed_t sy) {
                                        WaylandWindow* window =
                                            reinterpret_cast<WaylandWindow*>(
                                                data);
                                        window->x_ = sx;
                                        window->y_ = sy;
                                        if (window->pressed_) {
                                            if (action(Actions::Move, sx / DPI,
                                                    sy / DPI) == 1) {
                                                window->stop();
                                            }
                                        }
                                    },
                                    [](void* data, wl_pointer*, uint32_t serial,
                                        uint32_t, uint32_t button,
                                        uint32_t state) {
                                        WaylandWindow* window =
                                            reinterpret_cast<WaylandWindow*>(
                                                data);
                                        if (button == BTN_LEFT) {
                                            window->pressed_ =
                                                state ==
                                                WL_POINTER_BUTTON_STATE_PRESSED;
                                            if (window->pressed_) {
                                                wl_shell_surface_move(
                                                    window->shell_surface_,
                                                    window->seat_, serial);
                                                if (action(Actions::Click,
                                                        window->x_ / DPI,
                                                        window->y_ / DPI) ==
                                                    1) {
                                                    window->stop();
                                                }
                                            }
                                        }
                                    },
                                    nullptr, nullptr, nullptr, nullptr,
                                    nullptr};

                            WaylandWindow* window =
                                reinterpret_cast<WaylandWindow*>(data);
                            if ((caps & WL_SEAT_CAPABILITY_POINTER) &&
                                !window->pointer_) {
                                window->pointer_ = wl_seat_get_pointer(seat);
                                wl_pointer_add_listener(window->pointer_,
                                    &pointer_listener, window);
                            } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) &&
                                       window->pointer_) {
                                wl_pointer_destroy(window->pointer_);
                                window->pointer_ = NULL;
                            }
                        },
                        nullptr};

                    window->seat_ = reinterpret_cast<wl_seat*>(
                        wl_registry_bind(registry, id, &wl_seat_interface, 1));
                    wl_seat_add_listener(window->seat_, &seat_listener, window);
                } else if (strcmp(interface, "wl_shm") == 0) {
                    wl_shm* shm = reinterpret_cast<wl_shm*>(
                        wl_registry_bind(registry, id, &wl_shm_interface, 1));
                    window->cursor_theme_ = wl_cursor_theme_load(NULL, 32, shm);
                    window->default_cursor_ = wl_cursor_theme_get_cursor(
                        window->cursor_theme_, "left_ptr");
                }
            },
            nullptr};

        display_ = wl_display_connect(NULL);
        if (display_ == nullptr) {
            throw std::runtime_error("Can't connect to display");
        }
        registry_ = wl_display_get_registry(display_);
        wl_registry_add_listener(registry_, &registry_listener, this);
        wl_display_dispatch(display_);

        static const EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};

        EGLint config_attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1, EGL_BLUE_SIZE, 1,
            EGL_ALPHA_SIZE, 1, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_NONE};

        dpy_ = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(display_));
        eglInitialize(dpy_, NULL, NULL);
        eglBindAPI(EGL_OPENGL_ES_API);

        EGLint n;
        eglChooseConfig(dpy_, config_attribs, &conf_, 1, &n);
        ctx_ = eglCreateContext(dpy_, conf_, EGL_NO_CONTEXT, context_attribs);
        surface_ = wl_compositor_create_surface(compositor_);
        shell_surface_ = wl_shell_get_shell_surface(shell_, surface_);

        static const wl_shell_surface_listener shell_surface_listener = {
            [](void*, wl_shell_surface* shell_surface, uint32_t serial) {
                wl_shell_surface_pong(shell_surface, serial);
            },
            [](void*, wl_shell_surface*, uint32_t, int32_t, int32_t) {},
            [](void*, wl_shell_surface*) {}};

        wl_shell_surface_add_listener(
            shell_surface_, &shell_surface_listener, this);

        window_ = wl_egl_window_create(surface_, WindowWidth, WindowHeight);
        egl_surface_ = eglCreateWindowSurface(
            dpy_, conf_, reinterpret_cast<EGLNativeWindowType>(window_), NULL);

        wl_shell_surface_set_title(shell_surface_, NEAT_APPNAME);

        eglMakeCurrent(dpy_, egl_surface_, egl_surface_, ctx_);
        wl_shell_surface_set_toplevel(shell_surface_);

        callback_ = wl_display_sync(display_);

        static const wl_callback_listener frame_listener_ = {
            [](void* data, wl_callback* callback, uint32_t time) {
                static bool inited = false;
                WaylandWindow* window = reinterpret_cast<WaylandWindow*>(data);
                if (callback) {
                    wl_callback_destroy(callback);
                }

                if (inited) {
                    draw(time);
                } else {
                    inited = true;
                }

                window->callback_ = wl_surface_frame(window->surface_);

                wl_callback_add_listener(
                    window->callback_, &frame_listener_, window);
                eglSwapBuffers(window->dpy_, window->egl_surface_);
            }};

        wl_callback_add_listener(callback_, &frame_listener_, this);
        cursor_surface_ = wl_compositor_create_surface(compositor_);

        init(WindowWidth, WindowHeight);
    }

    ~WaylandWindow() {
        wl_egl_window_destroy(window_);

        wl_shell_surface_destroy(shell_surface_);
        wl_surface_destroy(surface_);

        if (callback_) {
            wl_callback_destroy(callback_);
        }

        eglMakeCurrent(dpy_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(dpy_);
        eglReleaseThread();

        wl_surface_destroy(cursor_surface_);
        if (cursor_theme_) {
            wl_cursor_theme_destroy(cursor_theme_);
        }

        if (shell_) {
            wl_shell_destroy(shell_);
        }

        if (compositor_) {
            wl_compositor_destroy(compositor_);
        }

        wl_registry_destroy(registry_);
        wl_display_flush(display_);
        wl_display_disconnect(display_);
    }

    int loop() {
        int ret;
        while (!stop_ && ret != -1) {
            ret = wl_display_dispatch(display_);
        }
        return 0;
    }

    void stop() {
        stop_ = true;
    }
};

WaylandWindow wnd;

void sigHandler(int) {
    wnd.stop();
}

int main(int, char**) {
    signal(SIGTERM, sigHandler);
    signal(SIGINT, sigHandler);
    signal(SIGKILL, sigHandler);
    return wnd.loop();
}
