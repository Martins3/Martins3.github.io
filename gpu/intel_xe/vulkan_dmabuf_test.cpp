/**
 * Vulkan DMA-BUF 测试
 * 使用 VK_EXT_external_memory_dma_buf 扩展测试 DMA-BUF 导出
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <vector>

#include <vulkan/vulkan.h>

#define CHECK_VK_RESULT(result, msg) \
    if (result != VK_SUCCESS) { \
        fprintf(stderr, "Vulkan Error: %s (code: %d)\n", msg, result); \
        return 1; \
    }

// 检查扩展支持
int check_dma_buf_extension(VkPhysicalDevice physicalDevice) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
    
    printf("可用设备扩展 (%d):\n", extensionCount);
    
    int has_dma_buf = 0;
    int has_external_memory = 0;
    int has_external_fd = 0;
    
    for (const auto& ext : extensions) {
        const char* name = ext.extensionName;
        if (strstr(name, "dma") || strstr(name, "external") || strstr(name, "memory")) {
            printf("  - %s\n", name);
        }
        
        if (strcmp(name, VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME) == 0) {
            has_dma_buf = 1;
        }
        if (strcmp(name, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME) == 0) {
            has_external_memory = 1;
        }
        if (strcmp(name, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME) == 0) {
            has_external_fd = 1;
        }
    }
    
    printf("\n关键扩展:\n");
    printf("  VK_EXT_external_memory_dma_buf: %s\n", has_dma_buf ? "支持" : "不支持");
    printf("  VK_KHR_external_memory: %s\n", has_external_memory ? "支持" : "不支持");
    printf("  VK_KHR_external_memory_fd: %s\n", has_external_fd ? "支持" : "不支持");
    
    return has_dma_buf && has_external_memory && has_external_fd;
}

int main(int argc, char** argv) {
    printf("=== Vulkan DMA-BUF 测试 ===\n\n");
    
    // 创建 Vulkan 实例
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "DMA-BUF Test";
    appInfo.apiVersion = VK_API_VERSION_1_1;
    
    // 启用实例扩展
    const char* instanceExtensions[] = {
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
    };
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = instanceExtensions;
    
    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    CHECK_VK_RESULT(result, "创建 Vulkan 实例失败");
    
    printf("Vulkan 实例创建成功\n");
    
    // 获取物理设备
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        fprintf(stderr, "没有发现 Vulkan 设备\n");
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    
    VkPhysicalDevice physicalDevice = devices[0];
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    printf("使用设备: %s\n\n", props.deviceName);
    
    // 检查 DMA-BUF 扩展
    printf("检查 DMA-BUF 支持...\n");
    int dma_buf_supported = check_dma_buf_extension(physicalDevice);
    
    if (!dma_buf_supported) {
        printf("\n设备不支持 DMA-BUF 扩展\n");
        printf("说明: Intel Xe 驱动可能未启用 DMA-BUF 支持，或 SR-IOV VF 模式限制\n");
        vkDestroyInstance(instance, nullptr);
        return 0;
    }
    
    printf("\n设备支持 DMA-BUF 扩展，继续测试...\n\n");
    
    // 查找计算队列
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
    
    uint32_t queueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueFamilyIndex = i;
            break;
        }
    }
    
    if (queueFamilyIndex == UINT32_MAX) {
        fprintf(stderr, "没有发现计算队列\n");
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    
    // 创建设备 (启用 DMA-BUF 扩展)
    const char* deviceExtensions[] = {
        VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
    };
    
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 3;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    
    VkDevice device;
    result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
    CHECK_VK_RESULT(result, "创建逻辑设备失败");
    
    printf("逻辑设备创建成功 (启用 DMA-BUF 扩展)\n");
    
    // 获取扩展函数指针
    PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR = 
        (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
    
    if (!vkGetMemoryFdKHR) {
        printf("\n错误: 无法获取 vkGetMemoryFdKHR 函数\n");
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    
    printf("vkGetMemoryFdKHR 函数获取成功\n\n");
    
    // 分配带 DMA-BUF 句柄的设备内存
    VkDeviceSize allocSize = 32 * 1024 * 1024;  // 32MB
    
    VkExportMemoryAllocateInfo exportAllocInfo = {};
    exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = allocSize;
    allocInfo.pNext = &exportAllocInfo;
    
    // 查找内存类型
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
    
    uint32_t memoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            memoryTypeIndex = i;
            break;
        }
    }
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    VkDeviceMemory memory;
    result = vkAllocateMemory(device, &allocInfo, nullptr, &memory);
    if (result != VK_SUCCESS) {
        printf("分配设备内存失败 (code: %d)\n", result);
        printf("说明: DMA-BUF 扩展可能需要特定驱动支持\n");
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    
    printf("设备内存分配成功: %zu MB\n", allocSize / (1024 * 1024));
    
    // 导出 DMA-BUF fd
    VkMemoryGetFdInfoKHR getFdInfo = {};
    getFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    getFdInfo.memory = memory;
    getFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    
    int fd = -1;
    result = vkGetMemoryFdKHR(device, &getFdInfo, &fd);
    if (result != VK_SUCCESS) {
        printf("导出 DMA-BUF fd 失败 (code: %d)\n", result);
        printf("说明: SR-IOV VF 模式可能不支持 DMA-BUF 导出\n");
        vkFreeMemory(device, memory, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    
    printf("[OK] DMA-BUF fd 导出成功: %d\n", fd);
    
    // 测试 mmap
    void* ptr = mmap(nullptr, allocSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        printf("[FAIL] mmap DMA-BUF 失败: %s\n", strerror(errno));
    } else {
        printf("[OK] mmap 成功, addr: %p\n", ptr);
        
        // 测试读写
        ((char*)ptr)[0] = 0xAB;
        if (((char*)ptr)[0] == 0xAB) {
            printf("[OK] CPU 读写测试通过\n");
        }
        
        munmap(ptr, allocSize);
    }
    
    // 清理
    close(fd);
    vkFreeMemory(device, memory, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    
    printf("\n=== DMA-BUF 测试完成 ===\n");
    printf("说明: DMA-BUF 导出成功，说明 Intel Xe 驱动支持 DMA-BUF 共享\n");
    printf("用途: 可以与其他设备 (如视频编码器、显示器) 零拷贝共享显存\n");
    
    return 0;
}
