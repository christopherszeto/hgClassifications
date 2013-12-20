/*  */

#ifndef HGPREDICTORPRINT_H
#define HGPREDICTORPRINT_H

#include "cart.h"

#define DEBUG 1
#define VERSION "0.1"

#define FLOAT_NULL -9999

#define DEFAULT_LABEL_HEIGHT 10

#define TITLE_HEIGHT 15
#define GUTTER_WIDTH 75

#define MAX_MODEL_RUNTIME 20
#define DEFAULT_PERMUTATIONS 3

/*** Prefixes for variables in cart we don't share with other apps. ***/
#define ppPrefix "pp_"

/*** Our various cart variables. ***/
#define ppMode ppPrefix "mode"
#define ppTaskId ppPrefix "taskId"
#define ppJobId ppPrefix "jobId"
#define ppUserData ppPrefix "userData"
#define ppUserDataURL ppPrefix "userDataURL"
#define ppUserDataTypeId ppPrefix "userDataTypeId"
#define ppUserMinAccGain ppPrefix "userMinAccGain"

/** Commands from Javascript client **/

/* ---- Global variables declared in hgClassifications.c */
extern struct cart *cart;	/* This holds cgi and other variables between clicks. */
char *cartSettingsString(char *prefix, char *functionName);

struct featureStats {
	struct featureStats *next;
	int id;
	int count;
	double weight;
};

#endif /* HGCPREDICTORPRINT_H */
