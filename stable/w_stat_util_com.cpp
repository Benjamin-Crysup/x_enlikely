#include "whodun_stat_util.h"

#include <math.h>
#include <float.h>
#include <stdint.h>
#include <algorithm>

#include "whodun_math_constants.h"

namespace whodun {
//TODO
};

using namespace whodun;

StatisticalFunctions::StatisticalFunctions(){}
StatisticalFunctions::~StatisticalFunctions(){}

double StatisticalFunctions::logSumExp(uintptr_t numCalc, double* calcFrom){
	//find the maximum
	double minf = -1.0 / 0.0;
	double maxVal = -1.0 / 0.0;
	if(numCalc == 0){ return maxVal; }
	for(uintptr_t i = 0; i<numCalc; i++){
		maxVal = std::max(maxVal, calcFrom[i]);
	}
	if(maxVal == minf){ return maxVal; }
	//calculate the sum
	double sumExp = 0.0;
	for(uintptr_t i = 0; i<numCalc; i++){
		sumExp += exp(calcFrom[i] - maxVal);
	}
	//and log it up
	return maxVal + log(sumExp);
}

double StatisticalFunctions::logGamma(double forVal){
	#ifdef C99_FALLBACK
		return logGammaT(forVal, DBL_EPSILON);
	#else
		return lgamma(forVal);
	#endif
}
double StatisticalFunctions::logGammaT(double forVal, double useEpsilon){
	#ifdef C99_FALLBACK
		if(forVal <= 0.0){
			return 1.0 / 0.0;
		}
		double fullTot = - WHODUN_EULERMASCHERONI * forVal;
		fullTot -= log(forVal);
		uintptr_t curK = 1;
		while(true){
			double curAdd = forVal/curK - log(1.0 + forVal/curK);
			if(fabs(fullTot) > useEpsilon){
				if(fabs(curAdd/fullTot) < useEpsilon){
					break;
				}
			}
			else if(fabs(curAdd) < useEpsilon){
				break;
			}
			fullTot += curAdd;
			curK++;
		}
		return fullTot;
	#else
		return lgamma(forVal);
	#endif
}

double StatisticalFunctions::errorFun(double forVal){
	#ifdef C99_FALLBACK
		if(forVal < 0){
			return -errorFun(-forVal);
		}
		//Abramowitz and Stegun
		double p = 0.3275911;
		double a1 = 0.254829592;
		double a2 = -0.284496736;
		double a3 = 1.421413741;
		double a4 = -1.453152027;
		double a5 = 1.061405429;
		
		double t = 1.0 / (1.0 + p*forVal);
		double t2 = t*t;
		double t3 = t2*t;
		double t4 = t3*t;
		double t5 = t4*t;
		
		double sumV = a1*t + a2*t2 + a3*t3 + a4*t4 + a5*t5;
		
		return 1.0 - sumV*exp(-forVal*forVal);
	#else
		return erf(forVal);
	#endif
}
double StatisticalFunctions::errorFunT(double forVal, double useEpsilon){
	return errorFun(forVal);
}

double StatisticalFunctions::invErrorFun(double forVal){
	return invErrorFunT(forVal, DBL_EPSILON);
}
double StatisticalFunctions::invErrorFunT(double forVal, double useEpsilon){
	if(forVal < 0.0){
		return -invErrorFunT(-forVal, useEpsilon);
	}
	if(forVal > 1.0){ return 0.0 / 0.0; }
	double nextMul = sqrt(WHODUN_PI) * forVal / 2.0;
	uintptr_t nextDiv = 1;
	double updMul = nextMul * nextMul;
	uintptr_t nextCI = 0;
	double curVal = 0.0;
	while(1){
		if(nextCI >= invErrCoeffs.size()){
			uintptr_t numNeed = invErrCoeffs.size();
			if(numNeed){
				double totSum = 0.0;
				for(uintptr_t m = 0; m<numNeed; m++){
					double cm = invErrCoeffs[m];
					double ck1m = invErrCoeffs[numNeed - (m+1)];
					totSum = totSum + cm*ck1m/((m+1)*(2*m+1));
				}
				invErrCoeffs.push_back(totSum);
			}
			else{
				invErrCoeffs.push_back(1.0);
			}
		}
		double curCoeff = invErrCoeffs[nextCI];
		nextCI++;
		double curUpd = curCoeff * nextMul / nextDiv;
		curVal += curUpd;
		if(curUpd < 0){ curUpd = -curUpd; }
		if(curUpd < useEpsilon){ break; }
		//isinf?
		if((curCoeff - curCoeff) != 0.0){ break; }
		nextMul = nextMul * updMul;
		nextDiv += 2;
	}
	return curVal;
}

double StatisticalFunctions::expMinusOne(double forVal){
	#ifdef C99_FALLBACK
		return expMinusOneT(forVal, DBL_EPSILON);
	#else
		return expm1(forVal);
	#endif
}
double StatisticalFunctions::expMinusOneT(double forVal, double useEpsilon){
	#ifdef C99_FALLBACK
		if((forVal < -0.69315) || (forVal > 0.40547)){
			return exp(forVal) - 1.0;
		}
		double toRet = 0;
		long curDen = 1;
		double toAdd = forVal;
		while(true){
			if(fabs(toRet) > useEpsilon){
				if(fabs(toAdd/toRet) < useEpsilon){
					break;
				}
			}
			else if(fabs(toAdd) < useEpsilon){
				break;
			}
			toRet += toAdd;
			curDen++;
			toAdd = toAdd * (forVal / curDen);
		}
		return toRet;
	#else
		return expm1(forVal);
	#endif
}

double StatisticalFunctions::logOnePlus(double forVal){
	#ifdef C99_FALLBACK
		return logOnePlusT(forVal, DBL_EPSILON);
	#else
		return log1p(forVal);
	#endif
}
double StatisticalFunctions::logOnePlusT(double forVal, double useEpsilon){
	#ifdef C99_FALLBACK
		//need to do this better
		return log(1 + forVal);
		//TODO
	#else
		return log1p(forVal);
	#endif
}

double StatisticalFunctions::logOneMinusExp(double forVal){
	//ln(2)
	if(forVal > -0.693){
		return log(-expMinusOne(forVal));
	}
	else{
		return logOnePlus(-exp(forVal));
	}
}
double StatisticalFunctions::logOneMinusExpT(double forVal, double useEpsilon){
	if(forVal > -0.693){
		return log(-expMinusOneT(forVal, useEpsilon));
	}
	else{
		return logOnePlusT(-exp(forVal), useEpsilon);
	}
}


