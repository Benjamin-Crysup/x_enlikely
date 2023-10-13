#include "whodun_math_matrix.h"

#include <math.h>
#include <string.h>
#include <iostream>

#include "whodun_string.h"

using namespace whodun;

DenseMatrix::DenseMatrix(uintptr_t rows, uintptr_t cols){
	nr = rows;
	nc = cols;
	needKill = 1;
	matVs = (double*)malloc(nr*nc*sizeof(double));
}
DenseMatrix::DenseMatrix(uintptr_t rows, uintptr_t cols, double* useStore){
	nr = rows;
	nc = cols;
	needKill = 0;
	matVs = useStore;
}
DenseMatrix::~DenseMatrix(){
	if(needKill){ free(matVs); }
}
double DenseMatrix::get(uintptr_t ri, uintptr_t ci){
	return matVs[ri*nc + ci];
}
void DenseMatrix::set(uintptr_t ri, uintptr_t ci, double toV){
	matVs[ri*nc + ci] = toV;
}
void DenseMatrix::recap(uintptr_t newR, uintptr_t newC){
	nr = newR;
	nc = newC;
	if(needKill){
		matVs = (double*)realloc(matVs,nr*nc*sizeof(double));
	}
	else{
		matVs = (double*)malloc(nr*nc*sizeof(double));
		needKill = 1;
	}
}
double* DenseMatrix::operator[](uintptr_t ri){
	return matVs + (ri*nc);
}

DenseMatrixMath::DenseMatrixMath(){}
DenseMatrixMath::~DenseMatrixMath(){}
void DenseMatrixMath::add(DenseMatrix* matA, DenseMatrix* matB, DenseMatrix* matR){
	uintptr_t numDo = (matA->nr * matA->nc);
	double* curA = matA->matVs;
	double* curB = matB->matVs;
	double* curR = matR->matVs;
	while(numDo){
		numDo--;
		curR[numDo] = curA[numDo] + curB[numDo];
	}
}
void DenseMatrixMath::subtract(DenseMatrix* matA, DenseMatrix* matB, DenseMatrix* matR){
	uintptr_t numDo = (matA->nr * matA->nc);
	double* curA = matA->matVs;
	double* curB = matB->matVs;
	double* curR = matR->matVs;
	while(numDo){
		numDo--;
		curR[numDo] = curA[numDo] - curB[numDo];
	}
}
void DenseMatrixMath::multiply(DenseMatrix* matA, DenseMatrix* matB, DenseMatrix* matR){
	double* curR = matR->matVs;
	uintptr_t nr = matA->nr;
	uintptr_t nk = matA->nc;
	uintptr_t nc = matB->nc;
	for(uintptr_t i = 0; i<nr; i++){
		double* curA = matA->matVs + (i*nk);
		for(uintptr_t j = 0; j<nc; j++){
			double* curB = matB->matVs + j;
			double curV = 0.0;
			for(uintptr_t k = 0; k<nk; k++){
				curV += curA[k] * *curB;
				curB += nc;
			}
			curR[j] = curV;
		}
		curR += nc;
	}
}
void DenseMatrixMath::add(DenseMatrix* matA, double matB, DenseMatrix* matR){
	uintptr_t numDo = (matA->nr * matA->nc);
	double* curA = matA->matVs;
	double* curR = matR->matVs;
	while(numDo){
		numDo--;
		curR[numDo] = curA[numDo] + matB;
	}
}
void DenseMatrixMath::multiply(DenseMatrix* matA, double matB, DenseMatrix* matR){
	uintptr_t numDo = (matA->nr * matA->nc);
	double* curA = matA->matVs;
	double* curR = matR->matVs;
	while(numDo){
		numDo--;
		curR[numDo] = curA[numDo] * matB;
	}
}
void DenseMatrixMath::transpose(DenseMatrix* matA, DenseMatrix* matR){
	//could make this more cache coherent
	uintptr_t nr = matA->nr;
	uintptr_t nc = matA->nc;
	for(uintptr_t i = 0; i<nr; i++){
		for(uintptr_t j = 0; j<nc; j++){
			matR->set(j,i, matA->get(i,j));
		}
	}
}
void DenseMatrixMath::ewisemultiply(DenseMatrix* matA, DenseMatrix* matB, DenseMatrix* matR){
	uintptr_t numDo = (matA->nr * matA->nc);
	double* curA = matA->matVs;
	double* curB = matB->matVs;
	double* curR = matR->matVs;
	while(numDo){
		numDo--;
		curR[numDo] = curA[numDo] * curB[numDo];
	}
}
void DenseMatrixMath::ewisedivide(DenseMatrix* matA, DenseMatrix* matB, DenseMatrix* matR){
	uintptr_t numDo = (matA->nr * matA->nc);
	double* curA = matA->matVs;
	double* curB = matB->matVs;
	double* curR = matR->matVs;
	while(numDo){
		numDo--;
		curR[numDo] = curA[numDo] / curB[numDo];
	}
}

GaussianElimination::GaussianElimination(){}
GaussianElimination::~GaussianElimination(){}
void GaussianElimination::solve(DenseMatrix* matA, DenseMatrix* matB){
	uintptr_t nr = matA->nr;
	uintptr_t nc = matA->nc;
	uintptr_t np = matB->nc;
	//foward elimination (with partial pivoting)
	for(uintptr_t i = 0; i<nr; i++){
		double* focusCRow = (*matA)[i];
		double* focusRRow = (*matB)[i];
		//find the pivot, do the pivot
		uintptr_t pi = i;
		double cpwin = fabs(matA->get(i,i));
		for(uintptr_t j = i+1; j<nr; j++){
			double ceval = fabs(matA->get(j,i));
			if(ceval > cpwin){
				cpwin = ceval;
				pi = j;
			}
		}
		if(pi != i){
			memswap((char*)(focusCRow), (char*)((*matA)[pi]), nc*sizeof(double));
			memswap((char*)(focusRRow), (char*)((*matB)[pi]), np*sizeof(double));
		}
		//eliminate
		for(uintptr_t j = i+1; j<nr; j++){
			double* curCRow = (*matA)[j];
			double* curRRow = (*matB)[j];
			double curC = curCRow[i] / focusCRow[i];
			curCRow[i] = 0.0;
			for(uintptr_t k = i+1; k<nc; k++){
				curCRow[k] -= curC*focusCRow[k];
			}
			for(uintptr_t k = 0; k<np; k++){
				curRRow[k] -= curC*focusRRow[k];
			}
		}
		//rescale while at it
		double curSFac = 1.0 / focusCRow[i];
		for(uintptr_t k = i; k<nc; k++){
			focusCRow[k] = curSFac*focusCRow[k];
		}
		for(uintptr_t k = 0; k<np; k++){
			focusRRow[k] = curSFac*focusRRow[k];
		}
	}
	//backward eliminate
	uintptr_t i = nr;
	while(i){
		i--;
		//double* focusCRow = (*matA)[i];
		double* focusRRow = (*matB)[i];
		uintptr_t j = i;
		while(j){
			j--;
			double* curCRow = (*matA)[j];
			double* curRRow = (*matB)[j];
			double curFac = curCRow[i];
			curCRow[i] = 0.0;
			for(uintptr_t k = 0; k<np; k++){
				curRRow[k] -= curFac*focusRRow[k];
			}
		}
	}
}
void GaussianElimination::invert(DenseMatrix* matA, DenseMatrix* matR){
	memset(matR->matVs, 0, (matA->nr * matA->nc)*sizeof(double));
	uintptr_t nr = matA->nr;
	for(uintptr_t i = 0; i<nr; i++){
		matR->set(i,i, 1.0);
	}
	solve(matA, matR);
}

CholeskyDecomposer::CholeskyDecomposer(){}
CholeskyDecomposer::~CholeskyDecomposer(){}
void CholeskyDecomposer::decompose(DenseMatrix* toDecomp, DenseMatrix* toStore){
	uintptr_t nr = toDecomp->nr;
	for(uintptr_t i = 0; i<nr; i++){
		for(uintptr_t j = 0; j<=i; j++){
			double* resI = (*toStore)[i];
			double* resJ = (*toStore)[j];
			double sum = 0.0;
			for(uintptr_t k = 0; k<j; k++){
				sum += resI[k]*resJ[k];
			}
			if(i == j){
				toStore->set(i,j, sqrt(toDecomp->get(i,j) - sum));
			}
			else{
				toStore->set(i,j, (toDecomp->get(i,j) - sum) / toStore->get(j,j));
			}
		}
		for(uintptr_t j = i+1; j<nr; j++){
			toStore->set(i,j, 0.0);
		}
	}
}

