// Copyright 2018 Keri Oleg

#include <signal.h>
#include <linux/input.h>
#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

#include <cstring>
#include <stdexcept>
#include <atomic>

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
    wl_pointer* pointer_;
    wl_seat* seat_;

    xdg_wm_base* shell_;
    xdg_surface* xdg_surface_;
    xdg_toplevel* toplevel_;

    EGLSurface egl_surface_;
    EGLDisplay dpy_;
    EGLContext ctx_;
    EGLConfig conf_;

    std::atomic_bool stop_;
    bool configured_;

    bool pressed_;
    int x_;
    int y_;

  public:
    WaylandWindow() : stop_(false) {
        static const wl_pointer_listener pointer_listener = {
            [](void* data, wl_pointer* pointer, uint32_t serial, wl_surface*,
                wl_fixed_t, wl_fixed_t) {
                WaylandWindow* window = reinterpret_cast<WaylandWindow*>(data);
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
                WaylandWindow* window = reinterpret_cast<WaylandWindow*>(data);
                window->x_ = sx;
                window->y_ = sy;
                if (window->pressed_) {
                    if (action(Actions::Move, sx / window->settings_.dpi,
                            sy / window->settings_.dpi) == 1) {
                        window->stop();
                    }
                }
            },
            [](void* data, wl_pointer*, uint32_t serial, uint32_t,
                uint32_t button, uint32_t state) {
                WaylandWindow* window = reinterpret_cast<WaylandWindow*>(data);
                if (button == BTN_LEFT) {
                    window->pressed_ = state == WL_POINTER_BUTTON_STATE_PRESSED;
                    if (window->pressed_) {
                        xdg_toplevel_move(
                            window->toplevel_, window->seat_, serial);
                        if (action(Actions::Click,
                                window->x_ / window->settings_.dpi,
                                window->y_ / window->settings_.dpi) == 1) {
                            window->stop();
                        }
                    }
                }
            },
            [](void*, struct wl_pointer*, uint32_t, uint32_t, wl_fixed_t) {},
            nullptr, nullptr, nullptr, nullptr};

        static const wl_seat_listener seat_listener = {
            [](void* data, wl_seat* seat, unsigned caps) {
                WaylandWindow* window = reinterpret_cast<WaylandWindow*>(data);
                if ((caps & WL_SEAT_CAPABILITY_POINTER) && !window->pointer_) {
                    window->pointer_ = wl_seat_get_pointer(seat);
                    wl_pointer_add_listener(
                        window->pointer_, &pointer_listener, window);
                } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) &&
                           window->pointer_) {
                    wl_pointer_destroy(window->pointer_);
                    window->pointer_ = NULL;
                }
            },
            nullptr};

        static const xdg_wm_base_listener xdg_wm_base_listener = {
            [](void*, struct xdg_wm_base* shell, uint32_t serial) {
                xdg_wm_base_pong(shell, serial);
            }};

        static const wl_registry_listener registry_listener = {
            [](void* data, wl_registry* registry, uint32_t id,
                const char* interface, uint32_t) {
                WaylandWindow* window = reinterpret_cast<WaylandWindow*>(data);
                if (strcmp(interface, wl_compositor_interface.name) == 0) {
                    window->compositor_ =
                        reinterpret_cast<wl_compositor*>(wl_registry_bind(
                            registry, id, &wl_compositor_interface, 1));
                } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
                    window->shell_ =
                        reinterpret_cast<xdg_wm_base*>(wl_registry_bind(
                            registry, id, &xdg_wm_base_interface, 1));
                    xdg_wm_base_add_listener(
                        window->shell_, &xdg_wm_base_listener, data);

                } else if (strcmp(interface, wl_seat_interface.name) == 0) {
                    window->seat_ = reinterpret_cast<wl_seat*>(
                        wl_registry_bind(registry, id, &wl_seat_interface, 1));
                    wl_seat_add_listener(window->seat_, &seat_listener, window);
                } else if (strcmp(interface, wl_shm_interface.name) == 0) {
                    wl_shm* shm = reinterpret_cast<wl_shm*>(
                        wl_registry_bind(registry, id, &wl_shm_interface, 1));
                    window->cursor_theme_ = wl_cursor_theme_load(NULL, 32, shm);
                    window->default_cursor_ = wl_cursor_theme_get_cursor(
                        window->cursor_theme_, "left_ptr");
                }
            },
            nullptr};

        static const xdg_surface_listener xdg_surface_listener = {
            [](void* data, xdg_surface* surface, uint32_t serial) {
                WaylandWindow* window = reinterpret_cast<WaylandWindow*>(data);
                xdg_surface_ack_configure(surface, serial);
                window->configured_ = true;
            }};

        static const xdg_toplevel_listener xdg_toplevel_listener = {
            [](void*, xdg_toplevel*, int32_t, int32_t, struct wl_array*) {},
            [](void*, xdg_toplevel*) {}};

        queryData(&settings_);
        display_ = wl_display_connect(NULL);
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

        static const EGLint context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};

        EGLint config_attribs[] = {EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8, EGL_BUFFER_SIZE, 8, EGL_DEPTH_SIZE, 16,
            EGL_ALPHA_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_NONE};

        dpy_ = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(display_));
        eglInitialize(dpy_, NULL, NULL);
        eglBindAPI(EGL_OPENGL_ES_API);

        EGLint n;
        eglChooseConfig(dpy_, config_attribs, &conf_, 1, &n);
        ctx_ = eglCreateContext(dpy_, conf_, EGL_NO_CONTEXT, context_attribs);

        surface_ = wl_compositor_create_surface(compositor_);
        xdg_surface_ = xdg_wm_base_get_xdg_surface(shell_, surface_);
        toplevel_ = xdg_surface_get_toplevel(xdg_surface_);

        xdg_surface_add_listener(xdg_surface_, &xdg_surface_listener, this);
        xdg_toplevel_add_listener(toplevel_, &xdg_toplevel_listener, nullptr);
        xdg_toplevel_set_title(toplevel_, settings_.caption);
        cursor_surface_ = wl_compositor_create_surface(compositor_);
        configured_ = false;
        wl_surface_commit(surface_);

        window_ =
            wl_egl_window_create(surface_, settings_.width, settings_.height);
        egl_surface_ = eglCreateWindowSurface(dpy_, conf_,
            reinterpret_cast<EGLNativeWindowType>(window_), nullptr);

        wl_display_roundtrip(display_);
        wl_surface_commit(surface_);

        eglMakeCurrent(dpy_, egl_surface_, egl_surface_, ctx_);

        init(settings_.width, settings_.height);
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

    void draw() {
        static uint64_t time = 0;
        ::draw(time++);
        wl_surface_frame(surface_);
        eglSwapBuffers(dpy_, egl_surface_);
    }

    int loop() {
        int ret;
        while (!stop_ && ret != -1) {
            if (configured_) {
                ret = wl_display_dispatch_pending(display_);
                draw();
            } else {
                ret = wl_display_dispatch(display_);
            }
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
