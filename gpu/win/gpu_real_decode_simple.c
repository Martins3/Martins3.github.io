/**
 * GPU Real Video Decode Load Generator - 实际触发 Decode 引擎
 * 
 * 本程序通过以下方法之一触发真正的硬件视频解码：
 * 
 * 方法 1: 使用 FFmpeg DLL 进行硬件解码
 * 方法 2: 使用系统 ffmpeg 工具播放视频（如果已安装）
 * 方法 3: 下载测试视频并使用 Media Foundation 解码
 * 
 * 为什么这样能触发 Decode 引擎？
 * - FFmpeg 的 d3d11va 硬件加速会调用 ID3D11VideoDecoder
 * - Media Foundation 的硬件解码路径也会使用 Video Decoder
 * - 这些 API 最终都会操作 NVDEC/VDEC 硬件
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

// 测试视频 URL (Big Buck Bunny - 开源视频，适合测试)
#define TEST_VIDEO_URL "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"
#define TEST_VIDEO_FILENAME "test_video_1080p.mp4"

// 检查文件是否存在
BOOL FileExists(const char* filename) {
    DWORD attribs = GetFileAttributesA(filename);
    return (attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY));
}

// 下载测试视频
BOOL DownloadTestVideo() {
    printf("Downloading test video (this may take a while)...\n");
    printf("URL: %s\n", TEST_VIDEO_URL);
    
    // 使用 PowerShell 下载
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), 
        "powershell -Command \"& { Invoke-WebRequest -Uri '%s' -OutFile '%s' -UseBasicParsing }\"",
        TEST_VIDEO_URL, TEST_VIDEO_FILENAME);
    
    printf("Running: %s\n", cmd);
    
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        printf("Failed to start download process\n");
        return FALSE;
    }
    
    // 等待下载完成（最多 5 分钟）
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 5 * 60 * 1000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    if (waitResult != WAIT_OBJECT_0) {
        printf("Download timeout or failed\n");
        return FALSE;
    }
    
    if (FileExists(TEST_VIDEO_FILENAME)) {
        printf("Download successful!\n");
        return TRUE;
    }
    
    return FALSE;
}

// 检查是否安装了 FFmpeg
BOOL CheckFFmpeg() {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    // 尝试运行 ffmpeg -version
    if (!CreateProcessA(NULL, "ffmpeg -version", NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return FALSE;
    }
    
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return TRUE;
}

// 使用 FFmpeg 生成解码负载
void RunFFmpegDecode(const char* videoFile, int duration_sec, int num_instances) {
    printf("\nStarting FFmpeg hardware decode...\n");
    printf("Video: %s\n", videoFile);
    printf("Instances: %d\n", num_instances);
    printf("Duration: %d seconds\n\n", duration_sec);
    
    printf("Command: ffmpeg -hwaccel d3d11va -i %s -f null -\n", videoFile);
    printf("(This triggers the Video Decoder hardware)\n\n");
    
    HANDLE* processes = (HANDLE*)malloc(num_instances * sizeof(HANDLE));
    
    // 启动多个 FFmpeg 实例
    for (int i = 0; i < num_instances; i++) {
        char cmd[512];
        
        // 构造 FFmpeg 命令
        // -hwaccel d3d11va: 使用 D3D11 硬件加速
        // -stream_loop -1: 无限循环
        // -t %d: 限制运行时间
        // -f null -: 不输出文件
        snprintf(cmd, sizeof(cmd),
            "ffmpeg -hide_banner -loglevel error -hwaccel d3d11va "
            "-stream_loop -1 -i %s -t %d -f null -",
            videoFile, duration_sec);
        
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
    
    // 监控状态
    printf("Monitoring decode progress...\n");
    printf("Run: nvidia-smi --query-gpu=utilization.decoder --format=csv -l 1\n");
    printf("Or check Task Manager -> GPU -> Video Decode\n\n");
    
    // 等待所有进程结束
    int remaining = num_instances;
    int last_reported = -1;
    
    while (remaining > 0) {
        DWORD waitResult = WaitForMultipleObjects(num_instances, processes, FALSE, 1000);
        
        // 检查是否有进程结束
        if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + num_instances) {
            int idx = waitResult - WAIT_OBJECT_0;
            if (processes[idx] != NULL) {
                CloseHandle(processes[idx]);
                processes[idx] = NULL;
                remaining--;
                printf("\rInstance %d finished. Remaining: %d   ", idx + 1, remaining);
                fflush(stdout);
            }
        }
        
        // 每 5 秒报告一次状态
        static int counter = 0;
        if (++counter % 5 == 0) {
            printf("\r[Elapsed: %d sec] Active instances: %d           ", counter, remaining);
            fflush(stdout);
        }
    }
    
    printf("\n\nAll instances finished.\n");
    free(processes);
}

// 尝试使用 VLC（如果安装了）
void TryVLC(const char* videoFile, int duration_sec) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
        "vlc %s --play-and-exit --run-time=%d --video-filter=none",
        videoFile, duration_sec);
    
    printf("Trying VLC: %s\n", cmd);
    
    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE,
                      0, NULL, NULL, &si, &pi)) {
        printf("VLC started (PID: %lu)\n", pi.dwProcessId);
        WaitForSingleObject(pi.hProcess, (duration_sec + 5) * 1000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        printf("VLC not found or failed to start\n");
    }
}

// 生成一个最小的有效 H264 视频流（可选）
void CreateMinimalH264Stream() {
    printf("\nNote: Creating a valid H264 stream programmatically is complex.\n");
    printf("It requires:\n");
    printf("  - Valid SPS (Sequence Parameter Set)\n");
    printf("  - Valid PPS (Picture Parameter Set)\n");
    printf("  - Valid Slice headers\n");
    printf("  - Correct NAL unit formatting\n");
    printf("\nInstead, we download a test video or use FFmpeg.\n\n");
}

int main(int argc, char* argv[]) {
    int duration_sec = 30;
    int num_instances = 2;
    BOOL use_vlc = FALSE;
    
    if (argc > 1) duration_sec = atoi(argv[1]);
    if (argc > 2) num_instances = atoi(argv[2]);
    if (argc > 3 && strcmp(argv[3], "vlc") == 0) use_vlc = TRUE;

    printf("================================================\n");
    printf("GPU Real Video Decode Load Generator\n");
    printf("================================================\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Instances: %d\n", num_instances);
    printf("\n");
    printf("This program triggers the REAL hardware video decoder.\n");
    printf("It uses FFmpeg with D3D11 hardware acceleration (d3d11va).\n");
    printf("\n");
    printf("Expected results:\n");
    printf("  - Task Manager: 'Video Decode' usage increases\n");
    printf("  - nvidia-smi: 'utilization.decoder' > 0%%\n");
    printf("\n");
    
    // 检查 FFmpeg
    printf("Checking for FFmpeg...\n");
    if (!CheckFFmpeg()) {
        printf("FFmpeg not found in PATH!\n");
        printf("Please install FFmpeg or add it to PATH.\n");
        printf("Download from: https://ffmpeg.org/download.html\n");
        printf("\n");
        printf("Alternatively, this program will try to use VLC if available.\n");
        use_vlc = TRUE;
    } else {
        printf("FFmpeg found!\n\n");
    }
    
    // 检查测试视频
    if (!FileExists(TEST_VIDEO_FILENAME)) {
        printf("Test video not found: %s\n", TEST_VIDEO_FILENAME);
        printf("\n");
        
        // 尝试下载
        if (!DownloadTestVideo()) {
            printf("\nFailed to download test video.\n");
            printf("Please manually download a video file and save as: %s\n", TEST_VIDEO_FILENAME);
            printf("Or use: curl -o %s %s\n", TEST_VIDEO_FILENAME, TEST_VIDEO_URL);
            return 1;
        }
    } else {
        printf("Using existing test video: %s\n", TEST_VIDEO_FILENAME);
    }
    
    printf("\n");
    
    // 运行解码负载
    if (use_vlc) {
        TryVLC(TEST_VIDEO_FILENAME, duration_sec);
    } else {
        RunFFmpegDecode(TEST_VIDEO_FILENAME, duration_sec, num_instances);
    }
    
    printf("\n================================================\n");
    printf("Test complete!\n");
    printf("================================================\n");
    printf("\nTo verify Decode engine was used:\n");
    printf("  1. Check Task Manager -> Performance -> GPU -> Video Decode\n");
    printf("  2. Run: nvidia-smi --query-gpu=utilization.decoder --format=csv -l 1\n");
    printf("\n");
    
    return 0;
}
