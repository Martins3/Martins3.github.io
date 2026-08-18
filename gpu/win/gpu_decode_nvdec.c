/**
 * GPU Video Decode Load Generator - NVDEC Edition
 * 真正触发 NVDEC (Video Decode) 引擎
 * 
 * 关键区别：
 * - 使用 FFmpeg 的 h264_cuvid 解码器（NVIDIA NVDEC）
 * - 而不是 d3d11va 或 Video Processor
 * 
 * 监控方法：
 * - nvidia-smi --query-gpu=utilization.decoder
 * - Windows: \GPU Engine(*VideoDecode*)\Utilization Percentage
 * - Task Manager: GPU -> Video Decode
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_VIDEO "test_video_1080p.mp4"

// 检查 FFmpeg 是否可用
BOOL CheckFFmpeg() {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    if (!CreateProcessA(NULL, "ffmpeg -version", NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return FALSE;
    }
    
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return TRUE;
}

// 检查测试视频是否存在
BOOL CheckVideoFile() {
    DWORD attribs = GetFileAttributesA(TEST_VIDEO);
    return (attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY));
}

// 下载测试视频
BOOL DownloadVideo() {
    printf("Downloading test video...\n");
    
    // 使用较小的测试视频
    const char* url = "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerBlazes.mp4";
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "powershell -Command \"& { Invoke-WebRequest -Uri '%s' -OutFile '%s' -UseBasicParsing }\"",
        url, TEST_VIDEO);
    
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return FALSE;
    }
    
    DWORD result = WaitForSingleObject(pi.hProcess, 120000);  // 2分钟超时
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return (result == WAIT_OBJECT_0) && CheckVideoFile();
}

// 运行 NVDEC 解码负载
void RunNVDECLoad(int duration_sec, int num_instances) {
    printf("\nStarting NVDEC (Video Decoder) Load Generator\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Instances: %d\n", num_instances);
    printf("\n");
    printf("Command: ffmpeg -c:v h264_cuvid -i %s -f null -\n", TEST_VIDEO);
    printf("(This uses NVIDIA NVDEC hardware decoder)\n\n");
    
    HANDLE* processes = (HANDLE*)malloc(num_instances * sizeof(HANDLE));
    
    // 启动多个 FFmpeg 实例
    for (int i = 0; i < num_instances; i++) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd),
            "ffmpeg -hide_banner -loglevel error "
            "-c:v h264_cuvid "  // 关键：使用 NVDEC 解码器
            "-stream_loop -1 "   // 无限循环
            "-i %s -t %d -f null -",
            TEST_VIDEO, duration_sec);
        
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        
        if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE,
                          CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            processes[i] = pi.hProcess;
            CloseHandle(pi.hThread);
            printf("  [Instance %d] PID: %lu\n", i + 1, pi.dwProcessId);
        } else {
            printf("  [Instance %d] Failed to start\n", i + 1);
            processes[i] = NULL;
        }
    }
    
    printf("\n");
    printf("Monitoring (check Task Manager or run in another window):\n");
    printf("  nvidia-smi --query-gpu=utilization.decoder --format=csv -l 1\n");
    printf("\n");
    
    // 等待所有进程结束
    int remaining = num_instances;
    int elapsed = 0;
    
    while (remaining > 0 && elapsed < duration_sec + 5) {
        DWORD waitResult = WaitForMultipleObjects(num_instances, processes, FALSE, 1000);
        
        if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + num_instances) {
            int idx = waitResult - WAIT_OBJECT_0;
            if (processes[idx] != NULL) {
                CloseHandle(processes[idx]);
                processes[idx] = NULL;
                remaining--;
            }
        }
        
        elapsed++;
        if (elapsed % 5 == 0) {
            printf("\rRunning... %d/%d sec, %d instances active", 
                   elapsed, duration_sec, remaining);
            fflush(stdout);
        }
    }
    
    // 强制结束所有剩余进程
    for (int i = 0; i < num_instances; i++) {
        if (processes[i] != NULL) {
            TerminateProcess(processes[i], 0);
            CloseHandle(processes[i]);
        }
    }
    
    printf("\n\nAll instances finished.\n");
    free(processes);
}

// 显示监控指南
void ShowMonitoringGuide() {
    printf("\n");
    printf("================================================\n");
    printf("How to verify Decode engine is working:\n");
    printf("================================================\n");
    printf("\n");
    printf("1. Windows Task Manager:\n");
    printf("   - Open Task Manager -> Performance -> GPU\n");
    printf("   - Look for 'Video Decode' graph\n");
    printf("\n");
    printf("2. nvidia-smi (command line):\n");
    printf("   nvidia-smi --query-gpu=utilization.decoder --format=csv -l 1\n");
    printf("\n");
    printf("3. Windows Performance Counters:\n");
    printf("   typeperf \"\\\\GPU Engine(*)\\\\Utilization Percentage\" -sc 10\n");
    printf("   (Look for 'VideoDecode' entries)\n");
    printf("\n");
    printf("Expected result:\n");
    printf("   - Video Decode utilization should be > 0%%\n");
    printf("   - On RTX 4060 with 4 instances: 50-100%%\n");
    printf("\n");
}

int main(int argc, char* argv[]) {
    int duration_sec = 30;
    int num_instances = 4;
    
    if (argc > 1) duration_sec = atoi(argv[1]);
    if (argc > 2) num_instances = atoi(argv[2]);

    printf("================================================\n");
    printf("GPU Video Decode Load Generator (NVDEC Edition)\n");
    printf("================================================\n");
    printf("\n");
    printf("This program triggers the REAL NVDEC hardware decoder.\n");
    printf("Uses FFmpeg with h264_cuvid codec.\n");
    printf("\n");
    
    // 检查 FFmpeg
    printf("Checking for FFmpeg...\n");
    if (!CheckFFmpeg()) {
        printf("ERROR: FFmpeg not found in PATH\n");
        printf("Please install FFmpeg with NVIDIA codec support\n");
        printf("Download: https://www.gyan.dev/ffmpeg/builds/\n");
        return 1;
    }
    printf("FFmpeg found\n\n");
    
    // 检查测试视频
    if (!CheckVideoFile()) {
        printf("Test video not found: %s\n", TEST_VIDEO);
        printf("Attempting to download...\n");
        
        if (!DownloadVideo()) {
            printf("Failed to download test video\n");
            printf("Please manually download a H264 video and save as: %s\n", TEST_VIDEO);
            return 1;
        }
    }
    printf("Using test video: %s\n\n", TEST_VIDEO);
    
    // 显示监控指南
    ShowMonitoringGuide();
    
    printf("Press Enter to start...\n");
    getchar();
    
    // 运行解码负载
    RunNVDECLoad(duration_sec, num_instances);
    
    printf("\n================================================\n");
    printf("Test complete!\n");
    printf("================================================\n");
    
    return 0;
}
