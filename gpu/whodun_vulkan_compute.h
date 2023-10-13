#ifndef WHODUN_VULKAN_COMPUTE_H
#define WHODUN_VULKAN_COMPUTE_H 1

/**
 * @file
 * @brief Helper classes for doing gpu compute.
 */

#include <vector>

#include <vulkan/vulkan.h>

#include "whodun_vulkan.h"
#include "whodun_container.h"
#include "whodun_vulkan_spirv.h"

namespace whodun {

class VulkanComputeShaderBuilder;
class VulkanComputePackage;

/**Provide access to a gpu-side buffer.*/
class VulkanComputeGPUBuffer{
public:
	/**Set up an uninitialized*/
	VulkanComputeGPUBuffer();
	/**Tear down*/
	~VulkanComputeGPUBuffer();
	
	/**
	 * Read some bytes from the buffer.
	 * @param fromInd The index to start from.
	 * @param toInd The index to stop at.
	 * @param onCPU The place to copy the data to.
	 */
	void read(uintptr_t fromInd, uintptr_t toInd, char* onCPU);
	
	/**
	 * Write some bytes to the buffer.
	 * @param fromInd The index to start from.
	 * @param toInd The index to stop at.
	 * @param fromCPU The data to update with.
	 */
	void write(uintptr_t fromInd, uintptr_t toInd, const char* fromCPU);
	
	/**The package this is for.*/
	VulkanComputePackage* forPack;
	/**CPU access to the buffer, if available.*/
	char* cpuBuffer;
	/**The gpu view of the buffer.*/
	VulkanBuffer* gpuBuffer;
};

/**The amount of data to move in one go to/from the gpu.*/
#define WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE 0x01000

/**Read from a buffer.*/
class VulkanComputeGPUBufferInStream : public InStream{
public:
	/**
	 * Start reading.
	 * @param fromB The buffer to read.
	 * @param fromInd The byte index to read from.
	 * @param toInd The byte index to read to.
	 */
	VulkanComputeGPUBufferInStream(VulkanComputeGPUBuffer* fromB, uintptr_t fromInd, uintptr_t toInd);
	/**Clean up.*/
	~VulkanComputeGPUBufferInStream();
	int read();
	uintptr_t read(char* toR, uintptr_t numR);
	void close();
	
	/**The buffer to read from.*/
	VulkanComputeGPUBuffer* mainBuff;
	/**Save data temporarily.*/
	char tmpStore[WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE];
	/**The number of things left in tmpStore.*/
	uintptr_t numTmpLeft;
	/**The offset to the first thing in tmpStore.*/
	uintptr_t tmpOffset;
	/**The number of things left in the buffer.*/
	uintptr_t numBuffLeft;
	/**The index of the next thing in the buffer.*/
	uintptr_t nextBuffOff;
};

/**Write to a buffer.*/
class VulkanComputeGPUBufferOutStream : public OutStream{
public:
	/**
	 * Start writing.
	 * @param fromB The buffer to write.
	 * @param fromInd The byte index to write from.
	 * @param toInd The byte index to write to.
	 */
	VulkanComputeGPUBufferOutStream(VulkanComputeGPUBuffer* fromB, uintptr_t fromInd, uintptr_t toInd);
	/**Clean up.*/
	~VulkanComputeGPUBufferOutStream();
	void write(int toW);
	void write(const char* toW, uintptr_t numW);
	void flush();
	void close();
	
	/**The buffer to read from.*/
	VulkanComputeGPUBuffer* mainBuff;
	/**Save data temporarily.*/
	char tmpStore[WHODUN_VULKAN_GPU_BUFFER_STREAM_BLOCKSIZE];
	/**The number of things in tmpStore.*/
	uintptr_t numTmpQueue;
	/**The index of the next thing in the buffer.*/
	uintptr_t nextBuffOff;
};

/**Marker for an input (cpu-side) buffer.*/
#define WHODUN_VULKAN_COMPUTE_BUFFER_INPUT 0
/**Marker for an output (cpu-side) buffer.*/
#define WHODUN_VULKAN_COMPUTE_BUFFER_OUTPUT 1
/**Marker for a data (gpu-side, transferable) buffer.*/
#define WHODUN_VULKAN_COMPUTE_BUFFER_DATA 2
/**Marker for a gpu only buffer.*/
#define WHODUN_VULKAN_COMPUTE_BUFFER_SIDE 3

/**Marker for using a buffer as input.*/
#define WHODUN_VULKAN_COMPUTE_BUFFER_IN 1
/**Marker for using a buffer as output.*/
#define WHODUN_VULKAN_COMPUTE_BUFFER_OUT 2
/**Marker for using a buffer as input and output.*/
#define WHODUN_VULKAN_COMPUTE_BUFFER_INOUT 3

/**A single step in a set of things to run.*/
class VulkanComputeSingleStepCallSpec{
public:
	/**Set up an empty specification.*/
	VulkanComputeSingleStepCallSpec();
	/**Clean up*/
	~VulkanComputeSingleStepCallSpec();
	
	/**The index of the program to run.*/
	uintptr_t programIndex;
	/**The spheres each buffer to assign comes from.*/
	std::vector<uintptr_t> assignSpheres;
	/**The indices in each sphere of the relevant buffers.*/
	std::vector<uintptr_t> assignIndices;
	/**How each buffer will be used.*/
	std::vector<uintptr_t> assignUse;
	//subrange?
};

/**A collection of gpu program calls to make.*/
class VulkanComputeProgramCallSpec{
public:
	/**Set up an empty specification.*/
	VulkanComputeProgramCallSpec();
	/**Clean up*/
	~VulkanComputeProgramCallSpec();
	
	/**The number of x-things to spin up for each. Can be changed after preparation.*/
	std::vector<uint32_t> numX;
	/**The number of y-things to spin up for each. Can be changed after preparation.*/
	std::vector<uint32_t> numY;
	/**The number of z-things to spin up for each. Can be changed after preparation.*/
	std::vector<uint32_t> numZ;
	
	/**The steps to run.*/
	std::vector<VulkanComputeSingleStepCallSpec> longhandSteps;
	
	/**
	 * Figure out stuff to set up for actually running.
	 * @param forPack The package this is for.
	 */
	void prepare(VulkanComputePackage* forPack);
	
	/**The assignments to update.*/
	std::vector<VulkanShaderDataAssignment*> toUpdate;
	/**The layout indices to update.*/
	std::vector<uintptr_t> toUpdateIndex;
	/**The buffers to update to.*/
	std::vector<VulkanBuffer*> toUpdateTo;
	
	/**The assignments to use for each program.*/
	std::vector<VulkanShaderDataAssignment*> stepUseAssign;
	/**Whether each step needs a barrier.*/
	std::vector<uintptr_t> stepNeedBarrier;
	/**The flavor of barrier needed.*/
	std::vector<VkMemoryBarrier> stepBarrier;
	/**Any barriers to wait on at the start.*/
	std::vector<VkBufferMemoryBarrier> startBarrier;
	/**Any barriers to wait on at the end.*/
	std::vector<VkBufferMemoryBarrier> finBarrier;
};

/**A collection of programs and buffers on a graphics card.*/
class VulkanComputePackage{
public:
	/**Set up an empty.*/
	VulkanComputePackage();
	/**Tear down.*/
	~VulkanComputePackage();
	
	/**
	 * Prepare to write an input buffer.
	 * @param index The index of the buffer of interest.
	 * @param toRead The thing to read with.
	 */
	void openInputBuffer(uintptr_t index, VulkanComputeGPUBuffer* toRead);
	
	/**
	 * Prepare to read an output buffer.
	 * @param index The index of the buffer of interest.
	 * @param toRead The thing to read with.
	 */
	void openOutputBuffer(uintptr_t index, VulkanComputeGPUBuffer* toRead);
	
	/**
	 * Prepare to read/write a data buffer.
	 * @param index The index of the buffer of interest.
	 * @param toRead The thing to read with.
	 */
	void openDataBuffer(uintptr_t index, VulkanComputeGPUBuffer* toRead);
	
	/**
	 * Prepare to read/write a side buffer: may not be allowed.
	 * @param index The index of the buffer of interest.
	 * @param toRead The thing to read with.
	 */
	void openSideBuffer(uintptr_t index, VulkanComputeGPUBuffer* toRead);
	/**
	 * Spin up a series of program runs.
	 * @param toRun The things to run. Should be prepared.
	 */
	void run(VulkanComputeProgramCallSpec* toRun);
	/**
	 * Wait for the last program to finish.
	 */
	void waitProgram();
	
	/**The device this is on.*/
	VulkanDevice* saveDev;
	
	/**All the buffers of interest.*/
	VulkanBufferMallocManager* allBuffers; //stage, input, output, data, side
	/**Save a link to the staging buffer.*/
	VulkanBuffer* stageBuffer;
	/**The size of the staging area.*/
	uintptr_t stageSize;
	/**Save a link to the memory of that buffer.*/
	char* stageMemory;
	/**The input buffers.*/
	std::vector<char*> inputBuffers;
	/**The output buffers.*/
	std::vector<char*> outputBuffers;
	/**The data buffers, if they could be mapped.*/
	std::vector<char*> dataBuffers;
	/**The side buffers, if they could be mapped.*/
	std::vector<char*> sideBuffers;
	
	/**Relevant shader layouts.*/
	std::vector<VulkanShaderDataLayout*> allShaderLayouts;
	/**All the shaders.*/
	std::vector<VulkanShader*> allShaders;
	/**All the linked pipelines available for use.*/
	std::vector<VulkanComputePipeline*> allPipelines;
	/**The linkage table.*/
	VulkanShaderDataAssignments* shadLinkTable;
	/**The index of the first binding applicable to each pipeline.*/
	std::vector<uintptr_t> pipelineLinkI0;
	
	/**The natural group size.*/
	std::vector<uint32_t> groupSizeX;
	/**The natural group size.*/
	std::vector<uint32_t> groupSizeY;
	/**The natural group size.*/
	std::vector<uint32_t> groupSizeZ;
	
	/**The compute queue.*/
	VulkanDeviceQueue* computeQueue;
	/**The compute pool.*/
	VulkanCommandPool* computeComPool;
	/**The compute list.*/
	VulkanCommandList* computeJob;
	/**The transfer queue.*/
	VulkanDeviceQueue* transferQueue;
	/**The transfer pool.*/
	VulkanCommandPool* transferComPool;
	/**The transfer list.*/
	VulkanCommandList* transferJob;
	
	/**Wait for commands.*/
	VulkanSyncFence* jobWaitFence;
	/**Wait for transfers.*/
	VulkanSyncFence* tranWaitFence;
};

/**A collection of programs and setup to put on a graphics card.*/
class VulkanComputePackageDescription{
public:
	/**Set up an empty package.*/
	VulkanComputePackageDescription();
	/**Allow subclasses*/
	~VulkanComputePackageDescription();
	
	/**
	 * Try to prepare this package on a given device.
	 * @param hotVulkan The vulkan instance.
	 * @param devIndex The device index.
	 * @return The prepared package.
	 */
	VulkanComputePackage* prepare(VulkanInstance* hotVulkan, uintptr_t devIndex);
	
	/**The sizes needed for input buffers (i.e. to keep on main ram).*/
	std::vector<uintptr_t> inputBufferSizes;
	/**The sizes needed for gpu buffers to which the main program will need (r/w) access.*/
	std::vector<uintptr_t> dataBufferSizes;
	/**The sizes needed for gpu only buffers.*/
	std::vector<uintptr_t> sideBufferSizes;
	/**The sizes needed for output buffers (i.e. to keep on main ram).*/
	std::vector<uintptr_t> outputBufferSizes;
	/**The natural transfer size to use.*/
	uintptr_t naturalStageSize;
	
	/**All the programs to run.*/
	std::vector<VulkanComputeShaderBuilder*> allPrograms;
	/**The maximum number of simultaneous bindings each program may need to use.*/
	std::vector<uintptr_t> progMaxSimulBind;
	
	/**Whether to allow read/write access to the side buffers.*/
	int debugSide;
	/**Put all the buffers on the main ram.*/
	int debugGpuOnCpu;
};

/**A value in a compute shader.*/
class VulkanCSValB{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of the value.*/
	uint32_t type;
	/**The id of the value.*/
	uint32_t id;
};

/**A value in a compute shader.*/
class VulkanCSValI{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of the value.*/
	uint32_t type;
	/**The id of the value.*/
	uint32_t id;
};

/**A value in a compute shader.*/
class VulkanCSValL{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of the value.*/
	uint32_t type;
	/**The id of the value.*/
	uint32_t id;
};

/**A value in a compute shader.*/
class VulkanCSValF{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of the value.*/
	uint32_t type;
	/**The id of the value.*/
	uint32_t id;
};

/**A value in a compute shader.*/
class VulkanCSValD{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of the value.*/
	uint32_t type;
	/**The id of the value.*/
	uint32_t id;
};

/**A variable in a compute shader.*/
class VulkanCSVarB{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of the value.*/
	uint32_t type;
	/**The id of the variable.*/
	uint32_t id;
	/**Get the current value. @return The value*/
	VulkanCSValB get();
	/**Set a new value @param newV The new value.*/
	void set(VulkanCSValB newV);
};

/**A variable in a compute shader.*/
class VulkanCSVarI{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of the value.*/
	uint32_t type;
	/**The id of the variable.*/
	uint32_t id;
	/**Get the current value. @return The value*/
	VulkanCSValI get();
	/**Set a new value @param newV The new value.*/
	void set(VulkanCSValI newV);
};

/**A variable in a compute shader.*/
class VulkanCSVarL{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of the value.*/
	uint32_t type;
	/**The id of the variable.*/
	uint32_t id;
	/**Get the current value. @return The value*/
	VulkanCSValL get();
	/**Set a new value @param newV The new value.*/
	void set(VulkanCSValL newV);
};

/**A variable in a compute shader.*/
class VulkanCSVarF{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of the value.*/
	uint32_t type;
	/**The id of the variable.*/
	uint32_t id;
	/**Get the current value. @return The value*/
	VulkanCSValF get();
	/**Set a new value @param newV The new value.*/
	void set(VulkanCSValF newV);
};

/**A variable in a compute shader.*/
class VulkanCSVarD{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of the value.*/
	uint32_t type;
	/**The id of the variable.*/
	uint32_t id;
	/**Get the current value. @return The value*/
	VulkanCSValD get();
	/**Set a new value @param newV The new value.*/
	void set(VulkanCSValD newV);
};

/**An array in a compute shader.*/
class VulkanCSArrI{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of pointers in this array.*/
	uint32_t ptrtype;
	/**The type of values in this array.*/
	uint32_t valtype;
	/**The id of the interface variable.*/
	uint32_t id;
	/**Get the current value of an entry. @param index The index to get. @return The value.*/
	VulkanCSValI get(VulkanCSValI index);
	/**Get the current value of an entry. @param index The index to get. @return The value.*/
	VulkanCSValI get(VulkanCSValL index);
	/**Set a new value. @param index The index to set. @param newV The new value.*/
	void set(VulkanCSValI index, VulkanCSValI newV);
	/**Set a new value. @param index The index to set. @param newV The new value.*/
	void set(VulkanCSValL index, VulkanCSValI newV);
};

/**An array in a compute shader.*/
class VulkanCSArrL{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of pointers in this array.*/
	uint32_t ptrtype;
	/**The type of values in this array.*/
	uint32_t valtype;
	/**The id of the interface variable.*/
	uint32_t id;
	/**Get the current value of an entry. @param index The index to get. @return The value.*/
	VulkanCSValL get(VulkanCSValI index);
	/**Get the current value of an entry. @param index The index to get. @return The value.*/
	VulkanCSValL get(VulkanCSValL index);
	/**Set a new value. @param index The index to set. @param newV The new value.*/
	void set(VulkanCSValI index, VulkanCSValL newV);
	/**Set a new value. @param index The index to set. @param newV The new value.*/
	void set(VulkanCSValL index, VulkanCSValL newV);
};

/**An array in a compute shader.*/
class VulkanCSArrF{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of pointers in this array.*/
	uint32_t ptrtype;
	/**The type of values in this array.*/
	uint32_t valtype;
	/**The id of the interface variable.*/
	uint32_t id;
	/**Get the current value of an entry. @param index The index to get. @return The value.*/
	VulkanCSValF get(VulkanCSValI index);
	/**Get the current value of an entry. @param index The index to get. @return The value.*/
	VulkanCSValF get(VulkanCSValL index);
	/**Set a new value. @param index The index to set. @param newV The new value.*/
	void set(VulkanCSValI index, VulkanCSValF newV);
	/**Set a new value. @param index The index to set. @param newV The new value.*/
	void set(VulkanCSValL index, VulkanCSValF newV);
};

/**An array in a compute shader.*/
class VulkanCSArrD{
public:
	/**The shader this is in.*/
	VulkanComputeShaderBuilder* forShad;
	/**The type of pointers in this array.*/
	uint32_t ptrtype;
	/**The type of values in this array.*/
	uint32_t valtype;
	/**The id of the interface variable.*/
	uint32_t id;
	/**Get the current value of an entry. @param index The index to get. @return The value.*/
	VulkanCSValD get(VulkanCSValI index);
	/**Get the current value of an entry. @param index The index to get. @return The value.*/
	VulkanCSValD get(VulkanCSValL index);
	/**Set a new value. @param index The index to set. @param newV The new value.*/
	void set(VulkanCSValI index, VulkanCSValD newV);
	/**Set a new value. @param index The index to set. @param newV The new value.*/
	void set(VulkanCSValL index, VulkanCSValD newV);
};

/**Help building a compute shader.*/
class VulkanComputeShaderBuilder{
public:
	/**
	 * Set up the common stuff for a compute shader.
	 * @param numInp The number of buffers in use.
	 * @param groupSizes The natural groupings to use on the graphics card.
	 */
	VulkanComputeShaderBuilder(uintptr_t numInp, uint32_t* groupSizes);
	/**Clean up*/
	~VulkanComputeShaderBuilder();
	
	/**The actual builder.*/
	VulkanSpirVBuilder* mainBuild;
	/**Access to glsl.*/
	VulkanSpirVGLSLExtension* glslBuild;
	
	/**The natural group size.*/
	uint32_t groupSizeX;
	/**The natural group size.*/
	uint32_t groupSizeY;
	/**The natural group size.*/
	uint32_t groupSizeZ;
	
	/****************************************************************/
	//type ids
	/** void */
	uint32_t typeVoidID;
	/** bool */
	uint32_t typeBoolID;
	/** int */
	uint32_t typeInt32ID;
	/** long */
	uint32_t typeInt64ID;
	/** float */
	uint32_t typeFloat32ID;
	/** double */
	uint32_t typeFloat64ID;
	
	/** int[] */
	uint32_t typeArrRInt32ID;
	/** long[] */
	uint32_t typeArrRInt64ID;
	/** float[] */
	uint32_t typeArrRFloat32ID;
	/** double[] */
	uint32_t typeArrRFloat64ID;
	
	/** &_global( int ) */
	uint32_t typePtrUniInt32ID;
	/** &_global( long ) */
	uint32_t typePtrUniInt64ID;
	/** &_global( float ) */
	uint32_t typePtrUniFloat32ID;
	/** &_global( double ) */
	uint32_t typePtrUniFloat64ID;
	
	/** &_local( bool ) */
	uint32_t typePtrFunBoolID;
	/** &_local( int ) */
	uint32_t typePtrFunInt32ID;
	/** &_local( long ) */
	uint32_t typePtrFunInt64ID;
	/** &_local( float ) */
	uint32_t typePtrFunFloat32ID;
	/** &_local( double ) */
	uint32_t typePtrFunFloat64ID;
	
	/** struct{int[]} */
	uint32_t typeStructArrRInt32;
	/** struct{long[]} */
	uint32_t typeStructArrRInt64;
	/** struct{float[]} */
	uint32_t typeStructArrRFloat32;
	/** struct{double[]} */
	uint32_t typeStructArrRFloat64;
	
	/** &_global struct{int[]} */
	uint32_t typePtrUniStructArrRInt32;
	/** &_global struct{long[]} */
	uint32_t typePtrUniStructArrRInt64;
	/** &_global struct{float[]} */
	uint32_t typePtrUniStructArrRFloat32;
	/** &_global struct{double[]} */
	uint32_t typePtrUniStructArrRFloat64;
	
	/****************************************************************/
	//constants
	
	/**The true value.*/
	VulkanCSValB conTrue;
	/**The false value.*/
	VulkanCSValB conFalse;
	
	/**
	 * Make an integer constant.
	 * @param value The value of the constant.
	 * @return The ID of the constant.
	 */
	VulkanCSValI conI(uint32_t value);
	
	/**
	 * Make an integer constant.
	 * @param value The value of the constant.
	 * @return The ID of the constant.
	 */
	VulkanCSValL conL(uint64_t value);
	
	/**
	 * Make a float constant.
	 * @param valuef The value of the constant.
	 * @return The ID of the constant.
	 */
	VulkanCSValF conF(float valuef);
	
	/**
	 * Make a float constant.
	 * @param valuef The value of the constant.
	 * @return The ID of the constant.
	 */
	VulkanCSValD conD(double valuef);
	
	/**The alocated integer constants.*/
	std::map<uint32_t,uint32_t> intCons;
	/**The alocated integer constants.*/
	std::map<uint64_t,uint32_t> longCons;
	/**The alocated float constants (no surprises with nan).*/
	std::map<uint32_t,uint32_t> fltCons;
	/**The alocated float constants (no surprises with nan).*/
	std::map<uint64_t,uint32_t> dblCons;
	/**Whether no more constants can be made.*/
	int noMoreConsts;
	
	/**Call once all constants are finished.*/
	void consDone();
	
	/****************************************************************/
	//interface variables
	
	/**The structure types of the interface variables.*/
	std::vector<uint32_t> interfaceVarTypes;
	/**The ids for the global variables (buffer inputs).*/
	std::vector<uint32_t> globalVarIDs;
	
	/**
	 * Mark an integer interface variable.
	 * @return The index.
	 */
	uintptr_t registerInterfaceI();
	/**
	 * Mark a long interface variable.
	 * @return The index.
	 */
	uintptr_t registerInterfaceL();
	/**
	 * Mark a float interface variable.
	 * @return The index.
	 */
	uintptr_t registerInterfaceF();
	/**
	 * Mark a double interface variable.
	 * @return The index.
	 */
	uintptr_t registerInterfaceD();
	
	/**
	 * Get an interface array.
	 * @param index The index of the interface to get.
	 * @return The array.
	 */
	VulkanCSArrI interfaceVarI(uintptr_t index);
	
	/**
	 * Get an interface array.
	 * @param index The index of the interface to get.
	 * @return The array.
	 */
	VulkanCSArrL interfaceVarL(uintptr_t index);
	
	/**
	 * Get an interface array.
	 * @param index The index of the interface to get.
	 * @return The array.
	 */
	VulkanCSArrF interfaceVarF(uintptr_t index);
	
	/**
	 * Get an interface array.
	 * @param index The index of the interface to get.
	 * @return The array.
	 */
	VulkanCSArrD interfaceVarD(uintptr_t index);
	
	/****************************************************************/
	//code
	
	/**Whether code can be made.*/
	int canDoCode;
	
	/**Start making the function.*/
	void functionStart();
	
	/**
	 * Make a function level variable.
	 * @return The variable.
	 */
	VulkanCSVarB defVarB();
	/**
	 * Make a function level variable.
	 * @return The variable.
	 */
	VulkanCSVarI defVarI();
	/**
	 * Make a function level variable.
	 * @return The variable.
	 */
	VulkanCSVarL defVarL();
	/**
	 * Make a function level variable.
	 * @return The variable.
	 */
	VulkanCSVarF defVarF();
	/**
	 * Make a function level variable.
	 * @return The variable.
	 */
	VulkanCSVarD defVarD();
	
	/**
	 * Get the invocation ID.
	 * @param xyz The dimension of interest.
	 * @return The value.
	 */
	VulkanCSValI invokeGID(int xyz);
	
	/**
	 * Start an if construct.
	 * @param testVal The value to test.
	 */
	void ifStart(VulkanCSValB testVal);
	/**Start working on the else block.*/
	void ifElse();
	/**End the if block.*/
	void ifEnd();
	/**Any outstanding if things (id of else block and id of following block).*/
	std::vector< std::pair<uint32_t,uint32_t> > oustandingIfs;
	
	/**Prepare for a while loop.*/
	void doStart();
	/**
	 * Decide whether to continue the loop.
	 * @param testVal The value to test for the decision.
	 */
	void doWhile(VulkanCSValB testVal);
	/**Prepare to advance to the next iteration.*/
	void doNext();
	/**End the loop.*/
	void doEnd();
	/**The main labels of outstanding loops.*/
	std::vector<uint32_t> outstandingDoMains;
	/**Any outstanding things for loops (id of body, continue and end).*/
	std::vector< triple<uint32_t,uint32_t,uint32_t> > outstandingDos;
	
	/**Close out the function.*/
	void functionEnd();
	
	/****************************************************************/
	//private stuff
	
	/**ID of the main function*/
	uint32_t mainFuncID;
	/**ID of the gl_GlobalInvocationID*/
	uint32_t globInvokeID;
	/** void f() */
	uint32_t typeVoidFuncID;
	/** int[3] */
	uint32_t typeVec3Int32ID;
	/** &_input(int) */
	uint32_t typePtrInpInt32ID;
	/** &_input(int[3]) */
	uint32_t typePtrInpVec3Int32ID;
};

/**Unary not @param operA The value to work over. @return The result.*/
VulkanCSValB operator!(VulkanCSValB operA);
/**Binary and @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator&(VulkanCSValB operA,VulkanCSValB operB);
/**Binary or @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator|(VulkanCSValB operA,VulkanCSValB operB);
/**Ternary operator. @param testV The selecting value. @param operA The true value. @param operB The false value. @return The result.*/
VulkanCSValB select(VulkanCSValB testV,VulkanCSValB operA,VulkanCSValB operB);
/**Ternary operator. @param testV The selecting value. @param operA The true value. @param operB The false value. @return The result.*/
VulkanCSValI select(VulkanCSValB testV,VulkanCSValI operA,VulkanCSValI operB);
/**Ternary operator. @param testV The selecting value. @param operA The true value. @param operB The false value. @return The result.*/
VulkanCSValL select(VulkanCSValB testV,VulkanCSValL operA,VulkanCSValL operB);
/**Ternary operator. @param testV The selecting value. @param operA The true value. @param operB The false value. @return The result.*/
VulkanCSValF select(VulkanCSValB testV,VulkanCSValF operA,VulkanCSValF operB);
/**Ternary operator. @param testV The selecting value. @param operA The true value. @param operB The false value. @return The result.*/
VulkanCSValD select(VulkanCSValB testV,VulkanCSValD operA,VulkanCSValD operB);

/**Unary + @param operA The value to work over. @return The result.*/
VulkanCSValI operator+(VulkanCSValI operA);
/**Unary - @param operA The value to work over. @return The result.*/
VulkanCSValI operator-(VulkanCSValI operA);
/**Unary ~ @param operA The value to work over. @return The result.*/
VulkanCSValI operator~(VulkanCSValI operA);
/**Binary + @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValI operator+(VulkanCSValI operA,VulkanCSValI operB);
/**Binary - @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValI operator-(VulkanCSValI operA,VulkanCSValI operB);
/**Binary * @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValI operator*(VulkanCSValI operA,VulkanCSValI operB);
/**Binary signed / @param operA The first thing. @param operB The second thing. @return The result. */
VulkanCSValI operator/(VulkanCSValI operA,VulkanCSValI operB);
/**Binary signed % @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValI operator%(VulkanCSValI operA,VulkanCSValI operB);
/**Binary << @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValI operator<<(VulkanCSValI operA,VulkanCSValI operB);
/**Binary signed >> @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValI operator>>(VulkanCSValI operA,VulkanCSValI operB);
/**Binary & @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValI operator&(VulkanCSValI operA,VulkanCSValI operB);
/**Binary | @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValI operator|(VulkanCSValI operA,VulkanCSValI operB);
/**Binary ^ @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValI operator^(VulkanCSValI operA,VulkanCSValI operB);
/**Signed compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator<(VulkanCSValI operA,VulkanCSValI operB);
/**Signed compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator>(VulkanCSValI operA,VulkanCSValI operB);
/**Signed compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator<=(VulkanCSValI operA,VulkanCSValI operB);
/**Signed compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator>=(VulkanCSValI operA,VulkanCSValI operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator==(VulkanCSValI operA,VulkanCSValI operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator!=(VulkanCSValI operA,VulkanCSValI operB);
/**Binary unsigned / @param operA The first thing. @param operB The second thing. @return The result. */
VulkanCSValI udiv(VulkanCSValI operA,VulkanCSValI operB);
/**Binary unsigned % @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValI umod(VulkanCSValI operA,VulkanCSValI operB);
/**Binary unsigned >> @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValI ursh(VulkanCSValI operA,VulkanCSValI operB);
/**Unsigned compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB uclt(VulkanCSValI operA,VulkanCSValI operB);
/**Unsigned compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB ucgt(VulkanCSValI operA,VulkanCSValI operB);
/**Unsigned compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB uclte(VulkanCSValI operA,VulkanCSValI operB);
/**Unsigned compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB ucgte(VulkanCSValI operA,VulkanCSValI operB);

/**Unary + @param operA The value to work over. @return The result.*/
VulkanCSValL operator+(VulkanCSValL operA);
/**Unary - @param operA The value to work over. @return The result.*/
VulkanCSValL operator-(VulkanCSValL operA);
/**Unary ~ @param operA The value to work over. @return The result.*/
VulkanCSValL operator~(VulkanCSValL operA);
/**Binary + @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValL operator+(VulkanCSValL operA,VulkanCSValL operB);
/**Binary - @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValL operator-(VulkanCSValL operA,VulkanCSValL operB);
/**Binary * @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValL operator*(VulkanCSValL operA,VulkanCSValL operB);
/**Binary signed / @param operA The first thing. @param operB The second thing. @return The result. */
VulkanCSValL operator/(VulkanCSValL operA,VulkanCSValL operB);
/**Binary signed % @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValL operator%(VulkanCSValL operA,VulkanCSValL operB);
/**Binary << @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValL operator<<(VulkanCSValL operA,VulkanCSValL operB);
/**Binary signed >> @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValL operator>>(VulkanCSValL operA,VulkanCSValL operB);
/**Binary & @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValL operator&(VulkanCSValL operA,VulkanCSValL operB);
/**Binary | @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValL operator|(VulkanCSValL operA,VulkanCSValL operB);
/**Binary ^ @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValL operator^(VulkanCSValL operA,VulkanCSValL operB);
/**Signed compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator<(VulkanCSValL operA,VulkanCSValL operB);
/**Signed compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator>(VulkanCSValL operA,VulkanCSValL operB);
/**Signed compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator<=(VulkanCSValL operA,VulkanCSValL operB);
/**Signed compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator>=(VulkanCSValL operA,VulkanCSValL operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator==(VulkanCSValL operA,VulkanCSValL operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator!=(VulkanCSValL operA,VulkanCSValL operB);
/**Binary unsigned / @param operA The first thing. @param operB The second thing. @return The result. */
VulkanCSValL udiv(VulkanCSValL operA,VulkanCSValL operB);
/**Binary unsigned % @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValL umod(VulkanCSValL operA,VulkanCSValL operB);
/**Binary unsigned >> @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValL ursh(VulkanCSValL operA,VulkanCSValL operB);
/**Unsigned compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB uclt(VulkanCSValL operA,VulkanCSValL operB);
/**Unsigned compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB ucgt(VulkanCSValL operA,VulkanCSValL operB);
/**Unsigned compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB uclte(VulkanCSValL operA,VulkanCSValL operB);
/**Unsigned compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB ucgte(VulkanCSValL operA,VulkanCSValL operB);

/**Unary + @param operA The value to work over. @return The result.*/
VulkanCSValF operator+(VulkanCSValF operA);
/**Unary - @param operA The value to work over. @return The result.*/
VulkanCSValF operator-(VulkanCSValF operA);
/**Binary + @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValF operator+(VulkanCSValF operA,VulkanCSValF operB);
/**Binary - @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValF operator-(VulkanCSValF operA,VulkanCSValF operB);
/**Binary * @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValF operator*(VulkanCSValF operA,VulkanCSValF operB);
/**Binary / @param operA The first thing. @param operB The second thing. @return The result. */
VulkanCSValF operator/(VulkanCSValF operA,VulkanCSValF operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator<(VulkanCSValF operA,VulkanCSValF operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator>(VulkanCSValF operA,VulkanCSValF operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator<=(VulkanCSValF operA,VulkanCSValF operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator>=(VulkanCSValF operA,VulkanCSValF operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator==(VulkanCSValF operA,VulkanCSValF operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator!=(VulkanCSValF operA,VulkanCSValF operB);

/**Unary + @param operA The value to work over. @return The result.*/
VulkanCSValD operator+(VulkanCSValD operA);
/**Unary - @param operA The value to work over. @return The result.*/
VulkanCSValD operator-(VulkanCSValD operA);
/**Binary + @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValD operator+(VulkanCSValD operA,VulkanCSValD operB);
/**Binary - @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValD operator-(VulkanCSValD operA,VulkanCSValD operB);
/**Binary * @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValD operator*(VulkanCSValD operA,VulkanCSValD operB);
/**Binary / @param operA The first thing. @param operB The second thing. @return The result. */
VulkanCSValD operator/(VulkanCSValD operA,VulkanCSValD operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator<(VulkanCSValD operA,VulkanCSValD operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator>(VulkanCSValD operA,VulkanCSValD operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator<=(VulkanCSValD operA,VulkanCSValD operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator>=(VulkanCSValD operA,VulkanCSValD operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator==(VulkanCSValD operA,VulkanCSValD operB);
/**Compare @param operA The first thing. @param operB The second thing. @return The result.*/
VulkanCSValB operator!=(VulkanCSValD operA,VulkanCSValD operB);

/**Convert to signed 32 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValI convI(VulkanCSValI operA);
/**Convert to signed 32 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValI convI(VulkanCSValL operA);
/**Convert to signed 32 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValI convI(VulkanCSValF operA);
/**Convert to signed 32 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValI convI(VulkanCSValD operA);
/**Convert to signed 64 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValL convL(VulkanCSValI operA);
/**Convert to signed 64 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValL convL(VulkanCSValL operA);
/**Convert to signed 64 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValL convL(VulkanCSValF operA);
/**Convert to signed 64 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValL convL(VulkanCSValD operA);
/**Convert signed to 32 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValF convF(VulkanCSValI operA);
/**Convert signed to 32 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValF convF(VulkanCSValL operA);
/**Convert signed to 32 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValF convF(VulkanCSValF operA);
/**Convert signed to 32 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValF convF(VulkanCSValD operA);
/**Convert signed to 64 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValD convD(VulkanCSValI operA);
/**Convert signed to 64 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValD convD(VulkanCSValL operA);
/**Convert signed to 64 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValD convD(VulkanCSValF operA);
/**Convert signed to 64 bit float. @param operA The thing to convert. @return The converted value.*/
VulkanCSValD convD(VulkanCSValD operA);

/**Convert to unsigned 32 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValI uconvI(VulkanCSValI operA);
/**Convert to unsigned 32 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValI uconvI(VulkanCSValL operA);
/**Convert to unsigned 32 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValI uconvI(VulkanCSValF operA);
/**Convert to unsigned 32 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValI uconvI(VulkanCSValD operA);
/**Convert to unsigned 64 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValL uconvL(VulkanCSValI operA);
/**Convert to unsigned 64 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValL uconvL(VulkanCSValL operA);
/**Convert to unsigned 64 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValL uconvL(VulkanCSValF operA);
/**Convert to unsigned 64 bit integer @param operA The thing to convert. @return The converted value.*/
VulkanCSValL uconvL(VulkanCSValD operA);
/**Convert unsigned to 32 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValF uconvF(VulkanCSValI operA);
/**Convert unsigned to 32 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValF uconvF(VulkanCSValL operA);
/**Convert unsigned to 32 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValF uconvF(VulkanCSValF operA);
/**Convert unsigned to 32 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValF uconvF(VulkanCSValD operA);
/**Convert unsigned to 64 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValD uconvD(VulkanCSValI operA);
/**Convert unsigned to 64 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValD uconvD(VulkanCSValL operA);
/**Convert unsigned to 64 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValD uconvD(VulkanCSValF operA);
/**Convert unsigned to 64 bit float @param operA The thing to convert. @return The converted value.*/
VulkanCSValD uconvD(VulkanCSValD operA);

/**Calculate ln. @param operA The value to work over. @return The result.*/
VulkanCSValF vlog(VulkanCSValF operA);
/**Calculate sqrt. @param operA The value to work over. @return The result.*/
VulkanCSValF vsqrt(VulkanCSValF operA);
/**Calculate exp. @param operA The value to work over. @return The result.*/
VulkanCSValF vexp(VulkanCSValF operA);

/**Calculate ln. @param operA The value to work over. @return The result.*/
VulkanCSValD vlog(VulkanCSValD operA);
/**Calculate sqrt. @param operA The value to work over. @return The result.*/
VulkanCSValD vsqrt(VulkanCSValD operA);
/**Calculate exp. @param operA The value to work over. @return The result.*/
VulkanCSValD vexp(VulkanCSValD operA);

/*
Integer u+ u- + - * / mod << >> ~ & | ^ < > <= >= == !=
Float u+ u- + - * / < > <= >= == !=
Integer <-> Float
Integer <-> Integer
Float <-> Float
log sqrt exp ...
//TODO
Float <-> Bits
*/

};

#endif


