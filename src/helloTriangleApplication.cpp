#include "helloTriangleApplication.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <unordered_set>

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "vertex.hpp"

PFN_vkCreateDebugUtilsMessengerEXT  pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT( VkInstance                                 instance,
                                                               const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo,
                                                               const VkAllocationCallbacks *              pAllocator,
                                                               VkDebugUtilsMessengerEXT *                 pMessenger )
{
  return pfnVkCreateDebugUtilsMessengerEXT( instance, pCreateInfo, pAllocator, pMessenger );
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks const * pAllocator )
{
  return pfnVkDestroyDebugUtilsMessengerEXT( instance, messenger, pAllocator );
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    // first we get the app
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->notifyFramebufferResized();
}

static std::vector<char> readShaderSPV(const std::string& filename) {
    // opening the file at the end to get the number of bytes to read
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open the file: " + filename);
    }

    std::vector<char> buffer(file.tellg());
    // go back to the beginning
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), buffer.size());
    file.close();
    return buffer;
}




void HelloTriangleApplication::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}


VKAPI_ATTR vk::Bool32 VKAPI_CALL HelloTriangleApplication::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    auto messageSeverityParsed = static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity);
    auto messageTypeFlagParsed = static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageType);

    std::cerr << "[" << vk::to_string(messageSeverityParsed) << ":" << vk::to_string(messageTypeFlagParsed) << "]: " << pCallbackData->pMessage << "\n";

    return vk::False;
}


void HelloTriangleApplication::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

}


void HelloTriangleApplication::initVulkan() {
    createVulkanInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createCommandBuffers();
    createSyncObjects();
}

uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {

    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

#ifndef NDEBUG
    static bool bMemoryTypeFirstTimeSearching = true;
    if (bMemoryTypeFirstTimeSearching) {
        for (size_t index = 0; index < memProperties.memoryHeapCount; ++index) {
            auto heapType = memProperties.memoryHeaps[index];
            std::cout << "HelloTriangleApplication::findMemoryType - HeapType " << index << " - size: "  << heapType.size;
            if (heapType.flags) {
                std::cout << " - flags: " << vk::to_string(heapType.flags);
            }
            std::cout << "\n";
    
        }
        for (size_t index = 0; index < memProperties.memoryTypeCount; ++index) {
            auto memoryType = memProperties.memoryTypes[index];
            std::cout << "HelloTriangleApplication::findMemoryType - MemoryType " << index << " - heap: " << memoryType.heapIndex;
            if (memoryType.propertyFlags) {
                std::cout << " - type: " << vk::to_string(memoryType.propertyFlags);
            }
            std::cout << "\n";
    
        }
        bMemoryTypeFirstTimeSearching = false;
    }
#endif

    for (const auto& [index, memoryType] : std::views::enumerate(memProperties.memoryTypes)) {
        if ((typeFilter & (1 << index)) && (memoryType.propertyFlags & properties) == properties) {
#ifndef NDEBUG
            std::cout << "HelloTriangleApplication::findMemoryType - Using memory " << index << "\n";
#endif
            return index;
        }
    }
    return 0u;
}


bool HelloTriangleApplication::checkValidationLayerSupport() const {
    std::vector<vk::LayerProperties> layersSupported = vk::enumerateInstanceLayerProperties();

    for (const char* validationLayer : validationLayers) {
        bool found = false;

        for (const vk::LayerProperties& layerSupported : layersSupported) {
            if (std::strcmp(layerSupported.layerName, validationLayer) == 0) {
                found = true;
                break;
            }
        }

        if (!found) return false;
    }

    return true;
}

bool HelloTriangleApplication::checkDeviceExtensionSupport(const vk::PhysicalDevice& device) const {
    std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();
    std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const vk::ExtensionProperties& extension : availableExtensions) {
        std::string extensionName = extension.extensionName;
        requiredExtensions.erase(extensionName);
    }

    return requiredExtensions.empty();
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions() const {

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

vk::DebugUtilsMessengerCreateInfoEXT HelloTriangleApplication::populateDebugMessengerCreateInfo() const {
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );
    return vk::DebugUtilsMessengerCreateInfoEXT(
        {},
        severityFlags,
        messageTypeFlags,
        vk::PFN_DebugUtilsMessengerCallbackEXT(HelloTriangleApplication::debugCallback)
    );
}

void HelloTriangleApplication::createVulkanInstance() {

    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    const vk::ApplicationInfo appInfo(
        "Hello Triangle",
        vk::makeApiVersion(0, 0, 1, 0),
        nullptr,
        0u,
        vk::ApiVersion12
    );

    
#ifndef NDEBUG
    // SHOWS THE LAYERS AVAILABLE
    std::vector<vk::LayerProperties> instanceLayerProperties = vk::enumerateInstanceLayerProperties();
    for (auto& layerProperty : instanceLayerProperties) {
        std::cout << "HelloTriangleApplication::createVulkanInstance - Found layer: " << layerProperty.layerName << "\n";
    }
#endif
    
    std::vector<const char*> enabledExtensions = getRequiredExtensions();

    std::vector<const char*> enabledLayers;
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = populateDebugMessengerCreateInfo();
    void* pNext = nullptr;
    if (enableValidationLayers) {
        enabledLayers = validationLayers;
        pNext = &debugCreateInfo;
    }

    const vk::InstanceCreateInfo instanceInfo(
        vk::InstanceCreateFlags(),
        &appInfo,
        enabledLayers,
        enabledExtensions,
        pNext
    );

    instance = vk::createInstance(instanceInfo);
}


void HelloTriangleApplication::loadDebugMessengerFunctions() {
    pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>( instance.getProcAddr( "vkCreateDebugUtilsMessengerEXT" ) );
    if ( !pfnVkCreateDebugUtilsMessengerEXT )
    {
        throw std::runtime_error("loadDebugMessengerFunctions: Unable to find pfnVkCreateDebugUtilsMessengerEXT function.");
    }

    pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>( instance.getProcAddr( "vkDestroyDebugUtilsMessengerEXT" ) );
    if ( !pfnVkDestroyDebugUtilsMessengerEXT )
    {
        throw std::runtime_error("loadDebugMessengerFunctions: Unable to find pfnVkDestroyDebugUtilsMessengerEXT function.");
    }
}


void HelloTriangleApplication::setupDebugMessenger() {
    loadDebugMessengerFunctions();
    vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo = populateDebugMessengerCreateInfo();
    debugUtilsMessenger = instance.createDebugUtilsMessengerEXT(messengerCreateInfo);
}

void HelloTriangleApplication::createSurface() {
    VkSurfaceKHR tmpSurface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &tmpSurface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::SurfaceKHR(tmpSurface);
}

void HelloTriangleApplication::pickPhysicalDevice() {
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    if (devices.size() == 0) {
        throw std::runtime_error("HelloTriangleApplication::pickPhysicalDevice - Failed to find GPUs with Vulkan support!");
    }

    for (const vk::PhysicalDevice& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            std::cout << "HelloTriangleApplication::pickPhysicalDevice - Found a great candidate for GPU: " << device.getProperties().deviceName << "\n";
        }
    }

    if (!physicalDevice) {
        throw std::runtime_error("HelloTriangleApplication::pickPhysicalDevice - Failed to find a suitable GPU!");
    }
}

SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(const vk::PhysicalDevice &device) const {

    return SwapChainSupportDetails {
        device.getSurfaceCapabilitiesKHR(surface),
        device.getSurfaceFormatsKHR(surface),
        device.getSurfacePresentModesKHR(surface)
    };
}

vk::SurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) const {
    for(const auto& format : availableFormats) {
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }
    return availableFormats[0];
}

vk::PresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) const {
    for (const auto& presentMode : availablePresentModes) {
        // We'll prefer mailbox present mode if exists
        if (presentMode == vk::PresentModeKHR::eMailbox) return presentMode;
    }
    // this one should be present for every vulkan device
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D HelloTriangleApplication::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    vk::Extent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return actualExtent;
}

QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(const vk::PhysicalDevice &physicalDevice) const
{
    QueueFamilyIndices indices;
    // Assign index to queue families that could be found
    std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

#ifndef NDEBUG
    static bool bFirstTimeSearching = true;
    if (bFirstTimeSearching)
    {
        for(const auto& [index, queueFamily] : std::views::enumerate(queueFamilies)) {
            std::cout << "HelloTriangleApplication::findQueueFamilies - Found Family " << index << ": "  << vk::to_string(queueFamily.queueFlags) << "\n";
        }
        bFirstTimeSearching = false;
    }
#endif

    for (size_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = static_cast<uint32_t>(i);
        }
        if (physicalDevice.getSurfaceSupportKHR(i, surface)) {
            indices.presentFamily = static_cast<uint32_t>(i);
        }
        if (indices.isComplete()) break;
    }

    return indices;
}


bool HelloTriangleApplication::isDeviceSuitable(const vk::PhysicalDevice &device) const {
    bool queueFamilySupported = findQueueFamilies(device).isComplete();

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainSupported = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainSupported = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return queueFamilySupported && extensionsSupported && swapChainSupported;
}


void HelloTriangleApplication::createLogicalDevice() {

    // Queue Creation
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::unordered_set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    queueCreateInfos.reserve(uniqueQueueFamilies.size());
    float queuePriority = 1.0f;
    for (auto queueFamily : uniqueQueueFamilies) {
        queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo({}, queueFamily, 1, &queuePriority));
    }

    // Validation Layers
    std::vector<const char*> enabledLayers;
    if (enableValidationLayers) {
        enabledLayers = validationLayers;
    }

    // Extensions
    std::vector<const char*> enabledExtensions(deviceExtensions.begin(), deviceExtensions.end());

    vk::PhysicalDeviceSynchronization2Features features(
        true
    );

    // The creation of the logical device
    vk::DeviceCreateInfo createInfo(
        {},
        queueCreateInfos,
        enabledLayers,
        enabledExtensions,
        {},
        &features
    );
    device = physicalDevice.createDevice(createInfo);

    // finally we get the queues we requested
    graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
    presentQueue = device.getQueue(indices.presentFamily.value(), 0);
}


void HelloTriangleApplication::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    auto extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    QueueFamilyIndices queueFamilyindices = findQueueFamilies(physicalDevice);
    std::unordered_set<uint32_t> queueFamilyindicesUnique = { queueFamilyindices.graphicsFamily.value(), queueFamilyindices.presentFamily.value() };
    std::vector<uint32_t> queueFamiliyIndices(queueFamilyindicesUnique.begin(), queueFamilyindicesUnique.end());

    vk::SharingMode sharingMode = (queueFamiliyIndices.size() > 1)? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo(
        {},
        surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        sharingMode,
        queueFamiliyIndices,
        swapChainSupport.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        vk::True
    );

    swapChain = device.createSwapchainKHR(swapChainCreateInfo);
    swapChainImages = device.getSwapchainImagesKHR(swapChain);
    swapChainImageCount = swapChainImages.size();
    swapChainExtent = extent;
    swapChainImageFormat = surfaceFormat.format;

}

void HelloTriangleApplication::recreateSwapChain() {

    framebufferResized = false;
    // Special case where you minimize the window
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while(width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    device.waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createFramebuffers();

}

void HelloTriangleApplication::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vk::ImageViewCreateInfo createInfo(
            {},
            swapChainImages[i],
            vk::ImageViewType::e2D,
            swapChainImageFormat,
            vk::ComponentMapping(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );

        swapChainImageViews[i] = device.createImageView(createInfo);
    }
}

void HelloTriangleApplication::createRenderPass() {
    vk::AttachmentDescription colorAttachment(
        {},
        swapChainImageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR
    );

    std::vector<vk::AttachmentDescription> attachments = {colorAttachment};

    vk::AttachmentReference colorAttachmentReference(
        0,
        vk::ImageLayout::eAttachmentOptimal
    );

    std::vector<vk::AttachmentReference> inputAttachmentRefs = {};
    std::vector<vk::AttachmentReference> colorAttachmentRefs = {colorAttachmentReference};

    vk::SubpassDescription subpass(
        {},
        vk::PipelineBindPoint::eGraphics,
        inputAttachmentRefs,
        colorAttachmentRefs
    );
    vk::SubpassDependency subpassDependency(
        vk::SubpassExternal,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits::eColorAttachmentWrite
    );

    std::vector<vk::SubpassDescription> subpasses = {subpass};
    std::vector<vk::SubpassDependency> subpassesDependencies = {subpassDependency};

    vk::RenderPassCreateInfo renderPassInfo(
        {},
        attachments,
        subpasses,
        subpassesDependencies
    );

    renderPass = device.createRenderPass(renderPassInfo);
}

void HelloTriangleApplication::createGraphicsPipeline() {
    auto vertexShaderCode = readShaderSPV("shaders/generic.vert.spv");
    auto fragmentShaderCode = readShaderSPV("shaders/triangle.frag.spv");

    vk::ShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    vk::ShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    vk::PipelineShaderStageCreateInfo vertexShaderStageInfo(
        {},
        vk::ShaderStageFlagBits::eVertex,
        vertexShaderModule,
        "main"
    );

    vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo(
        {},
        vk::ShaderStageFlagBits::eFragment,
        fragmentShaderModule,
        "main"
    );

    // Specify the number of programable stages
    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStagesInfo = { vertexShaderStageInfo, fragmentShaderStageInfo };

    // Dynamic State specification
    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicState({}, dynamicStates);

    // Vertex binding
    auto vertexBindingDescription = { Vertex::getBindingDescription() };
    auto vertexAttributeDescription = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, vertexBindingDescription, vertexAttributeDescription);

    // Specify the Topology (points, lines, triangles, stripe)
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, false);

    vk::Viewport viewport(
        0.0f,
        0.0f,
        static_cast<float>(swapChainExtent.width),
        static_cast<float>(swapChainExtent.height),
        0.0f,
        1.0f
    );
    vk::Rect2D scissor({0, 0}, swapChainExtent);

    std::vector<vk::Viewport> viewports = {viewport};
    std::vector<vk::Rect2D> scissors = {scissor};
    vk::PipelineViewportStateCreateInfo viewportState({}, viewports, scissors);

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {},
        false,
        false,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise,
        false,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    );

    vk::PipelineMultisampleStateCreateInfo multisampling(
        {},
        vk::SampleCountFlagBits::e1,
        false,
        1.0f,
        nullptr,
        false,
        false
    );

    // optional: depth and stencil buffers with
    vk::PipelineDepthStencilStateCreateInfo depthAndStencil(
        {},
        false,
        false,
        vk::CompareOp::eNever,
        false,
        false
    );

    // One vk::PipelineColorBlendAttachmentState per buffer (we only have 1)
    vk::PipelineColorBlendAttachmentState colorBlendAttachment(
        false, // There is no blend, the following parameters don't matter, the buffer colors will be replaced
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );

    // We save all those in an array
    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachmentArray = { colorBlendAttachment };

    // And one global blending settings struct
    vk::PipelineColorBlendStateCreateInfo colorBlending(
        {},
        false,
        vk::LogicOp::eCopy,
        colorBlendAttachmentArray,
        { 0.0f, 0.0f, 0.0f, 0.0f }
    );

    // Specify the pipeline layout (uniforms and all that)
    std::vector<vk::DescriptorSetLayout> setLayouts = {};
    std::vector<vk::PushConstantRange> constantRanges = {};

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {},
        setLayouts,
        constantRanges
    );

    pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        shaderStagesInfo,
        &vertexInputInfo,
        &inputAssembly,
        nullptr, // not using tessellation
        &viewportState,
        &rasterizer,
        &multisampling,
        nullptr, // not using depth or stencil
        &colorBlending,
        &dynamicState,
        pipelineLayout,
        renderPass,
        0,
        VK_NULL_HANDLE,
        -1
    );

    auto result = device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
    if (result.value)
    {
        pipeline = result.value;
    }

    device.destroyShaderModule(vertexShaderModule);
    device.destroyShaderModule(fragmentShaderModule);

}

void HelloTriangleApplication::createFramebuffers() {
    swapChainFramebuffers.reserve(swapChainImageViews.size());

    for (const vk::ImageView& imageView : swapChainImageViews) {
        std::vector<vk::ImageView> attachments = {imageView};

        vk::FramebufferCreateInfo framebufferInfo(
            {},
            renderPass,
            attachments,
            swapChainExtent.width,
            swapChainExtent.height,
            1
        );

        swapChainFramebuffers.push_back(device.createFramebuffer(framebufferInfo));
    }

}

void HelloTriangleApplication::createCommandPool() {

    QueueFamilyIndices familyIndices = findQueueFamilies(physicalDevice);

    vk::CommandPoolCreateInfo poolInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        familyIndices.graphicsFamily.value()
    );

    commandPool = device.createCommandPool(poolInfo);
}

void HelloTriangleApplication::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) {
    
    vk::BufferCreateInfo bufferInfo(
        {},
        size,
        usage,
        vk::SharingMode::eExclusive
    );
    
    buffer = device.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo(
        memRequirements.size,
        findMemoryType(
            memRequirements.memoryTypeBits,
            properties
        )
    );

    bufferMemory = device.allocateMemory(allocInfo);
    device.bindBufferMemory(buffer, bufferMemory, 0);
}

void HelloTriangleApplication::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
    
    vk::CommandBufferAllocateInfo allocInfo(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        1
    );

    vk::CommandBuffer commandBuffer;
    auto buffers = device.allocateCommandBuffers(allocInfo);
    if (buffers.size() != 1) throw std::runtime_error("There was an error allocating the copy command buffer");
    commandBuffer = buffers[0];

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commandBuffer.begin(beginInfo);
    
    vk::BufferCopy copyRegion(0, 0, size);
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

    commandBuffer.end();

    std::vector<vk::Semaphore> waitSemaphores{};
    std::vector<vk::PipelineStageFlags> dstStageMask{};
    std::vector<vk::Semaphore> signalSemaphores{};
    vk::SubmitInfo submitInfo(
        waitSemaphores,
        dstStageMask,
        buffers,
        signalSemaphores
    );

    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();

    device.freeCommandBuffers(commandPool, commandBuffer);
}

void HelloTriangleApplication::createVertexBuffer() {

    // We create a staging buffer to write from CPU and then copy to the vertex buffer
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );


    void* data = device.mapMemory(stagingBufferMemory, 0, bufferSize);
    std::memcpy(data, vertices.data(), bufferSize);
    device.unmapMemory(stagingBufferMemory);

    createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vertexBuffer,
        vertexBufferMemory
    );

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);

}

void HelloTriangleApplication::createIndexBuffer() {

    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data = device.mapMemory(stagingBufferMemory, 0, bufferSize);
    std::memcpy(data, indices.data(), bufferSize);
    device.unmapMemory(stagingBufferMemory);

    createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        indexBuffer,
        indexBufferMemory
    );

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);
}


void HelloTriangleApplication::createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        MAX_FRAMES_IN_FLIGHT
    );

    auto buffers = device.allocateCommandBuffers(allocInfo);
    // This should do the trick. Still TODO in case buffer allocation fails and `buffers` has less elements
    std::move(buffers.begin(), buffers.begin() + MAX_FRAMES_IN_FLIGHT, commandBuffers.begin());
}

void HelloTriangleApplication::createSyncObjects() {
    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        imageAvailableSemaphores[i] = device.createSemaphore({});
        // The signaled enum makes it start signaled, so it just skips the first wait
        inFlightFences[i] = device.createFence({vk::FenceCreateFlagBits::eSignaled});
    }
    
    renderFinishedSemaphores.resize(swapChainImageCount);
    for(size_t i = 0; i < swapChainImageCount; ++i) {
        // this do not follow the MAX_FRAME_IN_FLIGHT. see https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
        renderFinishedSemaphores[i] = device.createSemaphore({});
    }
}


vk::ShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code)
{
    vk::ShaderModuleCreateInfo shaderCreateInfo({}, code.size(), reinterpret_cast<const uint32_t*>(code.data()));
    return device.createShaderModule(shaderCreateInfo);
}

void HelloTriangleApplication::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
    vk::CommandBufferBeginInfo beginInfo(
        {},
        nullptr
    );

    commandBuffer.begin(beginInfo);

    std::vector<vk::ClearValue> clearValues = { vk::ClearValue{{0.0f, 0.0f, 0.0f, 1.0f}} };

    vk::RenderPassBeginInfo renderPassBeginInfo(
        renderPass,
        swapChainFramebuffers[imageIndex],
        vk::Rect2D({0, 0}, swapChainExtent),
        clearValues
    );

    commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    vk::Viewport viewport(
        0.0f,
        0.0f,
        swapChainExtent.width,
        swapChainExtent.height,
        0.0f,
        1.0f
    );
    std::vector<vk::Viewport> viewports = { viewport };
    commandBuffer.setViewport(0, viewports);

    vk::Rect2D scissor(
        vk::Offset2D(0, 0),
        swapChainExtent
    );
    std::vector<vk::Rect2D> scissors = {scissor};
    commandBuffer.setScissor(0, scissors);

    std::vector<vk::Buffer> vertexBuffers{ vertexBuffer };
    std::vector<vk::DeviceSize> offsets{ 0 };
    commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);

    commandBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);
    commandBuffer.endRenderPass();
    commandBuffer.end();
}

void HelloTriangleApplication::drawFrame() {

    // we should wait until the GPU is ready with the previous frame
    auto waitforFenceResult = device.waitForFences({inFlightFences[currentFrame]}, true, std::numeric_limits<uint64_t>::max());

    uint32_t imageIndex = 0u;

    try {
        vk::ResultValue<uint32_t> nextImageResult = device.acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame]);
        if (nextImageResult.result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapChain();
            return;
        }
        else if (nextImageResult.result != vk::Result::eSuccess && nextImageResult.result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("failed to acquire swap chain image");
        }
        imageIndex = nextImageResult.value;
    }
    catch(vk::OutOfDateKHRError& error) {
        // This tells us the swap chain is not correct to display and we need to create a new one to match the window
        recreateSwapChain();
        return;
    }

    // Only reset the fences once we know we are able to get an image to display
    device.resetFences({inFlightFences[currentFrame]});

    commandBuffers[currentFrame].reset();

    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    std::vector<vk::Semaphore> waitSemaphores = {imageAvailableSemaphores[currentFrame]};
    std::vector<vk::PipelineStageFlags> waitStages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    std::vector<vk::CommandBuffer> commandBuffersToUse = {commandBuffers[currentFrame]};
    std::vector<vk::Semaphore> signalSemaphores = {renderFinishedSemaphores[imageIndex]};
    vk::SubmitInfo submitInfo(
        waitSemaphores,
        waitStages,
        commandBuffersToUse,
        signalSemaphores
    );
    graphicsQueue.submit(submitInfo, inFlightFences[currentFrame]);

    std::vector<vk::SwapchainKHR> swapChains = {swapChain};
    std::vector<uint32_t> imageIndices = {imageIndex};
    vk::PresentInfoKHR presentInfo(
        signalSemaphores,
        swapChains,
        imageIndices
    );

    try {
        vk::Result presentResult = presentQueue.presentKHR(presentInfo);
        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR || framebufferResized) {
            recreateSwapChain();
            return;
        }
        else if (presentResult != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present swap chain image");
        }
    }
    catch(vk::OutOfDateKHRError& error) {
        recreateSwapChain();
        return;
    }

    // Advance the current frame index
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

void HelloTriangleApplication::mainLoop() {
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    device.waitIdle();
}

void HelloTriangleApplication::cleanupSwapChain() {

    for (vk::Framebuffer framebuffer : swapChainFramebuffers) {
        device.destroyFramebuffer(framebuffer);
    }
    swapChainFramebuffers.clear();

    for (vk::ImageView imageView : swapChainImageViews) {
        device.destroyImageView(imageView);
    }

    device.destroySwapchainKHR(swapChain);
}

void HelloTriangleApplication::cleanup() {

    cleanupSwapChain();

    device.destroyBuffer(vertexBuffer);
    device.freeMemory(vertexBufferMemory);

    device.destroyBuffer(indexBuffer);
    device.freeMemory(indexBufferMemory);

    device.destroyPipeline(pipeline);
    device.destroyPipelineLayout(pipelineLayout);

    device.destroyRenderPass(renderPass);

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        device.destroySemaphore(imageAvailableSemaphores[i]);
        device.destroyFence(inFlightFences[i]);
    }
    for(size_t i = 0; i < swapChainImageCount; ++i) {
        device.destroySemaphore(renderFinishedSemaphores[i]);
    }


    device.destroyCommandPool(commandPool);

    device.destroy();

    if (enableValidationLayers) {
        instance.destroyDebugUtilsMessengerEXT( debugUtilsMessenger );
    }

    instance.destroySurfaceKHR(surface);
    instance.destroy();

    glfwDestroyWindow(window);

    glfwTerminate();
}



