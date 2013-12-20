#ifndef _MLmatrix_
#define _MLmatrix_

/*************************************************************
** Copyright Christopher Szeto, September 2007
** last updated July 25 2009
**
** This describes the actions possible with a matrix object.
**
**************************************************************/
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include "common.h"
#include "linefile.h"
#include "jksql.h"
#include "hdb.h"
#include "obscure.h"
#include "sqlite3.h"

#define MAX_LABEL 512
#define MAX_TOKEN 200
#define MIN_RANDOM_VAL 0.0000001
#define NULL_FLAG LONG_MIN

/*******************
* Prototypes
********************/
typedef struct{
    double **graph;
    int rows;
    int cols;
	int labels;
	char **rowLabels;
	char **colLabels;
}matrix;


/*Assigns mem for a matrix. Must be freed by free_matrix*/
matrix * init_matrix(int rows, int cols);

/*Returns a new matrix that is a copy of the argument. New matrix must be freed with free_matrix*/
matrix * copy_matrix(matrix *A_ptr);

/*Copy labels from template to target (dir 1 is rows, dir 2 is cols.*/
/*Use cautiously: sets a flag for labels even if only one direction is copied*/
void copy_matrix_labels(matrix *target, matrix *template, int targetDirection, int templateDirection);

/*Return 1 if the row labels are the same between A_ptr and B_ptr, otherwise 0. Returns 0 on null labels too*/
int matchedRowLabels(matrix * A_ptr,matrix * B_ptr);
int matchedColLabels(matrix * A_ptr, matrix * B_ptr);

/*remove all rows from A_ptr that don't appear in B_ptr*/
matrix * matchOnRowLabels(matrix *  A_ptr, matrix * B_ptr);
matrix * matchOnColLabels(matrix * A_ptr, matrix * B_ptr);

/* Multiplies matrix a against b. If dimenions are incompatible exit occurs. */
void mult_matrices(matrix *A_ptr, matrix *B_ptr, matrix *AB_ptr);

/*Add A and B , put in AB*/
void add_matrices(matrix *A_ptr, matrix *B_ptr, matrix *AB_ptr);
void subtract_matrices(matrix *A_ptr, matrix *B_ptr, matrix *AB_ptr);
void delta_matrices(matrix *A_ptr, matrix *B_ptr, matrix *AB_ptr);

/*Add A and B by matching column labels. Saves over A_ptr*/
void add_matrices_by_colLabel(matrix * A_ptr, matrix * B_ptr);

/*Append B onto A in either direction (rows=1 or cols=2) */
matrix * append_matrices(matrix * A_ptr, matrix * B_ptr, int direction);

/* Randomly fill matrix A with values uniformly distributed between min and max */
matrix * r_fill_matrix(int rows, int cols, double min, double max);

/* Randomly fill matrix A with values. Seed randomness from value "seed"  (reproducibly random) */
matrix * sr_fill_matrix(unsigned int seed, int rows, int cols, double min, double max);

/*Fill the whole matrix with value*/
matrix * unif_fill_matrix(int rows, int cols, double val);

/* Randomly select values from a table to assign to matrix - allows you to define your own distribution*/
matrix * r_fill_matrix_from_table(int rows, int cols, double *table, int n);

/* Scale rows or columns to add to val, for constraint purposes. Direction = 1 is by row, direction=2 by column, direction=3 is whole table*/
void scale_matrix(matrix *A_ptr, int direction, double val);

/*Shifts the matrix up by minimum value so no values are negative or zero. Returns the shift value */
double shift_matrix(matrix *A_ptr);

/* Return a matrix from file fp. labels is a flag for row/col names(MUST BE BOTH). Exits on errors */
matrix * f_fill_matrix(FILE *fp,int labels);

/*Read the labels from data, match the raw_metadata to those labels. If a label isn't found, convert to NULLs.*/
matrix * copyMetadataByColLabels(matrix * data, matrix * raw_metadata);

/* Return a matrix from the table tableName in the bioInt database, including labels*/
matrix * bioInt_fill_matrix(struct sqlConnection * conn, char *tableName);

/* Return a matrix of clin values in the order found in V */
matrix * bioInt_fill_KH(struct sqlConnection * conn, matrix * V, char *clinField);

/* Return a matrix from the table tableName in the five3db database, including labels*/
matrix * five3db_fill_matrix(struct sqlite3 * conn, char * tableName);

/* Return a matrix of clin values in the order found in V */
matrix * five3db_fill_KH(struct sqlite3 * conn, matrix * V, char *clinField);

/*Selects n random samples from data, then permutes rows. Returns fully labeled matrix*/
matrix * permute_fill_matrix(matrix * data, int n);

/* Reassign memory to matrix A for the new dimensions given */
void grow_matrix(matrix *A_ptr, int new_rows, int new_cols);

/* print the contents of matrix A, tab-delimited*/
void fprint_matrix(FILE *fp, matrix *A_ptr);
void fprint_discreteMatrix(FILE *fp, matrix *A_ptr);

/*Return an array of the values in the specified row or col index*/
double * rowToArray(matrix *A, int row);
double * colToArray(matrix *A, int col);

/*Return a slice of the matrix without the nulls. Returns count of non-null values.*/
double * rowToArray_NArm(matrix *  A_ptr, int row, double null_val);
double * colToArray_NArm(matrix *  A_ptr, int col, double null_val);

/* Return a count of the non NA values in a row/col*/
int rowCountNonNA(matrix * A_ptr, int row, double null_val);
int colCountNonNA(matrix * A_ptr, int row, double null_val);

/*Returns a new matrix that is the transposed version of the argument. New matrix must be freed with free_matrix*/
matrix * transpose(matrix *A_ptr);

/*free mem assigned to a matrix*/
void free_matrix(matrix *A_ptr);

/*Count rows and cols of a given file*/
int lineCount(FILE * fp);
int colCount(FILE * fp); 

/*Count rows and cols of a given bioInt table*/
int bioInt_lineCount(struct sqlConnection * conn, char *tableName);
int bioInt_colCount(struct sqlConnection * conn, char *tableName);

/*COunt rows and cols of a given sqlite3 table*/
int five3db_lineCount(struct sqlite3 * conn, char *tableName);
int five3db_colCount(struct sqlite3 * conn, char *tableName);

/*Consolidates a pair of data matrices into SVM format*/
matrix * matricesToSVM(matrix * data, matrix * metadata);

/*Reads the metadata out of an SVMlight format and returns as a matrix*/
matrix * SVMtoMetadataMatrix(char * filename);

/*Reads the metadata out of a WEKA results file and returns as a matrix*/
matrix * WEKAtoMetadataMatrix(char * filename);

#endif
