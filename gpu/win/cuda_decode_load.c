/**
 * GPU Video Decode Load Generator - CUDA Edition
 * 
 * 使用 NVDEC (NVIDIA Video Decoder) API 进行硬件视频解码
 * 通过 CUDA Driver API 访问 NVDEC 引擎
 * 
 * 由于完整的 NVDEC 编程比较复杂，这里使用一个简化的方法：
 * 调用 FFmpeg 的 h264_cuvid 解码器（底层使用 NVDEC）
 * 这与之前的 gpu_decode_nvdec.exe 类似，但提供更详细的监控
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_VIDEO_URL "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"
#define TEST_VIDEO_FILE "test_video_cuda.mp4"

// 检查 FFmpeg 是否可用
BOOL CheckFFmpeg() {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(NULL, "ffmpeg -version", NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return FALSE;
    }
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return TRUE;
}

// 检查测试视频
BOOL CheckVideoFile() {
    DWORD attribs = GetFileAttributesA(TEST_VIDEO_FILE);
    return (attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY));
}

// 下载测试视频
BOOL DownloadTestVideo() {
    printf("Downloading test video...\n");
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), 
        "powershell -Command \"& { Invoke-WebRequest -Uri '%s' -OutFile '%s' -UseBasicParsing }\"",
        TEST_VIDEO_URL, TEST_VIDEO_FILE);
    
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return FALSE;
    }
    
    DWORD result = WaitForSingleObject(pi.hProcess, 5 * 60 * 1000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return (result == WAIT_OBJECT_0) && CheckVideoFile();
}

// 监控线程参数
typedef struct {
    int duration;
    volatile int* running;
} MonitorParams;

// 监控线程 - 定期显示 GPU 状态
DWORD WINAPI MonitorThread(LPVOID param) {
    MonitorParams* p = (MonitorParams*)param;
    
    printf("\n=== GPU Monitor (per second) ===\n");
    printf("Time | GPU%% | Decoder%% | Memory Used | Video Clock\n");
    printf("-----|------|----------|-------------|------------\n");
    
    for (int i = 0; i < p->duration && *p->running; i++) {
        Sleep(1000);
        
        // 使用 nvidia-smi 获取状态
        FILE* pipe = _popen("nvidia-smi.exe --query-gpu=utilization.gpu,utilization.decoder,memory.used,clocks.current.video --format=csv,noheader,nounits 2>nul", "r");
        if (pipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                int gpu, decoder, mem, clock;
                if (sscanf(buffer, "%d, %d, %d, %d", &gpu, &decoder, &mem, &clock) == 4) {
                    printf("%3ds | %3d%% | %8d%% | %7d MiB | %5d MHz\n", 
                           i + 1, gpu, decoder, mem, clock);
                }
            }
            _pclose(pipe);
        }
    }
    
    return 0;
}

// 运行 NVDEC 解码
void RunDecodeTest(int duration_sec, int numInstances) {
    printf("\n=== Starting NVDEC Decode Test ===\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Instances: %d\n", numInstances);
    printf("Video: %s\n", TEST_VIDEO_FILE);
    printf("Codec: h264_cuvid (NVDEC hardware decoder)\n\n");
    
    // 启动监控线程
    volatile int running = 1;
    MonitorParams monitorParams = { duration_sec, &running };
    HANDLE monitor = CreateThread(NULL, 0, MonitorThread, &monitorParams, 0, NULL);
    
    // 启动多个 FFmpeg 实例
    HANDLE* processes = (HANDLE*)malloc(numInstances * sizeof(HANDLE));
    
    printf("Starting %d FFmpeg instances...\n", numInstances);
    
    for (int i = 0; i < numInstances; i++) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd),
            "ffmpeg -hide_banner -loglevel error "
            "-c:v h264_cuvid "      // 使用 NVDEC 硬件解码
            "-hwaccel cuda "        // 使用 CUDA 硬件加速
            "-stream_loop -1 "      // 无限循环
            "-i %s -t %d "          // 输入和时间限制
            "-f null -",            // 不输出文件
            TEST_VIDEO_FILE, duration_sec + 5);
        
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        
        if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            processes[i] = pi.hProcess;
            CloseHandle(pi.hThread);
            printf("  [Instance %d] PID: %lu\n", i + 1, pi.dwProcessId);
        } else {
            printf("  [Instance %d] Failed\n", i + 1);
            processes[i] = NULL;
        }
    }
    
    printf("\nDecoding... (wait %d seconds)\n", duration_sec);
    
    // 等待所有进程或超时
    int remaining = numInstances;
    for (int i = 0; i < duration_sec && remaining > 0; i++) {
        Sleep(1000);
        
        // 检查进程状态
        for (int j = 0; j < numInstances; j++) {
            if (processes[j] != NULL) {
                DWORD exitCode;
                if (GetExitCodeProcess(processes[j], &exitCode) && exitCode != STILL_ACTIVE) {
                    CloseHandle(processes[j]);
                    processes[j] = NULL;
                    remaining--;
                }
            }
        }
    }
    
    // 停止监控
    running = 0;
    WaitForSingleObject(monitor, 2000);
    CloseHandle(monitor);
    
    // 清理进程
    printf("\nStopping instances...\n");
    for (int i = 0; i < numInstances; i++) {
        if (processes[i] != NULL) {
            TerminateProcess(processes[i], 0);
            CloseHandle(processes[i]);
        }
    }
    
    free(processes);
    printf("Done!\n");
}

void print_help(const char* prog) {
    printf("Usage: %s <command> [options]\n", prog);
    printf("\nCommands:\n");
    printf("  info              Show GPU decoder info\n");
    printf("  test <sec> [n]    Run decode test (n instances)\n");
    printf("\nExamples:\n");
    printf("  %s info\n", prog);
    printf("  %s test 30 4\n", prog);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    printf("================================================\n");
    printf("GPU Video Decode Load Generator - CUDA/NVDEC\n");
    printf("================================================\n\n");

    const char* cmd = argv[1];
    
    if (strcmp(cmd, "info") == 0) {
        // 显示解码器信息
        printf("Checking NVDEC support...\n\n");
        
        FILE* pipe = _popen("nvidia-smi.exe --query-gpu=name,pci.bus_id --format=csv,noheader 2>nul", "r");
        if (pipe) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                printf("GPU: %s", buffer);
            }
            _pclose(pipe);
        }
        
        printf("\nSupported NVDEC codecs (typical):\n");
        printf("  - H.264 (AVC)\n");
        printf("  - H.265 (HEVC)\n");
        printf("  - VP9\n");
        printf("  - AV1 (RTX 30xx+)\n");
        printf("\nNVDEC is the dedicated video decoding hardware in NVIDIA GPUs.\n");
        printf("It operates independently of CUDA cores.\n");
    }
    else if (strcmp(cmd, "test") == 0) {
        int duration = (argc > 2) ? atoi(argv[2]) : 30;
        int instances = (argc > 3) ? atoi(argv[3]) : 4;
        
        // 检查 FFmpeg
        if (!CheckFFmpeg()) {
            printf("ERROR: FFmpeg not found in PATH\n");
            printf("Please install FFmpeg from: https://ffmpeg.org/download.html\n");
            return 1;
        }
        
        // 检查/下载测试视频
        if (!CheckVideoFile()) {
            printf("Test video not found. Downloading...\n");
            if (!DownloadTestVideo()) {
                printf("Failed to download test video\n");
                printf("Please manually download a H264 video to: %s\n", TEST_VIDEO_FILE);
                return 1;
            }
        }
        
        // 运行测试
        RunDecodeTest(duration, instances);
    }
    else {
        print_help(argv[0]);
        return 1;
    }

    return 0;
}
