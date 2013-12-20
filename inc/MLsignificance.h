#ifndef _MLsignificance_
#define _MLsignificance_

/*************************************************************
** Copyright Christopher Szeto, September 2007
** last updated June 14 2012
**
** This Describes functions necessary to create a significance score
** for novel predictions
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

#define BACKGROUND_SAMPLES 100
#define BACKGROUND_GENES 40000

/*******************
* Prototypes
********************/
/*List the ids of the best models (one for each task) - dropped if no better than majority*/
struct slInt * listTopModelIds(struct sqlConnection * conn);

/*Create a dir in /data/trash for each model to save results to*/
int createBackgroundDirs(struct slInt * topModelIds);

/*Run the background models, writing results to .ra*/
int printBackgroundModelResults(struct sqlConnection * conn, struct slInt * topModelIds);

/*DEPRECATED from here on.*/

/*Write a a background table that will match the model constructed from the random background table*/
int writeBackgroundData(struct sqlConnection * conn, struct slInt *topModelIds);

/*Write the cluster jobs that will do the applying of topmodels and analysis of results*/
int writeBackgroundClusterJobs(struct sqlConnection * conn, struct slInt *topModelIds);

/*Write a minimal config file to go alone with each model application*/
int writeBackgroundConfigFiles(struct sqlConnection * conn, struct slInt *topModelIds);

/*Create a random mix of values picked from other tables in db*/
int buildBackgroundTables(char * profile, char * db);

/*Apply models to background data pointed to by topModelId list*/
int applyModelsToBackgroundData(struct sqlConnection * conn, struct slInt *topModelIds);

/*Fill a background matrix - these aren't properly labeled.*/
matrix * bioInt_fill_background_matrix(struct sqlConnection * conn, char * tableName);
matrix * NMFspoofDataMatchedToModel(matrix * data, char * modelPath);
matrix * glmnetspoofDataMatchedToModel(matrix * data, char * modelPath);
matrix * SVMspoofDataMatchedToModel(matrix * data, char * modelPath);

#endif
