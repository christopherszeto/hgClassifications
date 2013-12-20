#ifndef _MLcrossValidation_
#define _MLcrossValidation_

#include "common.h"
#include "linefile.h"
#include "MLmatrix.h"

#define BY_ROW 1
#define BY_COL 2

/*Create a list of indices from 0 to n*/
struct slInt * list_indices(int n);

/*Make a random list of chosen indices, where the proportion of vals between 0 and num are chosen*/
struct slInt * random_indices(int max, double proportion);
struct slInt * seeded_random_indices(int max, double proportion, int seed);

/*Create a list of indices which are the missing indices from the given list up to n-1*/
struct slInt * index_complement(int num,struct slInt * list);

/* Splits the original list (oriList) into two subLists. The subList will be the
** one with the proportion of the original list in it, the original list will have that
** subset removed from it.
*/
//DEPRECATED
//void split_list(double proportion, struct index * oriList,struct index * subList1, struct index * subList2);

/*Give the number of items in the given list*/
int count_indices(struct slInt * list);
int max_index(struct slInt * list);

/*Shuffle the order the indices appear in the list*/
struct slInt * orderIndicesByDescription(struct hash * config, matrix * data, int split);
struct slInt * shuffle_indices(struct slInt * list);
struct slInt * seeded_shuffle_indices(struct slInt * list, int seed);

/*print the indices you chose*/
void fprint_indices(FILE *fp, struct slInt *positions);

/*Make a fresh copy of the indices rows matrix m*/
matrix * copy_matrix_subset(matrix *m, struct slInt * rows, struct slInt * cols);

/*Make a copy of just the indices in the list from array*/
int * copy_array_subset(struct slInt *positions, int * array);

/*Make a list of the top features in a row or column of matrix. Direction: 1=rows, 2=cols*/
struct slInt * listTopN(matrix * A_ptr, int n, int direction, int ix);

/*Finds the cosine of the angle between two given vectors
** A.B = |A||B|cos(theta)
** (A.B)/(|A||B|) = cos(theta)
** (A.B)/(|A||B|) ~ cor(A,B) iff A and B are zero-centered
*/
double dotProd(double *a, double *b, int n);

/*Does the same as above, but ignores values in both vectors if one has the value null_val - essentially strips null fields*/
double dotProd_NArm(double *a, double *b, int n, double null_val);

/*Finds the pearson correlation between vectors a and b*/
double pearson(double *a, double *b, int n);

/*Does the same as above, but ignores values in both vectors if one has the value null_val - essentially strips null fields*/
double pearson_NArm(double *a, double *b, int n, double null_val);

/*Find the euclidean distance between the two points defined by the vectors a and b*/
double euclidean_NArm(double *a, double *b, int n, double null_val);

/* Report proportion of samples correct as reported in accuracies matrix*/
double avgTrainingAccuracy(matrix * accuracies);
double avgTestingAccuracy(matrix * accuracies);

/*populate the correlation matrix*/
matrix * populateCorrMatrix(matrix *testV, matrix *W);
matrix * populateCorrMatrix_NArm(matrix *testV, matrix *W, double null_val);

/*Functions to grab accuracy from a prediction run. Bounces to the correct one as set in config*/
matrix * getSampleAccuracy(struct hash * config);

/*Make class predictions from NMF - used in training and classifying*/
matrix * NMFpopulateClassPredictionsMatrix(matrix * data, matrix * model);

/*Populate a matrix of trainig/testing rows by samples and count how many are right*/
matrix * NMFpopulateAccuracyMatrix(matrix * predictions, matrix * metadata, struct slInt * trainingList);
matrix * SVMpopulateAccuracyMatrix(matrix * predictions, matrix * metadata, struct slInt * trainingList);
matrix * WEKApopulateAccuracyMatrix(struct hash * config, int split, int fold);

/*Populate a matrix of prediction values for each sample*/
matrix * NMFpopulatePredictionsMatrix(char * resultsFile);
matrix * SVMpopulatePredictionsMatrix(char * resultsFile);
matrix * WEKApopulatePredictionsMatrix(struct hash * config);
matrix * glmnetpopulatePredictionsMatrix(char * resultsFile);

#endif
