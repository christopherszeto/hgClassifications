#ifndef _NMFfilters_
#define _NMFfilters_

/*************************************************************
** Copyright Christopher Szeto, September 2007
** last updated July 25 2009
**
** This describes a couple of filter actions available for matrix objects
**
**************************************************************/

#include "common.h"
#include <ctype.h>
#include "NMFmatrix.h"

/*******************
* Prototypes
********************/
void pad_matrix(matrix *A_ptr, int mode);
void unpad_matrix(matrix *A_ptr, int mode);
void set_1D_filter(matrix *filter, int plusage);
void set_contextual_1D_filter(matrix *filter,int *distances, int plusage);
void set_2D_filter(matrix *filter, int plusage);
void set_contextual_2D_filter(matrix *filter, int* distances, int plusage);
double apply_filter(matrix *A_ptr, matrix *filter, int row, int col);

#endif
