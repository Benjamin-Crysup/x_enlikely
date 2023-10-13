#include "whodun_vulkan_spirv.h"

#include <string.h>

using namespace whodun;

#define STRING_NUM_BYTES(toGetNB, lenStoreN, lenStoreW) \
	uint32_t lenStoreN = strlen(toGetNB) + 1;\
	uint32_t lenStoreW = (lenStoreN / 4) + ((lenStoreN % 4) != 0);

VulkanSpirVBuilder::VulkanSpirVBuilder(){
	nextID = 1;
	
	//do you believe in magick?
	spirOps.push_back(0x07230203);
	//spirv version
	spirOps.push_back(0x00010000);
	//this thing's magick number
	spirOps.push_back(0);
	//the highest bound: later commands will update
	spirOps.push_back(nextID);
	//reserved
	spirOps.push_back(0);
	//datums...
}
VulkanSpirVBuilder::~VulkanSpirVBuilder(){}

uint32_t VulkanSpirVBuilder::allocID(){
	uint32_t toRet = nextID;
	nextID++;
	spirOps[3] = nextID;
	return toRet;
}

void VulkanSpirVBuilder::markCapability(uint32_t theCap){
	spirOps.push_back(0x00020011);
	spirOps.push_back(theCap);
}

uint32_t VulkanSpirVBuilder::markExtenstionImport(const char* extName){
	uint32_t useID = allocID();
	STRING_NUM_BYTES(extName, nameLen, nameLenW)
	spirOps.push_back(((2 + nameLenW) << 16) + 0x000B);
	spirOps.push_back(useID);
	helpPackString(extName);
	return useID;
}

void VulkanSpirVBuilder::markMemoryModel(uint32_t addrModel, uint32_t memModel){
	spirOps.push_back(0x0003000E);
	spirOps.push_back(addrModel);
	spirOps.push_back(memModel);
}

void VulkanSpirVBuilder::markEntryPoint(uint32_t execModel, uint32_t targetID, const char* name, uintptr_t numIntInps, uint32_t* intInpIDs){
	STRING_NUM_BYTES(name, nameLen, nameLenW)
	spirOps.push_back(((3 + nameLenW + numIntInps) << 16) + 0x000F);
	spirOps.push_back(execModel);
	spirOps.push_back(targetID);
	helpPackString(name);
	spirOps.insert(spirOps.end(), intInpIDs, intInpIDs + numIntInps);
}

void VulkanSpirVBuilder::helpPackString(const char* toPack){
	uint32_t nameLen = strlen(toPack) + 1;
	uintptr_t ni = 0;
	while(ni < nameLen){
		uint32_t curPack = 0;
		for(uintptr_t i = 0; i<4; i++){
			uint32_t curB = (ni < nameLen) ? (0x00FF & toPack[ni]) : 0;
			curPack = curPack | (curB << (8*i));
			ni++;
		}
		spirOps.push_back(curPack);
	}
}

void VulkanSpirVBuilder::tagEntryExecutionMode(uint32_t targetID, uint32_t toEdit, uintptr_t numLits, uint32_t* theLits){
	spirOps.push_back(((3 + numLits) << 16) + 0x0010);
	spirOps.push_back(targetID);
	spirOps.push_back(toEdit);
	spirOps.insert(spirOps.end(), theLits, theLits + numLits);
}

void VulkanSpirVBuilder::tagDecorateID(uint32_t targetID, uint32_t baseDecor, uint32_t numExt, uint32_t* theExts){
	spirOps.push_back(((3 + numExt) << 16) + 0x0047);
	spirOps.push_back(targetID);
	spirOps.push_back(baseDecor);
	spirOps.insert(spirOps.end(), theExts, theExts + numExt);
}

void VulkanSpirVBuilder::tagDecorateMemberID(uint32_t targetID, uint32_t memberNum, uint32_t baseDecor, uint32_t numExt, uint32_t* theExts){
	spirOps.push_back(((4 + numExt) << 16) + 0x0048);
	spirOps.push_back(targetID);
	spirOps.push_back(memberNum);
	spirOps.push_back(baseDecor);
	spirOps.insert(spirOps.end(), theExts, theExts + numExt);
}

uint32_t VulkanSpirVBuilder::typedefVoid(uint32_t typeID){
	uint32_t useID = typeID ? typeID : allocID();
	spirOps.push_back(0x00020013);
	spirOps.push_back(useID);
	return useID;
}

uint32_t VulkanSpirVBuilder::typedefBool(uint32_t typeID){
	uint32_t useID = typeID ? typeID : allocID();
	spirOps.push_back(0x00020014);
	spirOps.push_back(useID);
	return useID;
}

uint32_t VulkanSpirVBuilder::typedefInt(uint32_t width, uint32_t isSigned, uint32_t typeID){
	uint32_t useID = typeID ? typeID : allocID();
	spirOps.push_back(0x00040015);
	spirOps.push_back(useID);
	spirOps.push_back(width);
	spirOps.push_back(isSigned);
	return useID;
}

uint32_t VulkanSpirVBuilder::typedefFloat(uint32_t width, uint32_t typeID){
	uint32_t useID = typeID ? typeID : allocID();
	spirOps.push_back(0x00030016);
	spirOps.push_back(useID);
	spirOps.push_back(width);
	return useID;
}

uint32_t VulkanSpirVBuilder::typedefFunc(uint32_t retTypeID, uint32_t numArgs, uint32_t* argTypeIDs, uint32_t typeID){
	uint32_t useID = typeID ? typeID : allocID();
	spirOps.push_back(((3 + numArgs) << 16) + 0x0021);
	spirOps.push_back(useID);
	spirOps.push_back(retTypeID);
	spirOps.insert(spirOps.end(), argTypeIDs, argTypeIDs + numArgs);
	return useID;
}

uint32_t VulkanSpirVBuilder::typedefPointer(uint32_t storageFlavor, uint32_t toTypeID, uint32_t typeID){
	uint32_t useID = typeID ? typeID : allocID();
	spirOps.push_back(0x00040020);
	spirOps.push_back(useID);
	spirOps.push_back(storageFlavor);
	spirOps.push_back(toTypeID);
	return useID;
}

uint32_t VulkanSpirVBuilder::typedefVector(uint32_t toTypeID, uint32_t elemCount, uint32_t typeID){
	uint32_t useID = typeID ? typeID : allocID();
	spirOps.push_back(0x00040017);
	spirOps.push_back(useID);
	spirOps.push_back(toTypeID);
	spirOps.push_back(elemCount);
	return useID;
}

uint32_t VulkanSpirVBuilder::typedefStruct(uint32_t numCont, uint32_t* contTypes, uint32_t typeID){
	uint32_t useID = typeID ? typeID : allocID();
	spirOps.push_back(((2 + numCont) << 16) + 0x001E);
	spirOps.push_back(useID);
	spirOps.insert(spirOps.end(), contTypes, contTypes + numCont);
	return useID;
}

uint32_t VulkanSpirVBuilder::typedefRuntimeArray(uint32_t toTypeID, uint32_t typeID){
	uint32_t useID = typeID ? typeID : allocID();
	spirOps.push_back(0x0003001D);
	spirOps.push_back(useID);
	spirOps.push_back(toTypeID);
	return useID;
}

uint32_t VulkanSpirVBuilder::defVariable(uint32_t ofType, uint32_t storageFlavor, uint32_t valID){
	uint32_t useID = valID ? valID : allocID();
	spirOps.push_back(0x0004003B);
	spirOps.push_back(ofType);
	spirOps.push_back(useID);
	spirOps.push_back(storageFlavor);
	return useID;
}

uint32_t VulkanSpirVBuilder::defInitVariable(uint32_t ofType, uint32_t storageFlavor, uint32_t initID, uint32_t valID){
	uint32_t useID = valID ? valID : allocID();
	spirOps.push_back(0x0005003B);
	spirOps.push_back(ofType);
	spirOps.push_back(useID);
	spirOps.push_back(storageFlavor);
	spirOps.push_back(initID);
	return useID;
}

uint32_t VulkanSpirVBuilder::defConstantTrue(uint32_t ofType, uint32_t valID){
	uint32_t useID = valID ? valID : allocID();
	spirOps.push_back(0x00030029);
	spirOps.push_back(ofType);
	spirOps.push_back(useID);
	return useID;
}

uint32_t VulkanSpirVBuilder::defConstantFalse(uint32_t ofType, uint32_t valID){
	uint32_t useID = valID ? valID : allocID();
	spirOps.push_back(0x0003002A);
	spirOps.push_back(ofType);
	spirOps.push_back(useID);
	return useID;
}

uint32_t VulkanSpirVBuilder::defConstant(uint32_t ofType, uint32_t theValue, uint32_t valID){
	uint32_t useID = valID ? valID : allocID();
	spirOps.push_back(0x0004002B);
	spirOps.push_back(ofType);
	spirOps.push_back(useID);
	spirOps.push_back(theValue);
	return useID;
}

uint32_t VulkanSpirVBuilder::defConstantF(uint32_t ofType, float theValue, uint32_t valID){
	union {
		uint32_t asI;
		float asF;
	} punnyName;
	punnyName.asF = theValue;
	return defConstant(ofType, punnyName.asI, valID);
}

uint32_t VulkanSpirVBuilder::defConstant64(uint32_t ofType, uint64_t theValue, uint32_t valID){
	uint32_t useID = valID ? valID : allocID();
	spirOps.push_back(0x0005002B);
	spirOps.push_back(ofType);
	spirOps.push_back(useID);
	spirOps.push_back(theValue);
	spirOps.push_back(theValue >> 32);
	return useID;
}

uint32_t VulkanSpirVBuilder::defConstantF64(uint32_t ofType, double theValue, uint32_t valID){
	union {
		uint64_t asI;
		double asF;
	} punnyName;
	punnyName.asF = theValue;
	return defConstant64(ofType, punnyName.asI, valID);
}

uint32_t VulkanSpirVBuilder::defConstantStruct(uint32_t ofType, uint32_t numStuff, uint32_t* stuffIDs, uint32_t valID){
	uint32_t useID = valID ? valID : allocID();
	spirOps.push_back(((3 + numStuff) << 16) + 0x002C);
	spirOps.push_back(ofType);
	spirOps.push_back(useID);
	spirOps.insert(spirOps.end(), stuffIDs, stuffIDs + numStuff);
	return useID;
}

uint32_t VulkanSpirVBuilder::defFunction(uint32_t retType, uint32_t funcFlavor, uint32_t funcType, uint32_t valID){
	uint32_t useID = valID ? valID : allocID();
	spirOps.push_back(0x00050036);
	spirOps.push_back(retType);
	spirOps.push_back(useID);
	spirOps.push_back(funcFlavor);
	spirOps.push_back(funcType);
	return useID;
}

uint32_t VulkanSpirVBuilder::codeLabel(uint32_t valID){
	uint32_t useID = valID ? valID : allocID();
	spirOps.push_back(0x000200F8);
	spirOps.push_back(useID);
	return useID;
}

void VulkanSpirVBuilder::codeJump(uint32_t toLabel){
	spirOps.push_back(0x000200F9);
	spirOps.push_back(toLabel);
}

void VulkanSpirVBuilder::codeConditionalJump(uint32_t condID, uint32_t onTrue, uint32_t onFalse){
	spirOps.push_back(0x000400FA);
	spirOps.push_back(condID);
	spirOps.push_back(onTrue);
	spirOps.push_back(onFalse);
}

void VulkanSpirVBuilder::codeSelectionMerge(uint32_t mergeLID){
	spirOps.push_back(0x000300F7);
	spirOps.push_back(mergeLID);
	spirOps.push_back(0); //no special markers
}

void VulkanSpirVBuilder::codeLoopMerge(uint32_t mergeLID, uint32_t continueID){
	spirOps.push_back(0x000400F6);
	spirOps.push_back(mergeLID);
	spirOps.push_back(continueID);
	spirOps.push_back(0); //no special markers
}

void VulkanSpirVBuilder::codeReturn(){
	spirOps.push_back(0x000100FD);
}

void VulkanSpirVBuilder::codeFunctionEnd(){
	spirOps.push_back(0x00010038);
}

uint32_t VulkanSpirVBuilder::opLoad(uint32_t expecResType, uint32_t fromPtrID){
	uint32_t useID = allocID();
	spirOps.push_back(0x0004003D);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(fromPtrID);
	return useID;
}

uint32_t VulkanSpirVBuilder::opLoadMark(uint32_t expecResType, uint32_t fromPtrID, uint32_t flags){
	uint32_t useID = allocID();
	spirOps.push_back(0x0005003D);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(fromPtrID);
	spirOps.push_back(flags);
	return useID;
}

void VulkanSpirVBuilder::opStore(uint32_t toPtrID, uint32_t fromValID){
	spirOps.push_back(0x0003003E);
	spirOps.push_back(toPtrID);
	spirOps.push_back(fromValID);
}

void VulkanSpirVBuilder::opStoreMark(uint32_t toPtrID, uint32_t fromValID, uint32_t flags){
	spirOps.push_back(0x0004003E);
	spirOps.push_back(toPtrID);
	spirOps.push_back(fromValID);
	spirOps.push_back(flags);
}

uint32_t VulkanSpirVBuilder::opAccessChain(uint32_t expecResType, uint32_t fromValID, uint32_t numInds, uint32_t* indexIDs){
	uint32_t useID = allocID();
	spirOps.push_back(((4 + numInds) << 16) + 0x0041);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(fromValID);
	spirOps.insert(spirOps.end(), indexIDs, indexIDs + numInds);
	return useID;
}

uint32_t VulkanSpirVBuilder::opLogicalAnd(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500A7);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opLogicalOr(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500A6);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opLogicalNot(uint32_t expecResType, uint32_t opA){
	uint32_t useID = allocID();
	spirOps.push_back(0x000400A8);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	return useID;
}

uint32_t VulkanSpirVBuilder::opIConvert(uint32_t expecResType, uint32_t opA, int useSign){
	uint32_t useID = allocID();
	spirOps.push_back(useSign ? 0x00040072 : 0x00040071);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	return useID;
}

uint32_t VulkanSpirVBuilder::opINeg(uint32_t expecResType, uint32_t opA){
	uint32_t useID = allocID();
	spirOps.push_back(0x0004007E);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	return useID;
}

uint32_t VulkanSpirVBuilder::opIAdd(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x00050080);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opISub(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x00050082);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opIMul(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x00050084);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opIDiv(uint32_t expecResType, uint32_t opA, uint32_t opB, int useSign){
	uint32_t useID = allocID();
	spirOps.push_back(useSign ? 0x00050087 : 0x00050086);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opIMod(uint32_t expecResType, uint32_t opA, uint32_t opB, int useSign){
	uint32_t useID = allocID();
	spirOps.push_back(useSign ? 0x0005008B : 0x00050089);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opIAnd(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500C7);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opIOr(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500C5);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opIXor(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500C6);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opINot(uint32_t expecResType, uint32_t opA){
	uint32_t useID = allocID();
	spirOps.push_back(0x000400C8);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	return useID;
}

uint32_t VulkanSpirVBuilder::opIShiftL(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500C4);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opIShiftR(uint32_t expecResType, uint32_t opA, uint32_t opB, int useSign){
	uint32_t useID = allocID();
	spirOps.push_back(useSign ? 0x000500C3 : 0x000500C2);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opIEquals(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500AA);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opINotEquals(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500AB);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opILessThan(uint32_t expecResType, uint32_t opA, uint32_t opB, int useSign){
	uint32_t useID = allocID();
	spirOps.push_back(useSign ? 0x000500B1 : 0x000500B0);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opILessThanOrEqual(uint32_t expecResType, uint32_t opA, uint32_t opB, int useSign){
	uint32_t useID = allocID();
	spirOps.push_back(useSign ? 0x000500B3 : 0x000500B2);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opConvertIToF(uint32_t expecResType, uint32_t opA, int useSign){
	uint32_t useID = allocID();
	spirOps.push_back(useSign ? 0x0004006F : 0x00040070);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	return useID;
}

uint32_t VulkanSpirVBuilder::opConvertFToI(uint32_t expecResType, uint32_t opA, int useSign){
	uint32_t useID = allocID();
	spirOps.push_back(useSign ? 0x0004006E : 0x0004006D);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFConvert(uint32_t expecResType, uint32_t opA){
	uint32_t useID = allocID();
	spirOps.push_back(0x00040073);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFNeg(uint32_t expecResType, uint32_t opA){
	uint32_t useID = allocID();
	spirOps.push_back(0x0004007F);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFAdd(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x00050081);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFSub(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x00050083);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFMul(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x00050085);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFDiv(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x00050088);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFIsNaN(uint32_t expecResType, uint32_t opA){
	uint32_t useID = allocID();
	spirOps.push_back(0x0004009C);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFIsInf(uint32_t expecResType, uint32_t opA){
	uint32_t useID = allocID();
	spirOps.push_back(0x0004009D);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFEquals(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500B4);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFNotEquals(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500B7); //note: NaN != NaN; This is an odd choice of opcode.
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFLessThan(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500B8);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFLessThanEq(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500BC);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFGreaterThan(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500BA);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opFGreaterThanEq(uint32_t expecResType, uint32_t opA, uint32_t opB){
	uint32_t useID = allocID();
	spirOps.push_back(0x000500BE);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opA);
	spirOps.push_back(opB);
	return useID;
}

uint32_t VulkanSpirVBuilder::opSelect(uint32_t expecResType, uint32_t opCon, uint32_t opT, uint32_t opF){
	uint32_t useID = allocID();
	spirOps.push_back(0x000600A9);
	spirOps.push_back(expecResType);
	spirOps.push_back(useID);
	spirOps.push_back(opCon);
	spirOps.push_back(opT);
	spirOps.push_back(opF);
	return useID;
}




VulkanSpirVGLSLExtension::VulkanSpirVGLSLExtension(VulkanSpirVBuilder* target){
	tgt = target;
	extID = target->markExtenstionImport("GLSL.std.450");
}
VulkanSpirVGLSLExtension::~VulkanSpirVGLSLExtension(){}
uint32_t VulkanSpirVGLSLExtension::opExp(uint32_t expecResType, uint32_t opA){
	uint32_t useID = tgt->allocID();
	tgt->spirOps.push_back(0x0006000C);
	tgt->spirOps.push_back(expecResType);
	tgt->spirOps.push_back(useID);
	tgt->spirOps.push_back(extID);
	tgt->spirOps.push_back(0x0000001B);
	tgt->spirOps.push_back(opA);
	return useID;
}
uint32_t VulkanSpirVGLSLExtension::opLog(uint32_t expecResType, uint32_t opA){
	uint32_t useID = tgt->allocID();
	tgt->spirOps.push_back(0x0006000C);
	tgt->spirOps.push_back(expecResType);
	tgt->spirOps.push_back(useID);
	tgt->spirOps.push_back(extID);
	tgt->spirOps.push_back(0x0000001C);
	tgt->spirOps.push_back(opA);
	return useID;
}
uint32_t VulkanSpirVGLSLExtension::opSqrt(uint32_t expecResType, uint32_t opA){
	uint32_t useID = tgt->allocID();
	tgt->spirOps.push_back(0x0006000C);
	tgt->spirOps.push_back(expecResType);
	tgt->spirOps.push_back(useID);
	tgt->spirOps.push_back(extID);
	tgt->spirOps.push_back(0x0000001F);
	tgt->spirOps.push_back(opA);
	return useID;
}




