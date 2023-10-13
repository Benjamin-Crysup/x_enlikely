#include "genlike_progs.h"

#include <iostream>

#include "whodun_vulkan.h"
#include "whodun_stat_randoms.h"

#include "genlike_data.h"

using namespace whodun;

GenlikeSampleFinalLoadings::GenlikeSampleFinalLoadings() : 
	errorModel("errmod"),
	errorModelFlat("--flat-error",&errorModel),
	errorModelStutter("--stutter-error",&errorModel),
	errorRate("--error"),
	efficModel("efficiency"),
	efficModelFlat("--flat-efficiency",&efficModel),
	efficRate("--efficiency"),
	initLoad("--load"),
	numAllele("--allele"),
	numHaplo("--ploidies"),
	maxReact("--maxreact"),
	cycleCount("--cycles"),
	seedValue("--seed"),
	caseFan("--fan"),
	tgtGPU("--gpu"),
	maxSimLoad("--chunk"),
	outFileN("--out")
{
	name = "sampload";
	summary = "Sample final loadings after PCR amplification.";
	version = "genlike sampload 0.0\nCopyright (C) 2022 Benjamin Crysup\nLicense LGPLv3: GNU LGPL version 3\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n";
	usage = "sampload --load 10 --allele 4 --haplo 2 --out OUT.load";
	allOptions.push_back(&errorModelFlat);
	allOptions.push_back(&errorModelStutter);
	allOptions.push_back(&errorRate);
	allOptions.push_back(&efficModelFlat);
	allOptions.push_back(&efficRate);
	allOptions.push_back(&initLoad);
	allOptions.push_back(&numAllele);
	allOptions.push_back(&numHaplo);
	allOptions.push_back(&maxReact);
	allOptions.push_back(&cycleCount);
	allOptions.push_back(&seedValue);
	allOptions.push_back(&caseFan);
	allOptions.push_back(&tgtGPU);
	allOptions.push_back(&maxSimLoad);
	allOptions.push_back(&outFileN);
	
	errorModelFlat.summary = "Errors are uniform between all alleles.";
	errorModelStutter.summary = "Errors bias toward nearby alleles (with shrinking preferred).";
	errorRate.summary = "The rate of error in PCR.";
	efficModelFlat.summary = "All alleles are equally likely to copy.";
	efficRate.summary = "The base probability of copying in a round.";
	initLoad.summary = "The number of templates present at the start.";
	numAllele.summary = "The number of alleles.";
	numHaplo.summary = "The genotype ploidies in play.";
	maxReact.summary = "The maximum number of copies that can be made. Negative for no limit.";
	cycleCount.summary = "The number of cycles of pcr to run.";
	seedValue.summary = "The seed to use for RNG.";
	caseFan.summary = "The number of replicates to make per case.";
	tgtGPU.summary = "The gpu index to target.";
	maxSimLoad.summary = "The number of loadings to do at one time.";
	outFileN.summary = "The file to write the results to.";
	
	errorRate.usage = "--error 0.01";
	efficRate.usage = "--efficiency 1.0";
	initLoad.usage = "--load 10";
	numAllele.usage = "--allele 4";
	numHaplo.usage = "--ploidies 2/1/1";
	maxReact.usage = "--maxreact 1000000";
	cycleCount.usage = "--cycles 20";
	seedValue.usage = "--seed 1234";
	caseFan.usage = "--fan 1024";
	tgtGPU.usage = "--gpu 0";
	maxSimLoad.usage = "--chunk 512";
	outFileN.usage = "--out OUT.load";
	
	outFileN.required = 1;
	outFileN.validExts.push_back(".load");
}
GenlikeSampleFinalLoadings::~GenlikeSampleFinalLoadings(){}
void GenlikeSampleFinalLoadings::idiotCheck(){
	if((errorRate.value < 0.0) || (errorRate.value > 1.0)){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid error rate.", 0, 0);
	}
	if((efficRate.value < 0.0) || (efficRate.value > 1.0)){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid efficiency.", 0, 0);
	}
	if(initLoad.value < 0){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid initial loading.", 0, 0);
	}
	if(numAllele.value <= 1){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid number of alleles.", 0, 0);
	}
	if(cycleCount.value < 0){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid number of cycles.", 0, 0);
	}
	if(caseFan.value <= 0){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid number of replicates.", 0, 0);
	}
	if(maxSimLoad.value <= 0){
		throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Invalid working set size.", 0, 0);
	}
}
void GenlikeSampleFinalLoadings::baseRun(){
	int verbose = 1;
	FileOutStream dumpOStr(0, outFileN.value.c_str());
	try{
		#define CASE_GROUP_SIZE 32
		int needRoundUp = caseFan.value % CASE_GROUP_SIZE;
		caseFan.value = CASE_GROUP_SIZE * (caseFan.value / CASE_GROUP_SIZE);
		if(needRoundUp){ caseFan.value += CASE_GROUP_SIZE; }
		//get the simulation spec ready
			PCRGPUShaderBuilder makeSim;
			makeSim.caseFan = caseFan.value;
			makeSim.numAllele = numAllele.value;
			makeSim.numSite = 1;
			makeSim.maxSimulLoad = maxSimLoad.value / caseFan.value;
				if(makeSim.maxSimulLoad < 1){ makeSim.maxSimulLoad = 1; }
			PCRAmplificationStepSpec simAmpS;
				simAmpS.maxNumMake = maxReact.value;
				simAmpS.numCycles = cycleCount.value;
			makeSim.subSteps.push_back(&simAmpS);
		//build the shader
			uint32_t grpSize[] = {1,CASE_GROUP_SIZE,1};
			uintptr_t simNumBuff = makeSim.numBuffers();
			VulkanComputeShaderBuilder makeShad(1 + simNumBuff, grpSize);
			makeSim.shad = &makeShad;
			//constants
				makeSim.addConstants();
				VulkanCSValL conP1 = makeShad.conL(1);
				makeShad.consDone();
			//interface buffers
				makeSim.addInterface();
				uintptr_t outBuffSI = makeShad.registerInterfaceL();
			//code
				makeShad.functionStart();
				makeSim.addVariables();
				makeSim.addCode(0);
				VulkanCSValL finSetIndex = makeSim.numAL * (makeSim.loadInd * makeSim.numSPL + makeSim.caseInd);
				VulkanCSArrL finSetArray = makeShad.interfaceVarL(outBuffSI);
				for(uintptr_t i = 0; i<makeSim.numAllele; i++){
					finSetArray.set(finSetIndex, makeSim.countVariables[i].get());
					finSetIndex = finSetIndex + conP1;
				}
				makeShad.functionEnd();
//		{
//			//TODO
//			FileOutStream testSO(0, "tmp_shader");
//			testSO.write((char*)(&(makeShad.mainBuild->spirOps[0])), 4*makeShad.mainBuild->spirOps.size());
//			testSO.close();
//		}
		//get vulkan
			VulkanInstance vulkI("genlike_sampload");
				vulkI.toWarn = useErr;
				vulkI.wantLayers.push_back("VK_LAYER_KHRONOS_validation");
			vulkI.create();
		//make the package
			VulkanComputePackageDescription doPack;
			doPack.allPrograms.push_back(&makeShad);
			doPack.progMaxSimulBind.push_back(1);
			doPack.naturalStageSize = 8 * 0x010000;
			std::vector< std::pair<uintptr_t,uintptr_t> > bufferAddrs;
			for(uintptr_t i = 0; i<simNumBuff; i++){
				bufferAddrs.push_back(makeSim.addBuffer(&doPack, i));
			}
			uintptr_t outBuffI = doPack.outputBufferSizes.size();
			doPack.outputBufferSizes.push_back(makeSim.maxSimulLoad * makeSim.numAllele * makeSim.caseFan * 8);
		//and compile
			VulkanComputePackage* gotPack = doPack.prepare(&vulkI, tgtGPU.value);
		try{
			//parse the ploidies
				std::vector<uintptr_t> hotPloidies;
				parsePloidies(toSizePtr(numHaplo.value.c_str()), &hotPloidies);
				if(hotPloidies.size() == 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Missing ploidy information.", 0, 0); }
				uintptr_t totalHaplo = 0;
				for(uintptr_t i = 0; i<hotPloidies.size(); i++){
					uintptr_t curHaplo = hotPloidies[i];
					if(curHaplo == 0){ throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "What exactly would a zero-ploid individual be?", 0, 0); }
					totalHaplo += curHaplo;
				}
			//output the header for the final loadings
				{
					uint64_t curOut; char* curOutC = (char*)(&curOut);
					curOut = 0x0102030405060708; dumpOStr.write(curOutC, 8);
					curOut = makeSim.numAllele; dumpOStr.write(curOutC, 8);
					curOut = initLoad.value; dumpOStr.write(curOutC, 8);
					curOut = hotPloidies.size(); dumpOStr.write(curOutC, 8);
					for(uintptr_t i = 0; i<hotPloidies.size(); i++){
						curOut = hotPloidies[i]; dumpOStr.write(curOutC, 8);
					}
					curOut = makeSim.caseFan; dumpOStr.write(curOutC, 8);
				}
			//set up the stuff to run
				HaploGenotypeSet allGeno(makeSim.numAllele, totalHaplo);
				PloidyLoadingSet allLoad(hotPloidies.size(), &(hotPloidies[0]), initLoad.value);
				if(allLoad.hotLoads.size() == 0){
					throw WhodunError(WHODUN_ERROR_LEVEL_WARNING, WHODUN_ERROR_SDESC_BADCLIARG, __FILE__, __LINE__, "Loading is inconsistent with ploidy.", 0, 0);
				}
				VulkanComputeProgramCallSpec doSinRun;
					doSinRun.numX.push_back(makeSim.maxSimulLoad);
					doSinRun.numY.push_back(makeSim.caseFan / CASE_GROUP_SIZE);
					doSinRun.numZ.push_back(1);
					doSinRun.longhandSteps.resize(1);
					VulkanComputeSingleStepCallSpec* sinRunS0 = &(doSinRun.longhandSteps[0]);
					sinRunS0->programIndex = 0;
					for(uintptr_t i = 0; i<bufferAddrs.size(); i++){
						sinRunS0->assignSpheres.push_back(bufferAddrs[i].first);
						sinRunS0->assignIndices.push_back(bufferAddrs[i].second);
						switch(bufferAddrs[i].first){
							case WHODUN_VULKAN_COMPUTE_BUFFER_INPUT:
								sinRunS0->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_IN); break;
							case WHODUN_VULKAN_COMPUTE_BUFFER_OUTPUT:
								sinRunS0->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_OUT); break;
							case WHODUN_VULKAN_COMPUTE_BUFFER_DATA:
								sinRunS0->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_INOUT); break;
							case WHODUN_VULKAN_COMPUTE_BUFFER_SIDE:
								sinRunS0->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_INOUT); break;
							default:
								throw std::runtime_error("Da fuq?");
						};
					}
					sinRunS0->assignSpheres.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_OUTPUT);
					sinRunS0->assignIndices.push_back(outBuffI);
					sinRunS0->assignUse.push_back(WHODUN_VULKAN_COMPUTE_BUFFER_OUT);
				doSinRun.prepare(gotPack);
			//set all the sites
				{
					VulkanComputeGPUBuffer siteAcc;
					gotPack->openInputBuffer(bufferAddrs[1].second, &siteAcc);
					VulkanComputeGPUBufferOutStream siteOut(&siteAcc, 0, 8*makeSim.maxSimulLoad);
					try{
						uint64_t zeroV = 0;
						for(uintptr_t i = 0; i<makeSim.maxSimulLoad; i++){
							siteOut.write((char*)(&zeroV), 8);
						}
						siteOut.close();
					}
					catch(std::exception& errE){
						siteOut.close();
						throw;
					}
				}
			//set efficiency
				{
					std::vector<uint64_t> packEffs(makeSim.numAllele);
					if(efficModelFlat.value()){
						pcrEfficiencyModelFlat(makeSim.numAllele, efficRate.value, &(packEffs[0]));
					}
					else{
						throw std::runtime_error("Da fuq?");
					}
					VulkanComputeGPUBuffer effAcc;
					gotPack->openDataBuffer(bufferAddrs[3].second, &effAcc);
					effAcc.write(0, makeSim.numAllele * 8, (char*)&(packEffs[0]));
				}
			//set mutation
				{
					std::vector<uint64_t> packMuts(makeSim.numAllele * makeSim.numAllele);
					if(errorModelFlat.value()){
						pcrMutationModelFlat(makeSim.numAllele, errorRate.value, &(packMuts[0]));
					}
					else if(errorModelStutter.value()){
						pcrMutationModelStutter(makeSim.numAllele, errorRate.value, 0.1*errorRate.value, &(packMuts[0]));
					}
					else{
						throw std::runtime_error("Da fuq?");
					}
					VulkanComputeGPUBuffer mutAcc;
					gotPack->openDataBuffer(bufferAddrs[4].second, &mutAcc);
					mutAcc.write(0, makeSim.numAllele * makeSim.numAllele * 8, (char*)&(packMuts[0]));
				}
			//run through the cases making seeds, loadings, running and dumping
				#define SHUFFLE_SIZE 0x01000
				VulkanComputeGPUBuffer loadAcc;
					gotPack->openInputBuffer(bufferAddrs[0].second, &loadAcc);
				VulkanComputeGPUBuffer seedAcc;
					gotPack->openInputBuffer(bufferAddrs[2].second, &seedAcc);
				VulkanComputeGPUBuffer resAcc;
					gotPack->openOutputBuffer(outBuffI, &resAcc);
				uintptr_t curLoadI = 0;
				int* curLoad = &(allLoad.hotLoads[0]);
				uintptr_t numPloid = hotPloidies.size();
				uintptr_t numLoad = allLoad.hotLoads.size() / numPloid;
				uintptr_t curGenoI = 0;
				unsigned short* curGeno = &(allGeno.hotGenos[0]);
				uintptr_t numGeno = allGeno.hotGenos.size() / totalHaplo;
				StructVector<uint64_t> tmpLoad; tmpLoad.resize(makeSim.numAllele);
				StructVector<char> tmpSeed; tmpSeed.resize(makeSim.caseFan * 8);
				MersenneTwisterGenerator doRNG; doRNG.seedI(seedValue.value);
				while(curLoadI < numLoad){
					uintptr_t numToRun = 0;
					VulkanComputeGPUBufferOutStream loadOut(&loadAcc, 0, makeSim.maxSimulLoad * makeSim.numAllele * 8);
					VulkanComputeGPUBufferOutStream seedOut(&seedAcc, 0, makeSim.maxSimulLoad * makeSim.caseFan * 8);
					try{
						while(curLoadI < numLoad){
							while(curGenoI < numGeno){
								//genotype and loading into allele loading
									genotypeLoadingToAlleleLoading(makeSim.numAllele, tmpLoad[0], numPloid, &(hotPloidies[0]), curLoad, curGeno);
									loadOut.write((char*)(tmpLoad[0]), 8*makeSim.numAllele);
								//make a new seed
									doRNG.draw(tmpSeed.size(), tmpSeed[0]);
									seedOut.write(tmpSeed[0], tmpSeed.size());
								//and move along
									curGeno += totalHaplo;
									curGenoI++;
									numToRun++;
									if(numToRun == makeSim.maxSimulLoad){ goto run_a_bitch; }
							}
							curGeno = &(allGeno.hotGenos[0]);
							curGenoI = 0;
							curLoad += numPloid;
							curLoadI++;
						}
						
						run_a_bitch:
						loadOut.close();
						seedOut.close();
					}
					catch(std::exception& errE){
						loadOut.close();
						seedOut.close();
						throw;
					}
					
					if(verbose){
						double curProgress = (numGeno * curLoadI + curGenoI) * 100.0 / (numLoad * numGeno);
						char statusDumpBuff[7+3*(8*sizeof(uintmax_t)+8)];
						sprintf(statusDumpBuff, "%d%%", (int)curProgress);
						const char* passSDB = statusDumpBuff;
						useErr->logError(WHODUN_ERROR_LEVEL_DEBUG, WHODUN_ERROR_SDESC_UPDATE, __FILE__, __LINE__, "Progress", 1, &(passSDB));
					}
					
					if(numToRun){
						doSinRun.numX[0] = numToRun;
						gotPack->run(&doSinRun);
						gotPack->waitProgram();
						//output the results (in system order)
						VulkanComputeGPUBufferInStream doReadRes(&resAcc, 0, numToRun * makeSim.numAllele * makeSim.caseFan * 8);
						try{
							char tmpDumpArr[SHUFFLE_SIZE];
							uintptr_t numR = doReadRes.read(tmpDumpArr, SHUFFLE_SIZE);
							while(numR){
								dumpOStr.write(tmpDumpArr, numR);
								numR = doReadRes.read(tmpDumpArr, SHUFFLE_SIZE);
							}
							doReadRes.close();
						}
						catch(std::exception& errE){
							doReadRes.close();
							throw;
						}
					}
				}
			//clean up
				delete(gotPack); gotPack = 0;
				dumpOStr.close();
		}
		catch(std::exception& errE){
			if(gotPack){ delete(gotPack); }
			throw;
		}
	}
	catch(std::exception& errE){
		dumpOStr.close();
		throw;
	}
}



