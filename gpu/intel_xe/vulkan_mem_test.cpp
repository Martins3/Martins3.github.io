/**
 * Vulkan 显存测试
 * 测试 GPU 显存分配
 */

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <vulkan/vulkan.h>

#define DEFAULT_BUFFER_MB 512

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

    printf("=== Vulkan GPU 显存测试 ===\n");
    printf("目标显存: %zu MB\n\n", buffer_mb);

    // 创建 Vulkan 实例
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "GPU Memory Test";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    CHECK_VK_RESULT(result, "创建 Vulkan 实例失败");

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

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t computeQueueFamily = UINT32_MAX;

    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);

        const char* typeStr = "其他";
        switch (props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: typeStr = "独立 GPU"; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: typeStr = "集成 GPU"; break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: typeStr = "虚拟 GPU"; break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU: typeStr = "CPU"; break;
            default: typeStr = "其他"; break;
        }

        printf("  [%d] %s (%s)\n", i, props.deviceName, typeStr);

        // 获取显存信息
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(devices[i], &memProps);

        for (uint32_t j = 0; j < memProps.memoryHeapCount; j++) {
            VkMemoryHeap& heap = memProps.memoryHeaps[j];
            printf("      堆 %d: %zu MB%s\n", j,
                   heap.size / (1024 * 1024),
                   (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? " [设备本地]" : "");
        }

        // 查找计算队列
        uint32_t queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties> queues(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueCount, queues.data());

        for (uint32_t j = 0; j < queueCount; j++) {
            if (queues[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                printf("      计算队列: 家族 %d\n", j);
                if (physicalDevice == VK_NULL_HANDLE) {
                    physicalDevice = devices[i];
                    computeQueueFamily = j;
                    printf("      -> 已选择\n");
                }
                break;
            }
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "\n没有发现支持计算的设备\n");
        vkDestroyInstance(instance, nullptr);
        return 1;
    }

    printf("\n");

    // 创建设备
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = computeQueueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;

    VkDevice device;
    result = vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device);
    CHECK_VK_RESULT(result, "创建逻辑设备失败");

    printf("逻辑设备创建成功\n\n");

    // 获取内存属性
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    uint32_t deviceMemoryType = UINT32_MAX;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            deviceMemoryType = i;
            break;
        }
    }

    if (deviceMemoryType == UINT32_MAX) {
        fprintf(stderr, "没有发现设备本地内存\n");
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        return 1;
    }

    // 测试显存分配
    VkDeviceSize bufferSize = buffer_mb * 1024 * 1024;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = bufferSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer;
    result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer);
    CHECK_VK_RESULT(result, "创建缓冲区失败");

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device, buffer, &memReqs);

    printf("缓冲区大小: %zu MB\n", bufferSize / (1024 * 1024));
    printf("内存需求: %zu MB (对齐: %zu)\n",
           memReqs.size / (1024 * 1024), memReqs.alignment);

    // 分配显存
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = deviceMemoryType;

    VkDeviceMemory memory;
    result = vkAllocateMemory(device, &allocInfo, nullptr, &memory);

    if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
        fprintf(stderr, "\n显存不足! 无法分配 %zu MB\n", buffer_mb);
        vkDestroyBuffer(device, buffer, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    CHECK_VK_RESULT(result, "分配显存失败");

    // 绑定内存
    result = vkBindBufferMemory(device, buffer, memory, 0);
    CHECK_VK_RESULT(result, "绑定显存失败");

    printf("\n成功分配显存: %zu MB\n", buffer_mb);

    // 测试多个缓冲区
    printf("\n测试分配多个缓冲区...\n");
    int maxBuffers = 8;
    std::vector<VkBuffer> buffers;
    std::vector<VkDeviceMemory> memories;

    for (int i = 0; i < maxBuffers; i++) {
        VkBuffer buf;
        result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buf);
        if (result != VK_SUCCESS) break;

        VkDeviceMemory mem;
        result = vkAllocateMemory(device, &allocInfo, nullptr, &mem);
        if (result != VK_SUCCESS) {
            vkDestroyBuffer(device, buf, nullptr);
            break;
        }

        result = vkBindBufferMemory(device, buf, mem, 0);
        if (result != VK_SUCCESS) {
            vkFreeMemory(device, mem, nullptr);
            vkDestroyBuffer(device, buf, nullptr);
            break;
        }

        buffers.push_back(buf);
        memories.push_back(mem);

        printf("  缓冲区 %d: 分配成功 (累计: %zu MB)\n",
               i + 1, buffer_mb * (i + 1));
    }

    printf("\n总共分配: %zu MB (%d 个缓冲区)\n",
           buffer_mb * buffers.size(), (int)buffers.size());

    // 清理
    for (size_t i = 0; i < buffers.size(); i++) {
        vkFreeMemory(device, memories[i], nullptr);
        vkDestroyBuffer(device, buffers[i], nullptr);
    }

    vkFreeMemory(device, memory, nullptr);
    vkDestroyBuffer(device, buffer, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    printf("\n显存已释放\n");
    return 0;
}
