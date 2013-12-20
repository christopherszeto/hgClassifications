#ifndef _MLfilters_
#define _MLfilters_

/*************************************************************
** Copyright Christopher Szeto, September 2007
** last updated July 2011
**
** This describes the filters possible with a matrix object.
**
**************************************************************/
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include "common.h"
#include "linefile.h"

/*Function to exclude samples based on a csv file linked to in config*/
matrix * filterColumnsByExcludeList(struct hash * config, matrix * A_ptr);

/*** Feature selection functions ****/

/*Function to bounce tot he correct feature selection as per config instructions*/
matrix * featureSelection(struct hash * config, matrix *data);

/***** Discretizer functions *****/
matrix * medianDiscretize(matrix * A_ptr);
matrix * classDiscretize(matrix * A_ptr, double class1, double class2);
matrix * expressionDiscretize(matrix * A_ptr, char * expression1, char * expression2);
matrix * thresholdDiscretize(matrix * A_ptr, double low_threshold, double hi_threshold);
matrix * percentDiscretize(matrix * A_ptr, double percent);
matrix * quartileDiscretize(matrix * A_ptr);
matrix * signDiscretize(matrix * A_ptr);

/** Functions to bounce to the correct discretizer as per config instructions **/
matrix * discretizeData(struct hash * config, matrix * data);
matrix * discretizeMetadata(struct hash *config, matrix * metadata);

#endif
