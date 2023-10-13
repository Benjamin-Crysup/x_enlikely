#ifndef GENLIKE_PROGS_H
#define GENLIKE_PROGS_H 1

#include "whodun_args.h"
#include "whodun_stat_table.h"

namespace whodun {

/**Generate final loadings for initial loadings and genotypes.*/
class GenlikeSampleFinalLoadings : public StandardProgram{
public:
	/**Set up*/
	GenlikeSampleFinalLoadings();
	/**Tear down*/
	~GenlikeSampleFinalLoadings();
	void idiotCheck();
	void baseRun();
	
	/**The model of error to use.*/
	ArgumentOptionEnumValue errorModel;
	/**Uniform error.*/
	ArgumentOptionEnum errorModelFlat;
	/**Stutter error model.*/
	ArgumentOptionEnum errorModelStutter;
	/**The relevant error rate.*/
	ArgumentOptionFloat errorRate;
	
	/**The model of amplification efficiency to use.*/
	ArgumentOptionEnumValue efficModel;
	/**Uniform efficiency.*/
	ArgumentOptionEnum efficModelFlat;
	/**The base efficiency.*/
	ArgumentOptionFloat efficRate;
	
	/**The initial loading (total).*/
	ArgumentOptionInteger initLoad;
	/**The total number of alleles in play.*/
	ArgumentOptionInteger numAllele;
	/**The genotype structure.*/
	ArgumentOptionString numHaplo;
	/**The maximum number of copies that can be made.*/
	ArgumentOptionInteger maxReact;
	/**The number of cycles.*/
	ArgumentOptionInteger cycleCount;
	/**The main seed to use.*/
	ArgumentOptionInteger seedValue;
	/**The number of reps to run per case: rounded up to the nearest multiple of 32.*/
	ArgumentOptionInteger caseFan;
	
	/**The gpu to use.*/
	ArgumentOptionInteger tgtGPU;
	/**The maximum number of loadings to do in one go.*/
	ArgumentOptionInteger maxSimLoad;
	
	/**The file to write the final probabilities to.*/
	ArgumentOptionFileWrite outFileN;
};

/**Generate reads from supplied loadings, compare to base loadings.*/
class GenlikeSampleReadProbs : public StandardProgram{
public:
	/**Set up*/
	GenlikeSampleReadProbs();
	/**Tear down*/
	~GenlikeSampleReadProbs();
	void idiotCheck();
	void baseRun();
	
	/**The model of error to use for sequencing.*/
	ArgumentOptionEnumValue errorModel;
	/**Uniform error.*/
	ArgumentOptionEnum errorModelFlat;
	/**The relevant error rate.*/
	ArgumentOptionFloat errorRate;
	
	/**The number of bits to use for read counts.*/
	ArgumentOptionInteger maxReadBI;
	
	/**The gpu to use.*/
	ArgumentOptionInteger tgtGPU;
	/**The maximum number of reads to start in one go.*/
	ArgumentOptionInteger maxSimLoad;
	/**The main seed to use.*/
	ArgumentOptionInteger seedValue;
	
	/**The file containing loadings to draw reads from.*/
	ArgumentOptionFileWrite drawLoadFileN;
	/**The file containing loadings to compare the reads to.*/
	ArgumentOptionFileWrite compLoadFileN;
	
	/**The file to write the final results to: probability summary (read counts[], right X, load index, geno index, alt X, load index, geno index, right geno X, geno index, alt geno X, geno index, right load X, load index, alt load X, load index).*/
	ArgumentOptionFileWrite finPFileN;
	/**The file to write the loading/genotype map.*/
	ArgumentOptionFileWrite glMapFileN;
};

/**Turn sampled read probabilities to tsvs.*/
class GenlikeReadProbToTsv : public StandardProgram{
public:
	/**Set up*/
	GenlikeReadProbToTsv();
	/**Tear down*/
	~GenlikeReadProbToTsv();
	void baseRun();
	
	/**The file to write the final results to: probability summary (read counts[], right X, load index, geno index, alt X, load index, geno index, right geno X, geno index, alt geno X, geno index, right load X, load index, alt load X, load index).*/
	ArgumentOptionFileRead finPFileN;
	/**The file to write the loading/genotype map.*/
	ArgumentOptionFileRead glMapFileN;
	
	/**The file to write the tsv to.*/
	ArgumentOptionTextTableWrite outFileN;
};

//TODO

};

#endif

