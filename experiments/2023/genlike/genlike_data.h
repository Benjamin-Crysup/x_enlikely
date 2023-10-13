#ifndef GENLIKE_DATA_H
#define GENLIKE_DATA_H 1

#include "whodun_oshook.h"
#include "whodun_stat_table.h"
#include "whodun_math_matrix.h"
#include "whodun_vulkan_compute.h"

namespace whodun {

/**
 * Load a matrix from a file.
 * @param toRead The file to read.
 * @param toFill The place to put it.
 */
void loadMatrix(InStream* toRead, DenseMatrix* toFill);

/**
 * Load a matrix from a table file.
 * @param toRead The file to read.
 * @param toFill The place to put it.
 */
void loadMatrixTable(TextTableReader* toRead, DenseMatrix* toFill);

/**
 * Save a matrix to a file.
 * @param toRead The file to write to.
 * @param toFill The data to write.
 */
void saveMatrix(OutStream* toRead, DenseMatrix* toFill);

/**
 * Parse a ploidy argument.
 * @param toParse The argument to parse.
 * @param savePloids The place to save the ploidies.
 */
void parsePloidies(SizePtrString toParse, std::vector<uintptr_t>* savePloids);

/**List all the genotypes possible from a given number of haploid individuals.*/
class HaploGenotypeSet{
public:
	/**
	 * Make an empty set.
	 */
	HaploGenotypeSet();
	/**
	 * Walk through, figuring out which items are valid.
	 * @param numAllele The number of alleles in play.
	 * @param numHaplo The number of haploid individuals.
	 */
	HaploGenotypeSet(uintptr_t numAllele, uintptr_t numHaplo);
	/**Clean up.*/
	~HaploGenotypeSet();
	
	/**The number of alleles in play.*/
	uintptr_t numAl;
	/**The number of haplotypes in play.*/
	uintptr_t numH;
	/**The genotypes of interest.*/
	std::vector<unsigned short> hotGenos;
};

/**List all the loadings possible for a given set of ploidies and a total loading. Limits to non-redundant (with earlier ploidies being major).*/
class PloidyLoadingSet{
public:
	/**
	 * Walk through, figuring out which items are valid.
	 * @param numPloid The number of ploidies.
	 * @param thePloids The ploidies in play.
	 * @param totalLoad The total loading.
	 */
	PloidyLoadingSet(uintptr_t numPloid, uintptr_t* thePloids, uintptr_t totalLoad);
	/**Clean up.*/
	~PloidyLoadingSet();
	
	/**The ploidies in play.*/
	std::vector<uintptr_t> hotPloids;
	/**The loadings.*/
	std::vector<int> hotLoads;
	
	/**
	 * Recursive method to fill out ploidies.
	 * @param nextPI The next ploidy to fill
	 * @param hotAss The current set of assignments.
	 * @param loadLeft The number of templates left to assign.
	 * @param highAss The highest possible assignment for each ploidy.
	 */
	void fillOutPloids(uintptr_t nextPI, int* hotAss, uintptr_t loadLeft, std::map<uintptr_t,uintptr_t>* highAss);
};

/**Figure out how to collapse haplotypes by ploidy.*/
class HaploPloidyCollapse{
public:
	/**
	 * Set up an empty.
	 */
	HaploPloidyCollapse();
	/**Clean up.*/
	~HaploPloidyCollapse();
	
	/**
	 * Figure out how to collapse redundant genotypes.
	 * @param toCollapse The genotypes to collapse.
	 * @param numPloid The number of ploidies in play.
	 * @param thePloids The ploidies in play.
	 */
	void figure(HaploGenotypeSet* toCollapse, uintptr_t numPloid, uintptr_t* thePloids);
	
	/**The number of things to sum.*/
	std::vector<uintptr_t> collapseCount;
	/**The sum of all prior counts.*/
	std::vector<uintptr_t> collapseOffset;
	/**The indices to sum.*/
	std::vector<uintptr_t> collapseInds;
	/**Which count each starting genotype maps to.*/
	std::vector<uintptr_t> collapseMap;
};

/**
 * Turn a per-genotype loading to allele loading.
 * @param numAl The number of alleles in play.
 * @param alLoad The final loadings.
 * @param numPloid The number of ploidies in play.
 * @param thePloids The ploidies in play.
 * @param ploidLoads The loadings of each ploid individual.
 * @param curGeno The current set of haplotypes.
 */
void genotypeLoadingToAlleleLoading(uintptr_t numAl, uint64_t* alLoad, uintptr_t numPloid, uintptr_t* thePloids, int* ploidLoads, unsigned short* curGeno);

/*
PCR simulation:

x is initial loading index
y is case index

input:
initial loading[num load * num allele]
site index[num load]
seed values[num out = num load * fan]

input per aliquot:
ratio[1]

input per amplify:
max make[1]
base efficiency[num site * num allele]
mutation matrix[num site * num allele * num allele]

output:
final loading[num load * fan * num allele]

For these builders, initial loadings, site index, seed and final loadings are all in variables.
Something else will put them where they belong.

*/

class PCRSimulationStepSpec;

/**Help build a shader.*/
class PCRGPUShaderBuilder{
public:
	/**Set up an empty.*/
	PCRGPUShaderBuilder();
	/**Clean up.*/
	~PCRGPUShaderBuilder();
	
	/**The number of simulations to run for each initial loading.*/
	uintptr_t caseFan;
	/**The number of alleles in play.*/
	uintptr_t numAllele;
	/**The number of sites in play.*/
	uintptr_t numSite;
	/**The maximum simultaneous loads to run.*/
	uintptr_t maxSimulLoad;
	/**The steps in the simulation.*/
	std::vector<PCRSimulationStepSpec*> subSteps;
	
	/**
	 * Get the number of buffers needed for this simulation.
	 * @return The number of buffers this uses.
	 */
	uintptr_t numBuffers();
	
	/**Where the shader is being built.*/
	VulkanComputeShaderBuilder* shad;
	
	/**
	 * Add constant values to the shader.
	 */
	void addConstants();
	/**
	 * Add the input and data buffers to the shader.
	 */
	void addInterface();
	/**
	 * Add any useful variables to main.
	 */
	void addVariables();
	/**
	 * Add code to main.
	 * @param sideBuffOff The offset to buffers for this simulation.
	 */
	void addCode(uintptr_t sideBuffOff);
	
	/**
	 * Add a buffer to a package description.
	 * @param toAdd The package description to add to.
	 * @param buffInd The index of the input buffer: init load, site index, seed values, then step specific buffers.
	 * @return The sphere the buffer is in (in, out, data, side) and the index.
	 */
	std::pair<uintptr_t,uintptr_t> addBuffer(VulkanComputePackageDescription* toAdd, uintptr_t buffIndex);
	
	/**The variables for the allele counts: will be updated after each step.*/
	std::vector<VulkanCSVarL> countVariables;
	/**The current seed in play.*/
	VulkanCSVarL seedVar;
	/**The current site index.*/
	VulkanCSValL siteIV;
	/**The value to multiply by for the random numbers.*/
	VulkanCSValL randMV;
	/**The value to add for the random numbers.*/
	VulkanCSValL randAV;
	/**The number of simulations per initial loading.*/
	VulkanCSValL numSPL;
	/**The number of alleles.*/
	VulkanCSValL numAL;
	/**The loading this is working over.*/
	VulkanCSValL loadInd;
	/**The case this is working over.*/
	VulkanCSValL caseInd;
	
	/**The offset to each substep.*/
	std::vector<uintptr_t> subStepBuffOff;
};

/**A single step in a pcr simulation.*/
class PCRSimulationStepSpec{
public:
	/**Set up empty.*/
	PCRSimulationStepSpec();
	/**Allow subclasses.*/
	virtual ~PCRSimulationStepSpec();
	/**
	 * Get the number of extra per-site buffers this needs.
	 * @return The number of site specific buffers needed.
	 */
	virtual uintptr_t numSiteBuffers() = 0;
	/**
	 * The number of bytes needed for a site buffer per site.
	 * @param toBuild The thing being built.
	 * @param siteInd The index in question.
	 * @return The number of bytes needed per site.
	 */
	virtual uintptr_t siteBufferSize(PCRGPUShaderBuilder* toBuild, uintptr_t siteInd) = 0;
	/**
	 * Add constant values to the shader.
	 * @param toBuild The thing being built.
	 */
	virtual void addConstants(PCRGPUShaderBuilder* toBuild) = 0;
	/**
	 * Add any needed arrays.
	 * @param toBuild The thing being built.
	 */
	virtual void addInterface(PCRGPUShaderBuilder* toBuild) = 0;
	/**
	 * Add any useful variables to main.
	 * @param toBuild The thing being built.
	 */
	virtual void addVariables(PCRGPUShaderBuilder* toBuild) = 0;
	/**
	 * Add code to main.
	 * @param toBuild The thing being built.
	 * @param sideBuffOff The offset to buffers for this site.
	 */
	virtual void addCode(PCRGPUShaderBuilder* toBuild, uintptr_t sideBuffOff) = 0;
};

/**An amplification in a pcr simulation.*/
class PCRAmplificationStepSpec : public PCRSimulationStepSpec{
public:
	/**Set up empty.*/
	PCRAmplificationStepSpec();
	/**Allow subclasses.*/
	~PCRAmplificationStepSpec();
	uintptr_t numSiteBuffers();
	uintptr_t siteBufferSize(PCRGPUShaderBuilder* toBuild, uintptr_t siteInd);
	void addConstants(PCRGPUShaderBuilder* toBuild);
	void addInterface(PCRGPUShaderBuilder* toBuild);
	void addVariables(PCRGPUShaderBuilder* toBuild);
	void addCode(PCRGPUShaderBuilder* toBuild, uintptr_t sideBuffOff);
	
	/**The maximum number of new copies to make.*/
	int64_t maxNumMake;
	/**The number of cycles*/
	uint64_t numCycles;
	/**Keep track of the cycle index.*/
	VulkanCSVarL cycleIndV;
	/**Keep track of the cycle index.*/
	VulkanCSVarL cycleJV;
	/**The constant version of the number of cycles.*/
	VulkanCSValL numCycCV;
	/**The initial reactant count.*/
	VulkanCSValL initReactV;
	/**The number being copied in each round.*/
	std::vector<VulkanCSVarL> copyCounts;
	/**The number made in each round.*/
	std::vector<VulkanCSVarL> madeCounts;
	/**The amount of reactants remaining.*/
	VulkanCSVarL reactRemainV;
};

/**
 * Fill out a flat efficiency model.
 * @param numAl The number of alleles.
 * @param baseEff The efficiency.
 * @param toFill The place to put the threshold markers.
 */
void pcrEfficiencyModelFlat(uintptr_t numAl, double baseEff, uint64_t* toFill);

/**
 * Fill out a flat mutation model.
 * @param numAl The number of alleles.
 * @param mutRate The base mutation rate.
 * @param toFill The place to put the threshold markers (cdf_P(get j|copy i)): is transpose of paper.
 */
void pcrMutationModelFlat(uintptr_t numAl, double mutRate, uint64_t* toFill);

/**
 * Fill out a flat mutation model.
 * @param numAl The number of alleles.
 * @param negRate The rate of -1 stutter (should be more likely than +1).
 * @param posRate The rate of +1 stutter.
 * @param toFill The place to put the threshold markers (cdf_P(get j|copy i)): is transpose of paper.
 */
void pcrMutationModelStutter(uintptr_t numAl, double negRate, double posRate, uint64_t* toFill);

/**
 * Fill out a flat sequence error model.
 * @param numAl The number of alleles.
 * @param mutRate The base mutation rate.
 * @param toFill The place to put the threshold markers (P(get R_i|sequence A_j)).
 */
void pcrSequencerModelFlat(uintptr_t numAl, double mutRate, double* toFill);

/**
 * Calculate the draw probability for sequencing each read.
 * @param numAl The number of alleles.
 * @param errorMat The sequencing error matrix.
 * @param tempCount The number of templates of each allele in the soup.
 * @param tmpScratch Temporary scratch space.
 * @param drawProb The place to put the draw probability cdfs.
 */
void pcrSequenceCalcDrawProb(uintptr_t numAl, double* errorMat, uint64_t* tempCount, double* tmpScratch, uint64_t* drawProb);

//TODO

};

#endif

