#include "whodun_vulkan_compute.h"

namespace whodun {

/**Limit excessive use of binding sets.*/
bool operator<(const VulkanComputeSingleStepCallSpec& operA, const VulkanComputeSingleStepCallSpec& operB);

};

using namespace whodun;

VulkanComputeGPUBuffer::VulkanComputeGPUBuffer(){
	forPack = 0;
	cpuBuffer = 0;
	gpuBuffer = 0;
}
VulkanComputeGPUBuffer::~VulkanComputeGPUBuffer(){}
void VulkanComputeGPUBuffer::read(uintptr_t fromInd, uintptr_t toInd, char* onCPU){
	if(cpuBuffer){
		gpuBuffer->onMem->mappedDataUpdate();
		memcpy(onCPU, cpuBuffer + fromInd, toInd - fromInd);
	}
	else{
		uintptr_t stageSize = forPack->stageSize;
		char* stageMemory = forPack->stageMemory;
		uintptr_t curFrom = fromInd;
		char* curTgt = onCPU;
		while(curFrom < toInd){
			uintptr_t curCopy = toInd - curFrom;
				curCopy = std::min(curCopy, stageSize);
			forPack->transferComPool->reset();
			forPack->transferJob->begin(1);
			VkBufferCopy copyData;
				memset(&copyData, 0, sizeof(VkBufferCopy));
				copyData.srcOffset = curFrom;
				copyData.dstOffset = 0;
				copyData.size = curCopy;
			vkCmdCopyBuffer(forPack->transferJob->theComs, gpuBuffer->theBuff, forPack->stageBuffer->theBuff, 1, &copyData);
			forPack->transferJob->compile();
			forPack->tranWaitFence->reset();
			forPack->transferQueue->submit(forPack->transferJob, forPack->tranWaitFence);
			forPack->tranWaitFence->wait();
			forPack->stageBuffer->onMem->mappedDataUpdate();
			memcpy(curTgt, stageMemory, curCopy);
			curTgt += curCopy;
			curFrom += curCopy;
		}
	}
}
void VulkanComputeGPUBuffer::write(uintptr_t fromInd, uintptr_t toInd, const char* fromCPU){
	if(cpuBuffer){
		memcpy(cpuBuffer + fromInd, fromCPU, toInd - fromInd);
		gpuBuffer->onMem->mappedDataCommit();
	}
	else{
		uintptr_t stageSize = forPack->stageSize;
		char* stageMemory = forPack->stageMemory;
		uintptr_t curFrom = fromInd;
		const char* curSrc = fromCPU;
		while(curFrom < toInd){
			uintptr_t curCopy = toInd - curFrom;
				curCopy = std::min(curCopy, stageSize);
			memcpy(stageMemory, curSrc, curCopy);
			forPack->stageBuffer->onMem->mappedDataCommit();
			forPack->transferComPool->reset();
			forPack->transferJob->begin(1);
			VkBufferCopy copyData;
				memset(&copyData, 0, sizeof(VkBufferCopy));
				copyData.srcOffset = 0;
				copyData.dstOffset = curFrom;
				copyData.size = curCopy;
			vkCmdCopyBuffer(forPack->transferJob->theComs, forPack->stageBuffer->theBuff, gpuBuffer->theBuff, 1, &copyData);
			forPack->transferJob->compile();
			forPack->tranWaitFence->reset();
			forPack->transferQueue->submit(forPack->transferJob, forPack->tranWaitFence);
			forPack->tranWaitFence->wait();
			curSrc += curCopy;
			curFrom += curCopy;
		}
	}
}

VulkanComputeGPUBufferInStream::VulkanComputeGPUBufferInStream(VulkanComputeGPUBuffer* fromB, uintptr_t fromInd, uintptr_t toInd){
	mainBuff = fromB;
	numTmpLeft = 0;
	tmpOffset = 0;
	numBuffLeft = toInd - fromInd;
	nextBuffOff = fromInd;
}
VulkanComputeGPUBufferInStream::~VulkanComputeGPUBufferInStream(){}
int VulkanComputeGPUBufferInStream::read(){
	if(numTmpLeft || numBuffLeft){
		char curC;
		read(&curC, 1);
		return 0x00FF & curC;
	}
	return -1;
}
uintptr_t VulkanComputeGPUBufferInStream::read(char* toR, uintptr_t numR){
	char* curR = toR;
	uintptr_t curNum = numR;
	//handle any odd stuff at the start
		if(numTmpLeft){
			if(curNum <= numTmpLeft){
				memcpy(curR, tmpStore + tmpOffset, curNum);
				numTmpLeft -= curNum;
				tmpOffset += curNum;
				return curNum;
			}
			memcpy(curR, tmpStore + tmpOffset, numTmpLeft);
			uintptr_t origTL = numTmpLeft;
			numTmpLeft = 0;
			curR += origTL;
			curNum -= origTL;
		}
	//handle bulk stuff
		uintptr_t maxBigGo = std::min(curNum / WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE, numBuffLeft / WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE);
		for(uintptr_t gi = 0; gi<maxBigGo; gi++){
			mainBuff->read(nextBuffOff, nextBuffOff + WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE, curR);
			curR += WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE;
			curNum -= WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE;
			nextBuffOff += WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE;
			numBuffLeft -= WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE;
		}
	//handle the stuff at the end
		if(curNum && numBuffLeft){
			if(curNum < numBuffLeft){
				uintptr_t curBuffEat = std::min(numBuffLeft, (uintptr_t)WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE);
				mainBuff->read(nextBuffOff, nextBuffOff + curBuffEat, tmpStore);
				numBuffLeft -= curBuffEat;
				nextBuffOff += curBuffEat;
				numTmpLeft = curBuffEat;
				tmpOffset = 0;
				read(curR, curNum);
				curR += curNum;
				curNum = 0;
			}
			else{
				mainBuff->read(nextBuffOff, nextBuffOff + numBuffLeft, curR);
				curR += numBuffLeft;
				curNum -= numBuffLeft;
				nextBuffOff += numBuffLeft;
				numBuffLeft = 0;
			}
		}
	return numR - curNum;
}
void VulkanComputeGPUBufferInStream::close(){
	isClosed = 1;
}

VulkanComputeGPUBufferOutStream::VulkanComputeGPUBufferOutStream(VulkanComputeGPUBuffer* fromB, uintptr_t fromInd, uintptr_t toInd){
	mainBuff = fromB;
	numTmpQueue = 0;
	nextBuffOff = fromInd;
}
VulkanComputeGPUBufferOutStream::~VulkanComputeGPUBufferOutStream(){}
void VulkanComputeGPUBufferOutStream::write(int toW){
	char curW = toW;
	write(&curW, 1);
}
void VulkanComputeGPUBufferOutStream::write(const char* toW, uintptr_t numW){
	const char* curW = toW;
	uintptr_t curNum = numW;
	//if a small write, put on buffer and return
		if((numTmpQueue + curNum) < WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE){
			memcpy(tmpStore + numTmpQueue, curW, curNum);
			numTmpQueue += curNum;
			return;
		}
	//fill up the queue and dump
		if(numTmpQueue){
			uintptr_t numEat = WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE - numTmpQueue;
			memcpy(tmpStore + numTmpQueue, curW, numEat);
			mainBuff->write(nextBuffOff, nextBuffOff + WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE, tmpStore);
			numTmpQueue = 0;
			curW += numEat;
			curNum -= numEat;
			nextBuffOff += WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE;
		}
	//do any big writes
		uintptr_t numBig = curNum / WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE;
		for(uintptr_t gi = 0; gi<numBig; gi++){
			mainBuff->write(nextBuffOff, nextBuffOff + WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE, curW);
			curW += WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE;
			curNum -= WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE;
			nextBuffOff += WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE;
		}
	//do any leftover
		memcpy(tmpStore, curW, curNum);
		numTmpQueue = curNum;
}
void VulkanComputeGPUBufferOutStream::flush(){
	if(numTmpQueue){
		mainBuff->write(nextBuffOff, nextBuffOff + numTmpQueue, tmpStore);
		nextBuffOff += numTmpQueue;
		numTmpQueue = 0;
	}
}
void VulkanComputeGPUBufferOutStream::close(){
	flush();
	isClosed = 1;
}

VulkanComputeSingleStepCallSpec::VulkanComputeSingleStepCallSpec(){}
VulkanComputeSingleStepCallSpec::~VulkanComputeSingleStepCallSpec(){}

bool whodun::operator<(const VulkanComputeSingleStepCallSpec& operA, const VulkanComputeSingleStepCallSpec& operB){
	if(operA.programIndex != operB.programIndex){
		return operA.programIndex < operB.programIndex;
	}
	if(operA.assignSpheres.size() != operB.assignSpheres.size()){
		return operA.assignSpheres.size() < operB.assignSpheres.size();
	}
	for(uintptr_t i = 0; i<operA.assignSpheres.size(); i++){
		if(operA.assignSpheres[i] != operB.assignSpheres[i]){
			return operA.assignSpheres[i] < operB.assignSpheres[i];
		}
		if(operA.assignIndices[i] != operB.assignIndices[i]){
			return operA.assignIndices[i] < operB.assignIndices[i];
		}
		//assignUse does not impact assignment
	}
	return false;
}

VulkanComputeProgramCallSpec::VulkanComputeProgramCallSpec(){}
VulkanComputeProgramCallSpec::~VulkanComputeProgramCallSpec(){}
void VulkanComputeProgramCallSpec::prepare(VulkanComputePackage* forPack){
	std::vector<uintptr_t> numUseLayout(forPack->allPipelines.size());
	std::map<VulkanComputeSingleStepCallSpec,VulkanShaderDataAssignment*> useBuffAss;
	std::map<VulkanComputeSingleStepCallSpec,VulkanShaderDataAssignment*>::iterator buffAssIt;
	std::vector<uintptr_t> lastInputBufferUse(forPack->inputBuffers.size());
	std::vector<uintptr_t> lastOutputBufferUse(forPack->outputBuffers.size());
	std::vector<uintptr_t> lastDataBufferUse(forPack->dataBuffers.size());
	std::vector<uintptr_t> lastSideBufferUse(forPack->sideBuffers.size());
	std::set<VulkanBuffer*> allHotInputs;
	std::set<VulkanBuffer*> allHotOutputs;
	for(uintptr_t si = 0; si < longhandSteps.size(); si++){
		//figure out the assignments for the step
			VulkanComputeSingleStepCallSpec* curStep = &(longhandSteps[si]);
			buffAssIt = useBuffAss.find(*curStep);
			if(buffAssIt == useBuffAss.end()){
				VulkanShaderDataAssignment* toHitLay = &(forPack->shadLinkTable->allocAssns[forPack->pipelineLinkI0[curStep->programIndex] + numUseLayout[curStep->programIndex]]);
				numUseLayout[curStep->programIndex]++;
				for(uintptr_t i = 0; i<curStep->assignSpheres.size(); i++){
					VulkanBuffer* toHitBuff;
					uintptr_t toHitBuffInd = 1;
					switch(curStep->assignSpheres[i]){
					case WHODUN_VULKAN_COMPUTE_BUFFER_SIDE:
						toHitBuffInd += forPack->dataBuffers.size();
					case WHODUN_VULKAN_COMPUTE_BUFFER_DATA:
						toHitBuffInd += forPack->outputBuffers.size();
					case WHODUN_VULKAN_COMPUTE_BUFFER_OUTPUT:
						toHitBuffInd += forPack->inputBuffers.size();
					case WHODUN_VULKAN_COMPUTE_BUFFER_INPUT:
						toHitBuffInd += curStep->assignIndices[i];
						break;
					default:
						throw std::runtime_error("Da fuq?");
					};
					toHitBuff = forPack->allBuffers->allBuffs[toHitBuffInd];
					toUpdate.push_back(toHitLay);
					toUpdateIndex.push_back(i);
					toUpdateTo.push_back(toHitBuff);
					if(curStep->assignSpheres[i] == WHODUN_VULKAN_COMPUTE_BUFFER_INPUT){ allHotInputs.insert(toHitBuff); }
					if(curStep->assignSpheres[i] == WHODUN_VULKAN_COMPUTE_BUFFER_OUTPUT){ allHotOutputs.insert(toHitBuff); }
				}
				useBuffAss[*curStep] = toHitLay;
				buffAssIt = useBuffAss.find(*curStep);
			}
			stepUseAssign.push_back(buffAssIt->second);
		//see if the buffers it uses have been used for something else
			uintptr_t waitingOn = 0;
			uintptr_t waiting = 0;
			for(uintptr_t i = 0; i<curStep->assignSpheres.size(); i++){
				uintptr_t lastBuffUse;
				uintptr_t curAssnInd = curStep->assignIndices[i];
				uintptr_t curAssnUse = curStep->assignUse[i];
				switch(curStep->assignSpheres[i]){
					case WHODUN_VULKAN_COMPUTE_BUFFER_INPUT:
						lastBuffUse = lastInputBufferUse[curAssnInd];
						lastInputBufferUse[curAssnInd] = curAssnUse;
						break;
					case WHODUN_VULKAN_COMPUTE_BUFFER_OUTPUT:
						lastBuffUse = lastOutputBufferUse[curAssnInd];
						lastOutputBufferUse[curAssnInd] = curAssnUse;
						break;
					case WHODUN_VULKAN_COMPUTE_BUFFER_DATA:
						lastBuffUse = lastDataBufferUse[curAssnInd];
						lastDataBufferUse[curAssnInd] = curAssnUse;
						break;
					case WHODUN_VULKAN_COMPUTE_BUFFER_SIDE:
						lastBuffUse = lastSideBufferUse[curAssnInd];
						lastSideBufferUse[curAssnInd] = curAssnUse;
						break;
					default:
						throw std::runtime_error("Da fuq?");
				};
				waitingOn = waitingOn | lastBuffUse;
				waiting = waiting | curAssnUse;
			}
			stepNeedBarrier.push_back(waitingOn);
			VkMemoryBarrier flushCache;
				memset(&flushCache, 0, sizeof(VkMemoryBarrier));
				flushCache.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				flushCache.srcAccessMask = 0;
					if(waitingOn & WHODUN_VULKAN_COMPUTE_BUFFER_IN){
						flushCache.srcAccessMask = flushCache.srcAccessMask | VK_ACCESS_SHADER_READ_BIT;
					}
					if(waitingOn & WHODUN_VULKAN_COMPUTE_BUFFER_OUT){
						flushCache.srcAccessMask = flushCache.srcAccessMask | VK_ACCESS_SHADER_WRITE_BIT;
					}
				flushCache.dstAccessMask = 0;
					if(waiting & WHODUN_VULKAN_COMPUTE_BUFFER_IN){
						flushCache.dstAccessMask = flushCache.dstAccessMask | VK_ACCESS_SHADER_READ_BIT;
					}
					if(waiting & WHODUN_VULKAN_COMPUTE_BUFFER_OUT){
						flushCache.dstAccessMask = flushCache.dstAccessMask | VK_ACCESS_SHADER_WRITE_BIT;
					}
			stepBarrier.push_back(flushCache);
	}
	//add barriers on the waiting buffers
	std::set<VulkanBuffer*>::iterator hotIt;
	for(hotIt = allHotInputs.begin(); hotIt != allHotInputs.end(); hotIt++){
		VkBufferMemoryBarrier curBar;
		memset(&curBar, 0, sizeof(VkBufferMemoryBarrier));
		curBar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		curBar.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		curBar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		curBar.buffer = (*hotIt)->theBuff;
		curBar.offset = 0;
		curBar.size = VK_WHOLE_SIZE;
		startBarrier.push_back(curBar);
	}
	for(hotIt = allHotOutputs.begin(); hotIt != allHotOutputs.end(); hotIt++){
		VkBufferMemoryBarrier curBar;
		memset(&curBar, 0, sizeof(VkBufferMemoryBarrier));
		curBar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		curBar.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		curBar.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
		curBar.buffer = (*hotIt)->theBuff;
		curBar.offset = 0;
		curBar.size = VK_WHOLE_SIZE;
		finBarrier.push_back(curBar);
	}
}

VulkanComputePackage::VulkanComputePackage(){
	saveDev = 0;
	allBuffers = 0;
	shadLinkTable = 0;
	computeComPool = 0;
	computeJob = 0;
	transferComPool = 0;
	transferJob = 0;
	jobWaitFence = 0;
	tranWaitFence = 0;
}
VulkanComputePackage::~VulkanComputePackage(){
	#define DELETE_IF(toK) if(toK){ delete(toK); }
	//trash job stuff
		DELETE_IF(jobWaitFence);
		DELETE_IF(tranWaitFence);
		DELETE_IF(transferJob);
		DELETE_IF(computeJob);
		DELETE_IF(transferComPool);
		DELETE_IF(computeComPool);
	//kill the shaders
		DELETE_IF(shadLinkTable);
		for(uintptr_t i = 0; i<allPipelines.size(); i++){ delete(allPipelines[i]); }
		for(uintptr_t i = 0; i<allShaders.size(); i++){ delete(allShaders[i]); }
		for(uintptr_t i = 0; i<allShaderLayouts.size(); i++){ delete(allShaderLayouts[i]); }
	//memory
		DELETE_IF(allBuffers)
	//and the device itself
		DELETE_IF(saveDev)
}
void VulkanComputePackage::openInputBuffer(uintptr_t index, VulkanComputeGPUBuffer* toRead){
	toRead->forPack = this;
	toRead->cpuBuffer = inputBuffers[index];
	toRead->gpuBuffer = allBuffers->allBuffs[index+1];
}
void VulkanComputePackage::openOutputBuffer(uintptr_t index, VulkanComputeGPUBuffer* toRead){
	toRead->forPack = this;
	toRead->cpuBuffer = outputBuffers[index];
	toRead->gpuBuffer = allBuffers->allBuffs[index+1+inputBuffers.size()];
}
void VulkanComputePackage::openDataBuffer(uintptr_t index, VulkanComputeGPUBuffer* toRead){
	toRead->forPack = this;
	toRead->cpuBuffer = dataBuffers[index];
	toRead->gpuBuffer = allBuffers->allBuffs[index+1+inputBuffers.size()+outputBuffers.size()];
}
void VulkanComputePackage::openSideBuffer(uintptr_t index, VulkanComputeGPUBuffer* toRead){
	toRead->forPack = this;
	toRead->cpuBuffer = sideBuffers[index];
	toRead->gpuBuffer = allBuffers->allBuffs[index+1+inputBuffers.size()+outputBuffers.size()+dataBuffers.size()];
}
void VulkanComputePackage::run(VulkanComputeProgramCallSpec* toRun){
	//fix up the assignments
		for(uintptr_t i = 0; i<toRun->toUpdate.size(); i++){
			toRun->toUpdate[i]->changeBufferBinding(toRun->toUpdateIndex[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, toRun->toUpdateTo[i]);
		}
	//set up the commands
		computeComPool->reset();
		computeJob->begin(1);
		if(toRun->startBarrier.size()){
			vkCmdPipelineBarrier(computeJob->theComs,
				VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
				0, 0,
				toRun->startBarrier.size(), &(toRun->startBarrier[0]),
				0, 0
				);
		}
		for(uintptr_t i = 0; i<toRun->stepUseAssign.size(); i++){
			if(toRun->stepNeedBarrier[i]){
				vkCmdPipelineBarrier(computeJob->theComs,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
					1, &(toRun->stepBarrier[i]),
					0, 0,
					0, 0
					);
			}
			uintptr_t curProgI = toRun->longhandSteps[i].programIndex;
			VulkanShaderDataAssignment* multAss = toRun->stepUseAssign[i];
			vkCmdBindPipeline(computeJob->theComs, VK_PIPELINE_BIND_POINT_COMPUTE, allPipelines[curProgI]->thePipe);
			vkCmdBindDescriptorSets(computeJob->theComs, VK_PIPELINE_BIND_POINT_COMPUTE, allPipelines[curProgI]->pipeLayout, 0, 1, &(multAss->hotAss), 0, 0);
			vkCmdDispatch(computeJob->theComs, toRun->numX[i], toRun->numY[i], toRun->numZ[i]);
		}
		if(toRun->finBarrier.size()){
			vkCmdPipelineBarrier(computeJob->theComs,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0,
				0, 0,
				toRun->finBarrier.size(), &(toRun->finBarrier[0]),
				0, 0
				);
		}
		computeJob->compile();
		jobWaitFence->reset();
		computeQueue->submit(computeJob, jobWaitFence);
}
void VulkanComputePackage::waitProgram(){
	jobWaitFence->wait();
}

VulkanComputePackageDescription::VulkanComputePackageDescription(){
	debugSide = 0;
	debugGpuOnCpu = 0;
}
VulkanComputePackageDescription::~VulkanComputePackageDescription(){
}
VulkanComputePackage* VulkanComputePackageDescription::prepare(VulkanInstance* hotVulkan, uintptr_t devIndex){
	VulkanComputePackage* toRet = new VulkanComputePackage();
	try{
		const char* packExtras[] = {hotVulkan->hotDevs[devIndex].rawProperties.deviceName};
		//figure out which queues to do on the device
			VulkanDeviceRequirements needReqs;
				needReqs.wantFeatures.shaderInt64 = 1;
				needReqs.wantFeatures.shaderFloat64 = 1;
				needReqs.reqFeatures = needReqs.wantFeatures;
				needReqs.addQueueRequirement(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);
				needReqs.addQueueRequirement(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
			uintptr_t totalInputSize = 0;
			for(uintptr_t i = 0; i<inputBufferSizes.size(); i++){ totalInputSize += inputBufferSizes[i]; }
			uintptr_t totalOutputSize = 0;
			for(uintptr_t i = 0; i<outputBufferSizes.size(); i++){ totalOutputSize += outputBufferSizes[i]; }
			uintptr_t totalDataSize = 0;
			for(uintptr_t i = 0; i<dataBufferSizes.size(); i++){ totalDataSize += dataBufferSizes[i]; }
			uintptr_t totalSideSize = 0;
			for(uintptr_t i = 0; i<sideBufferSizes.size(); i++){ totalSideSize += sideBufferSizes[i]; }
			uintptr_t cpuSideWantT = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			uintptr_t cpuSideWantF = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			uintptr_t cpuSideNeedT = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			uintptr_t cpuSideNeedF = 0;
			uintptr_t gpuSideWantT = debugGpuOnCpu ? cpuSideWantT : (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			uintptr_t gpuSideWantF = debugGpuOnCpu ? cpuSideWantF : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			uintptr_t gpuSideNeedT = debugGpuOnCpu ? cpuSideNeedT : 0;
			uintptr_t gpuSideNeedF = debugGpuOnCpu ? cpuSideNeedF : 0;
			if(totalInputSize){ needReqs.addMemoryRequirement(cpuSideWantT, cpuSideWantF, cpuSideNeedT, cpuSideNeedF, totalInputSize); }
			if(totalOutputSize){ needReqs.addMemoryRequirement(cpuSideWantT, cpuSideWantF, cpuSideNeedT, cpuSideNeedF, totalInputSize); }
			if(totalDataSize){ needReqs.addMemoryRequirement(gpuSideWantT, gpuSideWantF, gpuSideNeedT, gpuSideNeedF, totalDataSize); }
			if(totalSideSize){ needReqs.addMemoryRequirement(gpuSideWantT, gpuSideWantF, gpuSideNeedT, gpuSideNeedF, totalSideSize); }
			needReqs.addMemoryRequirement(cpuSideWantT, cpuSideWantF, cpuSideNeedT, cpuSideNeedF, naturalStageSize);
			VulkanDeviceRequirementMap satReqs(&needReqs, &(hotVulkan->hotDevs[devIndex]));
			if(!(satReqs.deviceAdequate)){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_OSCOMP, __FILE__, __LINE__, "Device not adequate to task.", 1, packExtras); }
		//make the device
			int comTranSameQ = satReqs.queueAssignments[0] == satReqs.queueAssignments[1];
			VulkanDevice* saveDev = new VulkanDevice();
			toRet->saveDev = saveDev;
			saveDev->needFeatures = needReqs.reqFeatures;
			saveDev->queueIndices.push_back(satReqs.queueAssignments[0]);
			if(!comTranSameQ){ saveDev->queueIndices.push_back(satReqs.queueAssignments[1]); }
			saveDev->create(hotVulkan, devIndex);
		//go ahead and prepare the queues
			toRet->computeQueue = &(saveDev->hotQueues[0]);
			toRet->transferQueue = &(saveDev->hotQueues[comTranSameQ ? 0 : 1]);
			toRet->computeComPool = new VulkanCommandPool(toRet->computeQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
			toRet->transferComPool = new VulkanCommandPool(toRet->transferQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
			toRet->computeJob = new VulkanCommandList(toRet->computeComPool);
			toRet->transferJob = new VulkanCommandList(toRet->transferComPool);
			toRet->jobWaitFence = new VulkanSyncFence(saveDev);
			toRet->tranWaitFence = new VulkanSyncFence(saveDev);
		//make the buffers
			toRet->allBuffers = new VulkanBufferMallocManager(saveDev);
			toRet->stageBuffer = new VulkanBuffer(saveDev, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, naturalStageSize);
			toRet->allBuffers->addBuffer(toRet->stageBuffer, cpuSideWantT, cpuSideWantF, cpuSideNeedT, cpuSideNeedF);
			for(uintptr_t i = 0; i<inputBufferSizes.size(); i++){
				toRet->allBuffers->addBuffer(new VulkanBuffer(saveDev, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, inputBufferSizes[i]), cpuSideWantT, cpuSideWantF, cpuSideNeedT, cpuSideNeedF);
			}
			for(uintptr_t i = 0; i<outputBufferSizes.size(); i++){
				toRet->allBuffers->addBuffer(new VulkanBuffer(saveDev, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, outputBufferSizes[i]), cpuSideWantT, cpuSideWantF, cpuSideNeedT, cpuSideNeedF);
			}
			for(uintptr_t i = 0; i<dataBufferSizes.size(); i++){
				toRet->allBuffers->addBuffer(new VulkanBuffer(saveDev, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, dataBufferSizes[i]), gpuSideWantT, gpuSideWantF, gpuSideNeedT, gpuSideNeedF);
			}
			for(uintptr_t i = 0; i<sideBufferSizes.size(); i++){
				toRet->allBuffers->addBuffer(new VulkanBuffer(saveDev, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | (debugSide ? (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT) : 0), sideBufferSizes[i]), gpuSideWantT, gpuSideWantF, gpuSideNeedT, gpuSideNeedF);
			}
			toRet->allBuffers->allocate();
		//map all the memory
			VulkanBuffer* curBuffWrap;
			char* curBuffLoc;
			std::map<VulkanMalloc*,char*> currentMaps;
			std::map<VulkanMalloc*,char*>::iterator curMapIt;
			#define GET_BUFFER_MEMORY_ADDR(forBuffer,toVar) \
				if((forBuffer)->onMem->canMap){\
					curMapIt = currentMaps.find((forBuffer)->onMem);\
					if(curMapIt == currentMaps.end()){\
						currentMaps[(forBuffer)->onMem] = (char*)((forBuffer)->onMem->map());\
						curMapIt = currentMaps.find((forBuffer)->onMem);\
					}\
					(toVar) = curMapIt->second + (forBuffer)->memOffset;\
				}\
				else{\
					(toVar) = (char*)0;\
				}
			toRet->stageSize = naturalStageSize;
			GET_BUFFER_MEMORY_ADDR(toRet->stageBuffer,toRet->stageMemory)
			for(uintptr_t i = 0; i<inputBufferSizes.size(); i++){
				curBuffWrap = toRet->allBuffers->allBuffs[i+1];
				GET_BUFFER_MEMORY_ADDR(curBuffWrap,curBuffLoc)
				toRet->inputBuffers.push_back(curBuffLoc);
			}
			for(uintptr_t i = 0; i<outputBufferSizes.size(); i++){
				curBuffWrap = toRet->allBuffers->allBuffs[i+1+inputBufferSizes.size()];
				GET_BUFFER_MEMORY_ADDR(curBuffWrap,curBuffLoc)
				toRet->outputBuffers.push_back(curBuffLoc);
			}
			if(debugGpuOnCpu){
				for(uintptr_t i = 0; i<dataBufferSizes.size(); i++){
					curBuffWrap = toRet->allBuffers->allBuffs[i+1+inputBufferSizes.size()+outputBufferSizes.size()];
					GET_BUFFER_MEMORY_ADDR(curBuffWrap,curBuffLoc)
					toRet->dataBuffers.push_back(curBuffLoc);
				}
				for(uintptr_t i = 0; i<sideBufferSizes.size(); i++){
					curBuffWrap = toRet->allBuffers->allBuffs[i+1+inputBufferSizes.size()+outputBufferSizes.size()+dataBufferSizes.size()];
					GET_BUFFER_MEMORY_ADDR(curBuffWrap,curBuffLoc)
					toRet->sideBuffers.push_back(curBuffLoc);
				}
			}
			else{
				toRet->dataBuffers.resize(dataBufferSizes.size());
				toRet->sideBuffers.resize(sideBufferSizes.size());
			}
		//build the individual shaders
			for(uintptr_t i = 0; i<allPrograms.size(); i++){
				//info
					VulkanComputeShaderBuilder* curProg = allPrograms[i];
					toRet->groupSizeX.push_back(curProg->groupSizeX);
					toRet->groupSizeY.push_back(curProg->groupSizeY);
					toRet->groupSizeZ.push_back(curProg->groupSizeZ);
				//layout
					std::vector<VkDescriptorSetLayoutBinding> curLayoutBindings(curProg->globalVarIDs.size());
					for(uintptr_t i = 0; i<curLayoutBindings.size(); i++){
						VkDescriptorSetLayoutBinding* curFill = &(curLayoutBindings[i]);
						curFill->binding = i;
						curFill->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						curFill->descriptorCount = 1;
						curFill->stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
						curFill->pImmutableSamplers = 0;
					}
					VulkanShaderDataLayout* curShaderLayout = new VulkanShaderDataLayout(saveDev, curLayoutBindings.size(), &(curLayoutBindings[0]));
					toRet->allShaderLayouts.push_back(curShaderLayout);
				//shader
					VulkanShader* curShader = new VulkanShader(saveDev, 4*curProg->mainBuild->spirOps.size(), &(curProg->mainBuild->spirOps[0]));
					toRet->allShaders.push_back(curShader);
				//"pipeline"
					VulkanComputePipeline* curProgram = new VulkanComputePipeline(saveDev, curShader, curShaderLayout, "main");
					toRet->allPipelines.push_back(curProgram);
			}
		//figure out the descriptor set situation
			std::vector<VulkanShaderDataLayout*> packLayouts;
			for(uintptr_t i = 0; i<allPrograms.size(); i++){
				toRet->pipelineLinkI0.push_back(packLayouts.size());
				for(uintptr_t j = 0; j<progMaxSimulBind[i]; j++){
					packLayouts.push_back(toRet->allShaderLayouts[i]);
				}
			}
			toRet->shadLinkTable = new VulkanShaderDataAssignments(saveDev, packLayouts.size(), &(packLayouts[0]));
	}
	catch(std::exception& errE){
		delete(toRet);
		throw;
	}
	return toRet;
}


#define CAPABILITY_SHADER 1
#define CAPABILITY_FLOAT_64 10
#define CAPABILITY_INT_64 11

#define ADDRESS_MODEL_LOGICAL 0
#define MEMORY_MODEL_SIMPLE 0

#define SHADER_TYPE_GRAPHICS_COMPUTE 5

#define SHADER_TAG_LOCAL_SIZE 17

#define DECORATE_BLOCK 2
#define DECORATE_BUFFER_BLOCK 3
#define DECORATE_ARRAY_STRIDE 6
#define DECORATE_BUILTIN 11
#define DECORATE_BUILTIN_GLOBALINVOCATIONID 28
#define DECORATE_BINDING 33
#define DECORATE_DESCRIPTOR_SET 34
#define DECORATE_OFFSET 35

#define STORAGE_CLASS_INPUT 1
#define STORAGE_CLASS_UNIFORM 2
#define STORAGE_CLASS_FUNCTION 7

#define FUNCTION_MARKUP_NONE 0

VulkanComputeShaderBuilder::VulkanComputeShaderBuilder(uintptr_t numInp, uint32_t* groupSizes){
	canDoCode = 0;
	noMoreConsts = 0;
	mainBuild = new VulkanSpirVBuilder();
	groupSizeX = groupSizes[0];
	groupSizeY = groupSizes[1];
	groupSizeZ = groupSizes[2];
	//allocate some ids
		typeVoidID = mainBuild->allocID();
		typeBoolID = mainBuild->allocID();
		typeInt32ID = mainBuild->allocID();
		typeInt64ID = mainBuild->allocID();
		typeFloat32ID = mainBuild->allocID();
		typeFloat64ID = mainBuild->allocID();
		typeArrRInt32ID = mainBuild->allocID();
		typeArrRInt64ID = mainBuild->allocID();
		typeArrRFloat32ID = mainBuild->allocID();
		typeArrRFloat64ID = mainBuild->allocID();
		typePtrUniInt32ID = mainBuild->allocID();
		typePtrUniInt64ID = mainBuild->allocID();
		typePtrUniFloat32ID = mainBuild->allocID();
		typePtrUniFloat64ID = mainBuild->allocID();
		typePtrFunBoolID = mainBuild->allocID();
		typePtrFunInt32ID = mainBuild->allocID();
		typePtrFunInt64ID = mainBuild->allocID();
		typePtrFunFloat32ID = mainBuild->allocID();
		typePtrFunFloat64ID = mainBuild->allocID();
		typeStructArrRInt32 = mainBuild->allocID();
		typeStructArrRInt64 = mainBuild->allocID();
		typeStructArrRFloat32 = mainBuild->allocID();
		typeStructArrRFloat64 = mainBuild->allocID();
		typePtrUniStructArrRInt32 = mainBuild->allocID();
		typePtrUniStructArrRInt64 = mainBuild->allocID();
		typePtrUniStructArrRFloat32 = mainBuild->allocID();
		typePtrUniStructArrRFloat64 = mainBuild->allocID();
	//allocate some global ids
		typeVoidFuncID = mainBuild->allocID();
		typeVec3Int32ID = mainBuild->allocID();
		typePtrInpInt32ID = mainBuild->allocID();
		typePtrInpVec3Int32ID = mainBuild->allocID();
		mainFuncID = mainBuild->allocID();
		globInvokeID = mainBuild->allocID();
		for(uintptr_t i = 0; i<numInp; i++){
			globalVarIDs.push_back(mainBuild->allocID());
		}
	//get the capabilities out the way
		mainBuild->markCapability(CAPABILITY_SHADER);
		mainBuild->markCapability(CAPABILITY_INT_64);
		mainBuild->markCapability(CAPABILITY_FLOAT_64);
		glslBuild = new VulkanSpirVGLSLExtension(mainBuild);
		mainBuild->markMemoryModel(ADDRESS_MODEL_LOGICAL, MEMORY_MODEL_SIMPLE);
	//entry point
		mainBuild->markEntryPoint(SHADER_TYPE_GRAPHICS_COMPUTE, mainFuncID, "main", 1, &(globInvokeID));
		mainBuild->tagEntryExecutionMode(mainFuncID, SHADER_TAG_LOCAL_SIZE, 3, groupSizes);
	//decorations
		uint32_t globIndBuiltIn = DECORATE_BUILTIN_GLOBALINVOCATIONID;
		mainBuild->tagDecorateID(globInvokeID, DECORATE_BUILTIN, 1, &globIndBuiltIn);
		uint32_t longdblArrStride = 8;
		mainBuild->tagDecorateID(typeArrRFloat64ID, DECORATE_ARRAY_STRIDE, 1, &longdblArrStride);
		mainBuild->tagDecorateID(typeArrRInt64ID, DECORATE_ARRAY_STRIDE, 1, &longdblArrStride);
		uint32_t intfltArrStride = 4;
		mainBuild->tagDecorateID(typeArrRFloat32ID, DECORATE_ARRAY_STRIDE, 1, &intfltArrStride);
		mainBuild->tagDecorateID(typeArrRInt32ID, DECORATE_ARRAY_STRIDE, 1, &intfltArrStride);
		uint32_t structFirstThingOffset = 0;
		mainBuild->tagDecorateMemberID(typeStructArrRInt32, 0, DECORATE_OFFSET, 1, &structFirstThingOffset);
		mainBuild->tagDecorateID(typeStructArrRInt32, DECORATE_BUFFER_BLOCK, 0, 0);
		mainBuild->tagDecorateMemberID(typeStructArrRInt64, 0, DECORATE_OFFSET, 1, &structFirstThingOffset);
		mainBuild->tagDecorateID(typeStructArrRInt64, DECORATE_BUFFER_BLOCK, 0, 0);
		mainBuild->tagDecorateMemberID(typeStructArrRFloat32, 0, DECORATE_OFFSET, 1, &structFirstThingOffset);
		mainBuild->tagDecorateID(typeStructArrRFloat32, DECORATE_BUFFER_BLOCK, 0, 0);
		mainBuild->tagDecorateMemberID(typeStructArrRFloat64, 0, DECORATE_OFFSET, 1, &structFirstThingOffset);
		mainBuild->tagDecorateID(typeStructArrRFloat64, DECORATE_BUFFER_BLOCK, 0, 0);
		uint32_t onlyDescSet = 0;
		for(uintptr_t i = 0; i<globalVarIDs.size(); i++){
			uint32_t curBinding = i;
			mainBuild->tagDecorateID(globalVarIDs[i], DECORATE_DESCRIPTOR_SET, 1, &onlyDescSet);
			mainBuild->tagDecorateID(globalVarIDs[i], DECORATE_BINDING, 1, &curBinding);
		}
	//types and type constants
		mainBuild->typedefVoid(typeVoidID);
		mainBuild->typedefBool(typeBoolID);
		mainBuild->typedefInt(32, 0, typeInt32ID);
		mainBuild->typedefInt(64, 0, typeInt64ID);
		mainBuild->typedefFloat(32, typeFloat32ID);
		mainBuild->typedefFloat(64, typeFloat64ID);
		mainBuild->typedefRuntimeArray(typeInt32ID, typeArrRInt32ID);
		mainBuild->typedefRuntimeArray(typeInt64ID, typeArrRInt64ID);
		mainBuild->typedefRuntimeArray(typeFloat32ID, typeArrRFloat32ID);
		mainBuild->typedefRuntimeArray(typeFloat64ID, typeArrRFloat64ID);
		mainBuild->typedefPointer(STORAGE_CLASS_UNIFORM, typeInt32ID, typePtrUniInt32ID);
		mainBuild->typedefPointer(STORAGE_CLASS_UNIFORM, typeInt64ID, typePtrUniInt64ID);
		mainBuild->typedefPointer(STORAGE_CLASS_UNIFORM, typeFloat32ID, typePtrUniFloat32ID);
		mainBuild->typedefPointer(STORAGE_CLASS_UNIFORM, typeFloat64ID, typePtrUniFloat64ID);
		mainBuild->typedefPointer(STORAGE_CLASS_FUNCTION, typeBoolID, typePtrFunBoolID);
		mainBuild->typedefPointer(STORAGE_CLASS_FUNCTION, typeInt32ID, typePtrFunInt32ID);
		mainBuild->typedefPointer(STORAGE_CLASS_FUNCTION, typeInt64ID, typePtrFunInt64ID);
		mainBuild->typedefPointer(STORAGE_CLASS_FUNCTION, typeFloat32ID, typePtrFunFloat32ID);
		mainBuild->typedefPointer(STORAGE_CLASS_FUNCTION, typeFloat64ID, typePtrFunFloat64ID);
		mainBuild->typedefStruct(1, &(typeArrRInt32ID), typeStructArrRInt32);
		mainBuild->typedefStruct(1, &(typeArrRInt64ID), typeStructArrRInt64);
		mainBuild->typedefStruct(1, &(typeArrRFloat32ID), typeStructArrRFloat32);
		mainBuild->typedefStruct(1, &(typeArrRFloat64ID), typeStructArrRFloat64);
		mainBuild->typedefPointer(STORAGE_CLASS_UNIFORM, typeStructArrRInt32, typePtrUniStructArrRInt32);
		mainBuild->typedefPointer(STORAGE_CLASS_UNIFORM, typeStructArrRInt64, typePtrUniStructArrRInt64);
		mainBuild->typedefPointer(STORAGE_CLASS_UNIFORM, typeStructArrRFloat32, typePtrUniStructArrRFloat32);
		mainBuild->typedefPointer(STORAGE_CLASS_UNIFORM, typeStructArrRFloat64, typePtrUniStructArrRFloat64);
		mainBuild->typedefFunc(typeVoidID, 0, 0, typeVoidFuncID);
		mainBuild->typedefVector(typeInt32ID, 3, typeVec3Int32ID);
		mainBuild->typedefPointer(STORAGE_CLASS_INPUT, typeInt32ID, typePtrInpInt32ID);
		mainBuild->typedefPointer(STORAGE_CLASS_INPUT, typeVec3Int32ID, typePtrInpVec3Int32ID);
	//open up the constants
		conTrue.forShad = this;
			conTrue.type = typeBoolID;
			conTrue.id = mainBuild->defConstantTrue(typeBoolID);
		conFalse.forShad = this;
			conFalse.type = typeBoolID;
			conFalse.id = mainBuild->defConstantFalse(typeBoolID);
	
}
VulkanComputeShaderBuilder::~VulkanComputeShaderBuilder(){
	delete(glslBuild);
	delete(mainBuild);
}
VulkanCSValI VulkanComputeShaderBuilder::conI(uint32_t value){
	std::map<uint32_t,uint32_t>::iterator mapIt = intCons.find(value);
	if(mapIt == intCons.end()){
		if(noMoreConsts){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot make new constants now.", 0, 0); }
		uint32_t valueId = mainBuild->defConstant(typeInt32ID, value);
		intCons[value] = valueId;
		mapIt = intCons.find(value);
	}
	VulkanCSValI toRet;
		toRet.forShad = this;
		toRet.type = typeInt32ID;
		toRet.id = mapIt->second;
	return toRet;
}
VulkanCSValL VulkanComputeShaderBuilder::conL(uint64_t value){
	std::map<uint64_t,uint32_t>::iterator mapIt = longCons.find(value);
	if(mapIt == longCons.end()){
		if(noMoreConsts){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot make new constants now.", 0, 0); }
		uint32_t valueId = mainBuild->defConstant64(typeInt64ID, value);
		longCons[value] = valueId;
		mapIt = longCons.find(value);
	}
	VulkanCSValL toRet;
		toRet.forShad = this;
		toRet.type = typeInt64ID;
		toRet.id = mapIt->second;
	return toRet;
}
VulkanCSValF VulkanComputeShaderBuilder::conF(float valuef){
	union {
		uint32_t asI;
		float asF;
	} punnyName;
	punnyName.asF = valuef;
	uint32_t value = punnyName.asI;
	std::map<uint32_t,uint32_t>::iterator mapIt = fltCons.find(value);
	if(mapIt == fltCons.end()){
		if(noMoreConsts){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot make new constants now.", 0, 0); }
		uint32_t valueId = mainBuild->defConstantF(typeFloat32ID, valuef);
		fltCons[value] = valueId;
		mapIt = fltCons.find(value);
	}
	VulkanCSValF toRet;
		toRet.forShad = this;
		toRet.type = typeFloat32ID;
		toRet.id = mapIt->second;
	return toRet;
}
VulkanCSValD VulkanComputeShaderBuilder::conD(double valuef){
	union {
		uint64_t asI;
		double asF;
	} punnyName;
	punnyName.asF = valuef;
	uint64_t value = punnyName.asI;
	std::map<uint64_t,uint32_t>::iterator mapIt = dblCons.find(value);
	if(mapIt == dblCons.end()){
		if(noMoreConsts){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot make new constants now.", 0, 0); }
		uint32_t valueId = mainBuild->defConstantF64(typeFloat64ID, valuef);
		dblCons[value] = valueId;
		mapIt = dblCons.find(value);
	}
	VulkanCSValD toRet;
		toRet.forShad = this;
		toRet.type = typeFloat64ID;
		toRet.id = mapIt->second;
	return toRet;
}
void VulkanComputeShaderBuilder::consDone(){
	//make sure there are a few constants available, and then block further creation
		conI(0); conI(1); conI(2);
		conF(0.0); conF(-1.0/0.0); conF(0.0/0.0);
		conD(0.0); conD(-1.0/0.0); conD(0.0/0.0);
		noMoreConsts = 1;
	//start with the global invokation id
		mainBuild->defVariable(typePtrInpVec3Int32ID, STORAGE_CLASS_INPUT, globInvokeID);
}

#define INTERFACE_REGISTER_CHECK \
	if(!noMoreConsts || canDoCode){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot register interface variables now.", 0, 0); } \
	if(interfaceVarTypes.size() >= globalVarIDs.size()){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Have already registered all interface types.", 0, 0); }

uintptr_t VulkanComputeShaderBuilder::registerInterfaceI(){
	INTERFACE_REGISTER_CHECK
	uintptr_t varInd = interfaceVarTypes.size();
	mainBuild->defVariable(typePtrUniStructArrRInt32, STORAGE_CLASS_UNIFORM, globalVarIDs[varInd]);
	interfaceVarTypes.push_back(typePtrUniStructArrRInt32);
	return varInd;
}
uintptr_t VulkanComputeShaderBuilder::registerInterfaceL(){
	INTERFACE_REGISTER_CHECK
	uintptr_t varInd = interfaceVarTypes.size();
	mainBuild->defVariable(typePtrUniStructArrRInt64, STORAGE_CLASS_UNIFORM, globalVarIDs[varInd]);
	interfaceVarTypes.push_back(typePtrUniStructArrRInt64);
	return varInd;
}
uintptr_t VulkanComputeShaderBuilder::registerInterfaceF(){
	INTERFACE_REGISTER_CHECK
	uintptr_t varInd = interfaceVarTypes.size();
	mainBuild->defVariable(typePtrUniStructArrRFloat32, STORAGE_CLASS_UNIFORM, globalVarIDs[varInd]);
	interfaceVarTypes.push_back(typePtrUniStructArrRFloat32);
	return varInd;
}
uintptr_t VulkanComputeShaderBuilder::registerInterfaceD(){
	INTERFACE_REGISTER_CHECK
	uintptr_t varInd = interfaceVarTypes.size();
	mainBuild->defVariable(typePtrUniStructArrRFloat64, STORAGE_CLASS_UNIFORM, globalVarIDs[varInd]);
	interfaceVarTypes.push_back(typePtrUniStructArrRFloat64);
	return varInd;
}
#define INTERFACE_GET_COMMON(ArrTypeName,WantStructTypeID,PointerTypeID,ValueTypeID) \
	if(!canDoCode){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot make code now.", 0, 0); } \
	if(interfaceVarTypes[index] != WantStructTypeID){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Wrong type of variable.", 0, 0); }\
	ArrTypeName toRet;\
		toRet.forShad = this;\
		toRet.ptrtype = PointerTypeID;\
		toRet.valtype = ValueTypeID;\
		toRet.id = globalVarIDs[index];\
	return toRet;

VulkanCSArrI VulkanComputeShaderBuilder::interfaceVarI(uintptr_t index){
	INTERFACE_GET_COMMON(VulkanCSArrI,typePtrUniStructArrRInt32,typePtrUniInt32ID,typeInt32ID)
}
VulkanCSArrL VulkanComputeShaderBuilder::interfaceVarL(uintptr_t index){
	INTERFACE_GET_COMMON(VulkanCSArrL,typePtrUniStructArrRInt64,typePtrUniInt64ID,typeInt64ID)
}
VulkanCSArrF VulkanComputeShaderBuilder::interfaceVarF(uintptr_t index){
	INTERFACE_GET_COMMON(VulkanCSArrF,typePtrUniStructArrRFloat32,typePtrUniFloat32ID,typeFloat32ID)
}
VulkanCSArrD VulkanComputeShaderBuilder::interfaceVarD(uintptr_t index){
	INTERFACE_GET_COMMON(VulkanCSArrD,typePtrUniStructArrRFloat64,typePtrUniFloat64ID,typeFloat64ID)
}

void VulkanComputeShaderBuilder::functionStart(){
	mainBuild->defFunction(typeVoidID, FUNCTION_MARKUP_NONE, typeVoidFuncID, mainFuncID);
	mainBuild->codeLabel();
	canDoCode = 1;
}
#define FUNC_VAR_DEF_COMMON(VarTypeName,VarTypeID,PtrVarTypeID) \
	if(!canDoCode){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot make code now.", 0, 0); } \
	VarTypeName toRet;\
		toRet.forShad = this;\
		toRet.type = VarTypeID;\
		toRet.id = mainBuild->defVariable(PtrVarTypeID, STORAGE_CLASS_FUNCTION);\
	return toRet;

VulkanCSVarB VulkanComputeShaderBuilder::defVarB(){
	FUNC_VAR_DEF_COMMON(VulkanCSVarB,typeBoolID,typePtrFunBoolID)
}
VulkanCSVarI VulkanComputeShaderBuilder::defVarI(){
	FUNC_VAR_DEF_COMMON(VulkanCSVarI,typeInt32ID,typePtrFunInt32ID)
}
VulkanCSVarL VulkanComputeShaderBuilder::defVarL(){
	FUNC_VAR_DEF_COMMON(VulkanCSVarL,typeInt64ID,typePtrFunInt64ID)
}
VulkanCSVarF VulkanComputeShaderBuilder::defVarF(){
	FUNC_VAR_DEF_COMMON(VulkanCSVarF,typeFloat32ID,typePtrFunFloat32ID)
}
VulkanCSVarD VulkanComputeShaderBuilder::defVarD(){
	FUNC_VAR_DEF_COMMON(VulkanCSVarD,typeFloat64ID,typePtrFunFloat64ID)
}

VulkanCSValI VulkanComputeShaderBuilder::invokeGID(int xyz){
	VulkanCSValI indexID = conI(xyz);
	uint32_t globIValPtrID = mainBuild->opAccessChain(typePtrInpInt32ID, globInvokeID, 1, &(indexID.id));
	uint32_t globIValID = mainBuild->opLoad(typeInt32ID, globIValPtrID);
	VulkanCSValI toRet;
		toRet.forShad = this;
		toRet.type = typeInt32ID;
		toRet.id = globIValID;
	return toRet;
}

void VulkanComputeShaderBuilder::ifStart(VulkanCSValB testVal){
	uint32_t trueBlockID = mainBuild->allocID();
	uint32_t falseBlockID = mainBuild->allocID();
	uint32_t endBlockID = mainBuild->allocID();
	mainBuild->codeSelectionMerge(endBlockID);
	mainBuild->codeConditionalJump(testVal.id, trueBlockID, falseBlockID);
	mainBuild->codeLabel(trueBlockID);
	oustandingIfs.push_back( std::pair<uint32_t,uint32_t>(falseBlockID,endBlockID) );
}
void VulkanComputeShaderBuilder::ifElse(){
	std::pair<uint32_t,uint32_t> outData = oustandingIfs[oustandingIfs.size()-1];
	mainBuild->codeJump(outData.second);
	mainBuild->codeLabel(outData.first);
}
void VulkanComputeShaderBuilder::ifEnd(){
	std::pair<uint32_t,uint32_t> outData = oustandingIfs[oustandingIfs.size()-1];
	mainBuild->codeJump(outData.second);
	mainBuild->codeLabel(outData.second);
	oustandingIfs.pop_back();
}

void VulkanComputeShaderBuilder::doStart(){
	uint32_t loopLabID = mainBuild->allocID();
	mainBuild->codeJump(loopLabID);
	mainBuild->codeLabel(loopLabID);
	uint32_t loopTstID = mainBuild->allocID();
	uint32_t loopBodID = mainBuild->allocID();
	uint32_t loopConID = mainBuild->allocID();
	uint32_t loopEndID = mainBuild->allocID();
	mainBuild->codeLoopMerge(loopEndID, loopConID);
	mainBuild->codeJump(loopTstID);
	mainBuild->codeLabel(loopTstID);
	outstandingDoMains.push_back(loopLabID);
	outstandingDos.push_back(triple<uint32_t,uint32_t,uint32_t>(loopBodID,loopConID,loopEndID));
}
void VulkanComputeShaderBuilder::doWhile(VulkanCSValB testVal){
	triple<uint32_t,uint32_t,uint32_t> outData = outstandingDos[outstandingDos.size()-1];
	mainBuild->codeConditionalJump(testVal.id, outData.first, outData.third);
	mainBuild->codeLabel(outData.first);
}
void VulkanComputeShaderBuilder::doNext(){
	triple<uint32_t,uint32_t,uint32_t> outData = outstandingDos[outstandingDos.size()-1];
	mainBuild->codeJump(outData.second);
	mainBuild->codeLabel(outData.second);
}
void VulkanComputeShaderBuilder::doEnd(){
	uint32_t outMain = outstandingDoMains[outstandingDoMains.size()-1];
	triple<uint32_t,uint32_t,uint32_t> outData = outstandingDos[outstandingDos.size()-1];
	mainBuild->codeJump(outMain);
	mainBuild->codeLabel(outData.third);
	outstandingDoMains.pop_back();
	outstandingDos.pop_back();
}

void VulkanComputeShaderBuilder::functionEnd(){
	mainBuild->codeReturn();
	mainBuild->codeFunctionEnd();
}

#define VARIABLE_COMMON(VarTypeName, ValTypeName) \
	ValTypeName VarTypeName::get(){\
		ValTypeName toRet;\
			toRet.forShad = forShad;\
			toRet.type = type;\
			toRet.id = forShad->mainBuild->opLoad(type, id);\
		return toRet;\
	}\
	void VarTypeName::set(ValTypeName newV){\
		forShad->mainBuild->opStore(id, newV.id);\
	}

VARIABLE_COMMON(VulkanCSVarB, VulkanCSValB)
VARIABLE_COMMON(VulkanCSVarI, VulkanCSValI)
VARIABLE_COMMON(VulkanCSVarL, VulkanCSValL)
VARIABLE_COMMON(VulkanCSVarF, VulkanCSValF)
VARIABLE_COMMON(VulkanCSVarD, VulkanCSValD)

#define ARRAY_COMMON(ArrTypeName, ValTypeName) \
	ValTypeName ArrTypeName::get(VulkanCSValI index){\
		if(!(forShad->canDoCode)){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot make code now.", 0, 0); }\
		uint32_t zeroID = forShad->intCons[0];\
		uint32_t accInds[] = {zeroID, index.id};\
		uint32_t accessID = forShad->mainBuild->opAccessChain(ptrtype, id, 2, accInds);\
		ValTypeName toRet;\
			toRet.forShad = forShad;\
			toRet.type = valtype;\
			toRet.id = forShad->mainBuild->opLoad(valtype, accessID);\
		return toRet;\
	}\
	ValTypeName ArrTypeName::get(VulkanCSValL index){\
		if(!(forShad->canDoCode)){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot make code now.", 0, 0); }\
		uint32_t zeroID = forShad->intCons[0];\
		uint32_t accInds[] = {zeroID, index.id};\
		uint32_t accessID = forShad->mainBuild->opAccessChain(ptrtype, id, 2, accInds);\
		ValTypeName toRet;\
			toRet.forShad = forShad;\
			toRet.type = valtype;\
			toRet.id = forShad->mainBuild->opLoad(valtype, accessID);\
		return toRet;\
	}\
	void ArrTypeName::set(VulkanCSValI index, ValTypeName newV){\
		if(!(forShad->canDoCode)){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot make code now.", 0, 0); }\
		uint32_t zeroID = forShad->intCons[0];\
		uint32_t accInds[] = {zeroID, index.id};\
		uint32_t accessID = forShad->mainBuild->opAccessChain(ptrtype, id, 2, accInds);\
		forShad->mainBuild->opStore(accessID, newV.id);\
	}\
	void ArrTypeName::set(VulkanCSValL index, ValTypeName newV){\
		if(!(forShad->canDoCode)){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot make code now.", 0, 0); }\
		uint32_t zeroID = forShad->intCons[0];\
		uint32_t accInds[] = {zeroID, index.id};\
		uint32_t accessID = forShad->mainBuild->opAccessChain(ptrtype, id, 2, accInds);\
		forShad->mainBuild->opStore(accessID, newV.id);\
	}

ARRAY_COMMON(VulkanCSArrI, VulkanCSValI)
ARRAY_COMMON(VulkanCSArrL, VulkanCSValL)
ARRAY_COMMON(VulkanCSArrF, VulkanCSValF)
ARRAY_COMMON(VulkanCSArrD, VulkanCSValD)

#define OPERATOR_CODE_CHECK \
	if(!(operA.forShad->canDoCode)){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Cannot make code now.", 0, 0); }

#define BINARY_STAGE_CHECK \
	if(operA.forShad != operB.forShad){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Operands come from different shaders.", 0, 0); }

#define TERNARY_STAGE_CHECK \
	if(testV.forShad != operB.forShad){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Operands come from different shaders.", 0, 0); }\
	if(operA.forShad != operB.forShad){ throw WhodunError(WHODUN_ERROR_LEVEL_FATAL, WHODUN_ERROR_SDESC_ASSERT, __FILE__, __LINE__, "Operands come from different shaders.", 0, 0); }

VulkanCSValB whodun::operator!(VulkanCSValB operA){
	OPERATOR_CODE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opLogicalNot(operA.type, operA.id);
	return toRet;
}
VulkanCSValB whodun::operator&(VulkanCSValB operA,VulkanCSValB operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opLogicalAnd(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator|(VulkanCSValB operA,VulkanCSValB operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opLogicalOr(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::select(VulkanCSValB testV,VulkanCSValB operA,VulkanCSValB operB){
	OPERATOR_CODE_CHECK
	TERNARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opSelect(operA.type, testV.id, operA.id, operB.id);
	return toRet;
}
VulkanCSValI whodun::select(VulkanCSValB testV,VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	TERNARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opSelect(operA.type, testV.id, operA.id, operB.id);
	return toRet;
}
VulkanCSValL whodun::select(VulkanCSValB testV,VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	TERNARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opSelect(operA.type, testV.id, operA.id, operB.id);
	return toRet;
}
VulkanCSValF whodun::select(VulkanCSValB testV,VulkanCSValF operA,VulkanCSValF operB){
	OPERATOR_CODE_CHECK
	TERNARY_STAGE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opSelect(operA.type, testV.id, operA.id, operB.id);
	return toRet;
}
VulkanCSValD whodun::select(VulkanCSValB testV,VulkanCSValD operA,VulkanCSValD operB){
	OPERATOR_CODE_CHECK
	TERNARY_STAGE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opSelect(operA.type, testV.id, operA.id, operB.id);
	return toRet;
}



VulkanCSValI whodun::operator+(VulkanCSValI operA){
	OPERATOR_CODE_CHECK
	return operA;
}
VulkanCSValI whodun::operator-(VulkanCSValI operA){
	OPERATOR_CODE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opINeg(operA.type, operA.id);
	return toRet;
}
VulkanCSValI whodun::operator~(VulkanCSValI operA){
	OPERATOR_CODE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opINot(operA.type, operA.id);
	return toRet;
}
VulkanCSValI whodun::operator+(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIAdd(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValI whodun::operator-(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opISub(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValI whodun::operator*(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIMul(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValI whodun::operator/(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIDiv(operA.type, operA.id, operB.id, 1);
	return toRet;
}
VulkanCSValI whodun::operator%(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIMod(operA.type, operA.id, operB.id, 1);
	return toRet;
}
VulkanCSValI whodun::operator<<(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIShiftL(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValI whodun::operator>>(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIShiftR(operA.type, operA.id, operB.id, 1);
	return toRet;
}
VulkanCSValI whodun::operator&(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIAnd(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValI whodun::operator|(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIOr(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValI whodun::operator^(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIXor(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator<(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThan(operA.forShad->typeBoolID, operA.id, operB.id, 1);
	return toRet;
}
VulkanCSValB whodun::operator>(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThan(operA.forShad->typeBoolID, operB.id, operA.id, 1);
	return toRet;
}
VulkanCSValB whodun::operator<=(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThanOrEqual(operA.forShad->typeBoolID, operA.id, operB.id, 1);
	return toRet;
}
VulkanCSValB whodun::operator>=(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThanOrEqual(operA.forShad->typeBoolID, operB.id, operA.id, 1);
	return toRet;
}
VulkanCSValB whodun::operator==(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opIEquals(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator!=(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opINotEquals(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValI whodun::udiv(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIDiv(operA.type, operA.id, operB.id, 0);
	return toRet;
}
VulkanCSValI whodun::umod(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIMod(operA.type, operA.id, operB.id, 0);
	return toRet;
}
VulkanCSValI whodun::ursh(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIShiftR(operA.type, operA.id, operB.id, 0);
	return toRet;
}
VulkanCSValB whodun::uclt(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThan(operA.forShad->typeBoolID, operA.id, operB.id, 0);
	return toRet;
}
VulkanCSValB whodun::ucgt(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThan(operA.forShad->typeBoolID, operB.id, operA.id, 0);
	return toRet;
}
VulkanCSValB whodun::uclte(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThanOrEqual(operA.forShad->typeBoolID, operA.id, operB.id, 0);
	return toRet;
}
VulkanCSValB whodun::ucgte(VulkanCSValI operA,VulkanCSValI operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThanOrEqual(operA.forShad->typeBoolID, operB.id, operA.id, 0);
	return toRet;
}



VulkanCSValL whodun::operator+(VulkanCSValL operA){
	OPERATOR_CODE_CHECK
	return operA;
}
VulkanCSValL whodun::operator-(VulkanCSValL operA){
	OPERATOR_CODE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opINeg(operA.type, operA.id);
	return toRet;
}
VulkanCSValL whodun::operator~(VulkanCSValL operA){
	OPERATOR_CODE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opINot(operA.type, operA.id);
	return toRet;
}
VulkanCSValL whodun::operator+(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIAdd(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValL whodun::operator-(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opISub(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValL whodun::operator*(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIMul(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValL whodun::operator/(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIDiv(operA.type, operA.id, operB.id, 1);
	return toRet;
}
VulkanCSValL whodun::operator%(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIMod(operA.type, operA.id, operB.id, 1);
	return toRet;
}
VulkanCSValL whodun::operator<<(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIShiftL(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValL whodun::operator>>(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIShiftR(operA.type, operA.id, operB.id, 1);
	return toRet;
}
VulkanCSValL whodun::operator&(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIAnd(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValL whodun::operator|(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIOr(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValL whodun::operator^(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIXor(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator<(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThan(operA.forShad->typeBoolID, operA.id, operB.id, 1);
	return toRet;
}
VulkanCSValB whodun::operator>(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThan(operA.forShad->typeBoolID, operB.id, operA.id, 1);
	return toRet;
}
VulkanCSValB whodun::operator<=(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThanOrEqual(operA.forShad->typeBoolID, operA.id, operB.id, 1);
	return toRet;
}
VulkanCSValB whodun::operator>=(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThanOrEqual(operA.forShad->typeBoolID, operB.id, operA.id, 1);
	return toRet;
}
VulkanCSValB whodun::operator==(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opIEquals(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator!=(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opINotEquals(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValL whodun::udiv(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIDiv(operA.type, operA.id, operB.id, 0);
	return toRet;
}
VulkanCSValL whodun::umod(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIMod(operA.type, operA.id, operB.id, 0);
	return toRet;
}
VulkanCSValL whodun::ursh(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opIShiftR(operA.type, operA.id, operB.id, 0);
	return toRet;
}
VulkanCSValB whodun::uclt(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThan(operA.forShad->typeBoolID, operA.id, operB.id, 0);
	return toRet;
}
VulkanCSValB whodun::ucgt(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThan(operA.forShad->typeBoolID, operB.id, operA.id, 0);
	return toRet;
}
VulkanCSValB whodun::uclte(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThanOrEqual(operA.forShad->typeBoolID, operA.id, operB.id, 0);
	return toRet;
}
VulkanCSValB whodun::ucgte(VulkanCSValL operA,VulkanCSValL operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opILessThanOrEqual(operA.forShad->typeBoolID, operB.id, operA.id, 0);
	return toRet;
}



VulkanCSValF whodun::operator+(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	return operA;
}
VulkanCSValF whodun::operator-(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opFNeg(operA.type, operA.id);
	return toRet;
}
VulkanCSValF whodun::operator+(VulkanCSValF operA,VulkanCSValF operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opFAdd(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValF whodun::operator-(VulkanCSValF operA,VulkanCSValF operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opFSub(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValF whodun::operator*(VulkanCSValF operA,VulkanCSValF operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opFMul(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValF whodun::operator/(VulkanCSValF operA,VulkanCSValF operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opFDiv(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator<(VulkanCSValF operA,VulkanCSValF operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFLessThan(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator>(VulkanCSValF operA,VulkanCSValF operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFGreaterThan(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator<=(VulkanCSValF operA,VulkanCSValF operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFLessThanEq(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator>=(VulkanCSValF operA,VulkanCSValF operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFGreaterThanEq(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator==(VulkanCSValF operA,VulkanCSValF operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFEquals(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator!=(VulkanCSValF operA,VulkanCSValF operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFNotEquals(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}



VulkanCSValD whodun::operator+(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	return operA;
}
VulkanCSValD whodun::operator-(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opFNeg(operA.type, operA.id);
	return toRet;
}
VulkanCSValD whodun::operator+(VulkanCSValD operA,VulkanCSValD operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opFAdd(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValD whodun::operator-(VulkanCSValD operA,VulkanCSValD operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opFSub(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValD whodun::operator*(VulkanCSValD operA,VulkanCSValD operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opFMul(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValD whodun::operator/(VulkanCSValD operA,VulkanCSValD operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->mainBuild->opFDiv(operA.type, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator<(VulkanCSValD operA,VulkanCSValD operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFLessThan(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator>(VulkanCSValD operA,VulkanCSValD operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFGreaterThan(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator<=(VulkanCSValD operA,VulkanCSValD operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFLessThanEq(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator>=(VulkanCSValD operA,VulkanCSValD operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFGreaterThanEq(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator==(VulkanCSValD operA,VulkanCSValD operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFEquals(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}
VulkanCSValB whodun::operator!=(VulkanCSValD operA,VulkanCSValD operB){
	OPERATOR_CODE_CHECK
	BINARY_STAGE_CHECK
	VulkanCSValB toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeBoolID;
		toRet.id = operA.forShad->mainBuild->opFNotEquals(operA.forShad->typeBoolID, operA.id, operB.id);
	return toRet;
}



VulkanCSValI whodun::convI(VulkanCSValI operA){
	OPERATOR_CODE_CHECK
	return operA;
}
VulkanCSValI whodun::convI(VulkanCSValL operA){
	OPERATOR_CODE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt32ID;
		toRet.id = operA.forShad->mainBuild->opIConvert(operA.forShad->typeInt32ID, operA.id, 1);
	return toRet;
}
VulkanCSValI whodun::convI(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt32ID;
		toRet.id = operA.forShad->mainBuild->opConvertFToI(operA.forShad->typeInt32ID, operA.id, 1);
	return toRet;
}
VulkanCSValI whodun::convI(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt32ID;
		toRet.id = operA.forShad->mainBuild->opConvertFToI(operA.forShad->typeInt32ID, operA.id, 1);
	return toRet;
}
VulkanCSValL whodun::convL(VulkanCSValI operA){
	OPERATOR_CODE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt64ID;
		toRet.id = operA.forShad->mainBuild->opIConvert(operA.forShad->typeInt64ID, operA.id, 1);
	return toRet;
}
VulkanCSValL whodun::convL(VulkanCSValL operA){
	OPERATOR_CODE_CHECK
	return operA;
}
VulkanCSValL whodun::convL(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt64ID;
		toRet.id = operA.forShad->mainBuild->opConvertFToI(operA.forShad->typeInt64ID, operA.id, 1);
	return toRet;
}
VulkanCSValL whodun::convL(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt64ID;
		toRet.id = operA.forShad->mainBuild->opConvertFToI(operA.forShad->typeInt64ID, operA.id, 1);
	return toRet;
}
VulkanCSValF whodun::convF(VulkanCSValI operA){
	OPERATOR_CODE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat32ID;
		toRet.id = operA.forShad->mainBuild->opConvertIToF(operA.forShad->typeFloat32ID, operA.id, 1);
	return toRet;
}
VulkanCSValF whodun::convF(VulkanCSValL operA){
	OPERATOR_CODE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat32ID;
		toRet.id = operA.forShad->mainBuild->opConvertIToF(operA.forShad->typeFloat32ID, operA.id, 1);
	return toRet;
}
VulkanCSValF whodun::convF(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	return operA;
}
VulkanCSValF whodun::convF(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat32ID;
		toRet.id = operA.forShad->mainBuild->opFConvert(operA.forShad->typeFloat32ID, operA.id);
	return toRet;
}
VulkanCSValD whodun::convD(VulkanCSValI operA){
	OPERATOR_CODE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat64ID;
		toRet.id = operA.forShad->mainBuild->opConvertIToF(operA.forShad->typeFloat64ID, operA.id, 1);
	return toRet;
}
VulkanCSValD whodun::convD(VulkanCSValL operA){
	OPERATOR_CODE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat64ID;
		toRet.id = operA.forShad->mainBuild->opConvertIToF(operA.forShad->typeFloat64ID, operA.id, 1);
	return toRet;
}
VulkanCSValD whodun::convD(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat64ID;
		toRet.id = operA.forShad->mainBuild->opFConvert(operA.forShad->typeFloat64ID, operA.id);
	return toRet;
}
VulkanCSValD whodun::convD(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	return operA;
}



VulkanCSValI whodun::uconvI(VulkanCSValI operA){
	OPERATOR_CODE_CHECK
	return operA;
}
VulkanCSValI whodun::uconvI(VulkanCSValL operA){
	OPERATOR_CODE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt32ID;
		toRet.id = operA.forShad->mainBuild->opIConvert(operA.forShad->typeInt32ID, operA.id, 0);
	return toRet;
}
VulkanCSValI whodun::uconvI(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt32ID;
		toRet.id = operA.forShad->mainBuild->opConvertFToI(operA.forShad->typeInt32ID, operA.id, 0);
	return toRet;
}
VulkanCSValI whodun::uconvI(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	VulkanCSValI toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt32ID;
		toRet.id = operA.forShad->mainBuild->opConvertFToI(operA.forShad->typeInt32ID, operA.id, 0);
	return toRet;
}
VulkanCSValL whodun::uconvL(VulkanCSValI operA){
	OPERATOR_CODE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt64ID;
		toRet.id = operA.forShad->mainBuild->opIConvert(operA.forShad->typeInt64ID, operA.id, 0);
	return toRet;
}
VulkanCSValL whodun::uconvL(VulkanCSValL operA){
	OPERATOR_CODE_CHECK
	return operA;
}
VulkanCSValL whodun::uconvL(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt64ID;
		toRet.id = operA.forShad->mainBuild->opConvertFToI(operA.forShad->typeInt64ID, operA.id, 0);
	return toRet;
}
VulkanCSValL whodun::uconvL(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	VulkanCSValL toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeInt64ID;
		toRet.id = operA.forShad->mainBuild->opConvertFToI(operA.forShad->typeInt64ID, operA.id, 0);
	return toRet;
}
VulkanCSValF whodun::uconvF(VulkanCSValI operA){
	OPERATOR_CODE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat32ID;
		toRet.id = operA.forShad->mainBuild->opConvertIToF(operA.forShad->typeFloat32ID, operA.id, 0);
	return toRet;
}
VulkanCSValF whodun::uconvF(VulkanCSValL operA){
	OPERATOR_CODE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat32ID;
		toRet.id = operA.forShad->mainBuild->opConvertIToF(operA.forShad->typeFloat32ID, operA.id, 0);
	return toRet;
}
VulkanCSValF whodun::uconvF(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	return operA;
}
VulkanCSValF whodun::uconvF(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat32ID;
		toRet.id = operA.forShad->mainBuild->opFConvert(operA.forShad->typeFloat32ID, operA.id);
	return toRet;
}
VulkanCSValD whodun::uconvD(VulkanCSValI operA){
	OPERATOR_CODE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat64ID;
		toRet.id = operA.forShad->mainBuild->opConvertIToF(operA.forShad->typeFloat64ID, operA.id, 0);
	return toRet;
}
VulkanCSValD whodun::uconvD(VulkanCSValL operA){
	OPERATOR_CODE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat64ID;
		toRet.id = operA.forShad->mainBuild->opConvertIToF(operA.forShad->typeFloat64ID, operA.id, 0);
	return toRet;
}
VulkanCSValD whodun::uconvD(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.forShad->typeFloat64ID;
		toRet.id = operA.forShad->mainBuild->opFConvert(operA.forShad->typeFloat64ID, operA.id);
	return toRet;
}
VulkanCSValD whodun::uconvD(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	return operA;
}



VulkanCSValF whodun::vlog(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	VulkanCSValF zeroF = operA.forShad->conF(0.0);
	VulkanCSValF minfF = operA.forShad->conF(-1.0/0.0);
	VulkanCSValF nanF = operA.forShad->conF(0.0/0.0);
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->glslBuild->opLog(operA.type, operA.id);
	return select(operA < zeroF, nanF, select(operA == zeroF, minfF, toRet));
}
VulkanCSValF whodun::vsqrt(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	VulkanCSValF zeroF = operA.forShad->conF(0.0);
	VulkanCSValF nanF = operA.forShad->conF(0.0/0.0);
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->glslBuild->opSqrt(operA.type, operA.id);
	return select(operA < zeroF, nanF, toRet);
}
VulkanCSValF whodun::vexp(VulkanCSValF operA){
	OPERATOR_CODE_CHECK
	VulkanCSValF toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->glslBuild->opExp(operA.type, operA.id);
	return toRet;
}

VulkanCSValD whodun::vlog(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	return convD(vlog(convF(operA)));
}
VulkanCSValD whodun::vsqrt(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	VulkanCSValD zeroF = operA.forShad->conD(0.0);
	VulkanCSValD nanF = operA.forShad->conD(0.0/0.0);
	VulkanCSValD toRet;
		toRet.forShad = operA.forShad;
		toRet.type = operA.type;
		toRet.id = operA.forShad->glslBuild->opSqrt(operA.type, operA.id);
	return select(operA < zeroF, nanF, toRet);
}
VulkanCSValD whodun::vexp(VulkanCSValD operA){
	OPERATOR_CODE_CHECK
	return convD(vexp(convF(operA)));
}



