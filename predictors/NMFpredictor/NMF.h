#ifndef _NMF_
#define _NMF_

/*******************
* Copyright Christopher Szeto, September 2007
********************/

#include <ctype.h>
#include "common.h"
#include "../../inc/MLmatrix.h"

/*******************
* Prototypes
********************/

/* Multiple runs of NMF over 'seeds' number of random H's, averaging the W results and returns */
matrix * multiWseed_nmf(matrix *V_ptr, matrix *W_ptr, matrix *H_ptr,int seeds, int reps,int vary_w, double w_resistance, int vary_h, double h_resistance);

/* Multiple runs of NMF over 'seeds' number of random W's, averaging the H results and returns */
matrix * multiHseed_nmf(matrix *V_ptr, matrix *W_ptr, matrix *H_ptr, int seeds, int reps, int vary_w, double w_resistance, int vary_h, double h_resisitance);

/* Main NMF function - will iteratively update W and H as desired */
void nmf(matrix *V_ptr, matrix *W_ptr, matrix *H_ptr, double residuals[], int reps,int vary_w,double w_resistance, int vary_h, double h_resistance);

/* Find the residual between V and WH */
double calc_residual(matrix *V_ptr, matrix *WH_ptr);

/* Given V, W and H, calculate residual and refine W */
void updateW(matrix *V_ptr, matrix *W_ptr, matrix *H_ptr, matrix *WH_ptr, double w_resistance);

/* Given V,W and H, calculate residual and refine H */
void updateH(matrix *V_ptr, matrix *W_ptr, matrix *H_ptr, matrix *WH_ptr, double h_resistance);

/*Set features that are not contributing to zero, so future learning has to up-weight other features*/
void constrainSparsity(matrix *w_ptr);

/*Prints a bar for how long nmf has to be done*/
void printProgress(double percentDone);

#endif
