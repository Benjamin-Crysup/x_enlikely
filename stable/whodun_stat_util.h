#ifndef WHODUN_STAT_UTIL_H
#define WHODUN_STAT_UTIL_H 1

/**
 * @file
 * @brief Extra statistical functions.
 */

#include <vector>
#include <stdint.h>

namespace whodun {

/**A collection of statistical functions for use.*/
class StatisticalFunctions{
public:
	/**Prepare to do some calculations.*/
	StatisticalFunctions();
	/**Clean up.*/
	~StatisticalFunctions();
	
	/**
	 * Calculate the ln(sum(exp(forVal))).
	 * @param numCalc The number of values to calculate for.
	 * @param calcFrom The values to calculate over.
	 * @return ln(sum(exp(calcFrom[])))
	 */
	double logSumExp(uintptr_t numCalc, double* calcFrom);
	
	/**
	 * Calculate the log gamma function.
	 * @param forVal The value to calculate for.
	 * @return The ln-gamma.
	 */
	double logGamma(double forVal);
	/**
	 * Calculate the error function.
	 * @param forVal The value to calculate for.
	 * @return The error function.
	 */
	double errorFun(double forVal);
	/**
	 * Calculate the inverse error function.
	 * @param forVal The value to calculate for.
	 * @return The inverse error function.
	 */
	double invErrorFun(double forVal);
	/**
	 * Calculate exp(x)-1.
	 * @param forVal The value to calculate for.
	 * @return The value, with more precision than naive.
	 */
	double expMinusOne(double forVal);
	/**
	 * Calculate ln(1+x).
	 * @param forVal The value to calculate for.
	 * @return The value, with more precision than naive.
	 */
	double logOnePlus(double forVal);
	/**
	 * Calculate log(1 - exp(a)).
	 * @param forVal The value to compute for.
	 * @return The value, with more precision than naive.
	 */
	double logOneMinusExp(double forVal);
	
	/**
	 * Calculate the log gamma function to some level of precision.
	 * @param forVal The value to calculate for.
	 * @param useEpsilon The precision.
	 * @return The calculated value.
	 */
	double logGammaT(double forVal, double useEpsilon);
	/**
	 * Calculate the error function.
	 * @param forVal The value to calculate for.
	 * @param useEpsilon The precision.
	 * @return The error function.
	 */
	double errorFunT(double forVal, double useEpsilon);
	/**
	 * Calculate the inverse error function.
	 * @param forVal The value to calculate for.
	 * @param useEpsilon The precision.
	 * @return The inverse error function.
	 */
	double invErrorFunT(double forVal, double useEpsilon);
	/**
	 * Calculate exp(x)-1 to some level of precision.
	 * @param forVal The value to calculate for.
	 * @param useEpsilon The precision.
	 * @return The value, with more precision than naive.
	 */
	double expMinusOneT(double forVal, double useEpsilon);
	/**
	 * Calculate ln(1+x) to some level of precision.
	 * @param forVal The value to calculate for.
	 * @param useEpsilon The precision.
	 * @return The value, with more precision than naive.
	 */
	double logOnePlusT(double forVal, double useEpsilon);
	/**
	 * Calculate log(1 - exp(a)) to some level of precision.
	 * @param forVal The value to compute for.
	 * @param useEpsilon The precision.
	 * @return The value, with more precision than naive.
	 */
	double logOneMinusExpT(double forVal, double useEpsilon);
	
	/**Save coefficients for the inverse error function.*/
	std::vector<double> invErrCoeffs;
};

}

#endif


