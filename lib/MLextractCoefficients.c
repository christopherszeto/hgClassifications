#include <ctype.h>
#include "common.h"
#include "options.h"
#include "linefile.h"
#include "ra.h"
#include "hash.h"
#include "../inc/MLmatrix.h"
#include "../inc/MLreadConfig.h"
#include "../inc/MLextractCoefficients.h"
#include "../inc/MLcrossValidation.h"

/*** Utiility functions **/
int chopByWhiteRespectSingleQuotes(char *in, char *outArray[], int outSize)
/* Like chopString, but specialized for white space separators.
 * Further, any singleQuotes (') are respected.
 * If doubleQuote is encloses whole string, then they are removed:
 *   'Fred and Ethyl' results in word [Fred and Ethyl]
 * If doubleQuotes exist inside string they are retained:
 *   Fred 'and Ethyl' results in word [Fred 'and Ethyl']
 * Special note '' is a valid, though empty word. */
{
int recordCount = 0;
char c;
char *quoteBegins = NULL;
boolean quoting = FALSE;
for (;;)
    {
    if (outArray != NULL && recordCount >= outSize)
        break;

    /* Skip initial separators. */
    while (isspace(*in)) ++in;
    if (*in == 0)
        break;

    /* Store start of word and look for end of word. */
    if (outArray != NULL)
        {
        outArray[recordCount] = in;
        if((*in == '\''))
            quoteBegins = (in+1);
        else
            quoteBegins = NULL;
        }
    recordCount += 1;
    quoting = FALSE;
    for (;;)
        {
        if ((c = *in) == 0)
            break;
        if(quoting)
            {
            if(c == '\'')
                {
                quoting = FALSE;
                if(quoteBegins != NULL) // implies out array
                    {
                    if((c = *(in+1) == 0 )|| isspace(c)) // whole word is quoted.
                        {
                        outArray[recordCount-1] = quoteBegins; // Fix beginning of word
                        quoteBegins = NULL;
                        break;
                        }
                    }
                }
            }
        else
            {
            quoting = (c == '\'');
            if (isspace(c))
                break;
            }
        ++in;
        }
    if (*in == 0)
        break;

    /* Tag end of word with zero. */
    if (outArray != NULL)
        *in = 0;
    /* And skip over the zero. */
    in += 1;
    }
    return recordCount;
}


/***** SVM-light-specific functions ******/
matrix * calcSVMcoefficients(struct hash * config)
/* Method to extract feature weights adapted from perl code 
** at http://www.cs.cornell.edu/People/tj/svm_light/svm2weight.pl.txt */
{
char file[1024];
FILE * fp;
char *trainingDir = hashMustFindVal(config, "trainingDir");
char *modelDir = hashMustFindVal(config, "modelDir");
char * line;
int i;

//get the labels out of an original data file
safef(file, sizeof(file), "%s/data.svm", trainingDir);
fp = fopen(file, "r");
if(!fp)
 	errAbort("ERROR: Couldn't open data file %s\n", file);
line = readLine(fp);
if(!startsWith("#", line))
	errAbort("Couldn't parse feature labels from %s. Exiting...\n", file);
//int numTokens = countChars(line, ' ') + 1;
int numTokens = chopByWhiteRespectDoubleQuotes(line, NULL, 0);
char *tokens[numTokens];
chopByWhiteRespectDoubleQuotes(line, tokens, numTokens);

matrix * model = init_matrix((numTokens - 1),1);
safef(model->colLabels[0], MAX_LABEL, "Metagene Score");
for(i = 1; i < numTokens; i++)
	{
	if(i > model->rows)
		errAbort("Too many row labels in %s", file);
	safef(model->rowLabels[(i-1)], MAX_LABEL, "%s", stripEnclosingDoubleQuotes(cloneString(skipBeyondDelimit(tokens[i], ':'))) );
	}
model->labels=1;
fclose(fp);
 
safef(file, sizeof(file), "%s/model.svm", modelDir);
fp = fopen(file, "r");
if(!fp)
	errAbort("ERROR: Couldn't open model %s\n", file);
int row = 0;
//advance to the line before weights start
while((line = readLine(fp)) && line != NULL)
	{
	if(row == 11)
		break;
	else if(row == 1 && !startsWithWord("0", line))
		return NULL;
	else if(row == 10 && !containsStringNoCase(line,"threshold b"))
		return NULL;
	row++;
	}

//iterate through results, assigning weights
int lineCounter=0;
while((line = readLine(fp)) && line != NULL)
	{
	chopByWhiteRespectDoubleQuotes(line, tokens, numTokens);
	double alpha = atof(tokens[0]);
	for(i = 1; i < numTokens; i++)
		{
		if(!startsWith("#", tokens[i])) 
			{
			//split each token by '[ix]:[value]'
			int ix = atoi(cloneFirstWordByDelimiter(tokens[i], ':'));
			ix--; //move from 1 to zero based.
			double val = atof(skipBeyondDelimit(tokens[i], ':'));
			if(lineCounter == 0)
				model->graph[ix][0] = (val*alpha);
			else
				model->graph[ix][0] += (val*alpha);
			}
		}
	lineCounter++;
	}
if(lineCounter == 0)
	return NULL; //if there aren't enough examples the model will be empty. Catch this
fclose(fp);
return model;
}

/****** WEKA specific functions *********/
matrix * calcWEKAcoefficients_SMO(struct hash * config)
{
char file[1024];
FILE * fp;
char *modelDir = hashMustFindVal(config, "modelDir");

safef(file, sizeof(file), "%s/output.weka", modelDir);
fp = fopen(file, "r");
if(!fp)
	errAbort("ERROR: Couldn't open model %s\n", file);

char * line;
int row = 0, rows = 0;
//count the number of lines in the model
while( (line = readLine(fp)) && line != NULL)
{
if(strstr(line, "(normalized)"))
	rows++;
}
if(rows == 0)
	return NULL; //If there aren't enough samples on each side of the plane the model will be empty. Return NULL
rewind(fp);

//assign mem to the model for a model this size
matrix * model = init_matrix(rows, 1);
safecpy(model->colLabels[0],MAX_LABEL, "Metagene Score");

//save the values to a matrix
while( (line = readLine(fp)) && line != NULL)
	{
	if(strstr(line, "(normalized)"))
		{
		if(startsWith(" +", line))
			line = skipBeyondDelimit(line, '+');
		line = skipLeadingSpaces(line);
		model->graph[row][0] = atof(firstWordInLine(cloneString(line)));
		char * nameStart = findWordByDelimiter("(normalized)", ' ', line);
		nameStart = skipToSpaces(nameStart);
		nameStart = skipLeadingSpaces(nameStart);
		safef(model->rowLabels[row], MAX_LABEL,"%s", cloneString(nameStart));
		row++;
		}
	}
model->labels=1;
return model;
}

matrix * calcWEKAcoefficients(struct hash * config)
{
if(sameString("weka.classifiers.functions.SMO", hashMustFindVal(config, "parameters")))
	{
	return calcWEKAcoefficients_SMO(config);
	}
return NULL;
}


/***** NMFpredictor-specific functions *****/
matrix * averageNMFmodels(struct hash * config)
/*read the results files in the modelDir's and average the best ones from each fold*/
{
char file[1024];
FILE * fp;
matrix * avgModel = NULL;
int fold, folds = foldsCountFromDataDir(config);
int split, splits = splitsCountFromDataDir(config);
char *modelDir = hashMustFindVal(config, "modelDir");

//average best models from each fold
for(split = 1; split <= splits; split++)
	{
	for(fold = 1; fold <= folds; fold++)
		{
		safef(file, sizeof(file), "%s/split%02d/fold%02d/model.tab", modelDir, split, fold);
		fp = fopen(file, "r");
		if(!fp)
			errAbort("ERROR: Couldn't open model %s\n", file);
		matrix * model = f_fill_matrix(fp, 1);
		fclose(fp);
	
		if(split == 1 && fold == 1)
			avgModel = copy_matrix(model);
		else
			add_matrices(avgModel, model, avgModel);
		free_matrix(model);
		}
	}

//scale by number of averaged models
int i, j;
for(i = 0; i < avgModel->rows; i++)
	{
	for(j = 0; j < avgModel->cols; j++)
		{
		if(avgModel->graph[i][j] != NULL_FLAG)
			avgModel->graph[i][j] = (avgModel->graph[i][j] / (folds*splits));
		}
	}
return avgModel;
}

matrix * calcNMFcoefficients(struct hash * config)
/*Report the ratio of scores for each class in NMF model*/
{
char * modelDir = hashMustFindVal(config, "modelDir");
char modelFile[1024];
safef(modelFile, sizeof(modelFile), "%s/model.tab", modelDir);
FILE * fp = fopen(modelFile, "r");
if(!fp)
	errAbort("Error! Model file %s couldn't be opened for reading\n", modelFile);
matrix * model = f_fill_matrix(fp, 1);
fclose(fp);

//check that this is a binary classifier NMFpredictor
if(model->cols != 2)
	errAbort("Error: Trying to calculate coefficients for NMFpredictor with more than 2 classes. Unsupported behaviour\n");

//copy the labels over
if(!model->labels)
    errAbort("Trying to obtain top features from a matrix with no labels.");

//otherwise make space for a linear model
matrix * result = init_matrix(model->rows, 1);
safef(result->colLabels[0], MAX_LABEL,  "Metagene Score");
copy_matrix_labels(result, model, 1,1);

//calc the vals
int i;
for(i = 0; i < model->rows; i++)
    {
    if((model->graph[i][0] != NULL_FLAG) && (model->graph[i][1] != NULL_FLAG))
		{
		if(model->graph[i][0] == 0 || model->graph[i][1] == 0)
			{
			result->graph[i][0] = 0;
			}
		else
			{
	        result->graph[i][0] = log2(model->graph[i][1] / model->graph[i][0]);
			}
		}
    else
		{
        result->graph[i][0] = 0; //consider using NULL_FLAG and handling NULL_FLAG in the frontend
		}
    }

//clean up
free_matrix(model);
return result;
}

matrix * calcglmnetcoefficients(struct hash * config)
/*Report the coefficients for the features in a glmnet model*/
{
char * modelDir = hashMustFindVal(config, "modelDir");
char coeffsFile[1024];
safef(coeffsFile, sizeof(coeffsFile), "%s/model.coeffs", modelDir);
FILE * fp = fopen(coeffsFile, "r");
if(!fp)
    errAbort("Error! Model file %s couldn't be opened for reading\n", coeffsFile);
matrix * result = f_fill_matrix(fp, 1);
fclose(fp);
return result;
}


/***** external functions ******/
matrix * extractCoefficients(struct hash *config)
/*Return coefficients from a model, if this is supported behavior*/
{
char * classifier = hashMustFindVal(config, "classifier");
char * classifierParameters = hashMustFindVal(config, "parameters");
matrix * result = NULL;
if(sameString(classifier, "NMFpredictor"))
	result = calcNMFcoefficients(config);
else if(sameString(classifier, "SVMlight") && sameString(classifierParameters, "t=0"))
	result = calcSVMcoefficients(config);
else if(sameString(classifier, "WEKA"))
	result = calcWEKAcoefficients(config);
else if(sameString(classifier, "glmnet"))
    result = calcglmnetcoefficients(config);

return result;
}


matrix * extractTopCoefficients(struct hash *config, int n)
/*Return top N coefficients from a model, if this is supported behavior*/
{
char * classifier = hashMustFindVal(config, "classifier");
char * classifierParameters = hashMustFindVal(config, "parameters");
matrix * result = NULL;
if(sameString(classifier, "NMFpredictor"))
    result = calcNMFcoefficients(config);
else if(sameString(classifier, "SVMlight") && sameString(classifierParameters, "t=0"))
    result = calcSVMcoefficients(config);
else if(sameString(classifier, "WEKA"))
    result = calcWEKAcoefficients(config);
else if(sameString(classifier, "glmnet"))
	result = calcglmnetcoefficients(config);
if(result)
	{
	struct slInt * topList = listTopN(result, n, 1, 0); //get topN by rows, column 0.
	matrix * tmp = copy_matrix_subset(result, topList, NULL);
	free_matrix(result);
	slFreeList(&topList);
	result = tmp;
	}
return result;
}
