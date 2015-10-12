#ifndef __matrix__
#define __matrix__

typedef struct {
  int rows;
  int cols;
  double * data;
} matrix;

matrix * newMatrix(int rows, int cols) ;
int deleteMatrix(matrix * mtx);

#define ELEM(mtx, row, col) \
  mtx->data[(col-1) * mtx->rows + (row-1)]

int setElement(matrix * mtx, int row, int col, double val);
int printMatrix(matrix * mtx);
int sum(matrix * mtx1, matrix * mtx2, matrix * sum);
int product(matrix * mtx1, matrix * mtx2, matrix * prod);


#endif