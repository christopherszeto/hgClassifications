#ifndef _MLextractCoefficients_
#define _MLextractCoefficients_

/*************************************************************
** Copyright Christopher Szeto, September 2007
** last updated June 2011
**
** This describes functions to recall coefficients from models.
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
#include "ra.h"
#include "hash.h"
#include "MLmatrix.h"
#include "MLreadConfig.h"
#include "MLcrossValidation.h"

#define NUM_FEATURES 200

/*Extracts full matrix of coefficients from model, if the model type is supported*/
matrix * extractCoefficients(struct hash *config);

/*Extracts top n features from coefficients (abs val)*/
matrix * extractTopCoefficients(struct hash *config, int n);

#endif
