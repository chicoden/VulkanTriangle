#define _CRT_SECURE_NO_WARNINGS

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#define alloc(type, count) (type*)malloc((count) * sizeof(type))
#define clamp(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

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

int main() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    if (!glfwInit()) die("glfwInit");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan Triangle", NULL, NULL);
    if (window == NULL) die("glfwCreateWindow");

    // --- CREATE INSTANCE ---
    VkInstance instance;
    {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Vulkan Triangle";
        app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_3;

        uint32_t ext_count;
        const char** ext_names = glfwGetRequiredInstanceExtensions(&ext_count);
        // Check extension and layer availability (vkEnumerateInstanceExtensionProperties, vkEnumerateInstanceLayerProperties)

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = ext_count;
        create_info.ppEnabledExtensionNames = ext_names;
        create_info.enabledLayerCount = 0;
        create_info.ppEnabledLayerNames = NULL;

        if (vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) die("vkCreateInstance");
    }

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
            if (
                !(queue_family_indices[GRAPHICS_FAMILY_INDEX].has_value && queue_family_indices[PRESENT_FAMILY_INDEX].has_value) ||
                queue_family_indices[GRAPHICS_FAMILY_INDEX].value != queue_family_indices[PRESENT_FAMILY_INDEX].value
            ) {
                if (queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    queue_family_indices[GRAPHICS_FAMILY_INDEX].has_value = 1;
                    queue_family_indices[GRAPHICS_FAMILY_INDEX].value = i;
                }

                VkBool32 present_support = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &present_support);
                if (present_support) {
                    queue_family_indices[PRESENT_FAMILY_INDEX].has_value = 1;
                    queue_family_indices[PRESENT_FAMILY_INDEX].value = i;
                }
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
        create_info.enabledLayerCount = 0;
        create_info.ppEnabledLayerNames = NULL;

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
            if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
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

        uint32_t graphics_and_present_indices[2] = { queue_family_indices[GRAPHICS_FAMILY_INDEX].value, queue_family_indices[PRESENT_FAMILY_INDEX].value };
        if (graphics_and_present_indices[0] == graphics_and_present_indices[1]) {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = NULL;
        } else {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = graphics_and_present_indices;
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

    // --- CREATE SHADER MODULE ---
    VkShaderModule shader_module;
    {
        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        FILE* file = fopen("../../../../VulkanTriangle/shaders/bin/simple.spv", "rb");
        if (file == NULL) die("opening spirv file");
        fseek(file, 0, SEEK_END);
        create_info.codeSize = ftell(file);

        uint32_t* code = alloc(uint32_t, (create_info.codeSize + 3) / 4);
        create_info.pCode = code;
        fseek(file, 0, SEEK_SET);
        fread(code, sizeof(char), create_info.codeSize, file);

        fclose(file);

        VkResult result = vkCreateShaderModule(device, &create_info, NULL, &shader_module);
        free(code);
        if (result != VK_SUCCESS) die("vkCreateShaderModule");
    }

    // --- CREATE RENDERPASS ---
    VkRenderPass renderpass;
    {
        VkAttachmentDescription color_attachment = {};
        color_attachment.format = format.format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment_ref = {};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = 1;
        create_info.pAttachments = &color_attachment;
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass;
        create_info.dependencyCount = 1;
        create_info.pDependencies = &dependency;
        if (vkCreateRenderPass(device, &create_info, NULL, &renderpass) != VK_SUCCESS) die("vkCreateRenderPass");
    }

    // --- CREATE PIPELINE LAYOUT ---
    VkPipelineLayout pipeline_layout;
    {
        VkPipelineLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.setLayoutCount = 0;
        create_info.pSetLayouts = NULL;
        create_info.pushConstantRangeCount = 0;
        create_info.pPushConstantRanges = NULL;
        if (vkCreatePipelineLayout(device, &create_info, NULL, &pipeline_layout) != VK_SUCCESS) die("vkCreatePipelineLayout");
    }

    // --- CREATE GRAPHICS PIPELINE ---
    VkPipeline graphics_pipeline;
    {
        VkPipelineShaderStageCreateInfo shader_stages[2] = {};

        // --- CONFIGURE VERTEX SHADER STAGE ---
        shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shader_stages[0].module = shader_module;
        shader_stages[0].pName = "vertex_main";

        // --- CONFIGURE FRAGMENT SHADER STAGE ---
        shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shader_stages[1].module = shader_module;
        shader_stages[1].pName = "fragment_main";

        // --- CONFIGURE VERTEX SHADER INPUT ---
        VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state.vertexBindingDescriptionCount = 0;
        vertex_input_state.pVertexBindingDescriptions = NULL;
        vertex_input_state.vertexAttributeDescriptionCount = 0;
        vertex_input_state.pVertexAttributeDescriptions = NULL;

        // --- CONFIGURE INPUT ASSEMBLY ---
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
        input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_state.primitiveRestartEnable = VK_FALSE;

        // --- CONFIGURE DYNAMIC PIPELINE STATE ---
        uint32_t dynamic_state_count = 2;
        VkDynamicState dynamic_states[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamic_state = {};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = dynamic_state_count;
        dynamic_state.pDynamicStates = dynamic_states;

        // --- CONFIGURE VIEWPORT STATE (DYNAMIC) ---
        VkPipelineViewportStateCreateInfo viewport_state = {};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        // --- CONFIGURE RASTERIZER STATE ---
        VkPipelineRasterizationStateCreateInfo rasterization_state = {};
        rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state.depthClampEnable = VK_FALSE;
        rasterization_state.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state.lineWidth = 1.0f;
        rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization_state.depthBiasEnable = VK_FALSE;
        rasterization_state.depthBiasConstantFactor = 0.0f;
        rasterization_state.depthBiasClamp = 0.0f;
        rasterization_state.depthBiasSlopeFactor = 0.0f;

        // --- CONFIGURE MULTISAMPLING STATE ---
        VkPipelineMultisampleStateCreateInfo multisample_state = {};
        multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state.sampleShadingEnable = VK_FALSE;
        multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample_state.minSampleShading = 1.0f;
        multisample_state.pSampleMask = NULL;
        multisample_state.alphaToCoverageEnable = VK_FALSE;
        multisample_state.alphaToOneEnable = VK_FALSE;

        // --- CONFIGURE COLOR BLENDING STATE FOR ONE ATTACHMENT ---
        VkPipelineColorBlendAttachmentState color_blend_attachment = {};
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_FALSE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

        // --- CONFIGURE COLOR BLENDING STATE ---
        VkPipelineColorBlendStateCreateInfo color_blend_state = {};
        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.logicOpEnable = VK_FALSE;
        color_blend_state.logicOp = VK_LOGIC_OP_COPY;
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &color_blend_attachment;
        color_blend_state.blendConstants[0] = 0.0f;
        color_blend_state.blendConstants[1] = 0.0f;
        color_blend_state.blendConstants[2] = 0.0f;
        color_blend_state.blendConstants[3] = 0.0f;

        VkGraphicsPipelineCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        create_info.stageCount = 2;
        create_info.pStages = shader_stages;
        create_info.pVertexInputState = &vertex_input_state;
        create_info.pInputAssemblyState = &input_assembly_state;
        create_info.pViewportState = &viewport_state;
        create_info.pRasterizationState = &rasterization_state;
        create_info.pMultisampleState = &multisample_state;
        create_info.pDepthStencilState = NULL;
        create_info.pColorBlendState = &color_blend_state;
        create_info.pDynamicState = &dynamic_state;
        create_info.layout = pipeline_layout;
        create_info.renderPass = renderpass;
        create_info.subpass = 0;
        create_info.basePipelineHandle = VK_NULL_HANDLE;
        create_info.basePipelineIndex = -1;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create_info, NULL, &graphics_pipeline) != VK_SUCCESS) die("vkCreateGraphicsPipelines");
    }

    // --- CREATE FRAMEBUFFERS FOR SWAPCHAIN IMAGES ---
    VkFramebuffer* swapchain_framebuffers = alloc(VkFramebuffer, image_count);
    for (uint32_t i = 0; i < image_count; i++) {
        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = renderpass;
        create_info.attachmentCount = 1;
        create_info.pAttachments = &swapchain_image_views[i];
        create_info.width = extent.width;
        create_info.height = extent.height;
        create_info.layers = 1;
        if (vkCreateFramebuffer(device, &create_info, NULL, &swapchain_framebuffers[i]) != VK_SUCCESS) die("vkCreateFramebuffer");
    }

    // --- CREATE COMMAND POOL TO ALLOCATE COMMAND BUFFERS IN ---
    VkCommandPool command_pool;
    {
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        create_info.queueFamilyIndex = queue_family_indices[GRAPHICS_FAMILY_INDEX].value;
        if (vkCreateCommandPool(device, &create_info, NULL, &command_pool) != VK_SUCCESS) die("vkCreateCommandPool");
    }

    // --- ALLOCATE A COMMAND BUFFER ---
    VkCommandBuffer command_buffer;
    {
        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(device, &alloc_info, &command_buffer) != VK_SUCCESS) die("vkAllocateCommandBuffers");
    }

    // --- CREATE SYNCHRONIZATION OBJECTS ---
    VkSemaphore image_available_sema;
    VkSemaphore render_finished_sema;
    VkFence in_flight_fence;
    {
        VkSemaphoreCreateInfo semaphore_create_info = {};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_create_info = {};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Make first wait a no-op since there won't be a frame in flight

        if (
            vkCreateSemaphore(device, &semaphore_create_info, NULL, &image_available_sema) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphore_create_info, NULL, &render_finished_sema) != VK_SUCCESS ||
            vkCreateFence(device, &fence_create_info, NULL, &in_flight_fence) != VK_SUCCESS
        ) {
            die("creating sync objects");
        }
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Wait for previous "in flight" frame to complete
        vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &in_flight_fence);

        // Acquire next swapchain image to render to
        uint32_t image_index;
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_sema, VK_NULL_HANDLE, &image_index);

        // Record command buffer
        vkResetCommandBuffer(command_buffer, 0);
        {
            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = 0;
            begin_info.pInheritanceInfo = NULL;
            if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) die("vkBeginCommandBuffer");

            VkClearValue clear_color = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
            VkRenderPassBeginInfo renderpass_info = {};
            renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderpass_info.renderPass = renderpass;
            renderpass_info.framebuffer = swapchain_framebuffers[image_index];
            renderpass_info.renderArea.offset.x = 0;
            renderpass_info.renderArea.offset.y = 0;
            renderpass_info.renderArea.extent = extent;
            renderpass_info.clearValueCount = 1;
            renderpass_info.pClearValues = &clear_color;
            vkCmdBeginRenderPass(command_buffer, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

            VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)extent.width;
            viewport.height = (float)extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(command_buffer, 0, 1, &viewport);

            VkRect2D scissor = {};
            scissor.offset.x = 0;
            scissor.offset.y = 0;
            scissor.extent = extent;
            vkCmdSetScissor(command_buffer, 0, 1, &scissor);

            vkCmdDraw(command_buffer, 3, 1, 0, 0);

            vkCmdEndRenderPass(command_buffer);

            if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) die("vkEndCommandBuffer");
        }

        // Submit command buffer
        {
            VkSubmitInfo submit_info = {};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &image_available_sema;
            submit_info.pWaitDstStageMask = wait_stages;

            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;

            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &render_finished_sema;

            if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fence) != VK_SUCCESS) die("vkQueueSubmit");
        }

        // Prepare for presentation
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &render_finished_sema;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &image_index;
        present_info.pResults = NULL;
        vkQueuePresentKHR(present_queue, &present_info);
    }

    vkDeviceWaitIdle(device);

    vkDestroyFence(device, in_flight_fence, NULL);
    vkDestroySemaphore(device, render_finished_sema, NULL);
    vkDestroySemaphore(device, image_available_sema, NULL);
    vkDestroyCommandPool(device, command_pool, NULL);

    for (uint32_t i = 0; i < image_count; i++) {
        vkDestroyFramebuffer(device, swapchain_framebuffers[i], NULL);
    }

    free(swapchain_framebuffers);

    vkDestroyPipeline(device, graphics_pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    vkDestroyRenderPass(device, renderpass, NULL);
    vkDestroyShaderModule(device, shader_module, NULL);

    for (uint32_t i = 0; i < image_count; i++) {
        vkDestroyImageView(device, swapchain_image_views[i], NULL);
    }

    free(swapchain_image_views);
    free(swapchain_images);

    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}