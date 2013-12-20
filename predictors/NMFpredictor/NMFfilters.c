#include "common.h"
#include <ctype.h>
#include "NMFfilters.h"
#include "NMFmatrix.h"

/*************************************************************
** Copyright Christopher Szeto, September 2007
** last updated July 25 2009
**
** This describes a couple of filter actions available for matrix objects
**
**************************************************************/

/*****************
** Local prototypes
******************/
void pad_matrixCols(matrix *A_ptr);
void unpad_matrixCols(matrix *A_ptr);
void pad_matrixRows(matrix *A_ptr);
void unpad_matrixRows(matrix *A_ptr);

/***********
** Functions
*************/
void pad_matrix(matrix *A_ptr, int mode){
	if(mode == 1)
		pad_matrixCols(A_ptr);
	if(mode == 2)
		pad_matrixRows(A_ptr);
	if(mode == 3){
		pad_matrixCols(A_ptr);
		pad_matrixRows(A_ptr);
	}
}

void unpad_matrix(matrix *A_ptr, int mode){
	if(mode == 1)
        unpad_matrixCols(A_ptr);
	if(mode == 2)
		unpad_matrixRows(A_ptr);
	if(mode == 3){
		unpad_matrixRows(A_ptr);
		unpad_matrixCols(A_ptr);
	}
}

/*Add empty row(s) and col(s) to the passed matrix, so the first/last positions can be filtered*/
void pad_matrixCols(matrix *A_ptr){
	int x, y;
	A_ptr->cols+=2;

	//reallocate memory for larger block
	for(x = 0; x < A_ptr->rows; x++){
		A_ptr=realloc(A_ptr->graph[x], A_ptr->cols*sizeof(double));
		if(A_ptr->graph[x] == NULL){
			fprintf(stderr, "ERROR: Couldn't assign memory to a matrix. Exiting..\n");
			exit(1);
		}
	}
	
	//move values over one column, and fill edges with null value
	for(x = (A_ptr->rows-1); x >= 0; x--){
		for(y = (A_ptr->cols-1); y >= 0; y--){
			if(y == (A_ptr->cols-1) || y==0)
				A_ptr->graph[x][y] = MIN_RANDOM_VAL;
			else
				A_ptr->graph[x][y] = A_ptr->graph[x][y-1];
		}
	}
}

/*Add empty row(s) and col(s) to the passed matrix, so the first/last positions can be filtered*/
void pad_matrixRows(matrix *A_ptr){
	int x, y;
    	A_ptr->rows+=2;

    	//reallocate memory for larger block
    	A_ptr=realloc(A_ptr->graph,A_ptr->rows*sizeof(double));
    	if(A_ptr->graph == NULL){
        	fprintf(stderr, "ERROR: Couldn't assign memory to a matrix. Exiting..\n");
       		exit(1);
    	}
   
    	//move values down one row, and fill edges with null value
    	for(x = (A_ptr->rows-1); x >= 0; x--){
        	for(y = (A_ptr->cols-1); y >= 0; y--){
            		if(x == (A_ptr->rows-1) || x == 0)
                		A_ptr->graph[x][y] = MIN_RANDOM_VAL;
            		else
                		A_ptr->graph[x][y] = A_ptr->graph[x-1][y];
        	}
    	}
}

/*Take padding off - use the same mode as was used to pad the matrix*/
/*Doesn't actually remove the memory, just sets boundaries again*/
void unpad_matrixRows(matrix *A_ptr){
	int x, y;
    	A_ptr->rows-=2;

   	//move values over one row and column
   	for(x = 0; x < A_ptr->rows; x++){
        	for(y = 0; y < (A_ptr->cols-1); y++){
            		A_ptr->graph[x][y] = A_ptr->graph[x+1][y];
        	}
    	}
}

/*Take padding off - use the same mode as was used to pad the matrix*/
void unpad_matrixCols(matrix *A_ptr){
	int x, y;
    	//decrease sizes
    	A_ptr->cols-=2;

    	//move values over one row and column
    	for(x = 0; x < A_ptr->rows; x++){
        	for(y = 0; y < (A_ptr->cols-1); y++){
            		A_ptr->graph[x][y] = A_ptr->graph[x][y+1];
        	}
    	}
}

void set_1D_filter(matrix *filter, int plusage){
	int i, centerCol;
	double sum = 0;

	if(filter->rows != 1 && ((filter->cols %2) != 1)){
        	fprintf(stderr, "ERROR: Dimensions %d by %d in a 1D filter matrix aren't supported\n", filter->rows, filter->cols);
        	exit(1);
    	}

    	centerCol = ceil(filter->cols/2);
    	for(i = 0; i < filter->cols; i++){
        	if(i != centerCol){
            		filter->graph[0][i] = -1; 
            		sum += filter->graph[0][i];
        	}else{
            		filter->graph[0][i] = 0;
        	}
    	}
    	filter->graph[0][centerCol] = -sum+plusage;
}

void set_contextual_1D_filter(matrix *filter,int *distances, int plusage){
	int i, centerCol;
	double sum = 0;
	
	if(filter->rows != 1 && ((filter->cols %2) != 1)){
		fprintf(stderr, "ERROR: Dimensions %d by %d in a 1D filter matrix aren't supported\n", filter->rows, filter->cols);
		exit(1);
	}
	
	centerCol = ceil(filter->cols/2);
	for(i = 0; i < filter->cols; i++){
		if(distances[i] != -1 && i != centerCol){
			filter->graph[0][i] = -exp(-((double)distances[i]/1000000)); //decays one half every 1Mb
			sum += filter->graph[0][i];
		}else{
			filter->graph[0][i] = 0;
		}
	}
	filter->graph[0][centerCol] = -sum+plusage;
}

/* This filter sets as follows:
**  -1       -1       -1
**  -1   +n+plusage  -1
**  -1       -1       -1
** Where an arbitrary number of -1 fields can be put in, and the center is the negative sum of them plus the plusage value
*/
void set_2D_filter(matrix *filter, int plusage){
	int i,j, centerCol, centerRow;
	double sum = 0;
  
	if((filter->rows %2) != 1  && ((filter->cols %2) != 1)){
        	fprintf(stderr, "ERROR: Dimensions %d by %d in a 2D filter matrix aren't supported\n", filter->rows, filter->cols);
        	exit(1);
    	}
  
	centerCol = ceil(filter->cols/2);
    	centerRow = ceil(filter->rows/2);
    	for(i = 0; i < filter->rows; i++){
        	for(j = 0; j < filter->cols; j++){
            		filter->graph[i][j] = -1;
            		sum += filter->graph[i][j];
        	}
    	}
    	filter->graph[centerRow][centerCol] = -(sum+1)+plusage;
}

/* This filter sets as follows:
**	-A  -A  -A
**	 0  +C   0
**	-B  -B  -B
** where A is cost function for distance to upstream probe, B is cost function to downstream probe, 
** and C is sum+plusage. This will make the probe level on the opposite group have no effect on this
** ones, only distance to neighbours on both.
**
** NB: NOT appropriate for mixed data types
*/
void set_contextual_2D_filter(matrix *filter, int* distances, int plusage){
	int i,j, centerCol, centerRow;
	double sum = 0;
   
	if((filter->rows %2) != 1  && ((filter->cols %2) != 1)){
        	fprintf(stderr, "ERROR: Dimensions %d by %d in a 2D filter matrix aren't supported\n", filter->rows, filter->cols);
        	exit(1);
    	}
   
	centerCol = ceil(filter->cols/2);
	centerRow = ceil(filter->rows/2);
	for(i = 0; i < filter->rows; i++){
		for(j = 0; j < filter->cols; j++){
			if(distances[i] != -1 && i != centerRow){ 
				filter->graph[i][j] = -exp(-((double)distances[i]/1000000)); //decays one half every 1Mb
				sum += filter->graph[i][j];
			}else{
				filter->graph[i][j] = 0;
			}
		}
	}
	filter->graph[centerRow][centerCol] = -sum+plusage;
}

double apply_filter(matrix * A_ptr, matrix *filter, int row, int col){
	double result = 0, row_coefficient = 1, col_coefficient = 1;
	int centerRowIndex = ceil(filter->rows/2);
	int centerColIndex = ceil(filter->cols/2);
	int currRow, currCol, i, j;
    
	//if the filter is larger than the data, scale the plusage square by the active area
    	if(A_ptr->rows < filter->cols)
        	row_coefficient = A_ptr->rows/filter->cols;
	if(A_ptr->cols < filter->rows)
        	col_coefficient = (double)A_ptr->cols/filter->rows;
    
	currRow = row-centerRowIndex;
    	for(i = 0; i < filter->rows; i++){
        	currCol = col-centerColIndex;
        	for(j = 0; j < filter->cols; j++){
            		if(currRow >=0 && currRow < A_ptr->rows && currCol >= 0 && currCol < A_ptr->cols &&
			   A_ptr->graph[currRow][currCol] != NULL_FLAG)
			{
                		if(filter->graph[i][j] > 0) //if it's the plusage square, scale it by active area
					result += row_coefficient*col_coefficient*filter->graph[i][j]*A_ptr->graph[currRow][currCol];
                		else
                    			result += filter->graph[i][j] * A_ptr->graph[currRow][currCol];
            		}
			currCol++;
        	}   
        	currRow++;
	}
    	return result; 
} 
