#ifndef WHODUN_MATH_MATRIX_H
#define WHODUN_MATH_MATRIX_H 1

/**
 * @file
 * @brief Matrix operations.
 */

#include <stdint.h>

namespace whodun {

/**A real-number matrix.*/
class DenseMatrix{
public:
	/**
	 * Make a matrix.
	 * @param rows The number of rows.
	 * @param cols The number of columns.
	 */
	DenseMatrix(uintptr_t rows, uintptr_t cols);
	/**
	 * Make a matrix, using the given memory for storage.
	 * @param rows The number of rows.
	 * @param cols The number of columns.
	 * @param useStore The place to use for storage.
	 */
	DenseMatrix(uintptr_t rows, uintptr_t cols, double* useStore);
	/**Clean up.*/
	~DenseMatrix();
	
	/**
	 * Get a value.
	 * @param ri The row index.
	 * @param ci The column index.
	 * @return The requested value.
	 */
	double get(uintptr_t ri, uintptr_t ci);
	
	/**
	 * Set a value.
	 * @param ri The row index.
	 * @param ci The column index.
	 * @param toV The new value.
	 */
	void set(uintptr_t ri, uintptr_t ci, double toV);
	
	/**The number of rows.*/
	uintptr_t nr;
	/**The number of columns.*/
	uintptr_t nc;
	/**The matrix values.*/
	double* matVs;
	/**Whether the storage needs to be freed.*/
	int needKill;
	
	/**
	 * Resize this matrix: trash all old values.
	 * @param newR The new number of rows.
	 * @param newC The new number of columns.
	 */
	void recap(uintptr_t newR, uintptr_t newC);
	
	/**
	 * Get a row from this thing.
	 * @param ri The row in question.
	 * @return The row.
	 */
	double* operator[](uintptr_t ri);
};

/**Do math on a dense matrix.*/
class DenseMatrixMath{
public:
	/**Basic set up.*/
	DenseMatrixMath();
	/**Basic tear down.*/
	~DenseMatrixMath();
	/**
	 * Add two matrices.
	 * @param matA The first matrix.
	 * @param matB The second matrix.
	 * @param matR The storage matrix.
	 */
	void add(DenseMatrix* matA, DenseMatrix* matB, DenseMatrix* matR);
	/**
	 * Subtract two matrices.
	 * @param matA The first matrix.
	 * @param matB The second matrix.
	 * @param matR The storage matrix.
	 */
	void subtract(DenseMatrix* matA, DenseMatrix* matB, DenseMatrix* matR);
	/**
	 * Multiply two matrices.
	 * @param matA The first matrix.
	 * @param matB The second matrix.
	 * @param matR The storage matrix.
	 */
	void multiply(DenseMatrix* matA, DenseMatrix* matB, DenseMatrix* matR);
	/**
	 * Add a scalar to a matrix.
	 * @param matA The first matrix.
	 * @param matB The scalar.
	 * @param matR The storage matrix.
	 */
	void add(DenseMatrix* matA, double matB, DenseMatrix* matR);
	/**
	 * Multiply a scalar and a matrix.
	 * @param matA The first matrix.
	 * @param matB The scalar.
	 * @param matR The storage matrix.
	 */
	void multiply(DenseMatrix* matA, double matB, DenseMatrix* matR);
	/**
	 * Transpose a matrix.
	 * @param matA The matrix.
	 * @param matR The storage matrix.
	 */
	void transpose(DenseMatrix* matA, DenseMatrix* matR);
	/**
	 * Elementwise multiply two matrices.
	 * @param matA The first matrix.
	 * @param matB The second matrix.
	 * @param matR The storage matrix.
	 */
	void ewisemultiply(DenseMatrix* matA, DenseMatrix* matB, DenseMatrix* matR);
	/**
	 * Elementwise divide two matrices.
	 * @param matA The first matrix.
	 * @param matB The second matrix.
	 * @param matR The storage matrix.
	 */
	void ewisedivide(DenseMatrix* matA, DenseMatrix* matB, DenseMatrix* matR);
};

/**Perform gaussian elimination.*/
class GaussianElimination{
public:
	/**Basic set up.*/
	GaussianElimination();
	/**Basic tear down.*/
	~GaussianElimination();
	
	/**
	 * Perform an elimination.
	 * @param matA On call, the coefficient matrix. On return, garbage (in this case, identity).
	 * @param matB On call, the right hand side. On return, the solution.
	 */
	void solve(DenseMatrix* matA, DenseMatrix* matB);
	
	/**
	 * Perform a matrix inversion.
	 * @param matA On call, the coefficient matrix. On return, garbage.
	 * @param matR On call, garbage. On return, the inverse.
	 */
	void invert(DenseMatrix* matA, DenseMatrix* matR);
};

/**Perform a Cholesky decomposition.*/
class CholeskyDecomposer{
public:
	/**Basic set up.*/
	CholeskyDecomposer();
	/**Basic tear down.*/
	~CholeskyDecomposer();
	
	/**
	 * Perform the decomposition.
	 * @param toDecomp The matrix to decompose.
	 * @param toStore The place to put the result.
	 */
	void decompose(DenseMatrix* toDecomp, DenseMatrix* toStore);
};

//TODO SVD!!! (blocks any linear flavored regression)

}

#endif
