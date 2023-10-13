#include "whodun_stat_dist.h"

#include <math.h>
#include <string.h>

#include "whodun_container.h"
#include "whodun_math_constants.h"


namespace whodun {

/**Actually calculate for a uniform distribution.*/
class UniformProbabilityDistributionCalc : public ProbabilityDistributionCalc{
public:
	/**
	 * Set up a calculation for the given distribution.
	 * @param useDistribution The distribution to calculate for.
	 */
	UniformProbabilityDistributionCalc(UniformProbabilityDistribution* useDistribution);
	/**Clean up.*/
	~UniformProbabilityDistributionCalc();
	
	void draw(RandomGenerator* useRand, StatisticalState* storeLoc);
	double pdf(StatisticalState* atLoc);
	
	/**The distribution to calculate for.*/
	UniformProbabilityDistribution* myBase;
};

/**Actually calculate for a gaussian distribution.*/
class GaussianProbabilityDistributionCalc : public ProbabilityDistributionCalc{
public:
	/**
	 * Set up a calculation for the given distribution.
	 * @param useDistribution The distribution to calculate for.
	 */
	GaussianProbabilityDistributionCalc(GaussianProbabilityDistribution* useDistribution);
	/**Clean up.*/
	~GaussianProbabilityDistributionCalc();
	
	void draw(RandomGenerator* useRand, StatisticalState* storeLoc);
	double pdf(StatisticalState* atLoc);
	
	/**The distribution to calculate for.*/
	GaussianProbabilityDistribution* myBase;
	/**A temporary matrix for calculation.*/
	DenseMatrix myTempA;
	/**A temporary matrix for calculation.*/
	DenseMatrix myTempB;
};

/**A node in a probability sum.*/
typedef struct{
	/**The value to split on.*/
	double splitVal;
	/**The index being cut between. [0,splitVal) -> [low,splitInd), [splitVal,1.0) -> [splitInd,high) */
	uintptr_t splitInd;
	/**The node to go to on a low cut (not guaranteed to exist).*/
	uintptr_t lowNode;
	/**The node to go to on a high cut (not guaranteed to exist).*/
	uintptr_t highNode;
} SumProbabilitySplitTreeNode;

/**Actually calculate for a distribution sum.*/
class SumProbabilityDistributionCalc : public ProbabilityDistributionCalc{
public:
	/**
	 * Set up a calculation for the given distribution.
	 * @param useDistribution The distribution to calculate for.
	 */
	SumProbabilityDistributionCalc(SumProbabilityDistribution* useDistribution);
	/**Clean up.*/
	~SumProbabilityDistributionCalc();
	
	void draw(RandomGenerator* useRand, StatisticalState* storeLoc);
	double pdf(StatisticalState* atLoc);
	
	/**The distribution to calculate for.*/
	SumProbabilityDistribution* myBase;
	/**The calculation methods for the pieces.*/
	std::vector<ProbabilityDistributionCalc*> subCalcs;
	/**Temporary storage for values.*/
	std::vector<double> subTemps;
};

};

using namespace whodun;

ProbabilityDistributionCalc::ProbabilityDistributionCalc(){}
ProbabilityDistributionCalc::~ProbabilityDistributionCalc(){}

ProbabilityDistribution::ProbabilityDistribution(){}
ProbabilityDistribution::~ProbabilityDistribution(){}

WellCharacterizedProbabilityDistribution::WellCharacterizedProbabilityDistribution(){}
WellCharacterizedProbabilityDistribution::~WellCharacterizedProbabilityDistribution(){}



UniformProbabilityDistribution::UniformProbabilityDistribution(DenseMatrix* lowCoordinate, DenseMatrix* highCoordinate) : lowC(lowCoordinate->nr, 1), highC(lowCoordinate->nr, 1){
	memcpy(lowC.matVs, lowCoordinate->matVs, lowC.nr*sizeof(double));
	memcpy(highC.matVs, highCoordinate->matVs, lowC.nr*sizeof(double));
	myLHyper = 0.0;
	for(uintptr_t i = 0; i<lowC.nr; i++){
		double curDimLen = highC.matVs[i] - lowC.matVs[i];
		myLHyper += log(curDimLen);
	}
}
UniformProbabilityDistribution::~UniformProbabilityDistribution(){}
uintptr_t UniformProbabilityDistribution::categoricalDim(){
	return 0;
}
uintptr_t UniformProbabilityDistribution::categoryCardinality(uintptr_t forDim){
	throw std::runtime_error("Da fuq?");
}
uintptr_t UniformProbabilityDistribution::numericalDim(){
	return lowC.nr;
}
ProbabilityDistributionCalc* UniformProbabilityDistribution::allocateCalculation(){
	return new UniformProbabilityDistributionCalc(this);
}
int UniformProbabilityDistribution::getBounds(double probEnc, uintptr_t* atCats, double* toFillLow, double* toFillHigh){
	memcpy(toFillLow, lowC.matVs, lowC.nr * sizeof(double));
	memcpy(toFillHigh, highC.matVs, lowC.nr * sizeof(double));
	return 1;
}
WellCharacterizedProbabilityDistribution* UniformProbabilityDistribution::getCategoricalMarginal(uintptr_t numSurvCats, uintptr_t* survCats){
	return new UniformProbabilityDistribution(&lowC, &highC);
}
WellCharacterizedProbabilityDistribution* UniformProbabilityDistribution::getNumericalMarginal(uintptr_t numSurvCats, uintptr_t* survCats){
	DenseMatrix margLow(numSurvCats, 1);
	DenseMatrix margHigh(numSurvCats, 1);
	for(uintptr_t i = 0; i<numSurvCats; i++){
		uintptr_t ri = survCats[i];
		margLow.matVs[i] = lowC.matVs[ri];
		margHigh.matVs[i] = highC.matVs[ri];
	}
	return new UniformProbabilityDistribution(&margLow, &margHigh);
}

UniformProbabilityDistributionCalc::UniformProbabilityDistributionCalc(UniformProbabilityDistribution* useDistribution){
	myBase = useDistribution;
}
UniformProbabilityDistributionCalc::~UniformProbabilityDistributionCalc(){}
void UniformProbabilityDistributionCalc::draw(RandomGenerator* useRand, StatisticalState* storeLoc){
	DenseMatrix* lowC = &(myBase->lowC);
	DenseMatrix* highC = &(myBase->highC);
	uintptr_t probDim = lowC->nr;
	useRand->draw(probDim, storeLoc->conts);
	for(uintptr_t i = 0; i<probDim; i++){
		double curR = storeLoc->conts[i];
		storeLoc->conts[i] = (highC->matVs[i] * curR) + (lowC->matVs[i] * (1-curR));
	}
}
double UniformProbabilityDistributionCalc::pdf(StatisticalState* atLoc){
	DenseMatrix* lowC = &(myBase->lowC);
	DenseMatrix* highC = &(myBase->highC);
	uintptr_t probDim = lowC->nr;
	for(uintptr_t i = 0; i<probDim; i++){
		double curNV = atLoc->conts[i];
		if((curNV < lowC->matVs[i]) || (curNV >= highC->matVs[i])){
			return -1.0 / 0.0;
		}
	}
	return - (myBase->myLHyper);
}





GaussianProbabilityDistribution::GaussianProbabilityDistribution(DenseMatrix* useMean, DenseMatrix* useCovar) :  saveCovar(useCovar->nr, useCovar->nc), saveCholesk(useCovar->nr, useCovar->nc), saveInvCovar(useCovar->nr, useCovar->nc), saveMean(useMean->nr, 1){
	uintptr_t probDim = useCovar->nr;
	//get the determinant
	CholeskyDecomposer doChol;
	doChol.decompose(useCovar, &saveCholesk);
	saveLPrefac = 0.0;
	for(uintptr_t i = 0; i<probDim; i++){
		saveLPrefac += log(saveCholesk[i][i]);
	}
	saveLPrefac = saveLPrefac + 0.5 * probDim * log(2*WHODUN_PI);
	saveLPrefac = - saveLPrefac;
	//get the inverse
	GaussianElimination doInv;
	memcpy(saveCovar.matVs, useCovar->matVs, probDim*probDim*sizeof(double));
	doInv.invert(&saveCovar, &saveInvCovar);
	//save the covariance
	memcpy(saveCovar.matVs, useCovar->matVs, probDim*probDim*sizeof(double));
	//save the mean
	memcpy(saveMean.matVs, useMean->matVs, probDim*sizeof(double));
}
GaussianProbabilityDistribution::~GaussianProbabilityDistribution(){}
uintptr_t GaussianProbabilityDistribution::categoricalDim(){
	return 0;
}
uintptr_t GaussianProbabilityDistribution::categoryCardinality(uintptr_t forDim){
	throw std::runtime_error("Da fuq?");
}
uintptr_t GaussianProbabilityDistribution::numericalDim(){
	return saveCovar.nr;
}
ProbabilityDistributionCalc* GaussianProbabilityDistribution::allocateCalculation(){
	return new GaussianProbabilityDistributionCalc(this);
}
#define GAUSS_BOUND_TOL 1.0e-3
int GaussianProbabilityDistribution::getBounds(double probEnc, uintptr_t* atCats, double* toFillLow, double* toFillHigh){
	//figure out how far out to look
	double zCrit = doStats.invErrorFunT(2*probEnc - 1.0, 1e-4)*sqrt(2.0);
	//figure the dimension with the highest variance
	uintptr_t pdim = saveCovar.nr;
	uintptr_t higI = 0;
	double higV = sqrt(saveCovar[0][0]);
	for(uintptr_t i = 1; i<pdim; i++){
		double curV = sqrt(saveCovar[i][i]);
		if(curV > higV){
			higV = curV;
			higI = i;
		}
	}
	//get the largest eigenvalue
	DenseMatrixMath doMath;
	DenseMatrix evec1(pdim, 1);
	DenseMatrix evec2(pdim, 1);
	double lastEV = 0.0;
	memset(evec1.matVs, 0, pdim*sizeof(double));
	evec1.matVs[higI] = 1.0;
	while(1){
		doMath.multiply(&saveCovar, &evec1, &evec2);
		double endNorm = 0.0;
		for(uintptr_t i = 0; i<pdim; i++){ double curV = evec2.matVs[i]; endNorm += (curV*curV); }
		endNorm = sqrt(endNorm);
		double normDiff = (endNorm - lastEV);
			if(normDiff < 0){ normDiff = -normDiff; }
			normDiff = normDiff / std::max(lastEV, endNorm);
		lastEV = endNorm;
		if(normDiff < GAUSS_BOUND_TOL){ break; }
		doMath.multiply(&evec2, 1.0 / lastEV, &evec1);
	}
	double distCrit = 2.0 * zCrit * sqrt(lastEV);
	//fill the bounds using that 
	for(uintptr_t i = 0; i<pdim; i++){
		toFillLow[i] = saveMean[i][0] - distCrit;
		toFillHigh[i] = saveMean[i][0] + distCrit;
	}
	return 1;
}
WellCharacterizedProbabilityDistribution* GaussianProbabilityDistribution::getCategoricalMarginal(uintptr_t numSurvCats, uintptr_t* survCats){
	return new GaussianProbabilityDistribution(&saveMean, &saveCovar);
}
WellCharacterizedProbabilityDistribution* GaussianProbabilityDistribution::getNumericalMarginal(uintptr_t numSurvCats, uintptr_t* survCats){
	DenseMatrix margMean(numSurvCats, 1);
	DenseMatrix margCov(numSurvCats, numSurvCats);
	for(uintptr_t i = 0; i<numSurvCats; i++){
		uintptr_t ri = survCats[i];
		margMean[i][0] = saveMean[ri][0];
		for(uintptr_t j = 0; j<numSurvCats; j++){
			uintptr_t rj = survCats[j];
			margCov[i][j] = saveCovar[ri][rj];
		}
	}
	return new GaussianProbabilityDistribution(&margMean, &margCov);
}

GaussianProbabilityDistributionCalc::GaussianProbabilityDistributionCalc(GaussianProbabilityDistribution* useDistribution) : myTempA(useDistribution->saveCovar.nr, 1), myTempB(useDistribution->saveCovar.nr, 1){
	myBase = useDistribution;
}
GaussianProbabilityDistributionCalc::~GaussianProbabilityDistributionCalc(){}
void GaussianProbabilityDistributionCalc::draw(RandomGenerator* useRand, StatisticalState* storeLoc){
	DenseMatrix endRes(myTempA.nr, 1, storeLoc->conts);
	//generate standard normal
		//the minimum double
		union{
			int64_t asI;
			double asD;
		} minNZDblU;
		minNZDblU.asI = 0x3FF0000000000001LL;
		double minNZDbl = minNZDblU.asD - 1.0;
		uintptr_t i = 0;
		while(i<myTempA.nr){
			//generate uniforms
			double workUnis[2];
			useRand->draw(2, workUnis);
			//nudge zeros to mindbl
			if(workUnis[0] == 0){ workUnis[0] = minNZDbl; }
			//box muller
			double prefac = sqrt(-2*log(workUnis[0]));
			myTempA.matVs[i] = prefac * cos(2.0*WHODUN_PI*workUnis[1]);
			i++;
			if(i < myTempA.nr){
				myTempA.matVs[i] = prefac * sin(2.0*WHODUN_PI*workUnis[1]);
			}
			i++;
		}
	//transform
		DenseMatrixMath doMath;
		doMath.multiply(&(myBase->saveCholesk), &myTempA, &myTempB);
		doMath.add(&(myBase->saveMean), &myTempB, &endRes);
}
double GaussianProbabilityDistributionCalc::pdf(StatisticalState* atLoc){
	DenseMatrix endRes(myTempA.nr, 1, atLoc->conts);
	DenseMatrixMath doMath;
	doMath.subtract(&endRes, &(myBase->saveMean), &myTempA);
	doMath.multiply(&(myBase->saveInvCovar), &myTempA, &myTempB);
	DenseMatrix myTempATrans(1, myTempA.nr, myTempA.matVs);
	double endDotP;
	DenseMatrix endDotPM(1,1,&endDotP);
	doMath.multiply(&myTempATrans, &myTempB, &endDotPM);
	return -0.5*endDotP + myBase->saveLPrefac;
}





SumProbabilityDistribution::SumProbabilityDistribution(){
	sumDrawTreeV = 0;
}
SumProbabilityDistribution::~SumProbabilityDistribution(){
	if(sumDrawTreeV){
		delete((std::vector<SumProbabilitySplitTreeNode>*)sumDrawTreeV);
	}
	for(uintptr_t i = 0; i<subDists.size(); i++){
		delete(subDists[i]);
	}
}
void SumProbabilityDistribution::addDistribution(ProbabilityDistribution* toAdd){
	subWeights.push_back(subDists.size() ? -1.0/0.0 : 0.0);
	subDists.push_back(toAdd);
}
void SumProbabilityDistribution::updateWeights(){
	std::vector<SumProbabilitySplitTreeNode>* sumDrawTree = (std::vector<SumProbabilitySplitTreeNode>*)sumDrawTreeV;
	if(sumDrawTree){ delete(sumDrawTree); }
	sumDrawTree = new std::vector<SumProbabilitySplitTreeNode>();
	sumDrawTreeV = sumDrawTree;
	if(subDists.size() == 1){ return; }
	std::vector< triple<uintptr_t,uintptr_t,uintptr_t> > openRanges;
		openRanges.push_back(triple<uintptr_t,uintptr_t,uintptr_t>(0,0,subDists.size()));
	while(openRanges.size()){
		//get the range
			triple<uintptr_t,uintptr_t,uintptr_t> curRange = openRanges[openRanges.size()-1];
			openRanges.pop_back();
			uintptr_t curRangeNI = sumDrawTree->size();
			sumDrawTree->resize(curRangeNI+1);
		//fill in the previous node
			SumProbabilitySplitTreeNode* prevNode = &((*sumDrawTree)[curRange.first]);
			if(prevNode->lowNode){
				prevNode->highNode = curRangeNI;
			}
			else{
				prevNode->lowNode = curRangeNI;
			}
		//figure out the split
			SumProbabilitySplitTreeNode* curNode = &((*sumDrawTree)[curRangeNI]);
			uintptr_t splitInd = curRange.second + ((curRange.third - curRange.second) / 2);
			uintptr_t numLeftL = splitInd - curRange.second;
			uintptr_t numLeftH = curRange.third - splitInd;
			double lsumPros[2];
				lsumPros[0] = doStats.logSumExp(numLeftL, &(subWeights[curRange.second]));
				lsumPros[1] = doStats.logSumExp(numLeftH, &(subWeights[splitInd]));
			double totLSP = doStats.logSumExp(2, lsumPros);
			curNode->splitVal = exp(lsumPros[0] - totLSP);
			curNode->splitInd = splitInd;
		//push some more ranges
			if(numLeftL == 1){
				curNode->lowNode = -1;
			}
			else{
				curNode->lowNode = 0;
				openRanges.push_back(triple<uintptr_t,uintptr_t,uintptr_t>(curRangeNI,curRange.second,splitInd));
			}
			if(numLeftH == 1){
				curNode->highNode = -1;
			}
			else{
				curNode->highNode = 0;
				openRanges.push_back(triple<uintptr_t,uintptr_t,uintptr_t>(curRangeNI,splitInd,curRange.third));
			}
	}
}
uintptr_t SumProbabilityDistribution::categoricalDim(){
	return subDists[0]->categoricalDim();
}
uintptr_t SumProbabilityDistribution::categoryCardinality(uintptr_t forDim){
	return subDists[0]->categoryCardinality(forDim);
}
uintptr_t SumProbabilityDistribution::numericalDim(){
	return subDists[0]->numericalDim();
}
ProbabilityDistributionCalc* SumProbabilityDistribution::allocateCalculation(){
	return new SumProbabilityDistributionCalc(this);
}

SumProbabilityDistributionCalc::SumProbabilityDistributionCalc(SumProbabilityDistribution* useDistribution){
	myBase = useDistribution;
	subTemps.resize(myBase->subDists.size());
	for(uintptr_t i = 0; i<subTemps.size(); i++){
		subCalcs.push_back(myBase->subDists[i]->allocateCalculation());
	}
}
SumProbabilityDistributionCalc::~SumProbabilityDistributionCalc(){
	for(uintptr_t i = 0; i<subTemps.size(); i++){
		delete(subCalcs[i]);
	}
}
void SumProbabilityDistributionCalc::draw(RandomGenerator* useRand, StatisticalState* storeLoc){
	uintptr_t curLI = 0;
	uintptr_t curHI = subTemps.size();
	uintptr_t curNI = 0;
	std::vector<SumProbabilitySplitTreeNode>* sumDrawTree = (std::vector<SumProbabilitySplitTreeNode>*)(myBase->sumDrawTreeV);
	while((curHI - curLI) > 1){
		SumProbabilitySplitTreeNode* curNode = &((*sumDrawTree)[curNI]);
		double curPV; useRand->draw(1, &curPV);
		if(curPV < curNode->splitVal){
			curHI = curNode->splitInd;
			curNI = curNode->lowNode;
		}
		else{
			curLI = curNode->splitInd;
			curNI = curNode->highNode;
		}
	}
	subCalcs[curLI]->draw(useRand, storeLoc);
}
double SumProbabilityDistributionCalc::pdf(StatisticalState* atLoc){
	for(uintptr_t i = 0; i<subTemps.size(); i++){
		subTemps[i] = myBase->subWeights[i] + subCalcs[i]->pdf(atLoc);
	}
	return myBase->doStats.logSumExp(subTemps.size(), &(subTemps[0]));
}

WellCharacterizedSumProbabilityDistribution::WellCharacterizedSumProbabilityDistribution(){}
WellCharacterizedSumProbabilityDistribution::~WellCharacterizedSumProbabilityDistribution(){}
void WellCharacterizedSumProbabilityDistribution::addDistribution(WellCharacterizedProbabilityDistribution* toAdd){
	baseDist.addDistribution(toAdd);
}
void WellCharacterizedSumProbabilityDistribution::updateWeights(){
	baseDist.updateWeights();
}
uintptr_t WellCharacterizedSumProbabilityDistribution::categoricalDim(){
	return baseDist.categoricalDim();
}
uintptr_t WellCharacterizedSumProbabilityDistribution::categoryCardinality(uintptr_t forDim){
	return baseDist.categoryCardinality(forDim);
}
uintptr_t WellCharacterizedSumProbabilityDistribution::numericalDim(){
	return baseDist.numericalDim();
}
ProbabilityDistributionCalc* WellCharacterizedSumProbabilityDistribution::allocateCalculation(){
	return baseDist.allocateCalculation();
}
int WellCharacterizedSumProbabilityDistribution::getBounds(double probEnc, uintptr_t* atCats, double* toFillLow, double* toFillHigh){
	uintptr_t numDim = numericalDim();
	std::vector<double> tmpStore(2*numDim);
	double* tmpLow = &(tmpStore[0]);
	double* tmpHigh = &(tmpStore[numDim]);
	int anyDense = 0;
	for(uintptr_t i = 0; i<baseDist.subDists.size(); i++){
		double curW = baseDist.subWeights[i];
		if(curW == (-1.0/0.0)){ continue; }
		WellCharacterizedProbabilityDistribution* curSProb = (WellCharacterizedProbabilityDistribution*)(baseDist.subDists[i]);
		if(anyDense){
			int curDense = curSProb->getBounds(probEnc, atCats, tmpLow, tmpHigh);
			if(curDense){
				for(uintptr_t j = 0; j<numDim; j++){
					toFillLow[j] = std::min(toFillLow[j], tmpLow[j]);
					toFillHigh[j] = std::max(toFillHigh[j], tmpHigh[j]);
				}
			}
		}
		else{
			anyDense = curSProb->getBounds(probEnc, atCats, toFillLow, toFillHigh);
		}
	}
	return anyDense;
}
WellCharacterizedProbabilityDistribution* WellCharacterizedSumProbabilityDistribution::getCategoricalMarginal(uintptr_t numSurvCats, uintptr_t* survCats){
	WellCharacterizedSumProbabilityDistribution* subDist = new WellCharacterizedSumProbabilityDistribution();
	for(uintptr_t i = 0; i<baseDist.subDists.size(); i++){
		WellCharacterizedProbabilityDistribution* curSProb = (WellCharacterizedProbabilityDistribution*)(baseDist.subDists[i]);
		subDist->addDistribution(curSProb->getCategoricalMarginal(numSurvCats, survCats));
	}
	subDist->baseDist.subWeights = baseDist.subWeights;
	return subDist;
}
WellCharacterizedProbabilityDistribution* WellCharacterizedSumProbabilityDistribution::getNumericalMarginal(uintptr_t numSurvCats, uintptr_t* survCats){
	WellCharacterizedSumProbabilityDistribution* subDist = new WellCharacterizedSumProbabilityDistribution();
	for(uintptr_t i = 0; i<baseDist.subDists.size(); i++){
		WellCharacterizedProbabilityDistribution* curSProb = (WellCharacterizedProbabilityDistribution*)(baseDist.subDists[i]);
		subDist->addDistribution(curSProb->getNumericalMarginal(numSurvCats, survCats));
	}
	subDist->baseDist.subWeights = baseDist.subWeights;
	return subDist;
}


