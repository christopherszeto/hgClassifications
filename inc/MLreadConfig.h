#ifndef _MLreadConfig_
#define _MLreadConfig_

/*************************************************************
** Copyright Christopher Szeto, September 2007
** last updated July 25 2009
**
** Utility functions for reading config files
** extended beyond the kent/src .ra functions.
**************************************************************/
#include "common.h"
#include "hdb.h"
#include "portable.h"

/*Grabs the parameters entry out of config and makes a hash of the csv entries with structure [field1]=[val],[field2]=[val] etc.*/
struct hash * parseParametersFromConfig(struct hash * config);

/*Using the settings in config, figure out the number of folds to be used*/
int foldsCountFromConfig(struct hash * config);

/*Counts the folds by ls in data dir*/
int foldsCountFromDataDir(struct hash * config);

/*Opens the file pointed to in config for fold description and counts folds from that*/
int foldsCountFromFoldsDescription(struct hash * config);

/*Using the settings in config, figure out the number of folds to be used*/
int splitsCountFromConfig(struct hash * config);

/*Counts the folds by ls in data dir*/
int splitsCountFromDataDir(struct hash * config);

/*Counts the number of lines in the folds description matrix for number of splits*/
int splitsCountFromFoldsDescription(struct hash * config);

/*Count features in the files that config specifies*/
int featuresCountFromConfig(struct hash * config);

/*Count the number of items in this split and fold*/
int itemsInFoldFromConfig(struct hash * config,int n, int split, int fold);

/*Return a matrix of folds description from where indicated in config*/
matrix * foldsDescriptionFromConfig(struct hash * config);

#endif
