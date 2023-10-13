#ifndef WHODUN_STAT_DIST_H
#define WHODUN_STAT_DIST_H 1

/**
 * @file
 * @brief Probability distributions.
 */

#include <vector>
#include <stdint.h>
#include <stdexcept>

#include "whodun_stat_util.h"
#include "whodun_math_matrix.h"
#include "whodun_stat_randoms.h"

namespace whodun {

/**A state in a statistical distribution or process.*/
typedef struct{
	/**The relevant categorical values for this state.*/
	uintptr_t* cats;
	/**The relevant real values for this state.*/
	double* conts;
} StatisticalState;

/**Do calculations for a probability distribution.*/
class ProbabilityDistributionCalc{
public:
	/**Simple setup*/
	ProbabilityDistributionCalc();
	/**Allow subclasses*/
	virtual ~ProbabilityDistributionCalc();
	
	/**
	 * Draw a value from this distribution.
	 * @param useRand The source of random numbers.
	 * @param storeLoc The place to put the drawn value.
	 */
	virtual void draw(RandomGenerator* useRand, StatisticalState* storeLoc) = 0;
	
	/**
	 * Get the probability density at a point.
	 * @param atLoc The location of interest.
	 * @return The (ln) probability density.
	 */
	virtual double pdf(StatisticalState* atLoc) = 0;
};

/**A probability distribution.*/
class ProbabilityDistribution{
public:
	/**Simple setup*/
	ProbabilityDistribution();
	/**Allow subclasses*/
	virtual ~ProbabilityDistribution();
	
	/**
	 * Get the number of categorical dimensions this thing has.
	 * @return The number of categorical dimensions.
	 */
	virtual uintptr_t categoricalDim() = 0;
	
	/**
	 * Get the cardinality of a given categorical dimension.
	 * @param forDim The categorical variable of interest.
	 * @return The number of distinct values that variable can take.
	 */
	virtual uintptr_t categoryCardinality(uintptr_t forDim) = 0;
	
	/**
	 * Get the number of real dimensions this thing has.
	 * @return The number of real dimensions.
	 */
	virtual uintptr_t numericalDim() = 0;
	
	/**
	 * Prepare something for a single thread to do calculations with.
	 * @return The thing to actually do calculations with.
	 */
	virtual ProbabilityDistributionCalc* allocateCalculation() = 0;
};

/**A probability distribution with more detailed information.*/
class WellCharacterizedProbabilityDistribution : public ProbabilityDistribution{
public:
	/**Simple setup*/
	WellCharacterizedProbabilityDistribution();
	/**Allow subclasses*/
	~WellCharacterizedProbabilityDistribution();
	
	/**
	 * Get the bounds that enclose some probability (approximately).
	 * @param probEnc The probability to enclose.
	 * @param atCats The categorical values of interest.
	 * @param toFillLow The place to put the low bound.
	 * @param toFillHigh The place to put the high bound.
	 * @return Whether there is any density at the given categorical value.
	 */
	virtual int getBounds(double probEnc, uintptr_t* atCats, double* toFillLow, double* toFillHigh) = 0;
	
	/**
	 * Get the marginal across some categorical variables.
	 * @param numSurvCats The number of surviving categorical variables.
	 * @param survCats The surviving categorical indices.
	 * @return A new distribution.
	 */
	virtual WellCharacterizedProbabilityDistribution* getCategoricalMarginal(uintptr_t numSurvCats, uintptr_t* survCats) = 0;
	
	/**
	 * Get the marginal across some numerical variables.
	 * @param numSurvCats The number of surviving numerical variables.
	 * @param survCats The surviving numerical indices.
	 * @return A new distribution.
	 */
	virtual WellCharacterizedProbabilityDistribution* getNumericalMarginal(uintptr_t numSurvCats, uintptr_t* survCats) = 0;
};

/**Uniform distribution in a region of hyperspace.*/
class UniformProbabilityDistribution : public WellCharacterizedProbabilityDistribution{
public:
	/**
	 * Set up a kernel.
	 * @param lowCoordinate The low coordinate, inclusive
	 * @param highCoordinate The high coordinate, exclusive.
	 */
	UniformProbabilityDistribution(DenseMatrix* lowCoordinate, DenseMatrix* highCoordinate);
	/**Clean up.*/
	~UniformProbabilityDistribution();
	
	uintptr_t categoricalDim();
	uintptr_t categoryCardinality(uintptr_t forDim);
	uintptr_t numericalDim();
	ProbabilityDistributionCalc* allocateCalculation();
	int getBounds(double probEnc, uintptr_t* atCats, double* toFillLow, double* toFillHigh);
	WellCharacterizedProbabilityDistribution* getCategoricalMarginal(uintptr_t numSurvCats, uintptr_t* survCats);
	WellCharacterizedProbabilityDistribution* getNumericalMarginal(uintptr_t numSurvCats, uintptr_t* survCats);
	
	/**Save the hypervolume.*/
	double myLHyper;
	/**The low coordinate.*/
	DenseMatrix lowC;
	/**The high coordinate.*/
	DenseMatrix highC;
};

/**A gaussian.*/
class GaussianProbabilityDistribution : public WellCharacterizedProbabilityDistribution{
public:
	/**
	 * Set up a kernel.
	 * @param useMean The mean to use.
	 * @param useCovar The covariance to use.
	 */
	GaussianProbabilityDistribution(DenseMatrix* useMean, DenseMatrix* useCovar);
	/**Clean up.*/
	~GaussianProbabilityDistribution();
	
	uintptr_t categoricalDim();
	uintptr_t categoryCardinality(uintptr_t forDim);
	uintptr_t numericalDim();
	ProbabilityDistributionCalc* allocateCalculation();
	int getBounds(double probEnc, uintptr_t* atCats, double* toFillLow, double* toFillHigh);
	WellCharacterizedProbabilityDistribution* getCategoricalMarginal(uintptr_t numSurvCats, uintptr_t* survCats);
	WellCharacterizedProbabilityDistribution* getNumericalMarginal(uintptr_t numSurvCats, uintptr_t* survCats);
	
	/**Save the natural log of the prefactor.*/
	double saveLPrefac;
	/**Save the covariance.*/
	DenseMatrix saveCovar;
	/**Save the cholesky factorization.*/
	DenseMatrix saveCholesk;
	/**Save the inverse of the covariance matrix.*/
	DenseMatrix saveInvCovar;
	/**Save the mean.*/
	DenseMatrix saveMean;
	/**Save the way to do statistics.*/
	StatisticalFunctions doStats;
};

/**A weighted sum of other distributions.*/
class SumProbabilityDistribution : public ProbabilityDistribution{
public:
	/**
	 * Set up an empty sum.
	 */
	SumProbabilityDistribution();
	/**Clean up.*/
	~SumProbabilityDistribution();
	
	/**The (ln of the) weights of the distributions.*/
	std::vector<double> subWeights;
	/**The distributions this adds.*/
	std::vector<ProbabilityDistribution*> subDists;
	
	/**
	 * Add a distribution to this sum.
	 * @param toAdd The distribution to add. Will be owned by this distribution.
	 */
	void addDistribution(ProbabilityDistribution* toAdd);
	/**
	 * Call before using the distribution, or after changing weights.
	 */
	void updateWeights();
	
	uintptr_t categoricalDim();
	uintptr_t categoryCardinality(uintptr_t forDim);
	uintptr_t numericalDim();
	ProbabilityDistributionCalc* allocateCalculation();
	
	/**A tree for drawing from the weighted sum.*/
	void* sumDrawTreeV;
	/**Save the way to do statistics.*/
	StatisticalFunctions doStats;
};

/**A weighted sum of well-characterized distributions.*/
class WellCharacterizedSumProbabilityDistribution : public WellCharacterizedProbabilityDistribution{
public:
	/**
	 * Set up an empty sum.
	 */
	WellCharacterizedSumProbabilityDistribution();
	/**Clean up.*/
	~WellCharacterizedSumProbabilityDistribution();
	
	/**The base implementation of the thing.*/
	SumProbabilityDistribution baseDist;
	
	/**
	 * Add a distribution to this sum.
	 * @param toAdd The distribution to add. Will be owned by this distribution.
	 */
	void addDistribution(WellCharacterizedProbabilityDistribution* toAdd);
	/**
	 * Call before using the distribution, or after changing weights.
	 */
	void updateWeights();
	
	uintptr_t categoricalDim();
	uintptr_t categoryCardinality(uintptr_t forDim);
	uintptr_t numericalDim();
	ProbabilityDistributionCalc* allocateCalculation();
	int getBounds(double probEnc, uintptr_t* atCats, double* toFillLow, double* toFillHigh);
	WellCharacterizedProbabilityDistribution* getCategoricalMarginal(uintptr_t numSurvCats, uintptr_t* survCats);
	WellCharacterizedProbabilityDistribution* getNumericalMarginal(uintptr_t numSurvCats, uintptr_t* survCats);
};

//TODO independent category split

};

#endif

