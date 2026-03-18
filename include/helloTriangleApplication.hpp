#pragma once

#include <cstdint>
#include <vector>
#include <optional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "vertex.hpp"

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class HelloTriangleApplication {

public:
    /**
     * The main entry of the app, start the app by calling this method
    */
    void run();
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    void notifyFramebufferResized() { framebufferResized = true; }

private:

    GLFWwindow* window;
    const uint32_t width = 800;
    const uint32_t height = 600;
    static constexpr int32_t MAX_FRAMES_IN_FLIGHT = 2;

    const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    // It's possible to use uint32_t
    const std::vector<uint16_t> indices = {
        0, 1, 2,
        2, 3, 0
    };

    uint32_t currentFrame = 0;

    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    vk::Queue graphicsQueue;
    vk::Queue presentQueue;

    vk::DebugUtilsMessengerEXT debugUtilsMessenger;

    vk::SurfaceKHR surface;

    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;

    std::vector<vk::ImageView> swapChainImageViews;

    vk::RenderPass renderPass;
    vk::PipelineLayout pipelineLayout;

    vk::Pipeline pipeline;

    std::vector<vk::Framebuffer> swapChainFramebuffers;

    vk::CommandPool commandPool;

    std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> commandBuffers;

    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;

    // Sync
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> inFlightFences;

    bool framebufferResized = false;


    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    /**
     * Creates the window using GLFW utilities
     */
    void initWindow();
    /**
     * Initializes all the basic things you need to start using Vulkan (instance, debug layers,
     * devices and surfaces)
     */
    void initVulkan();
    /**
     * Sets up the debug callback for the debug layers
     */
    void setupDebugMessenger();


    /**
     * Creates the vulkan instance and defines the app info
    */
    void createVulkanInstance();
    /**
     * Creates the main surface from the already created window
    */
    void createSurface();
    /**
     * Creates the logical device from the physical device requesting queues/layers/extensions needed
    */
    void createLogicalDevice();
    void createSwapChain();
    void recreateSwapChain();
    void createImageViews();

    void createRenderPass();

    void createGraphicsPipeline();

    void createFramebuffers();

    void createCommandPool();

    void createCommandBuffers();

    void createSyncObjects();

    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags proprties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
    void createVertexBuffer();
    void createIndexBuffer();

    vk::ShaderModule createShaderModule(const std::vector<char>& code);

    /**
     * The main loop of the program
     */
    void mainLoop();
    /**
     * Cleans up everything in the required order (devices/debug utils/surface/instance/window)
     */
    void cleanup();
    void cleanupSwapChain();


    void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);
    void drawFrame();

    /**
     * Checks all the required extensions needed for the program to work
     */
    std::vector<const char*> getRequiredExtensions() const;
    /**
     * Verifies which queue families are the ones that will be used for the program
     */
    QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& physicalDevice) const;
    /**
     * Loads the debug messenger functions as they are in an extension and must be loaded before
     * being used
     */
    void loadDebugMessengerFunctions();
    /**
     * Sets the physical device based on one that passes `isDeviceSuitable`
     */
    void pickPhysicalDevice();
    /**
     * Queries different properties from the physical device to see if it is compatible with the
     * current surface being used
     */
    SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice& device) const;
    /**
     * Picks the best surface format based on an internal ranking
     */
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const;
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;


    bool checkValidationLayerSupport() const;
    /**
     * Compares the supported extensions of the given physical device with the list of required
     * extensions `deviceExtensions`
     */
    bool checkDeviceExtensionSupport(const vk::PhysicalDevice& device) const;
    /**
     * Checks if the given physical device is suitable to work with the current/needed
     * objects (queues/extensions/surface)
    */
    bool isDeviceSuitable(const vk::PhysicalDevice& device) const;

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    /**
     * Utility to create the struct for debug callbacks info struct
    */
    vk::DebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo() const;

};