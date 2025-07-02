#define _CRT_SECURE_NO_WARNINGS

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define vkCreateDebugUtilsMessengerEXT _vkCreateDebugUtilsMessengerEXT
#define vkDestroyDebugUtilsMessengerEXT _vkDestroyDebugUtilsMessengerEXT
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef vkCreateDebugUtilsMessengerEXT
#undef vkDestroyDebugUtilsMessengerEXT

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#define alloc(type, count) (type*)malloc((count) * sizeof(type))
#define clamp(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define ENABLE_VALIDATION_LAYERS

#ifdef ENABLE_VALIDATION_LAYERS
uint32_t ENABLED_LAYER_COUNT = 1;
const char* ENABLED_LAYER_NAMES[] = {
    "VK_LAYER_KHRONOS_validation"
};
#else
uint32_t ENABLED_LAYER_COUNT = 0;
const char** ENABLED_LAYER_NAMES = NULL;
#endif

typedef struct {
    uint32_t value;
    int has_value;
} opt_u32_t;

typedef enum {
    GRAPHICS_FAMILY_INDEX,
    PRESENT_FAMILY_INDEX,
    QUEUE_FAMILY_COUNT
} queue_family_index_t;
typedef opt_u32_t queue_family_indices_t[QUEUE_FAMILY_COUNT];

VkResult NULL_vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) { return VK_ERROR_EXTENSION_NOT_PRESENT; }
void NULL_vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {}
VkResult (*vkCreateDebugUtilsMessengerEXT)(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void (*vkDestroyDebugUtilsMessengerEXT)(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

void die(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

const char* get_str(void* strs, uint32_t i) { return *((const char**)strs + i); }
const char* get_ext_name(void* ext_props, uint32_t i) { return ((VkExtensionProperties*)ext_props + i)->extensionName; }
const char* get_layer_name(void* layer_props, uint32_t i) { return ((VkLayerProperties*)layer_props + i)->layerName; }
const char* get_u32_str(void* ints, uint32_t i) { static char str[16]; sprintf(str, "%u", *((uint32_t*)ints + i)); return str; }
const char* get_opt_u32_str(void* ints, uint32_t i) { static char str[16]; if (((opt_u32_t*)ints + i)->has_value) sprintf(str, "%u", ((opt_u32_t*)ints + i)->value); else sprintf(str, "none"); return str; }
void print_strings(void* data, uint32_t count, const char* (*get_str)(void*, uint32_t)) {
    if (count > 0) {
        printf("\"%s\"", get_str(data, 0));
        for (uint32_t i = 1; i < count; i++) {
            printf(", \"%s\"", get_str(data, i));
        }
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        printf("Validation layer: %s\n", callback_data->pMessage);
    }

    return VK_FALSE;
}

int main() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    if (!glfwInit()) die("glfwInit");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan Triangle", NULL, NULL);
    if (window == NULL) die("glfwCreateWindow");

#ifdef ENABLE_VALIDATION_LAYERS
    printf("Validation enabled\n");
#endif

    // --- CREATE INSTANCE ---
    VkInstance instance;
    {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Vulkan Triangle";
        app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        // Include glfw required extensions (glfwGetRequiredInstanceExtensions)
#ifdef ENABLE_VALIDATION_LAYERS
        uint32_t ext_count = 3;
        const char* ext_names[] = {
            "VK_KHR_surface",
            "VK_KHR_win32_surface",
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        };
#else
        uint32_t ext_count = 2;
        const char* ext_names[] = {
            "VK_KHR_surface",
            "VK_KHR_win32_surface"
        };
#endif

        printf("Required extensions: ");
        print_strings(ext_names, ext_count, get_str);
        printf("\nRequired layers: ");
        print_strings(ENABLED_LAYER_NAMES, ENABLED_LAYER_COUNT, get_str);
        printf("\n");
        // Check extension and layer availability (vkEnumerateInstanceExtensionProperties, vkEnumerateInstanceLayerProperties)

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = ext_count;
        create_info.ppEnabledExtensionNames = ext_names;
        create_info.enabledLayerCount = ENABLED_LAYER_COUNT;
        create_info.ppEnabledLayerNames = ENABLED_LAYER_NAMES;

#ifdef ENABLE_VALIDATION_LAYERS
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debug_callback;
        debug_create_info.pUserData = NULL;
        create_info.pNext = &debug_create_info;
#endif

        if (vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) die("vkCreateInstance");
    }

    // --- LOAD EXTENSION FUNCTIONS ---
    vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT == NULL) vkCreateDebugUtilsMessengerEXT = NULL_vkCreateDebugUtilsMessengerEXT;
    if (vkDestroyDebugUtilsMessengerEXT == NULL) vkDestroyDebugUtilsMessengerEXT = NULL_vkDestroyDebugUtilsMessengerEXT;

    // --- CREATE DEBUG MESSENGER ---
#ifdef ENABLE_VALIDATION_LAYERS
    VkDebugUtilsMessengerEXT debug_messenger;
    {
        VkDebugUtilsMessengerCreateInfoEXT create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = debug_callback;
        create_info.pUserData = NULL;
        if (vkCreateDebugUtilsMessengerEXT(instance, &create_info, NULL, &debug_messenger) != VK_SUCCESS) die("CreateDebugUtilsMessengerEXT");
    }
#endif

    // --- CREATE SURFACE ---
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) die("glfwCreateWindowSurface");

    // --- SELECT PHYSICAL DEVICE ---
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    {
        uint32_t gpu_count = 0;
        vkEnumeratePhysicalDevices(instance, &gpu_count, NULL);
        if (gpu_count == 0) die("no gpus with vulkan support");
        VkPhysicalDevice* gpus = alloc(VkPhysicalDevice, gpu_count);
        vkEnumeratePhysicalDevices(instance, &gpu_count, gpus);

        for (uint32_t i = 0; i < gpu_count; i++) {
            VkPhysicalDeviceProperties gpu_props;
            vkGetPhysicalDeviceProperties(gpus[i], &gpu_props);

            //VkPhysicalDeviceFeatures gpu_features;
            //vkGetPhysicalDeviceFeatures(gpus[i], &gpu_features);

            // Check if supports required extensions (vkEnumerateDeviceExtensionProperties)

            if (gpu_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                printf("Selected GPU: %s\n", gpu_props.deviceName);
                gpu = gpus[i];
            }
        }

        free(gpus);
        if (gpu == VK_NULL_HANDLE) die("physical device selection");
    }

    // --- FIND QUEUE FAMILY INDICES ---
    queue_family_indices_t queue_family_indices = {};
    {
        uint32_t queue_family_prop_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_prop_count, NULL);
        VkQueueFamilyProperties* queue_family_props = alloc(VkQueueFamilyProperties, queue_family_prop_count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_prop_count, queue_family_props);

        for (uint32_t i = 0; i < queue_family_prop_count; i++) {
            if (!queue_family_indices[GRAPHICS_FAMILY_INDEX].has_value && queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queue_family_indices[GRAPHICS_FAMILY_INDEX].has_value = 1;
                queue_family_indices[GRAPHICS_FAMILY_INDEX].value = i;
            }

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &present_support);
            if (!queue_family_indices[PRESENT_FAMILY_INDEX].has_value && present_support) {
                queue_family_indices[PRESENT_FAMILY_INDEX].has_value = 1;
                queue_family_indices[PRESENT_FAMILY_INDEX].value = i;
            }
        }

        free(queue_family_props);
        if (!queue_family_indices[GRAPHICS_FAMILY_INDEX].has_value) die("missing graphics queue family index");
        if (!queue_family_indices[PRESENT_FAMILY_INDEX].has_value) die("missing present queue family index");
    }

    uint32_t unique_queue_family_indices[QUEUE_FAMILY_COUNT];
    uint32_t unique_queue_family_indices_count = 0;
    for (uint32_t i = 0; i < QUEUE_FAMILY_COUNT; i++) {
        if (queue_family_indices[i].has_value) {
            uint32_t queue_family_index = queue_family_indices[i].value;
            int is_unique = 1;
            for (uint32_t j = 0; j < unique_queue_family_indices_count; j++) {
                if (unique_queue_family_indices[j] == queue_family_index) {
                    is_unique = 0;
                    break;
                }
            }

            if (is_unique) {
                unique_queue_family_indices[unique_queue_family_indices_count++] = queue_family_index;
            }
        }
    }

    // --- CREATE LOGICAL DEVICE ---
    VkDevice device;
    {
        VkDeviceQueueCreateInfo queue_create_infos[QUEUE_FAMILY_COUNT] = {};
        float queue_priorities[1] = { 1.0f };
        for (uint32_t i = 0; i < unique_queue_family_indices_count; i++) {
            queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_infos[i].queueFamilyIndex = unique_queue_family_indices[i];
            queue_create_infos[i].queueCount = 1;
            queue_create_infos[i].pQueuePriorities = queue_priorities;
        }

        VkPhysicalDeviceFeatures enabled_features = {};

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = unique_queue_family_indices_count;
        create_info.pQueueCreateInfos = queue_create_infos;
        create_info.pEnabledFeatures = &enabled_features;

        uint32_t device_ext_count = 1;
        const char* device_ext_names[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        create_info.enabledExtensionCount = device_ext_count;
        create_info.ppEnabledExtensionNames = device_ext_names;

        // For compatibility...
        create_info.enabledLayerCount = ENABLED_LAYER_COUNT;
        create_info.ppEnabledLayerNames = ENABLED_LAYER_NAMES;

        if (vkCreateDevice(gpu, &create_info, NULL, &device) != VK_SUCCESS) die("vkCreateDevice");
    }

    // --- GET QUEUES ---
    VkQueue graphics_queue;
    VkQueue present_queue;
    vkGetDeviceQueue(device, queue_family_indices[GRAPHICS_FAMILY_INDEX].value, 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_family_indices[PRESENT_FAMILY_INDEX].value, 0, &present_queue);

    // --- SELECT SUITABLE SURFACE EXTENT AND SWAPCHAIN IMAGE COUNT ---
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &capabilities);
    VkExtent2D extent = capabilities.currentExtent;
    if (capabilities.currentExtent.width == ~(uint32_t)0) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        extent.width = (uint32_t)width;
        extent.height = (uint32_t)height;
        extent.width = clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    // --- SELECT SUITABLE SURFACE FORMAT ---
    VkSurfaceFormatKHR format;
    int have_suitable_format = 0;
    {
        uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, NULL);
        VkSurfaceFormatKHR* formats = alloc(VkSurfaceFormatKHR, format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, formats);

        for (uint32_t i = 0; i < format_count; i++) {
            if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB && formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
                format = formats[i];
                have_suitable_format = 1;
                break;
            }
        }

        free(formats);
        if (!have_suitable_format) die("format selection");
    }

    // --- SELECT SUITABLE PRESENTATION MODE ---
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    {
        uint32_t present_mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, NULL);
        VkPresentModeKHR* present_modes = alloc(VkPresentModeKHR, present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, present_modes);

        for (uint32_t i = 0; i < present_mode_count; i++) {
            if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                printf("Have VK_PRESENT_MODE_MAILBOX_KHR\n");
                present_mode = present_modes[i];
                break;
            }
        }

        free(present_modes);
    }

    // --- CREATE SWAPCHAIN ---
    VkSwapchainKHR swapchain;
    {
        VkSwapchainCreateInfoKHR create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = format.format;
        create_info.imageColorSpace = format.colorSpace;
        create_info.imageExtent = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t qf_indices[2] = { queue_family_indices[GRAPHICS_FAMILY_INDEX].value, queue_family_indices[PRESENT_FAMILY_INDEX].value };
        if (qf_indices[0] != qf_indices[1]) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = qf_indices;
        } else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = NULL;
        }

        create_info.preTransform = capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain) != VK_SUCCESS) die("vkCreateSwapchainKHR");
    }

    // --- GET SWAPCHAIN IMAGES ---
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
    VkImage* swapchain_images = alloc(VkImage, image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images);

    // --- CREATE SWAPCHAIN IMAGE VIEWS ---
    VkImageView* swapchain_image_views = alloc(VkImageView, image_count);
    for (uint32_t i = 0; i < image_count; i++) {
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swapchain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = format.format;

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &create_info, NULL, &swapchain_image_views[i]) != VK_SUCCESS) die("vkCreateImageView %u", i);
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    for (uint32_t i = 0; i < image_count; i++) vkDestroyImageView(device, swapchain_image_views[i], NULL);
    free(swapchain_image_views);
    free(swapchain_images);
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
#ifdef ENABLE_VALIDATION_LAYERS
    vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, NULL);
#endif
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}