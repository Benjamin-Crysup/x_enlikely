#ifndef WHODUN_VULKAN_H
#define WHODUN_VULKAN_H 1

/**
 * @file
 * @brief Basic interactions with the gpu.
 */

#include <vulkan/vulkan.h>

#include <map>
#include <set>
#include <string>
#include <vector>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <stdexcept>

#include "whodun_ermac.h"
#include "whodun_oshook.h"

namespace whodun {

/******************************************************************/
/*Data                                                            */
/******************************************************************/

/**Get information on the layers that can be in play.*/
class VulkanLayerSet{
public:
	/**Query vulkan for the layers.*/
	VulkanLayerSet();
	/**Clean up.*/
	~VulkanLayerSet();
	
	/**
	 * Dump out the validation layer info.
	 * @param toDump The place to write.
	 */
	void print(OutStream* toDump);
	
	/**Info on the layers.*/
	std::vector<VkLayerProperties> allLayers;
	/**Map from name to index.*/
	std::map<std::string,uintptr_t> layerNameMap;
};

/**Information on a device.*/
class VulkanDeviceData{
public:
	/**Set up some empty data.*/
	VulkanDeviceData();
	/**Clean up.*/
	~VulkanDeviceData();
	
	/**
	 * Dump out the device info.
	 * @param toDump The place to write.
	 */
	void print(OutStream* toDump);
	
	/**
	 * Check that this device supports some set of features.
	 * @param wantFeatures The desired features.
	 * @return Whether this device supports all requested features.
	 */
	int checkFeatures(VkPhysicalDeviceFeatures* wantFeatures);
	
	/**The raw data on the device.*/
	VkPhysicalDevice rawData;
	/**The raw properties of the device.*/
	VkPhysicalDeviceProperties rawProperties;
	/**The raw features of the device.*/
	VkPhysicalDeviceFeatures rawFeatures;
	
	/**The queues this thing supports.*/
	std::vector<VkQueueFamilyProperties> rawQueues;
	/**The memory flavors and quantities.*/
	VkPhysicalDeviceMemoryProperties rawMemory;
};

/**An interface to Vulkan.*/
class VulkanInstance{
public:
	/**
	 * Set up an uninitialized instance.
	 * @param applicationName The name of this application.
	 */
	VulkanInstance(const char* applicationName);
	/**Clean up*/
	~VulkanInstance();
	
	/**
	 * Actually create the instance.
	 * @param toWarn The place to write any warnings.
	 */
	void create();
	
	/**The place to write warnings, if any.*/
	ErrorLog* toWarn;
	/**The application name.*/
	const char* appName;
	/**The desired validation layers.*/
	std::vector<std::string> wantLayers;
	
	/**Whether an instance has been created.*/
	int isHot;
	/**The actual created instance.*/
	VkInstance theHot;
	
	/**Information on the installed devices.*/
	std::vector<VulkanDeviceData> hotDevs;
};

/******************************************************************/
/*Grab a device                                                   */
/******************************************************************/

class VulkanDevice;

/**A list of requirements for using a device.*/
class VulkanDeviceRequirements{
public:
	/**Set up a blank set of requirements.*/
	VulkanDeviceRequirements();
	/**Clean up.*/
	~VulkanDeviceRequirements();
	
	/**The desired features of the device.*/
	VkPhysicalDeviceFeatures wantFeatures;
	/**The required features of the device. Should be a subset of wantFeatures.*/
	VkPhysicalDeviceFeatures reqFeatures;
	
	/**
	 * Add a queue requirement.
	 * @param queueType The type(s) of queue needed.
	 * @param dodgeQueue The types of queue to try to avoid.
	 */
	void addQueueRequirement(uintptr_t queueType, uintptr_t dodgeQueue);
	
	/**The types of queues this requires.*/
	std::vector<uintptr_t> queueReqFlags;
	/**The corresponding types that are discouraged.*/
	std::vector<uintptr_t> queueNwantFlags;
	
	/**
	 * Add a memory requirement.
	 * @param wantMem The desired memory properties.
	 * @param dodgeMem Memory properties to try to avoid.
	 * @param needMem Required memory properties: must be a subset of wantMem.
	 * @param hateMem Memory properties to rule out: must be a subset of dodgeMem.
	 * @param size The required size.
	 */
	void addMemoryRequirement(uintptr_t wantMem, uintptr_t dodgeMem, uintptr_t needMem, uintptr_t hateMem, uintptr_t size);
	
	/**The types of memory this would like.*/
	std::vector<uintptr_t> memoryWantFlags;
	/**The types of memory this would like to not have.*/
	std::vector<uintptr_t> memoryNwantFlags;
	/**The types of memory this requires.*/
	std::vector<uintptr_t> memoryReqFlags;
	/**The corresponding types that are disallowed.*/
	std::vector<uintptr_t> memoryNreqFlags;
	/**The required size of each requested piece of memory.*/
	std::vector<uintptr_t> memorySizes;
};

/**A map from requirements to actual resources.*/
class VulkanDeviceRequirementMap{
public:
	/**
	 * Figure out where the requirements should be assigned in the device.
	 * @param forReq The requirements.
	 * @param forDev The device in question.
	 */
	VulkanDeviceRequirementMap(VulkanDeviceRequirements* forReq, VulkanDeviceData* forDev);
	/**Clean up.*/
	~VulkanDeviceRequirementMap();
	
	/**Whether the device can do the thing.*/
	int deviceAdequate;
	
	/**Whether all wanted features are present.*/
	int featuresExcellent;
	/**Whether all required features are present.*/
	int featuresAdequate;
	
	/**The number of queue types that did not have discouraged flags.*/
	uintptr_t numQueueExcellent;
	/**The number of queue types that are sufficient to the task.*/
	uintptr_t numQueueAdequate;
	/**The family assignments for each requested queue.*/
	std::vector<uintptr_t> queueAssignments;
	
	/**The number of memory types that did not have discouraged flags.*/
	uintptr_t numMemoryExcellent;
	/**The number of memory types that are sufficient to the task.*/
	uintptr_t numMemoryAdequate;
	/**The family assignments for each requested memory.*/
	std::vector<uintptr_t> memoryAssignments;
};

/******************************************************************/
/*Jobs and syncing                                                */
/******************************************************************/

/**A way to synchronize between the cpu and gpu.*/
class VulkanSyncFence{
public:
	/**
	 * Create an unset fence.
	 * @param forDev The device to create for.
	 */
	VulkanSyncFence(VulkanDevice* forDev);
	/**Clean up*/
	~VulkanSyncFence();
	
	/**Reset this fence.*/
	void reset();
	/**Wait for this fence to be set.*/
	void wait();
	
	/**The actual fence handle.*/
	VkFence theFence;
	/**Save the device this is for.*/
	VulkanDevice* saveDev;
};

/**A way to synchronize between queues.*/
class VulkanSyncSemaphore{
public:
	/**
	 * Create a semaphore.
	 * @param forDev The device to create for.
	 */
	VulkanSyncSemaphore(VulkanDevice* forDev);
	/**Clean up*/
	~VulkanSyncSemaphore();
	
	/**The actual semaphore handle.*/
	VkSemaphore theSem;
	/**Save the device this is for.*/
	VulkanDevice* saveDev;
};

/**A way to synchronize within a queue (remember that caches still need flushing/invalidating via vkCmdPipelineBarrier).*/
class VulkanSyncEvent{
public:
	/**
	 * Create an event.
	 * @param forDev The device to create for.
	 */
	VulkanSyncEvent(VulkanDevice* forDev);
	/**Clean up*/
	~VulkanSyncEvent();
	
	/**The actual event handle.*/
	VkEvent theEvent;
	/**Save the device this is for.*/
	VulkanDevice* saveDev;
};

class VulkanCommandList;

/**A queue for a hot device.*/
class VulkanDeviceQueue{
public:
	/**Set up an empty queue.*/
	VulkanDeviceQueue();
	/**Clean up.*/
	~VulkanDeviceQueue();
	
	/**
	 * Put something on the queue.
	 * @param toRun The thing to run.
	 */
	void submit(VulkanCommandList* toRun);
	
	/**
	 * Put something on the queue.
	 * @param toRun The thing to run.
	 * @param toWarn The thing to signal when done.
	 */
	void submit(VulkanCommandList* toRun, VulkanSyncFence* toWarn);
	
	/**
	 * Put something on the queue.
	 * @param toRun The thing to run.
	 * @param toWarn The cpu thing to signal when done.
	 * @param numWait The number of gpu things to wait on.
	 * @param toWait The things to wait on.
	 * @param waitStages The pipeline stages they wait on.
	 * @param numSig The number of gpu things to signal when done.
	 * @param toSig The things to signal.
	 */
	void submit(VulkanCommandList* toRun, VulkanSyncFence* toWarn, uintptr_t numWait, VulkanSyncSemaphore** toWait, uintptr_t* waitStages, uintptr_t numSig, VulkanSyncSemaphore** toSig);
	
	/**
	 * Wait for the queue to empty.
	 */
	void waitEmpty();
	
	/**The device this is for.*/
	VulkanDevice* hotDev;
	/**The family this queue is for.*/
	uintptr_t queueFamily;
	/**The queue in question*/
	VkQueue hotQueue;
};

/**A place to allocate command lists from*/
class VulkanCommandPool{
public:
	/**
	 * Set up a pool for a queue.
	 * @param forQueue The queue this is for.
	 * @param poolFlags Flags for the pool.
	 */
	VulkanCommandPool(VulkanDeviceQueue* forQueue, uintptr_t poolFlags);
	/**Clean up.*/
	~VulkanCommandPool();
	
	/**Reset all active command buffers: useful if you cannot individually reset.*/
	void reset();
	
	/**The queue this was for.*/
	VulkanDeviceQueue* origQueue;
	/**The actual command pool.*/
	VkCommandPool longPool;
};


/**A set of commands waiting to be submitted.*/
class VulkanCommandList{
public:
	/**
	 * Allocate a new list of commands.
	 * @param fromPool The pool to allocate from.
	 */
	VulkanCommandList(VulkanCommandPool* fromPool);
	/**Kill the list.*/
	~VulkanCommandList();
	
	/**
	 * Prepare to add commands to the list (vkBeginCommandBuffer).
	 * @param oneTime Whether this new spate will be used once.
	 */
	void begin(int oneTime);
	/**End adding submissions to the list (vkEndCommandBuffer).*/
	void compile();
	
	/**Reset this command buffer.*/
	void reset();
	
	/**The pool this was allocated from.*/
	VulkanCommandPool* origPool;
	/**The commands.*/
	VkCommandBuffer theComs;
};

/******************************************************************/
/*Memory                                                          */
/******************************************************************/

/**Malloc memory.*/
class VulkanMalloc{
public:
	/**
	 * Allocate some memory.
	 * @param onDev The device to allocate it on.
	 * @param memTypeI The type of memory to create.
	 * @param allocSize The amount of memory to allocate.
	 */
	VulkanMalloc(VulkanDevice* onDev, uintptr_t memTypeI, uintptr_t allocSize);
	/**Free the memory.*/
	~VulkanMalloc();
	
	/**
	 * Map this chunk into cpu address space.
	 * @return The mapped chunk.
	 */
	void* map();
	
	/**
	 * Call after the cpu has written to this chunk to make visible to gpus.
	 */
	void mappedDataCommit();
	
	/**
	 * Call after gpus have written to this chunk to make visible to cpu.
	 */
	void mappedDataUpdate();
	
	/**
	 * No longer map the chunk.
	 */
	void unmap();
	
	/**Save the memory size.*/
	uintptr_t memSize;
	/**Whether this memory can be mapped (host visible).*/
	int canMap;
	/**Whether this memory needs flushing (host coherrent).*/
	int needFlush;
	/**Save the allocation.*/
	VkDeviceMemory saveAlloc;
	/**Save the device this is for.*/
	VulkanDevice* saveDev;
};

/**A desired usage of memory.*/
class VulkanBuffer{
public:
	/**
	 * Prepare a desired usage (assuming exclusive access amongst queues).
	 * @param forDev The device this will be on.
	 * @param usages How will this buffer be used.
	 * @param size The number of bytes needed.
	 */
	VulkanBuffer(VulkanDevice* forDev, uintptr_t usages, uintptr_t size);
	/**Free willie.*/
	~VulkanBuffer();
	
	/**
	 * Bind the buffer to memory.
	 * @param toMem The memory to target.
	 * @param offset The offset in that memory.
	 */
	void bind(VulkanMalloc* toMem, uintptr_t offset);
	
	/**The requirements for this buffer.*/
	VkMemoryRequirements memData;
	/**Save the device this is for.*/
	VulkanDevice* saveDev;
	/**The buffer.*/
	VkBuffer theBuff;
	/**The memory this is attached to.*/
	VulkanMalloc* onMem;
	/**The offset of the buffer binding in memory.*/
	uintptr_t memOffset;
};

/**Manage some buffers and their mallocs.*/
class VulkanBufferMallocManager{
public:
	/**
	 * Set up an empty manager
	 * @param forDev The device this will be on.
	 */
	VulkanBufferMallocManager(VulkanDevice* forDev);
	/**Destroy all buffers and mallocs.*/
	~VulkanBufferMallocManager();
	
	/**
	 * Add a buffer to manage.
	 * @param toManage The buffer to manage: will be deleted by this.
	 * @param flagWant The flags wanted by the buffer.
	 * @param flagNWant The flags the buffer would like to do without.
	 * @param flagNeed The flags the buffer needs (subset of flagWant).
	 * @param flagNNeed The flags the buffer cannot have (subset of flagNWant).
	 */
	void addBuffer(VulkanBuffer* toManage, uintptr_t flagWant, uintptr_t flagNWant, uintptr_t flagNeed, uintptr_t flagNNeed);
	
	/**
	 * Generate allocations.
	 */
	void allocate();
	
	/**All the buffers this manages.*/
	std::vector<VulkanBuffer*> allBuffs;
	/**Flags the buffers want.*/
	std::vector<uintptr_t> buffMemWT;
	/**Flags the buffers would rather not have.*/
	std::vector<uintptr_t> buffMemWF;
	/**Flags the buffers need.*/
	std::vector<uintptr_t> buffMemNT;
	/**Flags the buffers cannot use.*/
	std::vector<uintptr_t> buffMemNF;
	/**All the memory this manages.*/
	std::vector<VulkanMalloc*> allAlloc;
	/**Save the device this is for.*/
	VulkanDevice* saveDev;
};

/******************************************************************/
/*Programs                                                        */
/******************************************************************/

/**A shader: a collection of compiled routines.*/
class VulkanShader{
public:
	/**
	 * Set up a shader.
	 * @param forDev The device this shader is for.
	 * @param shadLen The number of bytes in the shader payload.
	 * @param shadTxt The shader payload.
	 */
	VulkanShader(VulkanDevice* forDev, uintptr_t shadLen, void* shadTxt);
	/**Free the memory.*/
	~VulkanShader();
	
	/**Save the device this is for.*/
	VulkanDevice* saveDev;
	/**The compiled shader.*/
	VkShaderModule compShad;
};

/**Store a layout for input to a shader.*/
class VulkanShaderDataLayout{
public:
	/**
	 * Set up a layout.
	 * @param forDev The device this is for.
	 * @param numBindings The number of things the shaders get.
	 * @param theBindings Descriptions of the things the shaders get.
	 */
	VulkanShaderDataLayout(VulkanDevice* forDev, uintptr_t numBindings, VkDescriptorSetLayoutBinding* theBindings);
	/**Clean up.*/
	~VulkanShaderDataLayout();
	
	/**Save the device this is for.*/
	VulkanDevice* saveDev;
	/**The compiled layout.*/
	VkDescriptorSetLayout shadLayout;
	/**Save the layout information*/
	std::vector<VkDescriptorSetLayoutBinding> saveInfo;
};

class VulkanShaderDataAssignments;

/**An assignment for a shader.*/
class VulkanShaderDataAssignment{
public:
	/**Create an uninitialized assignment.*/
	VulkanShaderDataAssignment();
	/**Clean up.*/
	~VulkanShaderDataAssignment();
	
	/**
	 * Change one of the references to a new buffer.
	 * @param layoutI The layout to change.
	 * @param usage The type of the layout.
	 * @param toHit The buffer to point it to.
	 */
	void changeBufferBinding(uintptr_t layoutI, VkDescriptorType usage, VulkanBuffer* toHit);
	/**
	 * Change one of the references to a new buffer.
	 * @param layoutI The layout to change.
	 * @param usage The type of the layout.
	 * @param toHit The buffer to point it to.
	 * @param fromI The byte to start at.
	 * @param toI The byte to end at.
	 */
	void changeBufferBinding(uintptr_t layoutI, VkDescriptorType usage, VulkanBuffer* toHit, uintptr_t fromI, uintptr_t toI);
	
	/**The pool these assignments are in.*/
	VulkanShaderDataAssignments* inPool;
	/**The allocated assignment.*/
    VkDescriptorSet hotAss;
};

/**A pool of assignments for shaders.*/
class VulkanShaderDataAssignments{
public:
	/**
	 * Create an assignment pool.
	 * @param forDev The device to create for.
	 * @param numLays The number of layouts to allocate for
	 * @param forLays The layouts to allocate for.
	 */
	VulkanShaderDataAssignments(VulkanDevice* forDev, uintptr_t numLays, VulkanShaderDataLayout** forLays);
	/**Clean up.*/
	~VulkanShaderDataAssignments();
	
	/**Save the device this is for.*/
	VulkanDevice* saveDev;
	/**The allocated pool.*/
	VkDescriptorPool descripPool;
	/**The allocated assignments.*/
	std::vector<VulkanShaderDataAssignment> allocAssns;
};

/**A compute pipeline: link up the shaders.*/
class VulkanComputePipeline{
public:
	/**
	 * Set up an uninitialized compute pipeline.
	 * @param forDev The device to create for.
	 * @param useShad The shader to use.
	 * @param useLay The layout to use.
	 * @param mainFN The entry point to use.
	 */
	VulkanComputePipeline(VulkanDevice* forDev, VulkanShader* useShad, VulkanShaderDataLayout* useLay, const char* mainFN);
	/**Clean up*/
	~VulkanComputePipeline();
	
	/**Save the device this is for.*/
	VulkanDevice* saveDev;
	/**The compiled layouts.*/
	VkPipelineLayout pipeLayout;
	/**The compiled pipeline*/
	VkPipeline thePipe;
};

/******************************************************************/
/*The big one                                                     */
/******************************************************************/

/**A hot device.*/
class VulkanDevice{
public:
	/**Create an uninitialized device.*/
	VulkanDevice();
	/**Give up*/
	~VulkanDevice();
	
	/**The family indices of the queues to create.*/
	std::vector<uintptr_t> queueIndices;
	/**The features needed.*/
	VkPhysicalDeviceFeatures needFeatures;
	
	/**
	 * Start talking to a device.
	 * @param hotVulkan The vulkan instance.
	 * @param devIndex The device index.
	 */
	void create(VulkanInstance* hotVulkan, uintptr_t devIndex);
	
	/**Whether an instance has been created.*/
	int isHot;
	/**The vulkan instance.*/
	VulkanInstance* hotVulk;
	/**Save data on the device.*/
	VulkanDeviceData* devData;
	/**The device handle.*/
	VkDevice hotDev;
	/**The queues.*/
	std::vector<VulkanDeviceQueue> hotQueues;
};

};

#endif

