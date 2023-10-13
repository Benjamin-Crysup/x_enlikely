#include "whodungmain_programs.h"

#include "whodun_vulkan.h"

using namespace whodun;

WhodunGPUInfoProgram::WhodunGPUInfoProgram(){
	name = "gpuinfo";
	summary = "Spit out info on gpus.";
	version = "whodunu gpuinfo 0.0\nCopyright (C) 2023 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	usage = "gpuinfo";
}
WhodunGPUInfoProgram::~WhodunGPUInfoProgram(){}
void WhodunGPUInfoProgram::baseRun(){
	char asciiBuff[8*sizeof(uintmax_t)+16];
	//set up the instance
		VulkanInstance vulkInst("whodun-gpuinfo");
		vulkInst.toWarn = useErr;
		vulkInst.wantLayers.push_back("VK_LAYER_KHRONOS_validation");
		vulkInst.create();
	//list the devices
	for(uintptr_t i = 0; i<vulkInst.hotDevs.size(); i++){
		VulkanDeviceData* curDev = &(vulkInst.hotDevs[i]);
		VkPhysicalDeviceProperties* curProps = &(curDev->rawProperties);
			useOut->write(curProps->deviceName); useOut->write('\n');
			const char* devTypeS;
			switch(curProps->deviceType){
				case VK_PHYSICAL_DEVICE_TYPE_CPU:
					devTypeS = "cpu"; break;
				case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
					devTypeS = "virtual"; break;
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
					devTypeS = "discrete"; break;
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
					devTypeS = "integrated"; break;
				case VK_PHYSICAL_DEVICE_TYPE_OTHER:
					devTypeS = "other"; break;
				default:
					devTypeS = "???";
			};
			useOut->write(devTypeS); useOut->write('\n');
		for(uintptr_t j = 0; j<curDev->rawQueues.size(); j++){
			VkQueueFamilyProperties* curQueue = &(curDev->rawQueues[j]);
			sprintf(asciiBuff, "Q%d ", (int)j);
			useOut->write(asciiBuff);
			if(curQueue->queueFlags & VK_QUEUE_GRAPHICS_BIT){ useOut->write('G'); }
			if(curQueue->queueFlags & VK_QUEUE_COMPUTE_BIT){ useOut->write('C'); }
			if(curQueue->queueFlags & VK_QUEUE_TRANSFER_BIT){ useOut->write('T'); }
			sprintf(asciiBuff, "x%d", (int)(curQueue->queueCount));
			useOut->write(asciiBuff);
			useOut->write('\n');
		}
		for(uintptr_t j = 0; j<curDev->rawMemory.memoryTypeCount; j++){
			VkMemoryType* curMemTp = &(curDev->rawMemory.memoryTypes[j]);
			sprintf(asciiBuff, "Mtp%d ", (int)j);
			useOut->write(asciiBuff);
			if(curMemTp->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT){ useOut->write('L'); }
			if(curMemTp->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT){ useOut->write('V'); }
			if(curMemTp->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT){ useOut->write('C'); }
			if(curMemTp->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT){ useOut->write('$'); }
			sprintf(asciiBuff, " -> M[%d]", (int)(curMemTp->heapIndex));
			useOut->write(asciiBuff);
			useOut->write('\n');
		}
		for(uintptr_t j = 0; j<curDev->rawMemory.memoryHeapCount; j++){
			VkMemoryHeap* curMemH = &(curDev->rawMemory.memoryHeaps[j]);
			sprintf(asciiBuff, "M%d ", (int)j);
			useOut->write(asciiBuff);
			VkDeviceSize memSize = curMemH->size;
			const char* memUnits = "bkMGTPEZYRQ";
			while(memSize > 2048){
				if(memUnits[1] == 0){ break; }
				memSize = memSize / 1024;
				memUnits++;
			}
			
			sprintf(asciiBuff, "%ld", (long)memSize);
			useOut->write(asciiBuff); useOut->write(memUnits[0]);;
			useOut->write(' ');
			if(curMemH->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT){ useOut->write('L'); }
			useOut->write('\n');
		}
		//TODO
		useOut->write("\n");
	}
	//useOut
	//TODO
}

