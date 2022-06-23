
#ifndef OSX_IOS_SUBMIT_H
#define OSX_IOS_SUBMIT_H
#ifdef USE_VOLK
#include <volk.h>
#else
#include <vulkan/vulkan.h>
#endif
class osx_ios_submit {
public:
	static VkResult Submit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence);
};

#endif