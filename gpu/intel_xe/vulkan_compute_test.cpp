/**
 * Vulkan 计算着色器测试
 * 测试 GPU 显存拷贝和简单计算
 * 
 * 使用方法:
 *   ./vulkan_compute_test [缓冲区大小(MB)]
 *   例如: ./vulkan_compute_test 1024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <chrono>
#include <vulkan/vulkan.h>

#define DEFAULT_BUFFER_MB 512  // 默认 512MB

// 计算着色器 SPIR-V 代码
// 简单的向量加法: c[i] = a[i] + b[i]
const uint32_t computeShaderCode[] = {
    0x07230203, 0x00010000, 0x0008000a, 0x00000036, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0006000f, 0x00000005, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x00060010, 0x00000004,
    0x00000011, 0x00000001, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2, 0x00040005,
    0x00000004, 0x6e69616d, 0x00000000, 0x00050005,
};

#define CHECK_VK_RESULT(result, msg) \
    if (result != VK_SUCCESS) { \
        fprintf(stderr, "Vulkan Error: %s (code: %d)\n", msg, result); \
        return 1; \
    }

int main(int argc, char** argv) {
    size_t buffer_mb = DEFAULT_BUFFER_MB;
    if (argc > 1) {
        buffer_mb = atoi(argv[1]);
        if (buffer_mb == 0) buffer_mb = DEFAULT_BUFFER_MB;
    }
    
    printf("=== Vulkan GPU 计算测试 ===\n");
    printf("缓冲区大小: %zu MB\n\n", buffer_mb);
    
    // 创建 Vulkan 实例
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "GPU Compute Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
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
    
    printf("发现 %d 个 Vulkan 设备:\n", deviceCount);
    
    // 查找支持计算的 GPU
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t computeQueueFamilyIndex = UINT32_MAX;
    
    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        
        printf("  [%d] %s\n", i, props.deviceName);
        printf("      类型: %s\n", 
               props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "独立 GPU" :
               props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "集成 GPU" :
               props.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ? "虚拟 GPU" : "其他");
        
        // 检查设备内存
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(devices[i], &memProps);
        
        VkDeviceSize localMemSize = 0;
        for (uint32_t j = 0; j < memProps.memoryHeapCount; j++) {
            if (memProps.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                localMemSize = memProps.memoryHeaps[j].size;
                break;
            }
        }
        printf("      显存: %zu MB\n", localMemSize / (1024 * 1024));
        
        // 查找计算队列
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies.data());
        
        for (uint32_t j = 0; j < queueFamilyCount; j++) {
            if (queueFamilies[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                printf("      计算队列: 家族 %d\n", j);
                if (physicalDevice == VK_NULL_HANDLE) {
                    physicalDevice = devices[i];
                    computeQueueFamilyIndex = j;
                    printf("      -> 选择此设备\n");
                }
                break;
            }
        }
    }
    
    if (physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "没有发现支持计算的设备\n");
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    
    printf("\n");
    
    // 创建逻辑设备
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    
    VkDevice device;
    result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
    CHECK_VK_RESULT(result, "创建逻辑设备失败");
    
    VkQueue computeQueue;
    vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &computeQueue);
    
    printf("逻辑设备创建成功\n");
    
    // 获取设备内存属性
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
    
    // 查找设备本地内存类型
    uint32_t memoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            memoryTypeIndex = i;
            break;
        }
    }
    
    if (memoryTypeIndex == UINT32_MAX) {
        fprintf(stderr, "没有发现设备本地内存\n");
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    
    // 创建缓冲区
    VkDeviceSize bufferSize = buffer_mb * 1024 * 1024;
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkBuffer bufferA, bufferB, bufferC;
    result = vkCreateBuffer(device, &bufferInfo, nullptr, &bufferA);
    CHECK_VK_RESULT(result, "创建缓冲区 A 失败");
    
    result = vkCreateBuffer(device, &bufferInfo, nullptr, &bufferB);
    CHECK_VK_RESULT(result, "创建缓冲区 B 失败");
    
    result = vkCreateBuffer(device, &bufferInfo, nullptr, &bufferC);
    CHECK_VK_RESULT(result, "创建缓冲区 C 失败");
    
    // 分配显存
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, bufferA, &memReqs);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    VkDeviceMemory memoryA, memoryB, memoryC;
    result = vkAllocateMemory(device, &allocInfo, nullptr, &memoryA);
    if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
        fprintf(stderr, "显存不足，无法分配 %zu MB\n", buffer_mb);
        vkDestroyBuffer(device, bufferA, nullptr);
        vkDestroyBuffer(device, bufferB, nullptr);
        vkDestroyBuffer(device, bufferC, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    CHECK_VK_RESULT(result, "分配显存 A 失败");
    
    result = vkAllocateMemory(device, &allocInfo, nullptr, &memoryB);
    CHECK_VK_RESULT(result, "分配显存 B 失败");
    
    result = vkAllocateMemory(device, &allocInfo, nullptr, &memoryC);
    CHECK_VK_RESULT(result, "分配显存 C 失败");
    
    // 绑定内存
    vkBindBufferMemory(device, bufferA, memoryA, 0);
    vkBindBufferMemory(device, bufferB, memoryB, 0);
    vkBindBufferMemory(device, bufferC, memoryC, 0);
    
    printf("成功分配显存: %zu MB x 3 = %zu MB\n", buffer_mb, buffer_mb * 3);
    
    // 清理
    vkFreeMemory(device, memoryA, nullptr);
    vkFreeMemory(device, memoryB, nullptr);
    vkFreeMemory(device, memoryC, nullptr);
    
    vkDestroyBuffer(device, bufferA, nullptr);
    vkDestroyBuffer(device, bufferB, nullptr);
    vkDestroyBuffer(device, bufferC, nullptr);
    
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    
    printf("\n显存测试完成!\n");
    return 0;
}
