#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <wayland-client.h>

#ifdef WITH_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#endif

static struct wl_display *wayland_display = NULL;

static const char *vertex_shader_source =
    "attribute vec4 position;\n"
    "void main() {\n"
    "    gl_Position = position;\n"
    "}\n";

static const char *fragment_shader_source =
    "precision mediump float;\n"
    "void main() {\n"
    "    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color\n"
    "}\n";

static GLuint create_shader(const char *source, GLenum shader_type)
{
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buffer[512];
        glGetShaderInfoLog(shader, 512, NULL, buffer);
        printf("Shader compilation failed: %s\n", buffer);
        return 0;
    }
    return shader;
}

static GLuint create_triangle_program(void)
{
    GLuint vertex_shader = create_shader(vertex_shader_source, GL_VERTEX_SHADER);
    if (!vertex_shader) {
        printf("Failed to create vertex shader\n");
        return 0;
    }

    GLuint fragment_shader = create_shader(fragment_shader_source, GL_FRAGMENT_SHADER);
    if (!fragment_shader) {
        printf("Failed to create fragment shader\n");
        glDeleteShader(vertex_shader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char buffer[512];
        glGetProgramInfoLog(program, 512, NULL, buffer);
        printf("Program linking failed: %s\n", buffer);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return 0;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

static const char *egl_error_string(EGLint error)
{
    switch (error) {
    case EGL_SUCCESS:             return "EGL_SUCCESS";
    case EGL_NOT_INITIALIZED:     return "EGL_NOT_INITIALIZED";
    case EGL_BAD_ACCESS:          return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC:           return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE:       return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONTEXT:         return "EGL_BAD_CONTEXT";
    case EGL_BAD_CONFIG:          return "EGL_BAD_CONFIG";
    case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY:         return "EGL_BAD_DISPLAY";
    case EGL_BAD_SURFACE:         return "EGL_BAD_SURFACE";
    case EGL_BAD_MATCH:           return "EGL_BAD_MATCH";
    case EGL_BAD_PARAMETER:       return "EGL_BAD_PARAMETER";
    case EGL_BAD_NATIVE_PIXMAP:   return "EGL_BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW:   return "EGL_BAD_NATIVE_WINDOW";
    case EGL_CONTEXT_LOST:        return "EGL_CONTEXT_LOST";
    default:                      return "EGL_UNKNOWN_ERROR";
    }
}

static EGLDisplay get_egl_display(void)
{
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
        (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress(
            "eglGetPlatformDisplayEXT");

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display != EGL_NO_DISPLAY)
        return display;

    if (getenv("XDG_RUNTIME_DIR") && getenv("WAYLAND_DISPLAY")) {
        wayland_display = wl_display_connect(NULL);
        if (wayland_display) {
            if (eglGetPlatformDisplayEXT) {
#ifdef EGL_PLATFORM_WAYLAND_EXT
                display = eglGetPlatformDisplayEXT(EGL_PLATFORM_WAYLAND_EXT,
                                                   wayland_display, NULL);
#else
                display = eglGetDisplay((EGLNativeDisplayType)wayland_display);
#endif
            } else {
                display = eglGetDisplay((EGLNativeDisplayType)wayland_display);
            }
            if (display != EGL_NO_DISPLAY) {
                printf("Using Wayland EGL display\n");
                return display;
            }
            wl_display_disconnect(wayland_display);
            wayland_display = NULL;
        }
    }

    if (!eglGetPlatformDisplayEXT)
        return EGL_NO_DISPLAY;

#ifdef EGL_PLATFORM_SURFACELESS_MESA
    display = eglGetPlatformDisplayEXT(EGL_PLATFORM_SURFACELESS_MESA,
                                       EGL_DEFAULT_DISPLAY, NULL);
    if (display != EGL_NO_DISPLAY) {
        printf("Using surfaceless EGL platform\n");
        return display;
    }
#endif

#ifdef EGL_PLATFORM_DEVICE_EXT
    PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT =
        (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");
    if (eglQueryDevicesEXT) {
        EGLDeviceEXT devices[8];
        EGLint num_devices = 0;
        if (eglQueryDevicesEXT(8, devices, &num_devices) && num_devices > 0) {
            display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT,
                                               devices[0], NULL);
            if (display != EGL_NO_DISPLAY) {
                printf("Using EGL device platform\n");
                return display;
            }
        }
    }
#endif

    return EGL_NO_DISPLAY;
}

static int save_ppm(const char *path, int width, int height,
                    const unsigned char *pixels)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        perror("fopen");
        return -1;
    }
    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            const unsigned char *pixel = pixels + (y * width + x) * 4;
            fwrite(pixel, 1, 3, fp);
        }
    }
    fclose(fp);
    return 0;
}

static int verify_triangle(int width, int height)
{
    size_t size = (size_t)width * height * 4;
    unsigned char *pixels = malloc(size);
    if (!pixels) {
        printf("Failed to allocate pixel buffer\n");
        return -1;
    }
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    int red_pixels = 0;
    for (int i = 0; i < width * height; ++i) {
        unsigned char *pixel = pixels + i * 4;
        if (pixel[0] > 200 && pixel[1] < 32 && pixel[2] < 32)
            red_pixels++;
    }
    if (save_ppm("triangle.ppm", width, height, pixels) == 0)
        printf("Saved framebuffer snapshot to triangle.ppm\n");
    free(pixels);
    printf("Detected %d red pixels\n", red_pixels);
    return red_pixels > 1000 ? 0 : -1;
}

static EGLContext setup_pbuffer_context(EGLDisplay *display, EGLSurface *surface)
{
    *display = get_egl_display();
    if (*display == EGL_NO_DISPLAY) {
        printf("Failed to get EGL display: %s\n",
               egl_error_string(eglGetError()));
        return EGL_NO_CONTEXT;
    }

    EGLint major, minor;
    if (!eglInitialize(*display, &major, &minor)) {
        printf("Failed to initialize EGL: %s\n",
               egl_error_string(eglGetError()));
        return EGL_NO_CONTEXT;
    }
    printf("EGL initialized: version %d.%d\n", major, minor);

    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        printf("Failed to bind OpenGL ES API: %s\n",
               egl_error_string(eglGetError()));
        return EGL_NO_CONTEXT;
    }

    EGLint num_configs;
    EGLConfig config;
    if (!eglChooseConfig(*display, config_attribs, &config, 1, &num_configs) ||
        num_configs == 0) {
        printf("No suitable EGL config found\n");
        return EGL_NO_CONTEXT;
    }

    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext context = eglCreateContext(*display, config, EGL_NO_CONTEXT,
                                          context_attribs);
    if (context == EGL_NO_CONTEXT) {
        printf("Failed to create EGL context: %s\n",
               egl_error_string(eglGetError()));
        return EGL_NO_CONTEXT;
    }

    EGLint pbuffer_attribs[] = {
        EGL_WIDTH, 512,
        EGL_HEIGHT, 512,
        EGL_NONE
    };
    *surface = eglCreatePbufferSurface(*display, config, pbuffer_attribs);
    if (*surface == EGL_NO_SURFACE) {
        printf("Failed to create pbuffer surface: %s\n",
               egl_error_string(eglGetError()));
        eglDestroyContext(*display, context);
        return EGL_NO_CONTEXT;
    }

    if (!eglMakeCurrent(*display, *surface, *surface, context)) {
        printf("Failed to make EGL context current: %s\n",
               egl_error_string(eglGetError()));
        eglDestroySurface(*display, *surface);
        eglDestroyContext(*display, context);
        return EGL_NO_CONTEXT;
    }
    return context;
}

#ifdef WITH_X11
static int setup_x11_window(int width, int height,
                            EGLDisplay *display, EGLSurface *surface,
                            EGLContext *context,
                            Display **out_xdpy, Window *out_win)
{
    Display *xdpy = XOpenDisplay(NULL);
    if (!xdpy) {
        printf("XOpenDisplay failed\n");
        return -1;
    }

    EGLDisplay edpy = eglGetDisplay((EGLNativeDisplayType)xdpy);
    if (edpy == EGL_NO_DISPLAY) {
        printf("eglGetDisplay(X11) failed: %s\n", egl_error_string(eglGetError()));
        XCloseDisplay(xdpy);
        return -1;
    }

    EGLint major, minor;
    if (!eglInitialize(edpy, &major, &minor)) {
        printf("eglInitialize failed: %s\n", egl_error_string(eglGetError()));
        XCloseDisplay(xdpy);
        return -1;
    }
    printf("EGL initialized: version %d.%d (X11)\n", major, minor);

    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        printf("eglBindAPI failed: %s\n", egl_error_string(eglGetError()));
        eglTerminate(edpy);
        XCloseDisplay(xdpy);
        return -1;
    }

    EGLint num_configs;
    EGLConfig config;
    if (!eglChooseConfig(edpy, config_attribs, &config, 1, &num_configs) ||
        num_configs == 0) {
        printf("eglChooseConfig failed: %s\n", egl_error_string(eglGetError()));
        eglTerminate(edpy);
        XCloseDisplay(xdpy);
        return -1;
    }

    EGLint vid;
    if (!eglGetConfigAttrib(edpy, config, EGL_NATIVE_VISUAL_ID, &vid)) {
        printf("eglGetConfigAttrib(NATIVE_VISUAL_ID) failed\n");
        eglTerminate(edpy);
        XCloseDisplay(xdpy);
        return -1;
    }

    XVisualInfo xvi_template;
    xvi_template.visualid = (VisualID)vid;
    int num_visuals = 0;
    XVisualInfo *xvi = XGetVisualInfo(xdpy, VisualIDMask, &xvi_template,
                                      &num_visuals);
    if (!xvi || num_visuals == 0) {
        printf("XGetVisualInfo failed for visualid %lu\n", (unsigned long)vid);
        if (xvi) XFree(xvi);
        eglTerminate(edpy);
        XCloseDisplay(xdpy);
        return -1;
    }

    Window root = RootWindow(xdpy, DefaultScreen(xdpy));
    XSetWindowAttributes swa;
    swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;
    Window win = XCreateWindow(xdpy, root, 0, 0, width, height, 0,
                               xvi[0].depth, InputOutput, xvi[0].visual,
                               CWEventMask, &swa);
    XFree(xvi);
    if (!win) {
        printf("XCreateWindow failed\n");
        eglTerminate(edpy);
        XCloseDisplay(xdpy);
        return -1;
    }

    XMapWindow(xdpy, win);
    XStoreName(xdpy, win, "Red Triangle");

    Atom wm_delete = XInternAtom(xdpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(xdpy, win, &wm_delete, 1);

    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext ctx = eglCreateContext(edpy, config, EGL_NO_CONTEXT,
                                      context_attribs);
    if (ctx == EGL_NO_CONTEXT) {
        printf("eglCreateContext failed: %s\n", egl_error_string(eglGetError()));
        XDestroyWindow(xdpy, win);
        eglTerminate(edpy);
        XCloseDisplay(xdpy);
        return -1;
    }

    EGLSurface surf = eglCreateWindowSurface(edpy, config, win, NULL);
    if (surf == EGL_NO_SURFACE) {
        printf("eglCreateWindowSurface failed: %s\n",
               egl_error_string(eglGetError()));
        eglDestroyContext(edpy, ctx);
        XDestroyWindow(xdpy, win);
        eglTerminate(edpy);
        XCloseDisplay(xdpy);
        return -1;
    }

    if (!eglMakeCurrent(edpy, surf, surf, ctx)) {
        printf("eglMakeCurrent failed: %s\n", egl_error_string(eglGetError()));
        eglDestroySurface(edpy, surf);
        eglDestroyContext(edpy, ctx);
        XDestroyWindow(xdpy, win);
        eglTerminate(edpy);
        XCloseDisplay(xdpy);
        return -1;
    }

    *display = edpy;
    *surface = surf;
    *context = ctx;
    *out_xdpy = xdpy;
    *out_win = win;
    return 0;
}
#endif

int main(void)
{
    setbuf(stdout, NULL);

    printf("Rendering a red triangle using OpenGL ES and EGL\n");
    printf("================================================\n");

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;

    int use_x11 = 0;

#ifdef WITH_X11
    Display *xdpy = NULL;
    Window win = 0;
    if (getenv("DISPLAY")) {
        if (setup_x11_window(512, 512, &display, &surface, &context,
                             &xdpy, &win) == 0) {
            use_x11 = 1;
            printf("Using X11 window\n");
        } else {
            printf("X11 setup failed, trying headless fallback...\n");
        }
    }
#endif

    if (!use_x11) {
        context = setup_pbuffer_context(&display, &surface);
        if (context == EGL_NO_CONTEXT) {
            printf("Failed to setup EGL context\n");
            return EXIT_FAILURE;
        }
        printf("Using headless pbuffer (no display server)\n");
    }

    printf("EGL context created successfully\n");

    GLuint program = create_triangle_program();
    if (!program) {
        printf("Failed to create triangle program\n");
        goto cleanup;
    }
    printf("Triangle program created successfully\n");

    glUseProgram(program);

    GLfloat vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLint position_attr = glGetAttribLocation(program, "position");
    if (position_attr >= 0) {
        glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_attr);
    }

    glViewport(0, 0, 512, 512);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    if (use_x11) {
        int running = 1;
        int frames = 0;
        Atom wm_delete = XInternAtom(xdpy, "WM_DELETE_WINDOW", False);
        while (running && frames < 3000) { /* ~50 s max, 60 fps */
            while (XPending(xdpy)) {
                XEvent xev;
                XNextEvent(xdpy, &xev);
                if (xev.type == ClientMessage &&
                    (Atom)xev.xclient.data.l[0] == wm_delete) {
                    running = 0;
                } else if (xev.type == KeyPress) {
                    running = 0;
                }
            }
            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            eglSwapBuffers(display, surface);
            usleep(16000);
            frames++;
        }
        printf("Red triangle rendered to X11 window\n");
    } else {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        eglSwapBuffers(display, surface);
        glFinish();
        if (verify_triangle(512, 512) != 0) {
            printf("Rendered image verification failed\n");
        } else {
            printf("Red triangle rendered successfully (headless)\n");
        }
    }

cleanup:
    glDeleteBuffers(1, &vbo);
    if (program)
        glDeleteProgram(program);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);

#ifdef WITH_X11
    if (use_x11 && xdpy) {
        XDestroyWindow(xdpy, win);
        XCloseDisplay(xdpy);
    }
#endif
    if (wayland_display) {
        wl_display_disconnect(wayland_display);
    }

    return EXIT_SUCCESS;
}
