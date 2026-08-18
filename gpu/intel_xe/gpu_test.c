/**
 * GPU 显存测试程序
 * 使用 EGL + GLES 在 Intel Xe 显卡上分配显存
 * 
 * 使用方法:
 *   ./gpu_test [显存大小(MB)]
 *   例如: ./gpu_test 4096
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#define DEFAULT_MEMORY_MB 4096  // 默认 4GB 显存
#define TEXTURE_SIZE 4096       // 纹理大小 4096x4096

static void check_gl_error(const char* op) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "GL Error after %s: 0x%x\n", op, error);
    }
}

int main(int argc, char** argv) {
    EGLDisplay display;
    EGLContext context;
    EGLConfig config;
    EGLint num_config;
    int texture_count = 0;
    int target_mb = DEFAULT_MEMORY_MB;
    
    if (argc > 1) {
        target_mb = atoi(argv[1]);
        if (target_mb <= 0) target_mb = DEFAULT_MEMORY_MB;
    }
    
    printf("=== GPU 显存测试程序 ===\n");
    printf("目标显存: %d MB\n\n", target_mb);
    
    // 获取 EGL 显示
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        fprintf(stderr, "获取 EGL 显示失败\n");
        return 1;
    }
    
    // 初始化 EGL
    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor)) {
        fprintf(stderr, "EGL 初始化失败\n");
        eglTerminate(display);
        return 1;
    }
    printf("EGL 初始化成功: %d.%d\n", major, minor);
    printf("EGL 版本: %s\n", eglQueryString(display, EGL_VERSION));
    printf("EGL 供应商: %s\n", eglQueryString(display, EGL_VENDOR));
    printf("EGL 客户端 API: %s\n\n", eglQueryString(display, EGL_CLIENT_APIS));
    
    // 配置 EGL
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    
    if (!eglChooseConfig(display, attribs, &config, 1, &num_config)) {
        fprintf(stderr, "选择 EGL 配置失败\n");
        eglTerminate(display);
        return 1;
    }
    
    // 创建 EGL 上下文
    eglBindAPI(EGL_OPENGL_ES_API);
    
    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
    if (context == EGL_NO_CONTEXT) {
        fprintf(stderr, "创建 EGL 上下文失败\n");
        eglTerminate(display);
        return 1;
    }
    
    // 创建 PBuffer surface
    EGLint pbuffer_attribs[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };
    
    EGLSurface surface = eglCreatePbufferSurface(display, config, pbuffer_attribs);
    if (surface == EGL_NO_SURFACE) {
        fprintf(stderr, "创建 PBuffer surface 失败\n");
        eglDestroyContext(display, context);
        eglTerminate(display);
        return 1;
    }
    
    // 设置当前上下文
    if (!eglMakeCurrent(display, surface, surface, context)) {
        fprintf(stderr, "设置 EGL 上下文失败\n");
        eglDestroySurface(display, surface);
        eglDestroyContext(display, context);
        eglTerminate(display);
        return 1;
    }
    
    printf("GLES 版本: %s\n", glGetString(GL_VERSION));
    printf("GLES 供应商: %s\n", glGetString(GL_VENDOR));
    printf("GLES 渲染器: %s\n\n", glGetString(GL_RENDERER));
    
    // 检查渲染器名称判断使用的 GPU
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    printf("[GPU 信息]\n");
    if (strstr(renderer, "Intel") || strstr(renderer, "intel") || strstr(renderer, "Xe")) {
        printf("检测到 Intel GPU: %s\n\n", renderer);
    } else if (strstr(renderer, "llvmpipe") || strstr(renderer, "LLVM")) {
        printf("警告: 使用软件渲染 (llvmpipe)，不是硬件 GPU\n\n");
    } else {
        printf("渲染器: %s\n\n", renderer);
    }
    
    // 计算需要分配的纹理数量
    // 每个 4096x4096 RGBA 纹理占用 4096 * 4096 * 4 = 64MB
    int texture_size_bytes = TEXTURE_SIZE * TEXTURE_SIZE * 4;
    int texture_size_mb = texture_size_bytes / (1024 * 1024);
    int num_textures = (target_mb + texture_size_mb - 1) / texture_size_mb;
    
    printf("开始分配显存...\n");
    printf("每个纹理: %dx%d RGBA (%d MB)\n", TEXTURE_SIZE, TEXTURE_SIZE, texture_size_mb);
    printf("需要分配: %d 个纹理\n\n", num_textures);
    
    GLuint* textures = malloc(num_textures * sizeof(GLuint));
    if (!textures) {
        fprintf(stderr, "分配纹理数组失败\n");
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(display, surface);
        eglDestroyContext(display, context);
        eglTerminate(display);
        return 1;
    }
    
    glGenTextures(num_textures, textures);
    check_gl_error("glGenTextures");
    
    // 分配显存
    int allocated_mb = 0;
    for (int i = 0; i < num_textures; i++) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        
        // 分配纹理内存
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTURE_SIZE, TEXTURE_SIZE, 
                     0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            fprintf(stderr, "\n分配第 %d 个纹理失败 (GL Error: 0x%x)\n", i + 1, error);
            fprintf(stderr, "已分配: %d MB\n", allocated_mb);
            break;
        }
        
        // 设置纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        allocated_mb += texture_size_mb;
        texture_count++;
        
        if ((i + 1) % 10 == 0 || i == num_textures - 1) {
            printf("已分配: %d / %d 个纹理 (%d MB / %d MB)\n", 
                   i + 1, num_textures, allocated_mb, target_mb);
        }
    }
    
    printf("\n=== 显存分配完成 ===\n");
    printf("成功分配: %d 个纹理\n", texture_count);
    printf("占用显存: %d MB\n", allocated_mb);
    printf("\n按 Enter 键释放显存并退出...\n");
    getchar();
    
    // 清理
    printf("\n正在释放显存...\n");
    glDeleteTextures(texture_count, textures);
    free(textures);
    
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    
    printf("显存已释放，程序退出\n");
    return 0;
}
