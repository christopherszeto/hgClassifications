#ifndef _MLio_
#define _MLio_

#include <ctype.h>
#include "common.h"
#include "linefile.h"
#include "jksql.h"
#include "hdb.h"
#include "hash.h"
#include "ra.h"
#include "dirent.h"
#include "MLmatrix.h"
#include "MLcrossValidation.h"
#include "MLreadConfig.h"
#include "MLextractCoefficients.h"
#include "MLio.h"
#include "MLfilters.h"
#include "portable.h"

#define CLUSTERROOTDIR "/data/home/common/topmodel_tools"

/*** Common functions ***/
/* Check a string is a number */
int isnum(char * str);
/*Create data and metadata from config*/
void splitData(struct hash * config);
/*Apply the data filteres specified in config (excluding, feature selection, and discretization)*/
void applyFilters(struct hash *config, matrix * data, matrix * metadata);
/*Create a validation and test list, then call printing function*/
void writeSplits(struct hash *config, matrix * data, matrix * metadata);
/*Write the data before it's been split up into the training dir, in the format expected for the classifier*/
void printPresplitData(struct hash * config, matrix * data, matrix * metadata);
/*Write this split and fold's files to directories*/
void printSplit(struct hash * config, matrix * data, matrix * metadata, int split, int fold, struct slInt * validationList);
/*Write to jobs file for running on the cluster*/
void writeClusterJobs(struct hash *config);
/*Optional if foldsDescriptionPath is specified: writes a report of which fold each sample goes in to for each split*/
matrix * reportFolds(struct hash * config, matrix * data, int folds, int itemsInSplit, struct slInt * shuffledList);

/*Check if this split  has been done before*/
int dataExists(struct hash *config);
/*Create the data directories as specified by config*/
void createDataDirs(struct hash * config);
/*Create the model directories as specified by config*/
void createModelDirs(struct hash * config);
/*Remove the data dirs associated with file 'config' if possible - intended for single-use configs*/
void removeDataDirs(struct hash * config);

/********** results writing functions **************/
struct hash *readRaFile(char *fileName);

int resultsExist(struct hash *config);

void printTasksRa(struct hash *config, char * file, int id);
void printClassifiersRa(struct hash *config, char * file, int id);
void printSubgroupsRa(struct hash * config, char * file, int id);
void printFeatureSelectionsRa(struct hash * config, char * file, int id);
void printTransformationsRa(struct hash * config, char * file, int id);
void printJobsRa(struct hash * config,char * file, int id);
void printBackgroundResultsRa(struct hash * config, char * file, int id);

/******* Functions for applying models to novel datasets *******/
/*Functions to read the features saved in a model and return a copy of the data matrix that matches those features*/
matrix * SVMgetDataMatchedToModel(matrix * data, char * modelPath);
matrix * NMFgetDataMatchedToModel(matrix * data, char * modelPath);
matrix * glmnetgetDataMatchedToModel(matrix * data, char *userDataPrefix, char *modelPath, char *parameters);

/*Functions to write necessary files to the prefix area then run the given model on them (using system calls)*/
matrix * applySVMlightModel(matrix * data, char * tmpDataPrefix, char * modelPath);
matrix * applyNMFpredictorModel(matrix * data, char * tmpDataPrefix, char * modelPath);
matrix * applyglmnetModel(matrix * data, char * tmpDataPrefix, char * modelPath, char * parameters);

/*Function to count the number of non-all-NULL data rows in data*/
int countFeatureUsage(matrix * data);

/****** NMF specific functions *******/
matrix * NMFrecreateMetadata(struct hash * config);

/***** SVMlight specific functions *******/
void fprint_SVMlite_matrix(FILE *fp, matrix *A_ptr);
void tagLabelsWithMetadata(matrix * data, matrix * metadata);
matrix * SVMrecreateMetadata(struct hash *config);

/****** WEKA (ARFF) specific functions **********/
void fprint_WEKA_matrix(FILE *fp, matrix *A_ptr);
matrix * WEKArecreateMetadata(struct hash *config);

/****** Generic functons ***********/
matrix * getPredictionScores(struct hash * config);

#endif
