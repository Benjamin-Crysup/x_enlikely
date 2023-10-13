#ifndef WHODUN_VULKAN_SPIRV_H
#define WHODUN_VULKAN_SPIRV_H 1

/**
 * @file
 * @brief Helper classes to build shaders.
 */

#include <vector>

#include <vulkan/vulkan.h>

namespace whodun {

/**Build a spir-v shader in memory.*/
class VulkanSpirVBuilder{
public:
	/**Set up an empty shader.*/
	VulkanSpirVBuilder();
	/**Tear down.*/
	~VulkanSpirVBuilder();
	
	/**
	 * Get an ID for later use.
	 * @return The ID.
	 */
	uint32_t allocID();
	
	/**
	 * Add a capability.
	 * @param theCap The required capability. https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html#Capabilities
	 */
	void markCapability(uint32_t theCap);
	
	/**
	 * Mark the import of an extension.
	 * @param extName The name of the extension.
	 * @return The id to use to refer to the extension.
	 */
	uint32_t markExtenstionImport(const char* extName);
	
	/**
	 * Set the memory model used by this shader.
	 * @param addrModel The address model (how are addresses stored).
	 * @param memModel The memory model (any weird constraints).
	 */
	void markMemoryModel(uint32_t addrModel, uint32_t memModel);
	
	/**
	 * Add an entry point.
	 * @param execModel The flavor of shader.
	 * @param targetID The id of the function.
	 * @param name The name to apply to this entry point.
	 * @param numIntInps The number of inputs in the interface.
	 * @param intInpIDs The IDs of the inputs in the interface.
	 */
	void markEntryPoint(uint32_t execModel, uint32_t targetID, const char* name, uintptr_t numIntInps, uint32_t* intInpIDs);
	
	/**
	 * Add some data to an entry point.
	 * @param targetID The id of the function.
	 * @param toEdit The value to tack on.
	 * @param numLits THe number of literal values supplied.
	 * @param theLits The literal values supplied.
	 */
	void tagEntryExecutionMode(uint32_t targetID, uint32_t toEdit, uintptr_t numLits, uint32_t* theLits);
	
	/**
	 * Add decoration to an ID.
	 * @param targetID The ID to decorate.
	 * @param baseDecor The decoration to add.
	 * @param numExt The number of extra things to add.
	 * @param theExts The extra things to add.
	 */
	void tagDecorateID(uint32_t targetID, uint32_t baseDecor, uint32_t numExt, uint32_t* theExts);
	
	/**
	 * Add decoration to a member of a structure ID.
	 * @param targetID The ID to decorate.
	 * @param memberNum The index of the member.
	 * @param baseDecor The decoration to add.
	 * @param numExt The number of extra things to add.
	 * @param theExts The extra things to add.
	 */
	void tagDecorateMemberID(uint32_t targetID, uint32_t memberNum, uint32_t baseDecor, uint32_t numExt, uint32_t* theExts);
	
	/**
	 * Allocate a type for void.
	 * @param typeID If the type was preallocated, what it's ID was.
	 * @return The typeID.
	 */
	uint32_t typedefVoid(uint32_t typeID = 0);
	
	/**
	 * Allocate a type for boolean.
	 * @param typeID If the type was preallocated, what it's ID was.
	 * @return The typeID.
	 */
	uint32_t typedefBool(uint32_t typeID = 0);
	
	/**
	 * Allocate a type for int.
	 * @param width The bit-width of the thing.
	 * @param isSigned Whether it is signed (used for id10t checking).
	 * @param typeID If the type was preallocated, what it's ID was.
	 * @return The typeID.
	 */
	uint32_t typedefInt(uint32_t width, uint32_t isSigned, uint32_t typeID = 0);
	
	/**
	 * Allocate a type for float.
	 * @param width The bit-width of the thing.
	 * @param typeID If the type was preallocated, what it's ID was.
	 * @return The typeID.
	 */
	uint32_t typedefFloat(uint32_t width, uint32_t typeID = 0);
	
	/**
	 * Allocate a type for a function.
	 * @param retTypeID The type ID for the return.
	 * @param numArgs The number of arguments.
	 * @param argTypeIDs The type IDs of the arguments.
	 * @param typeID If the type was preallocated, what it's ID was.
	 * @return The typeID.
	 */
	uint32_t typedefFunc(uint32_t retTypeID, uint32_t numArgs, uint32_t* argTypeIDs, uint32_t typeID = 0);
	
	/**
	 * Allocate a type for a pointer.
	 * @param storageFlavor The flavor of storage this uses.
	 * @param toTypeID The type of thing this points to.
	 * @param typeID If the type was preallocated, what it's ID was.
	 * @return The typeID.
	 */
	uint32_t typedefPointer(uint32_t storageFlavor, uint32_t toTypeID, uint32_t typeID = 0);
	
	/**
	 * Allocate a type for a vector.
	 * @param toTypeID The type of thing this is made of.
	 * @param elemCount The number of that thing in the vector.
	 * @param typeID If the type was preallocated, what it's ID was.
	 * @return The typeID.
	 */
	uint32_t typedefVector(uint32_t toTypeID, uint32_t elemCount, uint32_t typeID = 0);
	
	/**
	 * Allocate a type for a struct.
	 * @param numCont The number of things in the struct.
	 * @param contTypes The types of those things.
	 * @param typeID If the type was preallocated, what it's ID was.
	 * @return The typeID.
	 */
	uint32_t typedefStruct(uint32_t numCont, uint32_t* contTypes, uint32_t typeID = 0);
	
	/**
	 * Allocate a type for an array of unknown size.
	 * @param toTypeID The type of thing this is made of.
	 * @param typeID If the type was preallocated, what it's ID was.
	 * @return The typeID.
	 */
	uint32_t typedefRuntimeArray(uint32_t toTypeID, uint32_t typeID = 0);
	
	/**
	 * Allocate a variable.
	 * @param ofType The type of the variable.
	 * @param storageFlavor The type of storage to cram it in.
	 * @param valID If the variable was preallocated, what it's ID was.
	 * @return The value ID.
	 */
	uint32_t defVariable(uint32_t ofType, uint32_t storageFlavor, uint32_t valID = 0);
	
	/**
	 * Allocate a variable.
	 * @param ofType The type of the variable.
	 * @param storageFlavor The type of storage to cram it in.
	 * @param initID The id of the value to initialize it to.
	 * @param valID If the variable was preallocated, what it's ID was.
	 * @return The value ID.
	 */
	uint32_t defInitVariable(uint32_t ofType, uint32_t storageFlavor, uint32_t initID, uint32_t valID = 0);
	
	/**
	 * Allocate constant true.
	 * @param ofType The type of the constant.
	 * @param valID If the variable was preallocated, what it's ID was.
	 * @return The value ID.
	 */
	uint32_t defConstantTrue(uint32_t ofType, uint32_t valID = 0);
	
	/**
	 * Allocate constant false.
	 * @param ofType The type of the constant.
	 * @param valID If the variable was preallocated, what it's ID was.
	 * @return The value ID.
	 */
	uint32_t defConstantFalse(uint32_t ofType, uint32_t valID = 0);
	
	/**
	 * Allocate a small constant.
	 * @param ofType The type of the constant.
	 * @param theValue The value to use for the thing.
	 * @param valID If the variable was preallocated, what it's ID was.
	 * @return The value ID.
	 */
	uint32_t defConstant(uint32_t ofType, uint32_t theValue, uint32_t valID = 0);
	
	/**
	 * Allocate a small constant.
	 * @param ofType The type of the constant.
	 * @param theValue The value to use for the thing.
	 * @param valID If the variable was preallocated, what it's ID was.
	 * @return The value ID.
	 */
	uint32_t defConstantF(uint32_t ofType, float theValue, uint32_t valID = 0);
	
	/**
	 * Allocate a 64 bit constant.
	 * @param ofType The type of the constant.
	 * @param theValue The value to use for the thing.
	 * @param valID If the variable was preallocated, what it's ID was.
	 * @return The value ID.
	 */
	uint32_t defConstant64(uint32_t ofType, uint64_t theValue, uint32_t valID = 0);
	
	/**
	 * Allocate a 64 bit constant.
	 * @param ofType The type of the constant.
	 * @param theValue The value to use for the thing.
	 * @param valID If the variable was preallocated, what it's ID was.
	 * @return The value ID.
	 */
	uint32_t defConstantF64(uint32_t ofType, double theValue, uint32_t valID = 0);
	
	/**
	 * Allocate a composite constant.
	 * @param ofType The type of the constant.
	 * @param numStuff The number of values to use for the thing.
	 * @param stuffIDs The ids of the values to use for the thing.
	 * @param valID If the variable was preallocated, what it's ID was.
	 * @return The value ID.
	 */
	uint32_t defConstantStruct(uint32_t ofType, uint32_t numStuff, uint32_t* stuffIDs, uint32_t valID = 0);
	
	/**
	 * Start up a function.
	 * @param retType The return type of the function.
	 * @param funcFlavor Extra info to mark up the function.
	 * @param funcType The signature of the function.
	 * @param valID If the function was preallocated, what it's ID was.
	 * @return The value ID.
	 */
	uint32_t defFunction(uint32_t retType, uint32_t funcFlavor, uint32_t funcType, uint32_t valID = 0);
	
	/**
	 * Start a block.
	 * @param valID If the function was preallocated, what it's ID was.
	 * @return The value ID.
	 */
	uint32_t codeLabel(uint32_t valID = 0);
	
	/**
	 * Unconditional jump to label.
	 * @param toLabel The ID of the label to jump to.
	 */
	void codeJump(uint32_t toLabel);
	
	/**
	 * Conditional jump to label.
	 * @param condID The ID of the boolean to jump on.
	 * @param onTrue The place to jump on true.
	 * @param onFalse The place to jump on false.
	 */
	void codeConditionalJump(uint32_t condID, uint32_t onTrue, uint32_t onFalse);
	
	/**
	 * Prepare for an if statement.
	 * @param mergeLID The place both branches wind up.
	 */
	void codeSelectionMerge(uint32_t mergeLID);
	
	/**
	 * Set up a loop.
	 * @param mergeLID The ID of the label after the loop.
	 * @param continueID The ID of the label to jump to on continue.
	 */
	void codeLoopMerge(uint32_t mergeLID, uint32_t continueID);
	
	/**
	 * Return with no value.
	 */
	void codeReturn();
	
	/**
	 * End a function.
	 */
	void codeFunctionEnd();
	
	/**
	 * Load a value from a pointer.
	 * @param expecResType The expected type of the result.
	 * @param fromPtrID The pointer value to load from.
	 * @return The ID of the value.
	 */
	uint32_t opLoad(uint32_t expecResType, uint32_t fromPtrID);
	
	/**
	 * Load a value from a pointer.
	 * @param expecResType The expected type of the result.
	 * @param fromPtrID The pointer value to load from.
	 * @param flags Any extra flags to pass along.
	 * @return The ID of the value.
	 */
	uint32_t opLoadMark(uint32_t expecResType, uint32_t fromPtrID, uint32_t flags);
	
	/**
	 * Store a value to a pointer.
	 * @param toPtrID The pointer value to store to.
	 * @param fromValID The value to store.
	 */
	void opStore(uint32_t toPtrID, uint32_t fromValID);
	
	/**
	 * Store a value to a pointer.
	 * @param toPtrID The pointer value to store to.
	 * @param fromValID The value to store.
	 * @param flags Any extra flags to pass along.
	 */
	void opStoreMark(uint32_t toPtrID, uint32_t fromValID, uint32_t flags);
	
	/**
	 * Address of a thing in a composite type.
	 * @param expecResType The expected type of the address (pointer to something).
	 * @param fromValID The ID of the value to walk through.
	 * @param numInds The number of indices to walk through.
	 * @param indexIDs The indices to walk through.
	 * @return The ID of the resultant pointer.
	 */
	uint32_t opAccessChain(uint32_t expecResType, uint32_t fromValID, uint32_t numInds, uint32_t* indexIDs);
	
	/**
	 * Boolean and.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opLogicalAnd(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Boolean or.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opLogicalOr(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Boolean not.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @return The ID of the result.
	 */
	uint32_t opLogicalNot(uint32_t expecResType, uint32_t opA);
	
	/**
	 * Convert between integer types.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the starting value.
	 * @param useSign Whether to sign extend or zero extend.
	 * @return The ID of the result.
	 */
	uint32_t opIConvert(uint32_t expecResType, uint32_t opA, int useSign);
	
	/**
	 * Integer negate.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @return The ID of the result.
	 */
	uint32_t opINeg(uint32_t expecResType, uint32_t opA);
	
	/**
	 * Integer add.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opIAdd(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Integer subtract.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opISub(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Integer multiply.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opIMul(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Integer division.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @param useSign Whether this is a signed or unsigned division.
	 * @return The ID of the result.
	 */
	uint32_t opIDiv(uint32_t expecResType, uint32_t opA, uint32_t opB, int useSign);
	
	/**
	 * Integer modulus.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @param useSign Whether this is a signed or unsigned division. If signed, result will have same sign as opB.
	 * @return The ID of the result.
	 */
	uint32_t opIMod(uint32_t expecResType, uint32_t opA, uint32_t opB, int useSign);
	
	/**
	 * Bitwise and.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opIAnd(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Bitwise or.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opIOr(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Bitwise xor.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opIXor(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Bitwise not.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @return The ID of the result.
	 */
	uint32_t opINot(uint32_t expecResType, uint32_t opA);
	
	/**
	 * Bit shift left.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opIShiftL(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Bit shift right.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @param useSign Whether to sign extend or zero fill.
	 * @return The ID of the result.
	 */
	uint32_t opIShiftR(uint32_t expecResType, uint32_t opA, uint32_t opB, int useSign);
	
	/**
	 * Integer equality.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opIEquals(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Integer equality.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opINotEquals(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Integer less than.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @param useSign Whether this is a signed or unsigned comparison.
	 * @return The ID of the result.
	 */
	uint32_t opILessThan(uint32_t expecResType, uint32_t opA, uint32_t opB, int useSign);
	
	/**
	 * Integer less than or equal.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @param useSign Whether this is a signed or unsigned comparison.
	 * @return The ID of the result.
	 */
	uint32_t opILessThanOrEqual(uint32_t expecResType, uint32_t opA, uint32_t opB, int useSign);
	
	/**
	 * Convert an integer to a floating point number.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the starting value.
	 * @param useSign Whether the integer is signed.
	 * @return The ID of the result.
	 */
	uint32_t opConvertIToF(uint32_t expecResType, uint32_t opA, int useSign);
	
	/**
	 * Convert a floating point number to an integer.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the starting value.
	 * @param useSign Whether the integer is signed.
	 * @return The ID of the result.
	 */
	uint32_t opConvertFToI(uint32_t expecResType, uint32_t opA, int useSign);
	
	/**
	 * Convert between integer types.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the starting value.
	 * @return The ID of the result.
	 */
	uint32_t opFConvert(uint32_t expecResType, uint32_t opA);
	
	/**
	 * Floating point negate.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @return The ID of the result.
	 */
	uint32_t opFNeg(uint32_t expecResType, uint32_t opA);
	
	/**
	 * Floating point add.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opFAdd(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Floating point subtract.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opFSub(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Floating point multiply.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opFMul(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Floating point divide.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opFDiv(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Floating point check.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @return The ID of the result.
	 */
	uint32_t opFIsNaN(uint32_t expecResType, uint32_t opA);
	
	/**
	 * Floating point check.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @return The ID of the result.
	 */
	uint32_t opFIsInf(uint32_t expecResType, uint32_t opA);
	
	/**
	 * Float equality.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opFEquals(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Float inequality.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opFNotEquals(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Floating point comparison.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opFLessThan(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Floating point comparison.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opFLessThanEq(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Floating point comparison.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opFGreaterThan(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Floating point comparison.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the first operand.
	 * @param opB The ID of the second operand.
	 * @return The ID of the result.
	 */
	uint32_t opFGreaterThanEq(uint32_t expecResType, uint32_t opA, uint32_t opB);
	
	/**
	 * Ternary operator.
	 * @param expecResType The expected type of the result.
	 * @param opCon The ID of the value to select on.
	 * @param opT The ID of the value if true.
	 * @param opF The ID of the value if false.
	 * @return The ID of the result.
	 */
	uint32_t opSelect(uint32_t expecResType, uint32_t opCon, uint32_t opT, uint32_t opF);
	
	//TODO
	
	/**The data in the shader.*/
	std::vector<uint32_t> spirOps;
	/**The next unused ID.*/
	uint32_t nextID;
	
	/**
	 * Helper function, pack a string.
	 * @param toPack The string to pack.
	 */
	void helpPackString(const char* toPack);
};

/**Use glsl functions/opcodes.*/
class VulkanSpirVGLSLExtension{
public:
	/**
	 * Prepare to add extension instructions.
	 * @param target The place to add them.
	 */
	VulkanSpirVGLSLExtension(VulkanSpirVBuilder* target);
	/**Clean up*/
	~VulkanSpirVGLSLExtension();
	
	/**The thing being built.*/
	VulkanSpirVBuilder* tgt;
	/**The id of this extension.*/
	uint32_t extID;
	
	/**
	 * e^x.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the value of interest.
	 * @return The ID of the result.
	 */
	uint32_t opExp(uint32_t expecResType, uint32_t opA);
	
	/**
	 * Natural log.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the value of interest.
	 * @return The ID of the result.
	 */
	uint32_t opLog(uint32_t expecResType, uint32_t opA);
	
	/**
	 * Square root.
	 * @param expecResType The expected type of the result.
	 * @param opA The ID of the value of interest.
	 * @return The ID of the result.
	 */
	uint32_t opSqrt(uint32_t expecResType, uint32_t opA);
	
	//TODO
};

};

#endif

