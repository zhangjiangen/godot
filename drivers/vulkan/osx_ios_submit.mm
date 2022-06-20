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
