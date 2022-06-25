#include "osx_ios_submit.h"
VkResult osx_ios_submit::Submit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{  
    VkResult err;

    @autoreleasepool
    {
      err = vkQueueSubmit(queue, submitCount, pSubmits, fence);
    }
    return err;
}
VkResult osx_ios_submit::CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass)
{
    VkResult err;

    @autoreleasepool
    {
      err = vkCreateRenderPass(device, pCreateInfo,pAllocator, pRenderPass);
    }
    return err;

}
VkResult osx_ios_submit::CreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView)
{
    VkResult err;

    @autoreleasepool
    {
      err = vkCreateImageView(device, pCreateInfo,pAllocator, pView);
    }
    return err;

}
VkResult osx_ios_submit::CreateBuffer(VmaAllocator allocator, const VkBufferCreateInfo *pBufferCreateInfo, const VmaAllocationCreateInfo *pAllocationCreateInfo, VkBuffer *pBuffer, VmaAllocation *pAllocation, VmaAllocationInfo *pAllocationInfo)
{
    VkResult err;

    @autoreleasepool
    {
      err = vmaCreateBuffer(allocator, pBufferCreateInfo,pAllocationCreateInfo, pBuffer,pAllocation,pAllocationInfo);
    }
    return err;
  
}
	void osx_ios_submit::DestroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
  {

    @autoreleasepool
    {
      vmaDestroyBuffer(allocator, buffer,allocation);
    }

  }
VkResult osx_ios_submit::MapMemory(VmaAllocator allocator, VmaAllocation allocation, void **ppData)
{
    VkResult err;

    @autoreleasepool
    {
        err = vmaMapMemory(allocator, allocation,ppData);
    }
    return err;

}
VkResult osx_ios_submit::CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,const VkGraphicsPipelineCreateInfo *pCreateInfos,const VkAllocationCallbacks *pAllocator,VkPipeline *pPipelines)
{
  VkResult err;

  @autoreleasepool
  {
      err = vkCreateGraphicsPipelines(device, pipelineCache,createInfoCount,pCreateInfos,pAllocator,pPipelines);
  }
  return err;

}
VkResult osx_ios_submit::CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines)
  {
    VkResult err;

    @autoreleasepool
    {
      err = vkCreateComputePipelines(device, pipelineCache,createInfoCount,pCreateInfos,pAllocator,pPipelines);
    }
    return err;
  }

VkResult osx_ios_submit::CreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool)
{
    VkResult err;

    @autoreleasepool
    {
        err = vkCreateDescriptorPool(device, pCreateInfo,pAllocator,pDescriptorPool);
    }
    return err;

}
void osx_ios_submit::DestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator)
{

  @autoreleasepool
  {
      vkDestroyDescriptorPool(device, descriptorPool,pAllocator);
  }

}
VkResult osx_ios_submit::AllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo, VkDescriptorSet *pDescriptorSets)
{
  VkResult err;

  @autoreleasepool
  {
      err = vkAllocateDescriptorSets(device, pAllocateInfo,pDescriptorSets);
  }
  return err;

}
void osx_ios_submit::UpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies)
{

  @autoreleasepool
  {
      vkUpdateDescriptorSets(device, descriptorWriteCount,pDescriptorWrites,descriptorCopyCount,pDescriptorCopies);
  }

}

VkResult osx_ios_submit::CreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule)
{
  VkResult err;

  @autoreleasepool
  {
      err = vkCreateShaderModule(device, pCreateInfo,pAllocator,pShaderModule);
  }
  return err;
  
}
void osx_ios_submit::DestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks *pAllocator)
{

    @autoreleasepool
    {
      vkDestroyShaderModule(device, shaderModule,pAllocator);
    }

}

