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

#include <cstring>
#include <stdexcept>
#include <atomic>
#include <chrono>
#include <csignal>
#include <memory>

#include <linux/input.h>
#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

#include <NoCopy.hh>
#include <interface.hh>

#include <xdg-shell-client-protocol.h>

using namespace neat;

class WaylandWindow : private NoCopy {
    NeatWindowData settings_;
    wl_display* display_;
    wl_compositor* compositor_;
    wl_surface* surface_;
    wl_registry* registry_;
    wl_egl_window* window_;
    wl_cursor_theme* cursor_theme_;
    wl_cursor* default_cursor_;
    wl_surface* cursor_surface_;
    wl_pointer* pointer_{};
    wl_seat* seat_;

    xdg_wm_base* shell_;
    xdg_surface* xdg_surface_;
    xdg_toplevel* toplevel_;

    EGLSurface egl_surface_;
    EGLDisplay dpy_;
    EGLContext ctx_;
    EGLConfig conf_;

    std::atomic_bool stop_;

    bool pressed_;
    int x_;
    int y_;

  public:
    WaylandWindow(int argc, char* argv[]) : stop_(false) {
        static const wl_pointer_listener pointer_listener = {
            [](void* data, wl_pointer* pointer, uint32_t serial, wl_surface*,
                wl_fixed_t, wl_fixed_t) {
                auto* window = reinterpret_cast<WaylandWindow*>(data);
                wl_cursor_image* image = window->default_cursor_->images[0];
                wl_buffer* buffer = wl_cursor_image_get_buffer(image);
                wl_pointer_set_cursor(pointer, serial, window->cursor_surface_,
                    image->hotspot_x, image->hotspot_y);
                wl_surface_attach(window->cursor_surface_, buffer, 0, 0);
                wl_surface_damage(
                    window->cursor_surface_, 0, 0, image->width, image->height);
                wl_surface_commit(window->cursor_surface_);
            },
            [](void*, wl_pointer*, uint32_t, wl_surface*) {},
            [](void* data, wl_pointer*, uint32_t, wl_fixed_t sx,
                wl_fixed_t sy) {
                auto* window = reinterpret_cast<WaylandWindow*>(data);
                window->x_ = sx;
                window->y_ = sy;
                if (window->pressed_) {
                    if (action(Actions::Move, sx / window->settings_.dpi,
                            sy / window->settings_.dpi) == 1) {
                        window->stop();
                    }
                }
            },
            [](void* data, wl_pointer*, [[maybe_unused]] uint32_t serial,
                uint32_t, uint32_t button, uint32_t state) {
                auto* window = reinterpret_cast<WaylandWindow*>(data);
                if (button == BTN_LEFT) {
                    window->pressed_ = state == WL_POINTER_BUTTON_STATE_PRESSED;
                    if (action(window->pressed_ ? Actions::TouchDown
                                                : Actions::TouchUp,
                            window->x_ / window->settings_.dpi,
                            window->y_ / window->settings_.dpi) == 1) {
                        window->stop();
                    }
                }
            },
            [](void*, struct wl_pointer*, uint32_t, uint32_t, wl_fixed_t) {},
            [](void*, struct wl_pointer*) {},
            [](void*, struct wl_pointer*, uint32_t) {},
            [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
            [](void*, struct wl_pointer*, uint32_t, int32_t) {},
            [](void*, struct wl_pointer*, uint32_t, int32_t) {}};

        static const wl_seat_listener seat_listener = {
            [](void* data, wl_seat* seat, unsigned caps) {
                auto* window = reinterpret_cast<WaylandWindow*>(data);
                if ((caps & WL_SEAT_CAPABILITY_POINTER) && !window->pointer_) {
                    window->pointer_ = wl_seat_get_pointer(seat);
                    wl_pointer_add_listener(
                        window->pointer_, &pointer_listener, window);
                }
            },
            [](void*, struct wl_seat*, const char*) {}};

        static const xdg_wm_base_listener xdg_wm_base_listener = {
            [](void*, struct xdg_wm_base* shell, uint32_t serial) {
                xdg_wm_base_pong(shell, serial);
            }};

        static const wl_registry_listener registry_listener = {
            [](void* data, wl_registry* registry, uint32_t id,
                const char* interface, uint32_t) {
                auto* window = reinterpret_cast<WaylandWindow*>(data);
                if (strcmp(interface, wl_compositor_interface.name) == 0) {
                    window->compositor_ =
                        reinterpret_cast<wl_compositor*>(wl_registry_bind(
                            registry, id, &wl_compositor_interface, 4));
                } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
                    window->shell_ =
                        reinterpret_cast<xdg_wm_base*>(wl_registry_bind(
                            registry, id, &xdg_wm_base_interface, 2));
                    xdg_wm_base_add_listener(
                        window->shell_, &xdg_wm_base_listener, data);

                } else if (strcmp(interface, wl_seat_interface.name) == 0) {
                    window->seat_ = reinterpret_cast<wl_seat*>(
                        wl_registry_bind(registry, id, &wl_seat_interface, 7));
                    wl_seat_add_listener(window->seat_, &seat_listener, window);
                } else if (strcmp(interface, wl_shm_interface.name) == 0) {
                    auto* shm = reinterpret_cast<wl_shm*>(
                        wl_registry_bind(registry, id, &wl_shm_interface, 1));
                    window->cursor_theme_ =
                        wl_cursor_theme_load(nullptr, 32, shm);
                    window->default_cursor_ = wl_cursor_theme_get_cursor(
                        window->cursor_theme_, "left_ptr");
                }
            },
            nullptr};

        static const xdg_surface_listener xdg_surface_listener = {
            [](void*, xdg_surface* surface, uint32_t serial) {
                xdg_surface_ack_configure(surface, serial);
            }};

        static const xdg_toplevel_listener xdg_toplevel_listener = {
            [](void*, xdg_toplevel*, int32_t, int32_t, struct wl_array*) {},
            [](void* data, xdg_toplevel*) {
                auto* window = reinterpret_cast<WaylandWindow*>(data);
                window->stop();
            },
            [](void*, struct xdg_toplevel*, int32_t, int32_t) {},
            [](void*, struct xdg_toplevel*, struct wl_array*) {}};

        queryData(&settings_);
        display_ = wl_display_connect(nullptr);
        if (display_ == nullptr) {
            throw std::runtime_error("Can't connect to display");
        }

        registry_ = wl_display_get_registry(display_);
        wl_registry_add_listener(registry_, &registry_listener, this);
        wl_display_dispatch(display_);
        wl_display_roundtrip(display_);

        if (!compositor_ || !shell_) {
            throw std::runtime_error("Can't init wayland window");
        }

        surface_ = wl_compositor_create_surface(compositor_);
        xdg_surface_ = xdg_wm_base_get_xdg_surface(shell_, surface_);
        toplevel_ = xdg_surface_get_toplevel(xdg_surface_);

        xdg_surface_add_listener(xdg_surface_, &xdg_surface_listener, this);
        xdg_toplevel_add_listener(toplevel_, &xdg_toplevel_listener, this);
        xdg_toplevel_set_title(toplevel_, settings_.caption);
        cursor_surface_ = wl_compositor_create_surface(compositor_);
        wl_surface_commit(surface_);

        static const EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};

        EGLint config_attribs[] = {EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8, EGL_BUFFER_SIZE, 8, EGL_DEPTH_SIZE, 16,
            EGL_ALPHA_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_NONE};

        dpy_ = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(display_));
        eglInitialize(dpy_, nullptr, nullptr);
        eglBindAPI(EGL_OPENGL_ES_API);

        EGLint n;
        eglChooseConfig(dpy_, config_attribs, &conf_, 1, &n);
        ctx_ = eglCreateContext(dpy_, conf_, EGL_NO_CONTEXT, context_attribs);

        window_ =
            wl_egl_window_create(surface_, settings_.width, settings_.height);
        egl_surface_ = eglCreateWindowSurface(dpy_, conf_,
            reinterpret_cast<EGLNativeWindowType>(window_), nullptr);

        eglMakeCurrent(dpy_, egl_surface_, egl_surface_, ctx_);

        init(argc, argv);
    }

    ~WaylandWindow() {
        wl_egl_window_destroy(window_);

        xdg_toplevel_destroy(toplevel_);
        xdg_surface_destroy(xdg_surface_);
        wl_surface_destroy(surface_);

        eglMakeCurrent(dpy_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(dpy_);
        eglReleaseThread();

        wl_surface_destroy(cursor_surface_);
        if (cursor_theme_) {
            wl_cursor_theme_destroy(cursor_theme_);
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
            ret = wl_display_dispatch_pending(display_);
            draw(std::chrono::system_clock::now().time_since_epoch() /
                 std::chrono::milliseconds(1));
            eglSwapBuffers(dpy_, egl_surface_);
        }
        return 0;
    }

    void stop() {
        stop_ = true;
    }
};

std::unique_ptr<WaylandWindow> wnd;

void sigHandler([[maybe_unused]] int sig) {
    wnd->stop();
}

int main(int argc, char** argv) {
    signal(SIGINT, sigHandler);
    wnd = std::make_unique<WaylandWindow>(argc, argv);
    return wnd->loop();
}
