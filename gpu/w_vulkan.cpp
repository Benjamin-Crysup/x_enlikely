#include "whodun_vulkan.h"

using namespace whodun;

#define WRITE_I(toW) sprintf(asciiBuff, "%ju", (uintmax_t)(toW)); toDump->write(asciiBuff);
#define WRITE_S(toW) toDump->write(toW);
#define WRITE_LN(toW) toDump->write(toW); toDump->write('\n');

/******************************************************************/
/*Data                                                            */
/******************************************************************/

VulkanLayerSet::VulkanLayerSet(){
	uint32_t numValLay = 0;
	if(vkEnumerateInstanceLayerProperties(&numValLay, 0)){ throw WhodunError(WHODUN_ERROR_LEVEL_ERROR, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Could not get layer data.", 0, 0); }
	allLayers.resize(numValLay);
	if(vkEnumerateInstanceLayerProperties(&numValLay, numValLay ? &(allLayers[0]) : (VkLayerProperties*)0)){ throw WhodunError(WHODUN_ERROR_LEVEL_ERROR, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Could not get layer data.", 0, 0); }
	for(uintptr_t i = 0; i<numValLay; i++){
		layerNameMap[allLayers[i].layerName] = i;
	}
}
VulkanLayerSet::~VulkanLayerSet(){}
void VulkanLayerSet::print(OutStream* toDump){
	for(uintptr_t i = 0; i<allLayers.size(); i++){
		WRITE_S(allLayers[i].layerName);
		WRITE_S(" : ");
		WRITE_LN(allLayers[i].description);
	}
}

VulkanDeviceData::VulkanDeviceData(){}
VulkanDeviceData::~VulkanDeviceData(){}
void VulkanDeviceData::print(OutStream* toDump){
	char asciiBuff[8*sizeof(uintmax_t)];
	const char* devTypeList[] = {"Other","GPU (Integrated)","GPU (Discrete)","GPU (virtual)","CPU"};
	WRITE_LN(rawProperties.deviceName);
	if(rawProperties.deviceType < 5){
		WRITE_LN(devTypeList[rawProperties.deviceType]);
	}
	for(uintptr_t i = 0; i<rawQueues.size(); i++){
		WRITE_S("Queue ") WRITE_I(i) WRITE_S(" x") WRITE_I(rawQueues[i].queueCount) WRITE_S(" ")
		if(rawQueues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){ WRITE_S("G"); }
		if(rawQueues[i].queueFlags & VK_QUEUE_COMPUTE_BIT){ WRITE_S("C"); }
		if(rawQueues[i].queueFlags & VK_QUEUE_TRANSFER_BIT){ WRITE_S("t"); }
		if(rawQueues[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT){ WRITE_S("s"); }
		WRITE_S("\n")
	}
	for(uintptr_t i = 0; i<rawMemory.memoryHeapCount; i++){
		if(rawMemory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT){ WRITE_S("Local "); }
		WRITE_S("Heap ");
		WRITE_I(i) WRITE_S(" ") WRITE_I(rawMemory.memoryHeaps[i].size) WRITE_S("\n")
	}
	for(uintptr_t i = 0; i<rawMemory.memoryTypeCount; i++){
		WRITE_S("MType ") WRITE_I(i) WRITE_S("(") WRITE_I(rawMemory.memoryTypes[i].heapIndex) WRITE_S(")");
		if(rawMemory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT){ WRITE_S(" Local"); }
		if(rawMemory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT){ WRITE_S(" Host-visible"); }
		if(rawMemory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT){ WRITE_S(" Coherent"); }
		if(rawMemory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT){ WRITE_S(" Cached"); }
		if(rawMemory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT){ WRITE_S(" Lazy"); }
		WRITE_S("\n")
	}
}
int VulkanDeviceData::checkFeatures(VkPhysicalDeviceFeatures* wantFeatures){
	#define CHECK_FEATURE(featVName) if(wantFeatures->featVName && !(rawFeatures.featVName)){return 0;}
	CHECK_FEATURE(robustBufferAccess)
	CHECK_FEATURE(fullDrawIndexUint32)
	CHECK_FEATURE(imageCubeArray)
	CHECK_FEATURE(independentBlend)
	CHECK_FEATURE(geometryShader)
	CHECK_FEATURE(tessellationShader)
	CHECK_FEATURE(sampleRateShading)
	CHECK_FEATURE(dualSrcBlend)
	CHECK_FEATURE(logicOp)
	CHECK_FEATURE(multiDrawIndirect)
	CHECK_FEATURE(drawIndirectFirstInstance)
	CHECK_FEATURE(depthClamp)
	CHECK_FEATURE(depthBiasClamp)
	CHECK_FEATURE(fillModeNonSolid)
	CHECK_FEATURE(depthBounds)
	CHECK_FEATURE(wideLines)
	CHECK_FEATURE(largePoints)
	CHECK_FEATURE(alphaToOne)
	CHECK_FEATURE(multiViewport)
	CHECK_FEATURE(samplerAnisotropy)
	CHECK_FEATURE(textureCompressionETC2)
	CHECK_FEATURE(textureCompressionASTC_LDR)
	CHECK_FEATURE(textureCompressionBC)
	CHECK_FEATURE(occlusionQueryPrecise)
	CHECK_FEATURE(pipelineStatisticsQuery)
	CHECK_FEATURE(vertexPipelineStoresAndAtomics)
	CHECK_FEATURE(fragmentStoresAndAtomics)
	CHECK_FEATURE(shaderTessellationAndGeometryPointSize)
	CHECK_FEATURE(shaderImageGatherExtended)
	CHECK_FEATURE(shaderStorageImageExtendedFormats)
	CHECK_FEATURE(shaderStorageImageMultisample)
	CHECK_FEATURE(shaderStorageImageReadWithoutFormat)
	CHECK_FEATURE(shaderStorageImageWriteWithoutFormat)
	CHECK_FEATURE(shaderUniformBufferArrayDynamicIndexing)
	CHECK_FEATURE(shaderSampledImageArrayDynamicIndexing)
	CHECK_FEATURE(shaderStorageBufferArrayDynamicIndexing)
	CHECK_FEATURE(shaderStorageImageArrayDynamicIndexing)
	CHECK_FEATURE(shaderClipDistance)
	CHECK_FEATURE(shaderCullDistance)
	CHECK_FEATURE(shaderFloat64)
	CHECK_FEATURE(shaderInt64)
	CHECK_FEATURE(shaderInt16)
	CHECK_FEATURE(shaderResourceResidency)
	CHECK_FEATURE(shaderResourceMinLod)
	CHECK_FEATURE(sparseBinding)
	CHECK_FEATURE(sparseResidencyBuffer)
	CHECK_FEATURE(sparseResidencyImage2D)
	CHECK_FEATURE(sparseResidencyImage3D)
	CHECK_FEATURE(sparseResidency2Samples)
	CHECK_FEATURE(sparseResidency4Samples)
	CHECK_FEATURE(sparseResidency8Samples)
	CHECK_FEATURE(sparseResidency16Samples)
	CHECK_FEATURE(sparseResidencyAliased)
	CHECK_FEATURE(variableMultisampleRate)
	CHECK_FEATURE(inheritedQueries)
	return 1;
}

VulkanInstance::VulkanInstance(const char* applicationName){
	appName = applicationName;
	isHot = 0;
	toWarn = 0;
}
VulkanInstance::~VulkanInstance(){
	if(isHot){
		vkDestroyInstance(theHot, 0);
	}
}
void VulkanInstance::create(){
	//get the layers that are present
	VulkanLayerSet hotLayers;
	std::vector<const char*> allHaveWant;
	for(uintptr_t i = 0; i<wantLayers.size(); i++){
		if(hotLayers.layerNameMap.find(wantLayers[i]) == hotLayers.layerNameMap.end()){
			if(toWarn){
				const char* passLayN[1] = {wantLayers[i].c_str()};
				toWarn->logError(WHODUN_ERROR_LEVEL_INFO, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Layer not found.", 1, passLayN);
			}
		}
		else{
			allHaveWant.push_back(wantLayers[i].c_str());
		}
	}
	//set up the creation info
	VkApplicationInfo vulkanApInfo;
		memset(&vulkanApInfo, 0, sizeof(VkApplicationInfo));
		vulkanApInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		vulkanApInfo.pApplicationName = appName;
		vulkanApInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		vulkanApInfo.pEngineName = "No Engine";
		vulkanApInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		vulkanApInfo.apiVersion = VK_API_VERSION_1_0;
	//make it
	VkInstanceCreateInfo vulkanConArg;
		memset(&vulkanConArg, 0, sizeof(VkInstanceCreateInfo));
		vulkanConArg.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		vulkanConArg.pApplicationInfo = &vulkanApInfo;
		vulkanConArg.enabledLayerCount = allHaveWant.size();
		if(allHaveWant.size()){ vulkanConArg.ppEnabledLayerNames = &(allHaveWant[0]); }
	if(vkCreateInstance(&vulkanConArg, 0, &theHot)){ throw WhodunError(WHODUN_ERROR_LEVEL_ERROR, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Could not initialize instance.", 0, 0); };
	isHot = 1;
	
	//get the active device information
	uint32_t numHotDevs = 0;
	if(vkEnumeratePhysicalDevices(theHot, &numHotDevs, 0)){ throw WhodunError(WHODUN_ERROR_LEVEL_ERROR, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Could not get devices.", 0, 0); }
	std::vector<VkPhysicalDevice> hotRawDevs(numHotDevs);
	if(numHotDevs){
		if(vkEnumeratePhysicalDevices(theHot, &numHotDevs, &(hotRawDevs[0]))){ throw WhodunError(WHODUN_ERROR_LEVEL_ERROR, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Could not get devices.", 0, 0); }
	}
	hotDevs.resize(numHotDevs);
	for(uintptr_t i = 0; i<numHotDevs; i++){
		//get the device info (invokes no possibility of error)
		VulkanDeviceData* curFillD = &(hotDevs[i]);
		curFillD->rawData = hotRawDevs[i];
		vkGetPhysicalDeviceProperties(curFillD->rawData, &(curFillD->rawProperties));
		vkGetPhysicalDeviceFeatures(curFillD->rawData, &(curFillD->rawFeatures));
		//get queues for the device
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(curFillD->rawData, &queueFamilyCount, 0);
		curFillD->rawQueues.resize(queueFamilyCount);
		if(queueFamilyCount){
			vkGetPhysicalDeviceQueueFamilyProperties(curFillD->rawData, &queueFamilyCount, &(curFillD->rawQueues[0]));
		}
		//and get memory data
		vkGetPhysicalDeviceMemoryProperties(curFillD->rawData, &(curFillD->rawMemory));
	}
}

/******************************************************************/
/*Grab a device                                                   */
/******************************************************************/

VulkanDeviceRequirements::VulkanDeviceRequirements(){
	memset(&wantFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
	memset(&reqFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
}
VulkanDeviceRequirements::~VulkanDeviceRequirements(){}
void VulkanDeviceRequirements::addQueueRequirement(uintptr_t queueType, uintptr_t dodgeQueue){
	queueReqFlags.push_back(queueType);
	queueNwantFlags.push_back(dodgeQueue);
}
void VulkanDeviceRequirements::addMemoryRequirement(uintptr_t wantMem, uintptr_t dodgeMem, uintptr_t needMem, uintptr_t hateMem, uintptr_t size){
	memoryWantFlags.push_back(wantMem);
	memoryNwantFlags.push_back(dodgeMem);
	memoryReqFlags.push_back(needMem);
	memoryNreqFlags.push_back(hateMem);
	memorySizes.push_back(size);
}

VulkanDeviceRequirementMap::VulkanDeviceRequirementMap(VulkanDeviceRequirements* forReq, VulkanDeviceData* forDev){
	//check features
		featuresAdequate = forDev->checkFeatures(&(forReq->reqFeatures));
		featuresExcellent = forDev->checkFeatures(&(forReq->wantFeatures));
	//hunt queues to use
		numQueueExcellent = 0;
		numQueueAdequate = 0;
		for(uintptr_t i = 0; i<forReq->queueReqFlags.size(); i++){
			uintptr_t curNeedFlag = forReq->queueReqFlags[i];
			uintptr_t curDodgeFlag = forReq->queueNwantFlags[i];
			int haveExcQ = 0;
			uintptr_t foundQI = 0;
			for(uintptr_t j = 0; j<forDev->rawQueues.size(); j++){
				if(forDev->rawQueues[j].queueFlags & curDodgeFlag){ continue; }
				if((forDev->rawQueues[j].queueFlags & curNeedFlag) == curNeedFlag){
					haveExcQ = 1;
					foundQI = j;
					break;
				}
			}
			if(haveExcQ){
				numQueueExcellent++;
				numQueueAdequate++;
			}
			else{
				int haveAdqQ = 0;
				for(uintptr_t j = 0; j<forDev->rawQueues.size(); j++){
					if((forDev->rawQueues[j].queueFlags & curNeedFlag) == curNeedFlag){
						haveAdqQ = 1;
						foundQI = j;
						break;
					}
				}
				if(haveAdqQ){ numQueueAdequate++; }
			}
			queueAssignments.push_back(foundQI);
		}
	//hunt memory to use
		numMemoryExcellent = 0;
		numMemoryAdequate = 0;
		std::vector<uintptr_t> currentMemEats(forDev->rawMemory.memoryHeapCount);
		for(uintptr_t i = 0; i<forReq->memoryWantFlags.size(); i++){
			uintptr_t curNeedTFlag = forReq->memoryReqFlags[i];
			uintptr_t curNeedFFlag = forReq->memoryNreqFlags[i];
			uintptr_t curWantTFlag = forReq->memoryWantFlags[i];
			uintptr_t curWantFFlag = forReq->memoryNwantFlags[i];
			uintptr_t curChunkSize = forReq->memorySizes[i];
			int haveExcQ = 0;
			uintptr_t foundQI = 0;
			for(uintptr_t j = 0; j<forDev->rawMemory.memoryTypeCount; j++){
				if(forDev->rawMemory.memoryTypes[j].propertyFlags & curWantFFlag){ continue; }
				if((forDev->rawMemory.memoryTypes[j].propertyFlags & curWantTFlag) != curWantTFlag){ continue; }
				uintptr_t curMTHeap = forDev->rawMemory.memoryTypes[j].heapIndex;
				if((currentMemEats[curMTHeap] + curChunkSize) > forDev->rawMemory.memoryHeaps[curMTHeap].size){ continue; }
				currentMemEats[curMTHeap] += curChunkSize;
				haveExcQ = 1;
				foundQI = j;
				break;
			}
			if(haveExcQ){
				numMemoryExcellent++;
				numMemoryAdequate++;
			}
			else{
				int haveAdqQ = 0;
				for(uintptr_t j = 0; j<forDev->rawMemory.memoryTypeCount; j++){
					if(forDev->rawMemory.memoryTypes[j].propertyFlags & curNeedFFlag){ continue; }
					if((forDev->rawMemory.memoryTypes[j].propertyFlags & curNeedTFlag) != curNeedTFlag){ continue; }
					uintptr_t curMTHeap = forDev->rawMemory.memoryTypes[j].heapIndex;
					if((currentMemEats[curMTHeap] + curChunkSize) > forDev->rawMemory.memoryHeaps[curMTHeap].size){ continue; }
					currentMemEats[curMTHeap] += curChunkSize;
					haveAdqQ = 1;
					foundQI = j;
					break;
				}
				if(haveAdqQ){ numMemoryAdequate++; }
			}
			memoryAssignments.push_back(foundQI);
		}
	//make a final report
		deviceAdequate = featuresAdequate && (numQueueAdequate == queueAssignments.size()) && (numMemoryAdequate == memoryAssignments.size());
}
VulkanDeviceRequirementMap::~VulkanDeviceRequirementMap(){}

/******************************************************************/
/*Jobs and syncing                                                */
/******************************************************************/

VulkanSyncFence::VulkanSyncFence(VulkanDevice* forDev){
	saveDev = forDev;
	VkFenceCreateInfo createInf;
		memset(&createInf, 0, sizeof(VkFenceCreateInfo));
		createInf.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if(vkCreateFence(saveDev->hotDev, &createInf, 0, &theFence)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem creating fence.", 1, packExtras);
	}
}
VulkanSyncFence::~VulkanSyncFence(){
	vkDestroyFence(saveDev->hotDev, theFence, 0);
}
void VulkanSyncFence::reset(){
	if(vkResetFences(saveDev->hotDev, 1, &theFence)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem resetting fence.", 1, packExtras);
	}
}
void VulkanSyncFence::wait(){
	uint64_t longAssWait = 1;
		longAssWait = longAssWait << 32;
	VkResult waitRes = vkWaitForFences(saveDev->hotDev,1,&theFence,0,longAssWait);
	while(waitRes == VK_TIMEOUT){
		waitRes = vkWaitForFences(saveDev->hotDev,1,&theFence,0,longAssWait);
	}
	if(waitRes){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem waiting for fence.", 1, packExtras);
	}
}
VulkanSyncSemaphore::VulkanSyncSemaphore(VulkanDevice* forDev){
	saveDev = forDev;
	VkSemaphoreCreateInfo createInf;
		memset(&createInf, 0, sizeof(VkSemaphoreCreateInfo));
		createInf.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if(vkCreateSemaphore(saveDev->hotDev, &createInf, 0, &theSem)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem creating semaphore.", 1, packExtras);
	}
}
VulkanSyncSemaphore::~VulkanSyncSemaphore(){
	vkDestroySemaphore(saveDev->hotDev, theSem, 0);
}
VulkanSyncEvent::VulkanSyncEvent(VulkanDevice* forDev){
	saveDev = forDev;
	VkEventCreateInfo createInf;
		memset(&createInf, 0, sizeof(VkEventCreateInfo));
		createInf.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	if(vkCreateEvent(saveDev->hotDev, &createInf, 0, &theEvent)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem creating event.", 1, packExtras);
	}
}
VulkanSyncEvent::~VulkanSyncEvent(){
	vkDestroyEvent(saveDev->hotDev, theEvent, 0);
}

VulkanDeviceQueue::VulkanDeviceQueue(){}
VulkanDeviceQueue::~VulkanDeviceQueue(){}
void VulkanDeviceQueue::submit(VulkanCommandList* toRun){
	VkSubmitInfo subInfo;
		memset(&subInfo, 0, sizeof(VkSubmitInfo));
		subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		subInfo.commandBufferCount = 1;
		subInfo.pCommandBuffers = &(toRun->theComs);
	if(vkQueueSubmit(hotQueue, 1, &subInfo, 0)){
		const char* packExtras[] = {hotDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem submitting job.", 1, packExtras);
	}
}
void VulkanDeviceQueue::submit(VulkanCommandList* toRun, VulkanSyncFence* toWarn){
	VkSubmitInfo subInfo;
		memset(&subInfo, 0, sizeof(VkSubmitInfo));
		subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		subInfo.commandBufferCount = 1;
		subInfo.pCommandBuffers = &(toRun->theComs);
	if(vkQueueSubmit(hotQueue, 1, &subInfo, toWarn->theFence)){
		const char* packExtras[] = {hotDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem submitting job.", 1, packExtras);
	}
}
void VulkanDeviceQueue::submit(VulkanCommandList* toRun, VulkanSyncFence* toWarn, uintptr_t numWait, VulkanSyncSemaphore** toWait, uintptr_t* waitStages, uintptr_t numSig, VulkanSyncSemaphore** toSig){
	std::vector<VkSemaphore> packWaits(numWait + 1);
	std::vector<VkPipelineStageFlags> packFlags(numWait + 1);
	for(uintptr_t i = 0; i<numWait; i++){
		packWaits[i] = toWait[i]->theSem;
		packFlags[i] = waitStages[i];
	}
	std::vector<VkSemaphore> packSigs(numSig+1);
	for(uintptr_t i = 0; i<numSig; i++){
		packSigs[i] = toSig[i]->theSem;
	}
	VkSubmitInfo subInfo;
		memset(&subInfo, 0, sizeof(VkSubmitInfo));
		subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		subInfo.commandBufferCount = 1;
		subInfo.pCommandBuffers = &(toRun->theComs);
		subInfo.waitSemaphoreCount = numWait;
		subInfo.pWaitSemaphores = &(packWaits[0]);
		subInfo.pWaitDstStageMask = &(packFlags[0]);
		subInfo.signalSemaphoreCount = numSig;
		subInfo.pSignalSemaphores = &(packSigs[0]);
	if(vkQueueSubmit(hotQueue, 1, &subInfo, toWarn->theFence)){
		const char* packExtras[] = {hotDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem submitting job.", 1, packExtras);
	}
}
void VulkanDeviceQueue::waitEmpty(){
	if(vkQueueWaitIdle(hotQueue)){
		const char* packExtras[] = {hotDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem waiting for queue to empty.", 1, packExtras);
	}
}

VulkanCommandPool::VulkanCommandPool(VulkanDeviceQueue* forQueue, uintptr_t poolFlags){
	origQueue = forQueue;
	VkCommandPoolCreateInfo comPoolInfo;
		memset(&comPoolInfo, 0, sizeof(VkCommandPoolCreateInfo));
		comPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		comPoolInfo.queueFamilyIndex = forQueue->queueFamily;
		comPoolInfo.flags = poolFlags;
	if(vkCreateCommandPool(origQueue->hotDev->hotDev, &comPoolInfo, 0, &longPool)){
		const char* packExtras[] = {origQueue->hotDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem creating command pool.", 1, packExtras);
	}
}
VulkanCommandPool::~VulkanCommandPool(){
	vkDestroyCommandPool(origQueue->hotDev->hotDev, longPool, 0);
}
void VulkanCommandPool::reset(){
	if(vkResetCommandPool(origQueue->hotDev->hotDev, longPool, 0)){
		const char* packExtras[] = {origQueue->hotDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem resetting command pool (leak?).", 1, packExtras);
	}
}

VulkanCommandList::VulkanCommandList(VulkanCommandPool* fromPool){
	origPool = fromPool;
	VkCommandBufferAllocateInfo comBufferInfo;
		memset(&comBufferInfo, 0, sizeof(VkCommandBufferAllocateInfo));
		comBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		comBufferInfo.commandPool = origPool->longPool;
		comBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		comBufferInfo.commandBufferCount = 1;
	if(vkAllocateCommandBuffers(origPool->origQueue->hotDev->hotDev, &comBufferInfo, &theComs)){
		const char* packExtras[] = {origPool->origQueue->hotDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem creating command buffer.", 1, packExtras);
	}
}
VulkanCommandList::~VulkanCommandList(){
	vkFreeCommandBuffers(origPool->origQueue->hotDev->hotDev, origPool->longPool, 1, &theComs);
}
void VulkanCommandList::begin(int oneTime){
	VkCommandBufferBeginInfo commandBufferBeginInfo;
		memset(&commandBufferBeginInfo, 0, sizeof(VkCommandBufferBeginInfo));
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = oneTime ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
	if(vkBeginCommandBuffer(theComs, &commandBufferBeginInfo)){
		const char* packExtras[] = {origPool->origQueue->hotDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem starting command buffer.", 1, packExtras);
	}
}
void VulkanCommandList::compile(){
	if(vkEndCommandBuffer(theComs)){
		const char* packExtras[] = {origPool->origQueue->hotDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem with commands.", 1, packExtras);
	}
}
void VulkanCommandList::reset(){
	if(vkResetCommandBuffer(theComs, 0)){
		const char* packExtras[] = {origPool->origQueue->hotDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem resetting command buffer.", 1, packExtras);
	}
}

/******************************************************************/
/*Memory                                                          */
/******************************************************************/

VulkanMalloc::VulkanMalloc(VulkanDevice* onDev, uintptr_t memTypeI, uintptr_t allocSize){
	//get some info
	saveDev = onDev;
	uintptr_t memFlags = saveDev->devData->rawMemory.memoryTypes[memTypeI].propertyFlags;
	canMap =  memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	needFlush = (memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0;
	memSize = allocSize;
	//prepare to allocate
	VkMemoryAllocateInfo allocInInfo;
		memset(&allocInInfo, 0, sizeof(VkMemoryAllocateInfo));
		allocInInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInInfo.allocationSize = allocSize;
		allocInInfo.memoryTypeIndex = memTypeI;
	//and allocate
	if(vkAllocateMemory(saveDev->hotDev, &allocInInfo, 0, &saveAlloc)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem allocating memory.", 1, packExtras);
	}
}
VulkanMalloc::~VulkanMalloc(){
	vkFreeMemory(saveDev->hotDev, saveAlloc, 0);
}
void* VulkanMalloc::map(){
	void* mapDataP;
	if(vkMapMemory(saveDev->hotDev, saveAlloc, 0, memSize, 0, &mapDataP)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem mapping memory.", 1, packExtras);
	}
	return mapDataP;
}
void VulkanMalloc::mappedDataCommit(){
	if(needFlush){
		VkMappedMemoryRange pushRange;
			memset(&pushRange, 0, sizeof(VkMappedMemoryRange));
			pushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			pushRange.pNext = 0;
			pushRange.memory = saveAlloc;
			pushRange.offset = 0;
			pushRange.size = memSize;
		if(vkFlushMappedMemoryRanges(saveDev->hotDev, 1, &pushRange)){
			const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem updating memory.", 1, packExtras);
		}
	}
}
void VulkanMalloc::mappedDataUpdate(){
	if(needFlush){
		VkMappedMemoryRange pushRange;
			memset(&pushRange, 0, sizeof(VkMappedMemoryRange));
			pushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			pushRange.pNext = 0;
			pushRange.memory = saveAlloc;
			pushRange.offset = 0;
			pushRange.size = memSize;
		if(vkInvalidateMappedMemoryRanges(saveDev->hotDev, 1, &pushRange)){
			const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem updating memory.", 1, packExtras);
		}
	}
}
void VulkanMalloc::unmap(){
	vkUnmapMemory(saveDev->hotDev, saveAlloc);
}

VulkanBuffer::VulkanBuffer(VulkanDevice* forDev, uintptr_t usages, uintptr_t size){
	saveDev = forDev;
	VkBufferCreateInfo bufferInInfo;
		memset(&bufferInInfo, 0, sizeof(VkBufferCreateInfo));
		bufferInInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInInfo.size = size;
		bufferInInfo.usage = usages;
		bufferInInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if(vkCreateBuffer(saveDev->hotDev, &bufferInInfo, 0, &theBuff)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem allocating buffer.", 1, packExtras);
	}
	vkGetBufferMemoryRequirements(saveDev->hotDev, theBuff, &memData);
}
VulkanBuffer::~VulkanBuffer(){
	vkDestroyBuffer(saveDev->hotDev, theBuff, 0);
}
void VulkanBuffer::bind(VulkanMalloc* toMem, uintptr_t offset){
	if(vkBindBufferMemory(saveDev->hotDev, theBuff, toMem->saveAlloc, offset)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem binding buffer to memory.", 1, packExtras);
	}
	onMem = toMem;
	memOffset = offset;
}

VulkanBufferMallocManager::VulkanBufferMallocManager(VulkanDevice* forDev){
	saveDev = forDev;
}
VulkanBufferMallocManager::~VulkanBufferMallocManager(){
	for(uintptr_t i = 0; i<allBuffs.size(); i++){
		delete(allBuffs[i]);
	}
	for(uintptr_t i = 0; i<allAlloc.size(); i++){
		if(allAlloc[i]){ delete(allAlloc[i]); }
	}
}
void VulkanBufferMallocManager::addBuffer(VulkanBuffer* toManage, uintptr_t flagWant, uintptr_t flagNWant, uintptr_t flagNeed, uintptr_t flagNNeed){
	allBuffs.push_back(toManage);
	buffMemWT.push_back(flagWant);
	buffMemWF.push_back(flagNWant);
	buffMemNT.push_back(flagNeed);
	buffMemNF.push_back(flagNNeed);
}
void VulkanBufferMallocManager::allocate(){
	//assign buffers to memory types
		std::vector<uintptr_t> buffAssigns;
		for(uintptr_t i = 0; i<allBuffs.size(); i++){
			uintptr_t curTypeMask = allBuffs[i]->memData.memoryTypeBits;
			uintptr_t curNeedTFlag = buffMemNT[i];
			uintptr_t curNeedFFlag = buffMemNF[i];
			uintptr_t curWantTFlag = buffMemWT[i];
			uintptr_t curWantFFlag = buffMemWF[i];
			int haveExcQ = 0;
			uintptr_t foundQI = 0;
			for(uintptr_t j = 0; j<saveDev->devData->rawMemory.memoryTypeCount; j++){
				if(((curTypeMask >> j) & 0x01) == 0){ continue; }
				if(saveDev->devData->rawMemory.memoryTypes[j].propertyFlags & curWantFFlag){ continue; }
				if((saveDev->devData->rawMemory.memoryTypes[j].propertyFlags & curWantTFlag) != curWantTFlag){ continue; }
				haveExcQ = 1;
				foundQI = j;
				break;
			}
			if(haveExcQ){}else{
				int haveAdqQ = 0;
				for(uintptr_t j = 0; j<saveDev->devData->rawMemory.memoryTypeCount; j++){
					if(((curTypeMask >> j) & 0x01) == 0){ continue; }
					if(saveDev->devData->rawMemory.memoryTypes[j].propertyFlags & curNeedFFlag){ continue; }
					if((saveDev->devData->rawMemory.memoryTypes[j].propertyFlags & curNeedTFlag) != curNeedTFlag){ continue; }
					haveAdqQ = 1;
					foundQI = j;
					break;
				}
				if(haveAdqQ){}else{
					const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "No memory type valid for the buffer with the requested properties.", 1, packExtras);
				}
			}
			buffAssigns.push_back(foundQI);
		}
	//figure out true sizes, alignments and offsets for each
		std::vector<uintptr_t> buffOffsets;
		std::vector<uintptr_t> allocSizes;
		for(uintptr_t i = 0; i<buffAssigns.size(); i++){
			uintptr_t curAssn = buffAssigns[i];
			if(curAssn >= allocSizes.size()){ allocSizes.resize(curAssn + 1); }
			uintptr_t curBuffRS = allBuffs[i]->memData.size;
			uintptr_t curBuffAln = allBuffs[i]->memData.alignment;
			uintptr_t curAS = allocSizes[curAssn];
			//align
			curAS = curAS + ((curAS % curBuffAln) ? (curBuffAln - (curAS % curBuffAln)) : 0);
			//and assign
			buffOffsets.push_back(curAS);
			curAS += curBuffRS;
			allocSizes[curAssn] = curAS;
		}
	//allocate
		allAlloc.resize(allocSizes.size());
		for(uintptr_t i = 0; i<allocSizes.size(); i++){
			uintptr_t curSize = allocSizes[i];
			if(curSize){
				allAlloc[i] = new VulkanMalloc(saveDev, i, curSize);
			}
		}
	//bind
		for(uintptr_t i = 0; i<buffAssigns.size(); i++){
			allBuffs[i]->bind(allAlloc[buffAssigns[i]], buffOffsets[i]);
		}
}

/******************************************************************/
/*Programs                                                        */
/******************************************************************/

VulkanShader::VulkanShader(VulkanDevice* forDev, uintptr_t shadLen, void* shadTxt){
	saveDev = forDev;
	VkShaderModuleCreateInfo shadMakeInfo;
		memset(&shadMakeInfo, 0, sizeof(VkShaderModuleCreateInfo));
		shadMakeInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shadMakeInfo.codeSize = shadLen;
		shadMakeInfo.pCode = (uint32_t*)(shadTxt);
	if(vkCreateShaderModule(saveDev->hotDev, &shadMakeInfo, 0, &compShad)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem compiling shader.", 1, packExtras);
	}
}
VulkanShader::~VulkanShader(){
	vkDestroyShaderModule(saveDev->hotDev, compShad, 0);
}

VulkanShaderDataLayout::VulkanShaderDataLayout(VulkanDevice* forDev, uintptr_t numBindings, VkDescriptorSetLayoutBinding* theBindings){
	saveDev = forDev;
	VkDescriptorSetLayoutCreateInfo shadLayoutPackage;
		memset(&shadLayoutPackage, 0, sizeof(VkDescriptorSetLayoutCreateInfo));
		shadLayoutPackage.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		shadLayoutPackage.bindingCount = numBindings;
		shadLayoutPackage.pBindings = theBindings;
	if(vkCreateDescriptorSetLayout(forDev->hotDev, &shadLayoutPackage, 0, &shadLayout)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem creating shader layout.", 1, packExtras);
	}
	saveInfo.insert(saveInfo.end(), theBindings, theBindings + numBindings);
}
VulkanShaderDataLayout::~VulkanShaderDataLayout(){
	vkDestroyDescriptorSetLayout(saveDev->hotDev, shadLayout, 0);
}

VulkanShaderDataAssignment::VulkanShaderDataAssignment(){}
VulkanShaderDataAssignment::~VulkanShaderDataAssignment(){}
void VulkanShaderDataAssignment::changeBufferBinding(uintptr_t layoutI, VkDescriptorType usage, VulkanBuffer* toHit){
	VkDescriptorBufferInfo in_descriptorBufferInfo;
		memset(&in_descriptorBufferInfo, 0, sizeof(VkDescriptorBufferInfo));
		in_descriptorBufferInfo.buffer = toHit->theBuff;
		in_descriptorBufferInfo.offset = 0;
		in_descriptorBufferInfo.range = VK_WHOLE_SIZE;
	VkWriteDescriptorSet writeDescriptorSet;
		memset(&writeDescriptorSet, 0, sizeof(VkWriteDescriptorSet));
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = hotAss;
		writeDescriptorSet.dstBinding = layoutI;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = usage;
		writeDescriptorSet.pBufferInfo = &in_descriptorBufferInfo;
	vkUpdateDescriptorSets(inPool->saveDev->hotDev, 1, &writeDescriptorSet, 0, 0);
}
void VulkanShaderDataAssignment::changeBufferBinding(uintptr_t layoutI, VkDescriptorType usage, VulkanBuffer* toHit, uintptr_t fromI, uintptr_t toI){
	VkDescriptorBufferInfo in_descriptorBufferInfo;
		memset(&in_descriptorBufferInfo, 0, sizeof(VkDescriptorBufferInfo));
		in_descriptorBufferInfo.buffer = toHit->theBuff;
		in_descriptorBufferInfo.offset = fromI;
		in_descriptorBufferInfo.range = (toI - fromI);
	VkWriteDescriptorSet writeDescriptorSet;
		memset(&writeDescriptorSet, 0, sizeof(VkWriteDescriptorSet));
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = hotAss;
		writeDescriptorSet.dstBinding = layoutI;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = usage;
		writeDescriptorSet.pBufferInfo = &in_descriptorBufferInfo;
	vkUpdateDescriptorSets(inPool->saveDev->hotDev, 1, &writeDescriptorSet, 0, 0);
}

VulkanShaderDataAssignments::VulkanShaderDataAssignments(VulkanDevice* forDev, uintptr_t numLays, VulkanShaderDataLayout** forLays){
	saveDev = forDev;
	//figure out how many of each thing are needed in the pool
	std::map<VkDescriptorType,uintptr_t> numNeedEachType;
	for(uintptr_t i = 0; i<numLays; i++){
		VulkanShaderDataLayout* curLay = forLays[i];
		for(uintptr_t j = 0; j<curLay->saveInfo.size(); j++){
			numNeedEachType[curLay->saveInfo[j].descriptorType] += curLay->saveInfo[j].descriptorCount;
		}
	}
	//make the pool size requirements
	std::vector<VkDescriptorPoolSize> allSizeReqs(numNeedEachType.size());
	uintptr_t curSI = 0;
	std::map<VkDescriptorType,uintptr_t>::iterator mapIt;
	for(mapIt = numNeedEachType.begin(); mapIt != numNeedEachType.end(); mapIt++){
		allSizeReqs[curSI].type = mapIt->first;
		allSizeReqs[curSI].descriptorCount = mapIt->second;
		curSI++;
	}
	//make the pool
    VkDescriptorPoolCreateInfo descripPoolInfo;
		memset(&descripPoolInfo, 0, sizeof(VkDescriptorPoolCreateInfo));
		descripPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descripPoolInfo.maxSets = numLays;
		descripPoolInfo.poolSizeCount = allSizeReqs.size();
		descripPoolInfo.pPoolSizes = &(allSizeReqs[0]);
	if(vkCreateDescriptorPool(saveDev->hotDev, &descripPoolInfo, 0, &descripPool)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem creating shader pointer pool.", 1, packExtras);
	}
	//allocate the assignments
	allocAssns.resize(numLays);
	for(uintptr_t i = 0; i<numLays; i++){
		VkDescriptorSetAllocateInfo descripPointInfo;
			memset(&descripPointInfo, 0, sizeof(VkDescriptorSetAllocateInfo));
			descripPointInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descripPointInfo.descriptorPool = descripPool;
			descripPointInfo.descriptorSetCount = 1;
			descripPointInfo.pSetLayouts = &(forLays[i]->shadLayout);
		if(vkAllocateDescriptorSets(saveDev->hotDev, &descripPointInfo, &(allocAssns[i].hotAss))){
			const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
			throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem creating shader pointer set.", 1, packExtras);
		}
		allocAssns[i].inPool = this;
	}
}
VulkanShaderDataAssignments::~VulkanShaderDataAssignments(){
	vkDestroyDescriptorPool(saveDev->hotDev, descripPool, 0);
}

VulkanComputePipeline::VulkanComputePipeline(VulkanDevice* forDev, VulkanShader* useShad, VulkanShaderDataLayout* useLay, const char* mainFN){
	saveDev = forDev;
	VkPipelineLayoutCreateInfo shadPipeLayoutInfo;
		memset(&shadPipeLayoutInfo, 0, sizeof(VkPipelineLayoutCreateInfo));
		shadPipeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		shadPipeLayoutInfo.setLayoutCount = 1;
		shadPipeLayoutInfo.pSetLayouts = &(useLay->shadLayout);
	if(vkCreatePipelineLayout(saveDev->hotDev, &shadPipeLayoutInfo, 0, &pipeLayout)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem creating pipeline layout.", 1, packExtras);
	}
	VkPipelineShaderStageCreateInfo doCalcStageInfo;
		memset(&doCalcStageInfo, 0, sizeof(VkPipelineShaderStageCreateInfo));
		doCalcStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		doCalcStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		doCalcStageInfo.module = useShad->compShad;
		doCalcStageInfo.pName = mainFN;
	VkComputePipelineCreateInfo shadPipeInfo;
		memset(&shadPipeInfo, 0, sizeof(VkComputePipelineCreateInfo));
		shadPipeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		shadPipeInfo.stage = doCalcStageInfo;
		shadPipeInfo.layout = pipeLayout;
	if(vkCreateComputePipelines(saveDev->hotDev, 0, 1, &shadPipeInfo, 0, &thePipe)){
		const char* packExtras[] = {saveDev->devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem creating pipeline.", 1, packExtras);
	}
}
VulkanComputePipeline::~VulkanComputePipeline(){
	vkDestroyPipeline(saveDev->hotDev, thePipe, 0);
	vkDestroyPipelineLayout(saveDev->hotDev, pipeLayout, 0);
}

/******************************************************************/
/*The big one                                                     */
/******************************************************************/

VulkanDevice::VulkanDevice(){
	isHot = 0;
	memset(&needFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
}
VulkanDevice::~VulkanDevice(){
	if(isHot){
		vkDestroyDevice(hotDev, 0);
	}
}
void VulkanDevice::create(VulkanInstance* hotVulkan, uintptr_t devIndex){
	hotVulk = hotVulkan;
	hotQueues.resize(queueIndices.size());
	VulkanDeviceData* curLookD = &(hotVulk->hotDevs[devIndex]);
	devData = curLookD;
	//figure out how many of each class of queue are in play
		std::vector<uintptr_t> queueClassCounts(curLookD->rawQueues.size());
		for(uintptr_t i = 0; i<queueIndices.size(); i++){
			queueClassCounts[queueIndices[i]]++;
		}
	//prepare the queue creation structures
		std::vector<float> highPriorities;
		highPriorities.insert(highPriorities.end(), queueIndices.size(), 1.0);
		std::vector<VkDeviceQueueCreateInfo> queueMakeS(queueClassCounts.size());
		uintptr_t curPriI = 0;
		uintptr_t curSetQMI = 0;
		for(uintptr_t i = 0; i<queueClassCounts.size(); i++){
			if(queueClassCounts[i] == 0){ continue; }
			VkDeviceQueueCreateInfo* curSetI = &(queueMakeS[curSetQMI]);
			memset(curSetI, 0, sizeof(VkDeviceQueueCreateInfo));
			curSetI->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			curSetI->queueFamilyIndex = i;
			curSetI->queueCount = queueClassCounts[i];
			curSetI->pQueuePriorities = &(highPriorities[curPriI]);
			curSetQMI++;
			curPriI += queueClassCounts[i];
		}
	//note which validators are in play
		VulkanLayerSet hotLayers;
		std::vector<const char*> allHaveWant;
		for(uintptr_t i = 0; i<hotVulk->wantLayers.size(); i++){
			if(hotLayers.layerNameMap.find(hotVulk->wantLayers[i]) != hotLayers.layerNameMap.end()){
				allHaveWant.push_back(hotVulk->wantLayers[i].c_str());
			}
		}
	//prepare to hit the device
	VkDeviceCreateInfo getDevQInfo;
		memset(&getDevQInfo, 0, sizeof(VkDeviceCreateInfo));
		getDevQInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		getDevQInfo.queueCreateInfoCount = curSetQMI;
		getDevQInfo.pQueueCreateInfos = curSetQMI ? &(queueMakeS[0]) : (VkDeviceQueueCreateInfo*)0;
		getDevQInfo.enabledLayerCount = allHaveWant.size();
		if(allHaveWant.size()){ getDevQInfo.ppEnabledLayerNames = &(allHaveWant[0]); }
		getDevQInfo.pEnabledFeatures = &needFeatures;
	//get the device
	if(vkCreateDevice(curLookD->rawData, &getDevQInfo, 0, &hotDev)){
		const char* packExtras[] = {devData->rawProperties.deviceName};
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Problem initializing device.", 1, packExtras);
	}
	isHot = 1;
	//get the queues
	std::vector<uintptr_t> queueClassInds(queueClassCounts.size());
	for(uintptr_t i = 0; i<queueIndices.size(); i++){
		VulkanDeviceQueue* curFill = &(hotQueues[i]);
		uintptr_t queueClassI = queueIndices[i];
		uintptr_t famInd = queueClassInds[queueClassI];
		queueClassInds[queueClassI]++;
		curFill->hotDev = this;
		curFill->queueFamily = queueClassI;
		vkGetDeviceQueue(hotDev, queueClassI, famInd, &(curFill->hotQueue));
	}
}



