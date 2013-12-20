/*  */

#ifndef HGCLASSIFICATIONS_H
#define HGCLASSIFICATIONS_H

#include "cart.h"

#define DEBUG 1
#define VERSION "0.1"

#define FLOAT_NULL -9999

#define DEFAULT_LABEL_HEIGHT 10

#define TITLE_HEIGHT 15
#define GUTTER_WIDTH 75

/*** Prefixes for variables in cart we don't share with other apps. ***/
#define clPrefix "cl_"

/*** Our various cart variables. ***/
#define clMode clPrefix "mode"
#define clTaskId clPrefix "taskId"
#define clJobId clPrefix "jobId"
#define clUserData clPrefix "userData"
#define clUserDataURL clPrefix "userDataURL"

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

#endif /* HGCLASSIFICATIONS_H */
