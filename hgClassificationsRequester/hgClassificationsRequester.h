/*  */

#ifndef HGCLASSIFICATIONSREQUEST_H
#define HGCLASSIFICATIONSREQUEST_H

#include "cart.h"

#define DEBUG 1
#define VERSION "0.1"

#define FLOAT_NULL -9999

#define DEFAULT_LABEL_HEIGHT 10

#define TITLE_HEIGHT 15
#define GUTTER_WIDTH 75

/*** Prefixes for variables in cart we don't share with other apps. ***/
#define rqPrefix "rq_"

/*** Our various cart variables. ***/
#define rqMode rqPrefix "mode"
#define rqRequestId rqPrefix "requestId"
#define rqEstimatedRunTime rqPrefix "estimatedRunTime"
#define rqDataset rqPrefix "dataset"
#define rqProfile rqPrefix "profile"
#define rqDb rqPrefix "db"
#define rqTask rqPrefix "task"
#define rqInputType rqPrefix "inputType"
#define rqTableName rqPrefix "tableName"
#define rqClinField rqPrefix "clinField"
#define rqDataFilepath rqPrefix "dataFilepath"
#define rqMetadataFilepath rqPrefix "metadataFilepath"
#define rqTrainingDir rqPrefix "trainingDir"
#define rqValidationDir rqPrefix "validationDir"
#define rqModelDir rqPrefix "modelDir"
#define rqCrossValidation rqPrefix "crossValidation"
#define rqFolds rqPrefix "folds"
#define rqFoldMultiplier rqPrefix "foldMultiplier"
#define rqClassifier rqPrefix "classifier"
#define rqOutputType rqPrefix "outputType"
#define rqParameters rqPrefix "parameters"
#define rqDataDiscretizer rqPrefix "dataDiscretizer"
#define rqDataDiscretizerParameters rqPrefix "dataDiscretizerParameters"
#define rqClinDiscretizer rqPrefix "clinDiscretizer"
#define rqClinDiscretizerParameters rqPrefix "clinDiscretizerParameters"
#define rqFeatureSelection rqPrefix "featureSelection"
#define rqFeatureSelectionParameters rqPrefix "featureSelectionParameters"

/* ---- Global variables declared in hgClassifications.c */
extern struct cart *cart;	/* This holds cgi and other variables between clicks. */
char *cartSettingsString(char *prefix, char *functionName);

#endif /* HGCLASSIFICATIONSREQUEST_H */
