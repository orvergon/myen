/*************************************************************************************
 * @file renderBackend.hpp
 * @brief Declaration for all backend rendering classes.
 ************************************************************************************/

#pragma once

#include <cstdint>
#include <functional>
#include <sys/types.h>
#include <unordered_map>
#include <vector>
#include <stack>
#include <optional>
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "common/common.hpp"

namespace RenderBackend {

enum BufferType
{
    eVertexBuffer,
    eIndexBuffer,
    eStageBuffer,
    eUniformBuffer,
};

enum ImageType
{
    eDepth,
    eTexture,
};

typedef uint64_t BufferId;
typedef uint64_t ImageId;

class Commands
{
public:
    Commands(vk::Device device,vk::Queue queue, int queueFamilyId, int nPools);
    void setPool(int poolNumber);
    vk::CommandBuffer BeginSingleTimeCommand();
    void EndSingleTimeCommand(vk::CommandBuffer commandBuffer, bool wait);
    std::vector<BufferId> allocateCommandBuffers(int noBuffers);
    vk::CommandBuffer beginCommand(BufferId bufferId);
    void endCommand(vk::CommandBuffer buffer,
		    std::vector<vk::Semaphore> waitSemaphores,
		    std::vector<vk::PipelineStageFlags> waitStages,
		    std::vector<vk::Semaphore> signalSemaphores,
		    vk::Fence fence);

private:
    vk::Device device;
    vk::Queue queue;
    int queueFamilyId;
    std::vector<vk::CommandPool> pools;
    vk::CommandPool pool;
    std::unordered_map<BufferId, vk::CommandBuffer> buffers;
};


class ResourceManager
{
public:
    ResourceManager(vk::Device device,
		    vk::PhysicalDevice physicalDevice,
		    Commands* commands);
    ~ResourceManager();
    BufferId createBuffer(BufferType type, vk::DeviceSize size);
    void insertDataBuffer(BufferId id, vk::DeviceSize size, void* data);
    void copyBuffers(BufferId source, BufferId destination, vk::DeviceSize size);
    vk::Buffer getBuffer(BufferId id);

    ImageId createImage(vk::Extent2D size, ImageType type);
    void transitionImage(ImageId imageId, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void copyBufferToImage(BufferId bufferId, ImageId imageId, vk::Extent2D size);
    vk::ImageView getImageView(ImageId imageId);
    
private:
    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    Commands* commands;

    std::unordered_map<BufferId, vk::Buffer> buffers;
    std::unordered_map<BufferId, vk::DeviceMemory> bufferMemories;
    std::unordered_map<BufferId, vk::DeviceSize> bufferSizes;
    std::unordered_map<ImageId, vk::DeviceMemory> imageMemories;
    std::unordered_map<ImageId, vk::Image> images;
    std::unordered_map<ImageId, vk::ImageView> imageViews;
};


typedef uint64_t DSLayoutId;
typedef uint64_t DSId;

struct WriteDescriptorInfo{
    std::optional<vk::DescriptorBufferInfo> bufferInfo;
    std::optional<vk::DescriptorImageInfo> imageInfo;
};

class DescriptorManager
{
public:
    //TODO: Devia ter um helper pra criar o buffer e inserir logo o dado de uma vez.
    //TODO: check brainstorm about typed buffers.
    DescriptorManager(vk::Device device);
    DSLayoutId CreateLayout(std::vector<vk::DescriptorSetLayoutBinding> bindings);
    vk::DescriptorSetLayout getDSLayout(DSLayoutId ids);
    std::vector<vk::DescriptorSetLayout> getDSLayouts(std::vector<DSLayoutId> id);
    void preAllocateDescriptorSets(DSLayoutId id, uint32_t noSets);
    DSId getFreeDS(DSLayoutId id);
    DSId writeDS(DSLayoutId id, std::vector<WriteDescriptorInfo> writeInfos);
    void updateDS(DSId id, std::vector<WriteDescriptorInfo> writeInfos);
    void freeDS(DSId id);
    vk::DescriptorSet getDS(DSId id);
    vk::DescriptorPool getDescriptorPool();

private:
    struct DescriptorSet{
	DSId id;
	DSLayoutId layoutId;
	vk::DescriptorSet descriptorSet;
    };
    struct DescriptorSetLayout{
	DSLayoutId layoutId;
	vk::DescriptorSetLayout DSLayout;
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
    };

    vk::Device device;
    vk::DescriptorPool pool;
    std::unordered_map<DSLayoutId, DescriptorSetLayout> layouts;

    /*
      get descriptor by id
      get descriptors by layout -> pra deletar (tudo bem ser lento em um loop não é sempre que deleta descriptors)
      get free descriptors by layout -> pra usar tem que ser mega rápido
     */
    std::unordered_map<DSId, DescriptorSet> descriptors;
    std::unordered_map<DSLayoutId, std::stack<DescriptorSet>> freeDescriptorsByLayout;
};


typedef uint64_t PipelineID;
typedef uint64_t PipelineLayoutID;

class PipelineManager
{
public:
    struct PipelineInfo{
	std::string vertexShaderPath;
	std::string fragmentShaderPath;
	//this could be generated by analising the shader tho.
	std::vector<vk::VertexInputBindingDescription> vertexBinds;
	std::vector<vk::VertexInputAttributeDescription> vertexAttribs;
	std::optional<vk::PipelineDepthStencilStateCreateInfo> depthStencilStateCreateInfo;

	vk::RenderPass renderPass;
	std::optional<std::vector<DSLayoutId>> layoutIds;
    };

    PipelineManager(vk::Device device, DescriptorManager* descriptorManager);
    PipelineID CreatePipeline(PipelineInfo info);
    vk::Pipeline getPipeline(PipelineID id);
    vk::PipelineLayout getPipelineLayout(PipelineID id);

private:
    vk::Device device;
    DescriptorManager* descriptorManager;
    std::unordered_map<PipelineID, vk::Pipeline> pipelines;
    std::unordered_map<PipelineID, vk::PipelineLayout> layouts;

    std::vector<char> readFile(const std::string& filename);
    vk::ShaderModule compileShaderModule(const std::vector<char>& code);
    
};


struct Texture {
    ImageId image;
    vk::ImageView imageView;
    vk::Sampler sampler;
};


typedef uint64_t MeshId;
struct Mesh {
    BufferId vertexBufferId;
    BufferId indexBufferId;
    uint64_t vertexCount;
    uint64_t indexCount;
};


typedef uint64_t ModelId;
struct Model {
    ModelId id;
    MeshId meshId;
    glm::vec3 position;
    glm::vec3 rotation;

    Texture* texture;

    PipelineID pipeline;
    
    DSId descriptors[2];
    BufferId uniformBuffers[2];
};


class RenderBackend
{
public:
    RenderBackend(common::Window* window, common::Camera* camera);
    ~RenderBackend();

    void drawFrame();
    MeshId addMesh(common::Mesh* mesh);
    ModelId addModel(MeshId mesh,
                 glm::vec3 position,
                 glm::vec3 rotation,
                 common::Texture* texture);
    void updateModelPosition(ModelId model, glm::vec3 position);
    void addUICommands(std::string windowName, std::function<void(void)> function);
    
    common::Camera* camera; //should this be a pointer?
private:
    vk::Instance instance;
    vk::Device device;
    vk::SwapchainKHR swapchain;
    
    common::Window* window;
    ResourceManager* resourceManager;
    Commands* commands;
    PipelineManager* pipelineManager;
    DescriptorManager* descriptorManager;
    short mFrame = 0;

    std::vector<vk::Fence> inFlightFences;
    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<BufferId> commandBuffers;
    std::unordered_map<MeshId, Mesh> meshes;
    std::unordered_map<ModelId, Model> models;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
    vk::RenderPass renderPass;
    std::vector<vk::Framebuffer> framebuffers;
    std::vector<std::function<void(void)>> functions;
    BufferId vertexBuffer;
    BufferId indexBuffer;
};

}

