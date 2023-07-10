/*************************************************************************************
 * @file renderBackend.cpp
 * @brief Implementation for all backend rendering classes.
 ************************************************************************************/

#include "renderBackend/renderBackend.hpp"

#include <array>
#include <bits/types/cookie_io_functions_t.h>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <unordered_map>
#include <vector>
#include <set>
#include <optional>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>
#include "glm/glm.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace RenderBackend{

const uint8_t numberFramesInFlight = 2;
vk::Extent2D surfaceSize;

std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};


/*####################### Helper functions ########################*/
bool checkValidationLayerSupport()
{
    int counter = 0;
    auto availableLayers = vk::enumerateInstanceLayerProperties();
    for(int i = 0; i < availableLayers.size(); i ++){
        std::stringstream ss;
        ss << availableLayers[i].layerName;
        std::string layerName = ss.str();
        for(int j = 0; j < validationLayers.size(); j++){
            if(layerName == validationLayers[j]){
                counter++;
            }
        }
    }

    return counter == validationLayers.size();
}

void checkPhysicalDeviceExtensionSupport(vk::PhysicalDevice physicalDevice, std::set<std::string> &extensions)
{
    auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());    

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    if (requiredExtensions.empty()){
        return;
    }

    exit(0);
}

vk::PhysicalDevice selectPhysicalDevice(vk::Instance instance, std::vector<const char*> &extensions)
{
    std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());    
    auto availableInstanceExt = vk::enumerateInstanceExtensionProperties();
    for (const auto& extension : availableInstanceExt) {
        requiredExtensions.erase(extension.extensionName);
    }

    for (auto& device : instance.enumeratePhysicalDevices())
    {
        if(device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        {
            /*
            There are more checks that need to be done, such as shader support
            and swapchain support.
            */
            checkPhysicalDeviceExtensionSupport(device, requiredExtensions);
            return device;
        }
    } 
    exit(0);
}

void selectQueueFamilies(vk::PhysicalDevice physicalDevice,
             vk::SurfaceKHR surface,
             std::optional<uint32_t> &graphical,
             std::optional<uint32_t> &present)
{
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    for(uint32_t i = 0; i < queueFamilyProperties.size(); i++)
    {
        auto graphics = queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics;
        auto apresentacao = physicalDevice.getSurfaceSupportKHR(i, surface);
        if(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            graphical = i;
        }
        if(physicalDevice.getSurfaceSupportKHR(i, surface))
        {
            present = i;
        }
    }
    if(!(graphical.has_value() && present.has_value()))
    {
        std::cout << "ERROR: COULD NOT FIND GRAPHICAL OR PRESENT QUEUE" << std::endl;
        exit(0);
    }
}

uint32_t findMemoryType(vk::PhysicalDevice device, uint32_t type, vk::MemoryPropertyFlags properties){
    vk::PhysicalDeviceMemoryProperties devProperties = device.getMemoryProperties();

    for (uint32_t i = 0; i < devProperties.memoryTypeCount; i++){
        bool isType = type & (1 << i);
        bool hasProperties = (devProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if(isType && hasProperties){
            return i;
        }
    }
    throw std::runtime_error("Couldn't find the right type of memory to allocate");
}


/*###################### ResourceManager methods ######################################*/
ResourceManager::ResourceManager(vk::Device device,
                 vk::PhysicalDevice physicalDevice,
                 Commands* commands) :
    device(device), physicalDevice(physicalDevice), commands(commands)
{}

ResourceManager::~ResourceManager()
{}

BufferId ResourceManager::createBuffer(BufferType type, vk::DeviceSize size)
{
    static BufferId bufferId = 0;
    bufferId++;

    vk::BufferUsageFlags usageFlags;
    switch (type) {
        case BufferType::eVertexBuffer:
            usageFlags = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
            break;
        case BufferType::eIndexBuffer:
            usageFlags = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
            break;
        case BufferType::eStageBuffer:
            usageFlags = vk::BufferUsageFlagBits::eTransferSrc;
            break;
        case BufferType::eUniformBuffer:
            usageFlags = vk::BufferUsageFlagBits::eUniformBuffer;
            break;
    }

    vk::BufferCreateInfo bufferInfo{
        .size = size,
        .usage = usageFlags,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    auto buffer = device.createBuffer(bufferInfo);
    buffers[bufferId] = buffer;

    vk::MemoryPropertyFlags memFlags;
    switch (type) {
        case BufferType::eVertexBuffer:
            //TODO: I think this can be deviceCoherent? Since I just populate it once with a stage buffer and that's it.
            memFlags = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible;
            break;
        case BufferType::eIndexBuffer:
            memFlags = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible;
            break;
        case BufferType::eStageBuffer:
            memFlags = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible;
            break;
        case BufferType::eUniformBuffer:
            memFlags = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible;
            break;
    }

    auto memRequirements = device.getBufferMemoryRequirements(buffer);
    vk::MemoryAllocateInfo memAllocInfo{
        // I do this because in the vulkan spec it says that an allocation cannot be smaller
        // than the requirements.size (although it works either way)
        .allocationSize = bufferInfo.size >= memRequirements.size ?
                            bufferInfo.size :
                            memRequirements.size, 
        .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, memFlags),
    };
    auto memory = device.allocateMemory(memAllocInfo);
    bufferMemories[bufferId] = memory;
    device.bindBufferMemory(buffer, memory, 0);
    bufferSizes[bufferId] = size;
    
    return bufferId;
}

void ResourceManager::insertDataBuffer(BufferId id, vk::DeviceSize size, void* data)
{
    void* bufferStart = device.mapMemory(bufferMemories[id], 0, size);
    std::memcpy(bufferStart, data, (size_t) size);
    device.unmapMemory(bufferMemories[id]);
}

void ResourceManager::copyBuffers(BufferId source, BufferId destination, vk::DeviceSize size)
{
    auto commmandBuffer = commands->BeginSingleTimeCommand();
    vk::BufferCopy copyCommand{
        .size = size,
    };
    std::vector<vk::BufferCopy> copyCommands{copyCommand};
    
    commmandBuffer.copyBuffer(buffers[source], buffers[destination], copyCommands);
    commands->EndSingleTimeCommand(commmandBuffer, true);
}

vk::Buffer ResourceManager::getBuffer(BufferId id)
{
    return buffers[id];
}

ImageId ResourceManager::createImage(vk::Extent2D size, ImageType type)
{
    static ImageId imageId = 0;
    imageId++;

    //ImageType dependent properties
    static std::unordered_map<ImageType, vk::Format> imageFormats = {
        {ImageType::eDepth, vk::Format::eD32Sfloat},
        {ImageType::eTexture, vk::Format::eR8G8B8A8Srgb},
    };
    static std::unordered_map<ImageType, vk::ImageTiling> imageTilings = {
        {ImageType::eDepth, vk::ImageTiling::eOptimal},
        {ImageType::eTexture, vk::ImageTiling::eOptimal},
    };
    static std::unordered_map<ImageType, vk::ImageUsageFlags> imageUsages = {
        {ImageType::eDepth, vk::ImageUsageFlagBits::eDepthStencilAttachment},
        {ImageType::eTexture, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled},
    };
    static std::unordered_map<ImageType, vk::MemoryPropertyFlags> imageMemFlags = {
        {ImageType::eDepth, vk::MemoryPropertyFlagBits::eDeviceLocal},
        {ImageType::eTexture, vk::MemoryPropertyFlagBits::eDeviceLocal},
    };
    static std::unordered_map<ImageType, vk::ImageAspectFlags> imageAspectFlags = {
        {ImageType::eDepth, vk::ImageAspectFlagBits::eDepth},
        {ImageType::eTexture, vk::ImageAspectFlagBits::eColor},
    };

    vk::Format format = imageFormats[type];
    vk::ImageTiling tiling = imageTilings[type];
    vk::ImageUsageFlags usage = imageUsages[type];
    vk::MemoryPropertyFlags memFlags = imageMemFlags[type];

    vk::ImageCreateInfo imageCreateInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {
            .width = size.width,
            .height = size.height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    };
    auto image = device.createImage(imageCreateInfo);
    images[imageId] = image;

    auto memoryRequirements = device.getImageMemoryRequirements(image);
    vk::MemoryAllocateInfo memoryAllocateInfo{
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, memFlags),
    };
    auto memory = device.allocateMemory(memoryAllocateInfo);
    imageMemories[imageId] = memory;
    device.bindImageMemory(image, memory, vk::DeviceSize{0});

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = imageAspectFlags[type],
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    imageViews[imageId] = device.createImageView(imageViewCreateInfo);

    return imageId;
}

void ResourceManager::transitionImage(ImageId imageId, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    auto commandBuffer = commands->BeginSingleTimeCommand();


    vk::AccessFlags srcAccess;
    vk::AccessFlags dstAccess;
    vk::PipelineStageFlags srcStage;
    vk::PipelineStageFlags dstStage;
    if(oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        srcAccess = vk::AccessFlagBits::eNone;
        dstAccess = vk::AccessFlagBits::eTransferWrite;
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if(oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        srcAccess = vk::AccessFlagBits::eTransferWrite;
        dstAccess = vk::AccessFlagBits::eShaderRead;
        srcStage = vk::PipelineStageFlagBits::eTransfer;
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        std::cout << "Unsuported conversion between layouts." << std::endl;
        exit(0);
    }

    vk::ImageMemoryBarrier barrier{
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = images[imageId],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    std::vector<vk::ImageMemoryBarrier> barriers{barrier};
    commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags{},
                                    0, nullptr,
                                    0, nullptr,
                                    1, &barrier);
    commands->EndSingleTimeCommand(commandBuffer, true); //Should I wait tho?
}

void ResourceManager::copyBufferToImage(BufferId bufferId, ImageId imageId, vk::Extent2D size)
{
    auto commandBuffer = commands->BeginSingleTimeCommand();
    vk::BufferImageCopy bufferImageCopyCommand{
        .bufferOffset = 0,
        .bufferRowLength = 0, 
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor, //This implicitly expects a 4 channel image
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            .width = size.width,
            .height = size.height,
            .depth = 1,
        },
    };

    transitionImage(imageId, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    commandBuffer.copyBufferToImage(buffers[bufferId], images[imageId], vk::ImageLayout::eTransferDstOptimal, bufferImageCopyCommand);
    commands->EndSingleTimeCommand(commandBuffer, true);
    transitionImage(imageId, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}

vk::ImageView ResourceManager::getImageView(ImageId imageId)
{
    return imageViews[imageId];
}


/*####################### Command Methods ##################################*/
Commands::Commands(vk::Device device, vk::Queue queue, int queueFamilyId, int nPools) : device(device), queue(queue), queueFamilyId(queueFamilyId)
{
    pools.reserve(nPools);
    for(int i = 0; i < nPools; i++)
    {
        vk::CommandPoolCreateInfo poolCreateInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = static_cast<uint32_t>(queueFamilyId),
        };
        auto pool = device.createCommandPool(poolCreateInfo);
        pools.push_back(pool);
    }
    pool = pools[0];
}

vk::CommandBuffer Commands::BeginSingleTimeCommand()
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };
    auto buffer = device.allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };
    buffer.begin(beginInfo);

    return buffer;
}

void Commands::EndSingleTimeCommand(vk::CommandBuffer commandBuffer, bool wait)
{
    commandBuffer.end();
    vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    queue.submit(submitInfo);
    if(wait){
        queue.waitIdle();
    }    
}

void Commands::setPool(int poolNumber)
{
    device.destroyCommandPool(pools[poolNumber]);

    vk::CommandPoolCreateInfo poolCreateInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = static_cast<uint32_t>(queueFamilyId),
    };
    pools[poolNumber] = device.createCommandPool(poolCreateInfo);
    this->pool = pools[poolNumber];
}

std::vector<BufferId> Commands::allocateCommandBuffers(int noBuffers)
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = static_cast<uint32_t>(noBuffers),
    };
    auto newBuffers = device.allocateCommandBuffers(allocInfo);
    std::vector<BufferId> bufferIds;
    static BufferId id = 0;
    for(auto& buffer : newBuffers){
        buffers[id] = buffer;
        bufferIds.push_back(id);
        id++;
    }
    return bufferIds;
}

vk::CommandBuffer Commands::beginCommand(BufferId id)
{
    vk::CommandBufferBeginInfo beginInfo{};
    auto commandBuffer = buffers[id];
    commandBuffer.begin(beginInfo);
    return commandBuffer;
}

void Commands::endCommand(vk::CommandBuffer buffer,
            std::vector<vk::Semaphore> waitSemaphores,
            std::vector<vk::PipelineStageFlags> waitStages,
            std::vector<vk::Semaphore> signalSemaphores,
            vk::Fence fence)
{
    buffer.end();
    vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphores = waitSemaphores.data(),
        .pWaitDstStageMask = waitStages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &buffer,
        .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
        .pSignalSemaphores = signalSemaphores.data(),
    };
    queue.submit(submitInfo, fence);
}


/*##################### DescriptorManager methods ####################################*/
DescriptorManager::DescriptorManager(vk::Device device) : device(device)
{
    std::vector<vk::DescriptorPoolSize> poolSizes{
        {vk::DescriptorType::eUniformBuffer, 100},
        {vk::DescriptorType::eCombinedImageSampler, 100},
    };

    vk::DescriptorPoolCreateInfo createPoolInfo {
        .maxSets = static_cast<uint32_t>(10000),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    pool = device.createDescriptorPool(createPoolInfo);
}

DSLayoutId DescriptorManager::CreateLayout(std::vector<vk::DescriptorSetLayoutBinding> bindings)
{
    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    static DSLayoutId id = 0;
    auto _layout = device.createDescriptorSetLayout(layoutCreateInfo);
    DescriptorSetLayout layout = {
        .layoutId = id,
        .DSLayout = _layout,
        .bindings = bindings,
    };
    layouts[id] = layout;
    freeDescriptorsByLayout[id] = std::stack<DescriptorSet>();
    return id++;
}

vk::DescriptorSetLayout DescriptorManager::getDSLayout(DSLayoutId id)
{
    return layouts[id].DSLayout;
}

std::vector<vk::DescriptorSetLayout> DescriptorManager::getDSLayouts(std::vector<DSLayoutId> ids)
{
    std::vector<vk::DescriptorSetLayout> DSLayouts;
    for(auto& id : ids){
        DSLayouts.push_back(layouts[id].DSLayout);
    }
    return DSLayouts;
}

void DescriptorManager::preAllocateDescriptorSets(DSLayoutId layoutId, uint32_t noSets)
{
    auto layout = layouts[layoutId].DSLayout;
    std::vector<vk::DescriptorSetLayout> layouts(noSets, layout);
    vk::DescriptorSetAllocateInfo DSAllocInfo{
        .descriptorPool = pool,
        .descriptorSetCount = noSets,
        .pSetLayouts = layouts.data(),
    };

    static DSId id = 0;
    auto descriptorSets = device.allocateDescriptorSets(DSAllocInfo);
    for(auto& descriptorSet : descriptorSets){
        auto set =  DescriptorSet{
            .id = id,
            .layoutId = layoutId,
            .descriptorSet = descriptorSet
        };

        descriptors[id] = set;
        freeDescriptorsByLayout[layoutId].push(set);
        id++;
    }
}

DSId DescriptorManager::writeDS(DSLayoutId id, std::vector<WriteDescriptorInfo> writeInfos)
{
    DescriptorSet descriptor = freeDescriptorsByLayout[id].top();
    freeDescriptorsByLayout[id].pop();
    DescriptorSetLayout layout = layouts[descriptor.layoutId];
    if(writeInfos.size() != layout.bindings.size()){
        std::cout << "ERROR::writeDS => Layout has " << layout.bindings.size()
                  << " bindings but only " << writeInfos.size() << " write infos provided." << std::endl;
        exit(0);
    }

    std::vector<vk::WriteDescriptorSet> writes;
    for(int i = 0; i < layout.bindings.size(); i++){
        auto binding = layout.bindings[i];
        if(binding.descriptorType == vk::DescriptorType::eUniformBuffer){
            vk::WriteDescriptorSet write = {
                .dstSet = descriptor.descriptorSet,
                .dstBinding = binding.binding,
                .dstArrayElement = 0,
                .descriptorCount = binding.descriptorCount,
                .descriptorType = binding.descriptorType,
                .pBufferInfo = &writeInfos[i].bufferInfo.value(),
            };
            writes.push_back(write);
        }
        else if(binding.descriptorType == vk::DescriptorType::eCombinedImageSampler){
            vk::WriteDescriptorSet write = {
                .dstSet = descriptor.descriptorSet,
                .dstBinding = binding.binding,
                .dstArrayElement = 0,
                .descriptorCount = binding.descriptorCount,
                .descriptorType = binding.descriptorType,
                .pImageInfo = &writeInfos[i].imageInfo.value(),
            };
            writes.push_back(write);
        }
    }
    device.updateDescriptorSets(writes, nullptr);

    return descriptor.id;
}

void DescriptorManager::updateDS(DSId id, std::vector<WriteDescriptorInfo> writeInfos)
{
    DescriptorSet descriptor = descriptors[id];
    DescriptorSetLayout layout = layouts[descriptor.layoutId];

    std::vector<vk::WriteDescriptorSet> writes;
    for(int i = 0; i < layout.bindings.size(); i++){
        auto binding = layout.bindings[i];
        if(binding.descriptorType == vk::DescriptorType::eUniformBuffer){
            vk::WriteDescriptorSet write = {
                .dstSet = descriptor.descriptorSet,
                .dstBinding = binding.binding,
                .dstArrayElement = 0,
                .descriptorCount = binding.descriptorCount,
                .descriptorType = binding.descriptorType,
                .pBufferInfo = &writeInfos[i].bufferInfo.value(),
            };
            writes.push_back(write);
        }
        else if(binding.descriptorType == vk::DescriptorType::eCombinedImageSampler){
            vk::WriteDescriptorSet write = {
                .dstSet = descriptor.descriptorSet,
                .dstBinding = binding.binding,
                .dstArrayElement = 0,
                .descriptorCount = binding.descriptorCount,
                .descriptorType = binding.descriptorType,
                .pImageInfo = &writeInfos[i].imageInfo.value(),
            };
            writes.push_back(write);
        }
    }

    device.updateDescriptorSets(writes, nullptr);
}

void DescriptorManager::freeDS(DSId id)
{
    DescriptorSet descriptor = descriptors[id];
    freeDescriptorsByLayout[descriptor.layoutId].push(descriptor);
}

vk::DescriptorSet DescriptorManager::getDS(DSId id)
{
    return descriptors[id].descriptorSet;
}


/*############################### Pipeline manager Methods #################################*/
PipelineManager::PipelineManager(vk::Device device, DescriptorManager* descriptorManager) : device(device), descriptorManager(descriptorManager)
{}

PipelineID PipelineManager::CreatePipeline(PipelineInfo info)
{
    auto fragmentShader = compileShaderModule(readFile(info.fragmentShaderPath));
    auto vertexShader = compileShaderModule(readFile(info.vertexShaderPath));

    vk::PipelineShaderStageCreateInfo fragmentShaderStage{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = fragmentShader,
        .pName = "main",
    };

    vk::PipelineShaderStageCreateInfo vertexShaderStage{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = vertexShader,
        .pName = "main",
    };

    std::vector<vk::PipelineShaderStageCreateInfo> stages = {fragmentShaderStage, vertexShaderStage};

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .vertexBindingDescriptionCount = static_cast<uint32_t>(info.vertexBinds.size()),
        .pVertexBindingDescriptions = info.vertexBinds.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(info.vertexAttribs.size()),
        .pVertexAttributeDescriptions = info.vertexAttribs.data(),
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false,
    };

    vk::Viewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(surfaceSize.width),
        .height = static_cast<float>(surfaceSize.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vk::Rect2D sissor{
        .offset = {0, 0},
        .extent = surfaceSize,
    };

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &sissor,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = false,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = false,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable = false,
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState{
        .srcColorBlendFactor = vk::BlendFactor::eOne, 
        .dstColorBlendFactor = vk::BlendFactor::eZero, 
        .colorBlendOp = vk::BlendOp::eAdd, 
        .srcAlphaBlendFactor = vk::BlendFactor::eOne, 
        .dstAlphaBlendFactor = vk::BlendFactor::eZero, 
        .alphaBlendOp = vk::BlendOp::eAdd, 
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
        .logicOpEnable = false,
        .logicOp = vk::LogicOp::eOr,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
    };

    vk::PipelineLayout layout;

    if(info.layoutIds.has_value())
    {
        auto layouts = descriptorManager->getDSLayouts(info.layoutIds.value());
        vk::PipelineLayoutCreateInfo layoutCreateInfo{
            .setLayoutCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data(),
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
        };
        layout = device.createPipelineLayout(layoutCreateInfo);
    }
    else
    {
        vk::PipelineLayoutCreateInfo layoutCreateInfo{
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
        };
        layout = device.createPipelineLayout(layoutCreateInfo);
    }

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = (info.depthStencilStateCreateInfo.has_value())? &info.depthStencilStateCreateInfo.value() : NULL,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = nullptr,
        .layout = layout,
        .renderPass = info.renderPass,
        .subpass = 0,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = -1,
    };

    static PipelineID id = 0;
    auto pipeline = device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
    pipelines[id] = pipeline.value;
    layouts[id] = layout;
    return id++;
}

vk::Pipeline PipelineManager::getPipeline(PipelineID id)
{
    return pipelines[id];
}

vk::PipelineLayout PipelineManager::getPipelineLayout(PipelineID id)
{
    return layouts[id];
}

std::vector<char> PipelineManager::readFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Erro ao ler arquivo");
    }
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;    
}

vk::ShaderModule PipelineManager::compileShaderModule(const std::vector<char> &code)
{
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = static_cast<uint32_t>(code.size()),
        .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };
    return device.createShaderModule(createInfo);
}


/*############################## Debug functions ######################################*/
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
    static long debugMessages = 1;

    std::string icon;
    std::string color;
    std::string name; 
    if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        icon = "🗣️";
        color = "36";
        name = "Verbose";
    }
    else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        icon = "🛈";
        color = "34";
        name = "Info";
    }
    else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        icon = "⚠️";
        color = "35";
        name = "Warning";
    }
    else if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        icon = "🆘";
        color = "31";
        name = "Error";
    }

    std::cerr << "\033[" << color << ";1;4m" << icon << " " << debugMessages++ << " - " << name << "\033[0m " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

/*############################## Render Backend methods #############################*/
// temps
PipelineID pipelineid;
BufferId uniformBufferId;
DSId descriptor;

RenderBackend::RenderBackend(common::Window *window)
{
    //======== Vulkan Initialization ========
    //Vulkan Init
    vk::ApplicationInfo applicationInfo {
        .pApplicationName = "myen",
        .applicationVersion = 1,
        .pEngineName = "myen",
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION_1_0,
    };

    //Debug EXT & Validation
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        .pfnUserCallback = debugCallback,
        .pUserData = nullptr,
    };

    //Instance Creation
    if (!checkValidationLayerSupport()){
        std::cout << "ERROR: NO SUPPORT FOR VALIDATION LAYERS" << std::endl;
    }
    std::vector<const char*> extensionNames = window->getRequiredVulkanExtensions();
    extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    vk::InstanceCreateInfo instanceCreateInfo{
        .pNext                   = &debugCreateInfo,
        .pApplicationInfo        = &applicationInfo,
        .enabledLayerCount       = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames     = validationLayers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data(),
    };
    instance = vk::createInstance(instanceCreateInfo);
    //Debug config
    VkDebugUtilsMessengerEXT debugMessenger;
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfoAux = VkDebugUtilsMessengerCreateInfoEXT(debugCreateInfo);
    if (CreateDebugUtilsMessengerEXT(VkInstance(instance), &debugCreateInfoAux, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    } 

    //Surface
    vk::SurfaceKHR surface = window->createSurface(instance);

    //Physical Devices
    auto physicalDevice = selectPhysicalDevice(instance, extensionNames);

    //Queues & Device
    std::optional<uint32_t> graphicsFamilyId;
    std::optional<uint32_t> presentFamilyId;
    selectQueueFamilies(physicalDevice, surface, graphicsFamilyId, presentFamilyId);

    /*
      Vulkan doesn't allow for queueCreateInfos to have the same queueFamily,
      it's only allowed one queueCreateInfo for queueFamily.
      On my machine present and graphics capabilities are on the same family,
      so this code doesn't really work.
      What I need to do is create a way where there is some sort of "coalescing"
      so that if graphics and present are on the same family only one queueCreateInfo
      with 2 queueCount. But if they are different there should be 2 queueCreateInfo
      with one queue each.
     */ /*
    std::vector<uint32_t> uniqueQueueFamilies = {graphicsFamilyId.value(), presentFamilyId.value()};
    float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo queueCreateInfo{
        .queueFamilyIndex = queueFamily,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority,
    };
        queueCreateInfos.push_back(queueCreateInfo);
    }
    */
    float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{
    {
        .queueFamilyIndex = graphicsFamilyId.value(),
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority,
    }
    };

    std::vector<const char *> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    vk::PhysicalDeviceFeatures physicalDeviceFeatures{
    .samplerAnisotropy = true,
    };
    vk::DeviceCreateInfo deviceCreateInfo{
    .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
    .pQueueCreateInfos       = queueCreateInfos.data(),
    .enabledLayerCount       = static_cast<uint32_t>(validationLayers.size()),
    .ppEnabledLayerNames     = validationLayers.data(),
    .enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size()),
    .ppEnabledExtensionNames = deviceExtensions.data(),
    .pEnabledFeatures        = &physicalDeviceFeatures,
    };
    device = physicalDevice.createDevice(deviceCreateInfo);

    graphicsQueue = device.getQueue(graphicsFamilyId.value(), 0);
    presentQueue  = device.getQueue(presentFamilyId.value(), 0);

    //Commands
    commands = new Commands(device, graphicsQueue, graphicsFamilyId.value(), 2);
    commandBuffers = commands->allocateCommandBuffers(2); //TODO change to # of in flight frames

    //Resource Manager
    resourceManager = new ResourceManager(device, physicalDevice, commands);

    //SwapChain
    auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
    vk::SurfaceFormatKHR surfaceFormat = surfaceFormats[0];
    for (const auto& availableFormat : surfaceFormats)
    {
        if(availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear)
        {
            surfaceFormat = availableFormat;
        }
    }

    auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eMailbox; //Hard coded, could backfire

    auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface); //Useful for getting max and min extent + minImagecount
    surfaceSize = window->getSurfaceSize();
    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
    .surface = surface,
    .minImageCount = numberFramesInFlight,
    .imageFormat = surfaceFormat.format,
    .imageColorSpace = surfaceFormat.colorSpace,
    .imageExtent = surfaceSize,
    .imageArrayLayers = 1,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
    .preTransform = surfaceCapabilities.currentTransform,
    .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
    .presentMode = presentMode,
    .clipped = true,
    };
    std::vector<uint32_t> queueFamilyIndices = {graphicsFamilyId.value(), presentFamilyId.value()};
    if(graphicsFamilyId.value() != presentFamilyId.value()){
    swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    swapchainCreateInfo.queueFamilyIndexCount = queueFamilyIndices.size();
    swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else{
    swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    swapchain = device.createSwapchainKHR(swapchainCreateInfo);
    auto swapchainImages = device.getSwapchainImagesKHR(swapchain);
    std::vector<vk::ImageView> swapChainImageViews = std::vector<vk::ImageView>();
    swapChainImageViews.reserve(swapchainImages.size());
    for(const auto& image : swapchainImages)
    {
    vk::ImageViewCreateInfo imageViewCreateInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = surfaceFormat.format,
        .components = {
        .r = vk::ComponentSwizzle::eIdentity,
        .g = vk::ComponentSwizzle::eIdentity,
        .b = vk::ComponentSwizzle::eIdentity,
        .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
        }
    };
    auto imageView = device.createImageView(imageViewCreateInfo);
    swapChainImageViews.push_back(imageView);
    }

    //Render pass
    vk::AttachmentDescription colorAttachment{
        .format = surfaceFormat.format,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR,
    };
    vk::AttachmentReference colorAttachmentReference{
    .attachment = 0,
    .layout = vk::ImageLayout::eColorAttachmentOptimal,
    };

    auto depthFormat = vk::Format::eD32Sfloat; //hardcoded, can create problems
    vk::AttachmentDescription depthAttachment{
        .format = depthFormat,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };
    vk::AttachmentReference depthAttachmentReference{
    .attachment = 1,
    .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };

    vk::SubpassDescription subpassDescription{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentReference,
        .pDepthStencilAttachment = &depthAttachmentReference,
    };
    vk::SubpassDependency subpassDependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .srcAccessMask = vk::AccessFlagBits::eNone,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
    };
    std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    vk::RenderPassCreateInfo renderpassCreateInfo{
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpassDescription,
        .dependencyCount = 1,
        .pDependencies = &subpassDependency,
    };
    renderPass = device.createRenderPass(renderpassCreateInfo);
    auto depthImageId = resourceManager->createImage(surfaceSize, ImageType::eDepth);
    auto depthImageView = resourceManager->getImageView(depthImageId);
    framebuffers.reserve(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<vk::ImageView, 2> attachments = {
            swapChainImageViews[i],
            depthImageView
        };

    vk::FramebufferCreateInfo framebufferInfo{
        .renderPass = renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width =  surfaceSize.width,
        .height = surfaceSize.height,
        .layers = 1,
    };
    framebuffers.push_back(device.createFramebuffer(framebufferInfo));
    }

    //Sync objects
    vk::FenceCreateInfo fenceCreateInfo{
    .flags = vk::FenceCreateFlagBits::eSignaled,
    };
    vk::SemaphoreCreateInfo semaphoreCreateInfo{};
    for(int i = 0; i < numberFramesInFlight; i++)
    {
    inFlightFences.push_back(device.createFence(fenceCreateInfo));
    imageAvailableSemaphores.push_back(device.createSemaphore(semaphoreCreateInfo));
    renderFinishedSemaphores.push_back(device.createSemaphore(semaphoreCreateInfo));
    }
    //####### Vulkan Initialization #######

    std::vector<float> points = {
        -0.8f,  0.8f, 0.0f, 1.0f,//0
        -0.8f, -0.8f, 0.0f, 0.0f,//1
         0.8f,  0.8f, 1.0f, 1.0f,//2

         0.8f, -0.8f, 1.0f, 0.0f,//3 1 2
    };

    std::vector<uint32_t> indices = {
        0, 1, 2, 3, 2, 1 
    };

    auto stageBuffer = resourceManager->createBuffer(BufferType::eStageBuffer, sizeof(float) * points.size());
    resourceManager->insertDataBuffer(stageBuffer, sizeof(float) * points.size(), points.data());
    vertexBuffer = resourceManager->createBuffer(BufferType::eVertexBuffer, sizeof(float) * points.size());
    resourceManager->copyBuffers(stageBuffer, vertexBuffer, sizeof(float) * points.size());

    indexBuffer = resourceManager->createBuffer(BufferType::eIndexBuffer, sizeof(uint32_t) * indices.size());
    resourceManager->insertDataBuffer(stageBuffer, sizeof(uint32_t) * indices.size(), indices.data());
    resourceManager->copyBuffers(stageBuffer, indexBuffer, sizeof(uint32_t) * indices.size());

    int x,
        y,
        channels;
    auto file_data = stbi_load("/home/orvergon/myen/assets/textures/simple_texture.png", &x, &y, &channels, STBI_rgb_alpha);
    auto size_data = x * y * channels;
    auto stageBuffer3 = resourceManager->createBuffer(BufferType::eStageBuffer, size_data);
    resourceManager->insertDataBuffer(stageBuffer3, size_data, file_data);

    auto extent = vk::Extent2D{
        .width = static_cast<uint32_t>(x),
        .height = static_cast<uint32_t>(y)
    };
    auto image = resourceManager->createImage(extent, ImageType::eTexture);
    resourceManager->copyBufferToImage(stageBuffer3, image, extent);
    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = true,
        .maxAnisotropy = 4,
        .compareEnable = false,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = false,
    };
    auto sampler = device.createSampler(samplerInfo);

    //Descriptor manager test
    descriptorManager = new DescriptorManager(device);
    auto layout = descriptorManager->CreateLayout({
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eAll,
        },
        vk::DescriptorSetLayoutBinding {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eAll,
            .pImmutableSamplers = &sampler,
        }
    });
    descriptorManager->preAllocateDescriptorSets(layout, 100);

    pipelineManager = new PipelineManager(device, descriptorManager);
    pipelineid = pipelineManager->CreatePipeline({
        .vertexShaderPath   = "/home/orvergon/myen/assets/default-shaders/vert",
        .fragmentShaderPath = "/home/orvergon/myen/assets/default-shaders/frag",
        .vertexBinds = std::vector<vk::VertexInputBindingDescription>{
            vk::VertexInputBindingDescription{
                .binding = 0,
                .stride = static_cast<uint32_t>(sizeof(float) * 4),
                .inputRate = vk::VertexInputRate::eVertex,
            }
        },
        .vertexAttribs = std::vector<vk::VertexInputAttributeDescription>{
            vk::VertexInputAttributeDescription{
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = 0,
            },
            vk::VertexInputAttributeDescription{
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = sizeof(float) * 2,
            }
        },
        .depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo{
            .depthTestEnable = true,
            .depthWriteEnable = true,
            .depthCompareOp = vk::CompareOp::eLess,
            .depthBoundsTestEnable = false,
            .stencilTestEnable = false,
            .front = {},
            .back = {},
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        },
        .renderPass = renderPass,
        .layoutIds = std::vector<DSLayoutId> {layout},
    });

    std::vector<glm::vec3> descriptorBuffer = { glm::vec3{1.0f, 1.0f, 1.0f} };
    auto stageBuffer2 = resourceManager->createBuffer(BufferType::eUniformBuffer, sizeof(glm::vec3) * descriptorBuffer.size());
    resourceManager->insertDataBuffer(stageBuffer2, sizeof(glm::vec3) * descriptorBuffer.size(), descriptorBuffer.data());
    descriptor = descriptorManager->writeDS(layout, std::vector<WriteDescriptorInfo> {
        WriteDescriptorInfo{
            .bufferInfo = vk::DescriptorBufferInfo{
                .buffer = resourceManager->getBuffer(stageBuffer2),
                .offset = 0,
                .range = sizeof(glm::vec3) * descriptorBuffer.size(),
            },
        },
        WriteDescriptorInfo{
            .imageInfo = vk::DescriptorImageInfo{
                .sampler = sampler,
                .imageView = resourceManager->getImageView(image),
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            }
        },
    });
}

RenderBackend::~RenderBackend()
{
    instance.destroy();
}

void RenderBackend::drawFrame()
{
    //############# <frame render boilerplate> ###############
    short frame = this->mFrame%2; //TODO hardcoded frame in flights could be messy
    this->mFrame++;
    auto waitValue = device.waitForFences(inFlightFences[frame], false, UINT64_MAX); //Should I check this?
    device.resetFences(std::vector<vk::Fence>{inFlightFences[frame]});

    auto imageIndex = device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphores[frame]).value;
    auto commandBuffer = commands->beginCommand(commandBuffers[frame]);

    std::vector<vk::ClearValue> clearValues{
        vk::ClearValue{.color = {std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}}},
        vk::ClearValue{.depthStencil = {1.0f, 0}}
    };
    vk::RenderPassBeginInfo renderPassInfo{
        .renderPass = renderPass,
        .framebuffer = framebuffers[frame],
        .renderArea = vk::Rect2D{
            .offset = {0, 0},
            .extent = {surfaceSize}, 
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    vk::Viewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(surfaceSize.width),
        .height = static_cast<float>(surfaceSize.height),
        .minDepth = 0.0f,
        .maxDepth = 0.0f,
    };
    commandBuffer.setViewport(0, 1, &viewport);
    vk::Rect2D sissor{
        .offset = {0, 0},
        .extent = surfaceSize,
    };
    commandBuffer.setScissor(0, 1, &sissor);
    //############# </frame render boilerplate> ###############

    std::vector<vk::Buffer> buffers{resourceManager->getBuffer(vertexBuffer)};
    std::vector<vk::DeviceSize> offsets{vk::DeviceSize(0)};
    commandBuffer.bindVertexBuffers(0, buffers, offsets);
    commandBuffer.bindIndexBuffer(resourceManager->getBuffer(indexBuffer), vk::DeviceSize(0), vk::IndexType::eUint32);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelineManager->getPipeline(pipelineid)); //TODO: Create the Pipeline
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                     pipelineManager->getPipelineLayout(pipelineid),
                     0,
                     std::vector<vk::DescriptorSet>{descriptorManager->getDS(descriptor)}, nullptr);
    commandBuffer.drawIndexed(6, 1, 0, 0, 0);
    commandBuffer.endRenderPass();

    std::vector<vk::Semaphore> renderFinishedSemaphores = {this->renderFinishedSemaphores[frame]};
    commands->endCommand(commandBuffer,
             std::vector<vk::Semaphore>{imageAvailableSemaphores[frame]},
             std::vector<vk::PipelineStageFlags> {vk::PipelineStageFlagBits::eColorAttachmentOutput},
             renderFinishedSemaphores,
             inFlightFences[frame]);

    vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = static_cast<uint32_t>(renderFinishedSemaphores.size()),
        .pWaitSemaphores = renderFinishedSemaphores.data(),
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
    };
    //Don't know what to do with this result tho.
    auto result = presentQueue.presentKHR(presentInfo);
}

}

