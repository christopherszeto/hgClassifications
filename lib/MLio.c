#include "../inc/MLmatrix.h"
#include "../inc/MLcrossValidation.h"
#include "../inc/MLreadConfig.h"
#include "../inc/MLextractCoefficients.h"
#include "../inc/MLio.h"
#include "../inc/MLfilters.h"
#include "portable.h"
#include "../inc/sqlite3.h"

/* helper functions*/
int isnum(char * str){
        int result = 1;
        int flagDecimals = 0;
        char * c = str;
        while(*c != '\n' && *c != EOF && *c != '\0'){
                if(result){
                        if(*c == '-'){
                                if(c != str) // if there's a - and it's not the first char, it's not a number
                                        result = 0;
                        }else if(*c == '.')
                                flagDecimals++;
                        else if(*c < '0' || *c > '9')
                                result = 0;
                }
                c++;
        }
        if(flagDecimals > 1 || *c == '.') //if there are too many decimals, or it's at the end, not a number
                result = 0;
        return result;
}

/*** Functions to apply models to new data ***/
int countFeatureUsage(matrix * data)
/*Counts the rows of non-null data found in data matrix (i.e. useful features in data)*/
{
int matchedFeatures = 0;

int i, j;
for(i = 0; i < data->rows; i++)
    {
    int dataRow = 0;
    for(j = 0; j < data->cols; j++)
        {
        if(data->graph[i][j] != NULL_FLAG)
            {
            dataRow++;
            break;
            }
        }
    if(dataRow)
        matchedFeatures++;
    }
return matchedFeatures;
}

matrix * SVMgetDataMatchedToModel(matrix * data, char * modelPath)
/*Return a copy of the data matrix matched to the SVM model - filled with nulls if necessary*/
{
//strip enclosing chars if they exist
matrix * matchedData = copy_matrix(data);
int i;
for(i = 0; i < matchedData->rows; i++)
	safef(matchedData->rowLabels[i], MAX_LABEL, "%s", cloneString(stripEnclosingSingleQuotes(stripEnclosingDoubleQuotes(matchedData->rowLabels[i]))));

//Read the model features into a faked matrix object for matching labels.
char * lastLine = NULL,* line = NULL;
FILE * fp = fopen(modelPath, "r");
if(!fp)
    errAbort("Couldn't open file %s for conversion to an SVMlight for", modelPath);

int numSamples=0,lineCounter=0;
while((line = readLine(fp)) && line != NULL)
    {
    lineCounter++;
    lastLine = line;
    }
numSamples = lineCounter - 12; //header and labels lines add to 12 lines

int numFeatures = chopByWhiteRespectDoubleQuotes(lastLine, NULL, 0);
char *tokens[numFeatures];
chopByWhiteRespectDoubleQuotes(lastLine, tokens, numFeatures);
struct slName * thisLabel, *featureLabels = slNameListFromStringArray(tokens, numFeatures); 
thisLabel = featureLabels;
if(!sameString(thisLabel->name, "#Labels"))
    errAbort("The model file %s hasn't been tagged with feature labels. Can't apply this model to novel data\n", modelPath);
else
    thisLabel= thisLabel->next;
while(thisLabel)
    {
    thisLabel=thisLabel->next;
    numFeatures++;
    }
matrix * model = init_matrix(numFeatures, 1);
char * valPtr;
thisLabel = featureLabels->next; //skip the first "#Labels" name
numFeatures = 0;
while(thisLabel)
    {
    valPtr = strchr(thisLabel->name, ':') + 1;
    safef(model->rowLabels[numFeatures], MAX_LABEL, "%s", cloneString(stripEnclosingSingleQuotes(stripEnclosingDoubleQuotes(valPtr))));
    model->graph[numFeatures++][0] = NULL_FLAG;
    thisLabel = thisLabel->next;
    }
safef(model->colLabels[0], MAX_LABEL, "fakeData");
model->labels=1;

//Reduce data rows down to those in the 'fake' model data
if(!matchedRowLabels(matchedData, model))
    {
    matrix * tmp = matchOnRowLabels(matchedData, model);
    free_matrix(matchedData);
    matchedData = tmp;
    }
//clean up
free_matrix(model);

return matchedData;
}

matrix * NMFgetDataMatchedToModel(matrix * data, char * modelPath)
/*Return copy of data matrix with rows matching that in the model - filled with nulls if necessary*/
{
FILE * fp = fopen(modelPath, "r");
if(!fp)
    errAbort("Couldn't open data file %s for reading.\n", modelPath);
matrix * model = f_fill_matrix(fp, 1);
fclose(fp);

matrix * matchedData = copy_matrix(data);
//HACK: Put single quotes around names in data if model has them.
if(model->rowLabels[0][0] ==  '\'' && lastChar(model->rowLabels[0]) == '\'' && matchedData->rowLabels[0][0] != '\'')
	{
	int i;
	for(i = 0; i < matchedData->rows; i++)
		safef(matchedData->rowLabels[i], MAX_LABEL, "\'%s\'", cloneString(matchedData->rowLabels[i]));
	}

//make sure the feature labels match
if(!matchedRowLabels(matchedData, model))
    {
    matrix * tmp = matchOnRowLabels(matchedData, model);
    free_matrix(matchedData);
    matchedData = tmp;
    }

if(countFeatureUsage(matchedData) == 0)
	errAbort("ERROR: No features match between model and data\n");

//clean up 
free_matrix(model);

return matchedData;
}

matrix * glmnetgetDataMatchedToModel(matrix * data, char *tmpDataPrefix, char * modelPath, char * parameters)
/*TODO: This relies on system commands and copying to scratch space on disk! potentially a problem*/
{
matrix * matchedData = copy_matrix(data);
//do an 'extract coeffs' to get all features out of model, then read that in as a matrix
char coeffsFile[1024];
safef(coeffsFile, sizeof(coeffsFile), "%s.coeffs", tmpDataPrefix); 

char rawCommand[1024];
safef(rawCommand, sizeof(rawCommand), "%s/Rwrapper.sh %s/%s_extractCoefficients.R arg1=%s,arg2=%s %s.Rout",
	CLUSTERROOTDIR, CLUSTERROOTDIR,
	parameters, modelPath, coeffsFile, tmpDataPrefix);
char * command = cloneString(rawCommand);
//make sure the modelPAth has no parens, these will break command line processing
if(strchr(command, '(') || strchr(command, ')'))
    {
    char * tmpCpy = replaceChars(command, "(", "\\(");
    char * tmpCpy2 = replaceChars(tmpCpy, ")", "\\)");
    freeMem(command);
    freeMem(tmpCpy);
    command = tmpCpy2;
    }
system(command);
freeMem(command);

FILE * fp = fopen(coeffsFile, "r");
if(!fp)
	return NULL;
matrix * modelCoeffs = f_fill_matrix(fp, 1);
fclose(fp); 

//HACK: Put single quotes around names in data if model has them.
if(modelCoeffs->rowLabels[0][0] ==  '\'' && lastChar(modelCoeffs->rowLabels[0]) == '\'' && matchedData->rowLabels[0][0] != '\'')
    {
    int i;
    for(i = 0; i < matchedData->rows; i++)
        safef(matchedData->rowLabels[i], MAX_LABEL, "\'%s\'", cloneString(matchedData->rowLabels[i]));
	}

//make sure the feature labels match
if(!matchedRowLabels(matchedData, modelCoeffs))
    {
    matrix * tmp = matchOnRowLabels(matchedData, modelCoeffs);
    free_matrix(matchedData);
    matchedData = tmp;
    }

if(countFeatureUsage(matchedData) == 0)
    errAbort("ERROR: No features match between model and data\n");

//clean up
free_matrix(modelCoeffs);
unlink(coeffsFile);
safef(coeffsFile, sizeof(coeffsFile), "%s.Rout", tmpDataPrefix);
unlink(coeffsFile);

return matchedData;
}

matrix * applySVMlightModel(matrix * data, char * tmpDataPrefix, char * modelPath)
{
char tmpDataFile[256];

//make and print an SVM style matrix from reduced data
matrix * fakeMetadata = init_matrix(1, data->cols);
int i;
for(i = 0; i < data->cols; i++)
    fakeMetadata->graph[0][i] = NULL_FLAG;
matrix * svm_dataCpy = matricesToSVM(data, fakeMetadata);
safef(tmpDataFile, sizeof(tmpDataFile), "%s_data.svm", tmpDataPrefix);
FILE * fp = fopen(tmpDataFile, "w");
if(!fp)
    errAbort("Couldn't open file %s for writing.\n", tmpDataFile);
fprint_SVMlite_matrix(fp, svm_dataCpy);
fclose(fp);
//apply the model
char rawCommand[1024];
safef(rawCommand, sizeof(rawCommand), "%s/SVMlightWrapper.sh svm_classify %s_data.svm %s %s.tmpResults %s.log",
	CLUSTERROOTDIR,
    tmpDataPrefix, modelPath, tmpDataPrefix, tmpDataPrefix);
char * command = cloneString(rawCommand);
//make sure the modelPAth has no parens, these will break command line processing
if(strchr(command, '(') || strchr(command, ')'))
    {
    char * tmpCpy = replaceChars(command, "(", "\\(");
    char * tmpCpy2 = replaceChars(tmpCpy, ")", "\\)");
    freeMem(command);
    freeMem(tmpCpy);
    command = tmpCpy2;
    }
system(command);
freeMem(command);
//save the results to the correct format to 
safef(tmpDataFile, sizeof(tmpDataFile), "%s.tmpResults", tmpDataPrefix);
matrix * results = SVMpopulatePredictionsMatrix(tmpDataFile);
safef(results->rowLabels[0], MAX_LABEL, "Prediction");
copy_matrix_labels(results, data, 2,2);

//clean up
safef(tmpDataFile, sizeof(tmpDataFile), "%s_data.svm", tmpDataPrefix);
unlink(tmpDataFile);
safef(tmpDataFile, sizeof(tmpDataFile), "%s.log", tmpDataPrefix);
unlink(tmpDataFile);
safef(tmpDataFile, sizeof(tmpDataFile), "%s_data.svm", tmpDataPrefix);
unlink(tmpDataFile);
safef(tmpDataFile, sizeof(tmpDataFile), "%s.tmpResults", tmpDataPrefix);
unlink(tmpDataFile);
free_matrix(svm_dataCpy);
free_matrix(fakeMetadata);

return results;
}

matrix * applyNMFpredictorModel(matrix * data, char * tmpDataPrefix, char * modelPath)
/*Apply the model at modelPath to the data at tmpDataPrefix_data.tab*/
/*Writes to disk, but cleans up temporary files as it goes*/
{
char tmpDataFile[256];
safef(tmpDataFile, sizeof(tmpDataFile), "%s_matchedData.tab", tmpDataPrefix);
FILE * fp = fopen(tmpDataFile, "w");
if(!fp)
    errAbort("Couldn't open file %s for writing.\n", tmpDataFile);
fprint_matrix(fp,data);
fclose(fp);
shift_matrix(data);

char rawCommand[1024];
safef(rawCommand, sizeof(rawCommand), "%s/NMFpredictor_classify -rawCorrelations %s_matchedData.tab %s %s.tmpResults",
	CLUSTERROOTDIR,
    tmpDataPrefix, modelPath, tmpDataPrefix);
char * command = cloneString(rawCommand);
//make sure the modelPAth has no parens, these will break command line processing
if(strchr(command, '(') || strchr(command, ')'))
    {
    char * tmpCpy = replaceChars(command, "(", "\\(");
    char * tmpCpy2 = replaceChars(tmpCpy, ")", "\\)");
    freeMem(command);
    freeMem(tmpCpy);
    command = tmpCpy2;
    }
system(command);
freeMem(command);

//convert the results to a labeled matrix of coefficients for converting to JSON
safef(tmpDataFile, sizeof(tmpDataFile), "%s.tmpResults", tmpDataPrefix);
matrix * results = NMFpopulatePredictionsMatrix(tmpDataFile);
results->labels =0;
safef(results->rowLabels[0], MAX_LABEL, "Prediction");
copy_matrix_labels(results, data, 2,2);

//clean up
safef(tmpDataFile, sizeof(tmpDataFile), "%s_matchedData.tab", tmpDataPrefix);
unlink(tmpDataFile);
safef(tmpDataFile, sizeof(tmpDataFile), "%s.tmpResults", tmpDataPrefix);
unlink(tmpDataFile);
safef(tmpDataFile, sizeof(tmpDataFile), "%s.results", tmpDataPrefix);
unlink(tmpDataFile);

return results;
}

matrix * applyglmnetModel(matrix * data, char * tmpDataPrefix, char * modelPath, char * parameters)
{
char tmpDataFile[256];
safef(tmpDataFile, sizeof(tmpDataFile), "%s_matchedData.tab", tmpDataPrefix);
FILE * fp = fopen(tmpDataFile, "w");
if(!fp)
    errAbort("Couldn't open file %s for writing.\n", tmpDataFile);
fprint_matrix(fp,data);
fclose(fp);

char rawCommand[1024];
safef(rawCommand, sizeof(rawCommand), "%s/Rwrapper.sh %s/%s_classify.R arg1=%s_matchedData.tab,arg2=%s,arg3=%s.tmpResults %s.Rout",
	CLUSTERROOTDIR,CLUSTERROOTDIR,
    parameters, tmpDataPrefix, modelPath, tmpDataPrefix, tmpDataPrefix);
char * command = cloneString(rawCommand);
//make sure the modelPAth has no parens, these will break command line processing
if(strchr(command, '(') || strchr(command, ')'))
    {
    char * tmpCpy = replaceChars(command, "(", "\\(");
    char * tmpCpy2 = replaceChars(tmpCpy, ")", "\\)");
    freeMem(command);
    freeMem(tmpCpy);
    command = tmpCpy2;
    }
system(command);
freeMem(command);

//convert the results to a labeled matrix of coefficients for converting to JSON
safef(tmpDataFile, sizeof(tmpDataFile), "%s.tmpResults", tmpDataPrefix);
matrix * results = glmnetpopulatePredictionsMatrix(tmpDataFile);
results->labels =0;
safef(results->rowLabels[0], MAX_LABEL, "Prediction");
copy_matrix_labels(results, data, 2,2);

//clean up
safef(tmpDataFile, sizeof(tmpDataFile), "%s_matchedData.tab", tmpDataPrefix);
unlink(tmpDataFile);
safef(tmpDataFile, sizeof(tmpDataFile), "%s.tmpResults", tmpDataPrefix);
unlink(tmpDataFile);
safef(tmpDataFile, sizeof(tmpDataFile), "%s.Rout", tmpDataPrefix);
unlink(tmpDataFile);

return results;
}

/****** Common functions *****/
int dirExists(char * dirname)
{
DIR * dir = opendir(dirname);
if(dir)
    {
    closedir(dir);
    return 1;
    }
return 0;
}

matrix * reportFolds(struct hash * config, matrix * data, int folds, int itemsInSplit, struct slInt * shuffledList)
{
matrix * foldReport = init_matrix(1, data->cols);
safef(foldReport->rowLabels[0], MAX_LABEL, "validationFold");
copy_matrix_labels(foldReport,data,2,2);

struct slInt * curr = shuffledList;
int i, j;
for(i = 1; i <= folds; i++)
	{
    for(j = 0; j < itemsInSplit && curr != NULL; j++)
		{
        foldReport->graph[0][curr->val] = i;
        curr = curr->next;
		}
    }
while(curr != NULL)
    {
    foldReport->graph[0][curr->val] = folds;
    curr = curr->next;
    }
return foldReport;
}

/******* Flatfile printing functions ****/
void flatfilesPrintPresplitData(struct hash * config, matrix * data, matrix * metadata)
{
char targetFile[1024];
char * trainingDir = hashMustFindVal(config, "trainingDir");
safef(targetFile, sizeof(targetFile), "%s/data.tab", trainingDir);
FILE * fp = fopen(targetFile, "w");
if(!fp)
  errAbort("Error: couldn't write to the file %s", targetFile);
fprint_matrix(fp, data);
fclose(fp);

safef(targetFile, sizeof(targetFile), "%s/metadata.tab", trainingDir);
fp = fopen(targetFile, "w");
if(!fp)
    errAbort("Error: couldn't write to the file %s", targetFile);
fprint_discreteMatrix(fp, metadata);
fclose(fp);

}

void flatfilesPrintSplits(struct hash * config, matrix * data,matrix * metadata, int split, int fold, struct slInt * validationList)
{
char targetFile[1024];
FILE * fp;
char * validationDir = hashMustFindVal(config, "validationDir");
char * trainingDir = hashMustFindVal(config, "trainingDir");

if(count_indices(validationList))
    {
    //print validation data
    matrix *valData = copy_matrix_subset(data,NULL, validationList);
    safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.tab", validationDir,split, fold);
    fp = fopen(targetFile, "w");
    if(fp == NULL)
        errAbort("Couldn't open file %s for writing.\n", targetFile);
    fprint_matrix(fp, valData);
    fclose(fp);

    //print validation metadata
    matrix *valMetadata = copy_matrix_subset(metadata,NULL, validationList);
    safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/metadata.tab", validationDir, split, fold);
    fp = fopen(targetFile, "w");
    if(fp == NULL)
        errAbort("Couldn't open file %s for writing.\n", targetFile);
    fprint_matrix(fp, valMetadata);
    fclose(fp);

    //clean up
    free_matrix(valData);
    free_matrix(valMetadata);
    }

//get list of training samples
struct slInt * trainingList = index_complement(data->cols, validationList);

//print training data
if(count_indices(trainingList))
    {
    matrix *trData = copy_matrix_subset(data, NULL, trainingList);
    safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.tab", trainingDir, split, fold);
    fp = fopen(targetFile, "w");
    if(fp == NULL)
        errAbort("Couldn't open file %s for writing.\n", targetFile);
    fprint_matrix(fp, trData);
    fclose(fp);

    //print training metadata
    matrix *trMetadata = copy_matrix_subset(metadata, NULL, trainingList);
    safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/metadata.tab", trainingDir, split, fold);
    fp = fopen(targetFile, "w");
    if(fp == NULL)
        errAbort("Couldn't open file %s for writing.\n", targetFile);
    fprint_matrix(fp, trMetadata);
    fclose(fp);

    //clean up
    free_matrix(trData);
    free_matrix(trMetadata);
    }
slFreeList(&trainingList);
}

void NMFwriteClusterJobs(struct hash * config)
{
char * trainingDir = hashMustFindVal(config, "trainingDir");
char * modelDir = hashMustFindVal(config, "modelDir");
char * validationDir = hashMustFindVal(config, "validationDir");
char * parameters = hashMustFindVal(config, "parameters");
int fold,folds = foldsCountFromConfig(config);
int split,splits = splitsCountFromConfig(config);
char * classifiersRootDir = hashFindVal(config, "classifiersRootDir");
char classifierPath[1024];

//print the training jobs
char trainingJobsFile[1024];
char * clusterJobPrefix = hashFindVal(config, "clusterJobPrefix");

if(clusterJobPrefix)
    safef(trainingJobsFile, sizeof(trainingJobsFile), "%s_trainingJobList.txt", clusterJobPrefix);
else
    safef(trainingJobsFile, sizeof(trainingJobsFile), "trainingJobList.txt");

if(classifiersRootDir)
    safef(classifierPath,sizeof(classifierPath), "%s/NMFpredictor_train", classifiersRootDir);
else
    safef(classifierPath,sizeof(classifierPath), "%s/bin/NMFpredictor_train", CLUSTERROOTDIR);

FILE * fp = fopen(trainingJobsFile, "a");
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", trainingJobsFile);

if(sameString(parameters, "NA"))
	fprintf(fp, "%s %s/data.tab %s/metadata.tab %s/model.tab\n", classifierPath, trainingDir, trainingDir, modelDir);
else
	fprintf(fp, "%s %s %s/data.tab %s/metadata.tab %s/model.tab\n", classifierPath, parameters, trainingDir, trainingDir, modelDir);

for(split = 1; split <=splits; split++)
    {
	for(fold = 1; fold <= folds; fold++)
		{
		if(sameString(parameters, "NA"))
			{
	    	fprintf(fp, "%s %s/split%02d/fold%02d/data.tab %s/split%02d/fold%02d/metadata.tab %s/split%02d/fold%02d/model.tab\n",
	                classifierPath, trainingDir, split, fold, trainingDir, split, fold, modelDir, split, fold);
			}
		else
			{
	        fprintf(fp, "%s %s %s/split%02d/fold%02d/data.tab %s/split%02d/fold%02d/metadata.tab %s/split%02d/fold%02d/model.tab\n",
	                classifierPath, parameters, trainingDir, split, fold, trainingDir, split, fold, modelDir, split, fold);
			}
	    }
	}
fclose(fp);

//print the validation jobs
char validationJobsFile[1024];
if(clusterJobPrefix)
    safef(validationJobsFile, sizeof(validationJobsFile), "%s_validationJobList.txt", clusterJobPrefix);
else
    safef(validationJobsFile, sizeof(validationJobsFile), "validationJobList.txt");

if(classifiersRootDir)
    safef(classifierPath,sizeof(classifierPath), "%s/NMFpredictor_classify", classifiersRootDir);
else
    safef(classifierPath,sizeof(classifierPath), "%s/NMFpredictor_classify", CLUSTERROOTDIR);

fp = fopen(validationJobsFile, "a");
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", validationJobsFile);
fprintf(fp, "%s -rawCorrelations %s/data.tab %s/model.tab %s/NMFpredictor.training.results\n",
		classifierPath, trainingDir, modelDir, modelDir);
for(split = 1; split <= splits; split++)
	{
	for(fold = 1; fold <=folds; fold++)
   		{
   		fprintf(fp, "%s %s/split%02d/fold%02d/data.tab %s/split%02d/fold%02d/model.tab %s/split%02d/fold%02d/NMFpredictor.training.results\n", 
			classifierPath, trainingDir,split,fold, modelDir,split, fold, modelDir, split,fold);
    	fprintf(fp, "%s %s/split%02d/fold%02d/data.tab %s/split%02d/fold%02d/model.tab %s/split%02d/fold%02d/NMFpredictor.validation.results\n", 
			classifierPath, validationDir, split, fold, modelDir,split, fold, modelDir,split, fold);
    	}
	}
fclose(fp);

//print clean up jobs
char cleanupJobsFile[1024];
if(clusterJobPrefix)
    safef(cleanupJobsFile, sizeof(cleanupJobsFile), "%s_cleanupJobList.txt", clusterJobPrefix);
else
    safef(cleanupJobsFile, sizeof(cleanupJobsFile), "cleanupJobList.txt");

fp = fopen(cleanupJobsFile, "a");
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", cleanupJobsFile);
fprintf(fp, "rm -f %s/data.tab %s/metadata.tab\n", trainingDir, trainingDir);
for(split = 1; split <= splits; split++)
    {
    for(fold = 1; fold <=folds; fold++)
        {
        fprintf(fp, "rm -f %s/split%02d/fold%02d/data.tab %s/split%02d/fold%02d/metadata.tab",
            trainingDir,split,fold, trainingDir,split,fold);
        fprintf(fp, " %s/split%02d/fold%02d/data.tab %s/split%02d/fold%02d/metadata.tab\n",
            validationDir,split,fold, validationDir,split,fold);
        }
    }
fclose(fp);
}

/***** SVM printing functions *******/
void fprint_SVMlite_matrix(FILE *fp, matrix *A_ptr)
/*Modified matrix printing function for SVMlight format: space separated, column markers added*/
{
int i, a, j;
char tmpString[MAX_LABEL];
if(!A_ptr->labels)
    errAbort("ERROR: Trying to print to SVM format with no labels. Exiting..\n");

//print the feature labels out
fprintf(fp, "#Labels");
for(j = 0; j < A_ptr->cols; j++)
	fprintf(fp, " %d:\"%s\"", (j+1), A_ptr->colLabels[j]);
fprintf(fp, "\n");

for(i = 0; i < A_ptr->rows; i++)
	{
	//print the class label at the start of the line
	safef(tmpString,sizeof(tmpString), "%s", A_ptr->rowLabels[i]);
	for(j=0; j < MAX_LABEL && tmpString[j] != ','; j++)
 		fprintf(fp, "%c", tmpString[j]);
    fprintf(fp, " ");
    //print the data, space separated with column markers.
    for(a = 0; a < A_ptr->cols; a++)
		{
        if(A_ptr->graph[i][a] != NULL_FLAG)
            fprintf(fp, "%d:%.6f", (a+1), A_ptr->graph[i][a]);
        if(a != (A_ptr->cols -1))
            fprintf(fp, " ");
        else if(a == (A_ptr->cols - 1))
			{
			//print a hash then the sample label
            fprintf(fp, " #");
			char *c = strchr(A_ptr->rowLabels[i],',');
			if(c != NULL)
    			{
    			c++;
    			fprintf(fp, "%s", c);
    			}
			else
    			fprintf(fp, "NULL");
            fprintf(fp, "\n");
        	}
    	}
	}
}

void tagLabelsWithMetadata(matrix * data, matrix * metadata)
/*Small function to add the clinical value to the labels on a matrix for printing in SVMlight format*/
{
int i;
for(i = 0; i < data->cols; i++)
    {
	char * template = cloneString(data->colLabels[i]);
    if(metadata->graph[0][i] == 0)
        safef(data->colLabels[i], MAX_LABEL, "-1,%s",template);
    else if(metadata->graph[0][i] == 1)
        safef(data->colLabels[i], MAX_LABEL, "+1,%s", template);
    else
        safef(data->colLabels[i], MAX_LABEL, "0,%s", template);
	freeMem(template);
    }
}

void SVMprintPresplitData(struct hash * config, matrix * data, matrix * metadata)
{
char targetFile[1024];
char * trainingDir = hashMustFindVal(config, "trainingDir");
safef(targetFile, sizeof(targetFile), "%s/data.svm", trainingDir);
FILE * fp = fopen(targetFile, "w");
if(!fp)
    errAbort("Error: couldn't write to the file %s", targetFile);
matrix * svm_dataCpy = matricesToSVM(data, metadata);
fprint_SVMlite_matrix(fp, svm_dataCpy);
free_matrix(svm_dataCpy);
fclose(fp);
}

void SVMprintSplits(struct hash * config, matrix * data, matrix * metadata,int split, int fold, struct slInt * validationList)
{
char targetFile[1024];
FILE * fp;
char * validationDir = hashMustFindVal(config, "validationDir");
char * trainingDir = hashMustFindVal(config, "trainingDir");

//print validation data
if(count_indices(validationList))
	{
	matrix * validationData = copy_matrix_subset(data, NULL, validationList);
	matrix * validationMetadata = copy_matrix_subset(metadata, NULL, validationList);
	matrix * svm_validationData = matricesToSVM(validationData, validationMetadata);
	free_matrix(validationData);
	free_matrix(validationMetadata);
	safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.svm", validationDir, split, fold);
	fp = fopen(targetFile, "w");
	if(fp == NULL)
	    errAbort("Couldn't open file %s for writing.\n", targetFile);
	fprint_SVMlite_matrix(fp, svm_validationData);
	fclose(fp);
	free_matrix(svm_validationData);
	}

//get list of training samples
struct slInt * trainingList = index_complement(data->cols, validationList);

//print training data
if(count_indices(trainingList))
	{
	matrix * trainingData = copy_matrix_subset(data, NULL, trainingList);
	matrix * trainingMetadata = copy_matrix_subset(metadata, NULL, trainingList);
	matrix * svm_trainingData = matricesToSVM(trainingData, trainingMetadata);
	free_matrix(trainingData);
	free_matrix(trainingMetadata);
	safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.svm", trainingDir, split, fold);
	fp = fopen(targetFile, "w");
	if(fp == NULL)
	    errAbort("Couldn't open file %s for writing.\n", targetFile);
	fprint_SVMlite_matrix(fp, svm_trainingData);
	fclose(fp);
	free_matrix(svm_trainingData);
	}
	
//clean up
slFreeList(&trainingList);
}

void SVMwriteClusterJobs(struct hash * config)
{
char * trainingDir = hashMustFindVal(config, "trainingDir");
char * modelDir = hashMustFindVal(config, "modelDir");
char * validationDir = hashMustFindVal(config, "validationDir");
char * parameters = hashMustFindVal(config, "parameters");
int fold,folds = foldsCountFromConfig(config);
int split, splits= splitsCountFromConfig(config);
char * classifiersRootDir = hashFindVal(config, "classifiersRootDir");
char classifierPath[1024];

//print the training jobs
char trainingJobsFile[1024];
char * clusterJobPrefix = hashFindVal(config, "clusterJobPrefix");

if(clusterJobPrefix)
    safef(trainingJobsFile, sizeof(trainingJobsFile), "%s_trainingJobList.txt", clusterJobPrefix);
else
    safef(trainingJobsFile, sizeof(trainingJobsFile), "trainingJobList.txt");

if(classifiersRootDir)
    safef(classifierPath,sizeof(classifierPath), "%s/SVMlightWrapper.sh", classifiersRootDir);
else
    safef(classifierPath,sizeof(classifierPath), "%s/SVMlightWrapper.sh", CLUSTERROOTDIR);

FILE * fp = fopen(trainingJobsFile, "a");
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", trainingJobsFile);

fprintf(fp, "%s svm_learn %s,l=%s/trans_predictions %s/data.svm %s/model.svm\n", 
	classifierPath, parameters, modelDir, trainingDir, modelDir);
for(split = 1; split <= splits; split++)
	{
	for(fold = 1; fold <=folds; fold++)
    	{
    	fprintf(fp, "%s svm_learn %s,l=%s/split%02d/fold%02d/trans_predictions %s/split%02d/fold%02d/data.svm %s/split%02d/fold%02d/model.svm\n",
                classifierPath, parameters,modelDir,split, fold, trainingDir,split,fold,modelDir,split,fold);
    	}
	}
fclose(fp);

//print the validation jobs
char validationJobsFile[1024];
if(clusterJobPrefix)
    safef(validationJobsFile, sizeof(validationJobsFile), "%s_validationJobList.txt", clusterJobPrefix);
else
    safef(validationJobsFile, sizeof(validationJobsFile), "validationJobList.txt");

fp = fopen(validationJobsFile, "a");
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", validationJobsFile);
fprintf(fp, "%s svm_classify %s/data.svm %s/model.svm %s/svm.training.results %s/training.log\n", classifierPath, trainingDir, modelDir, modelDir, modelDir);
for(split = 1; split <= splits; split++)
	{
	for(fold = 1; fold <=folds; fold++)
	    {
	    fprintf(fp, "%s svm_classify %s/split%02d/fold%02d/data.svm %s/split%02d/fold%02d/model.svm %s/split%02d/fold%02d/svm.training.results %s/split%02d/fold%02d/training.results.log\n", 
				classifierPath, trainingDir, split, fold, modelDir, split, fold, modelDir, split, fold, modelDir, split, fold);
	    fprintf(fp, "%s svm_classify %s/split%02d/fold%02d/data.svm %s/split%02d/fold%02d/model.svm %s/split%02d/fold%02d/svm.validation.results %s/split%02d/fold%02d/validation.results.log\n", 
				classifierPath, validationDir, split, fold, modelDir, split, fold, modelDir, split, fold, modelDir, split, fold);
	    }
	}
fclose(fp);

//print clean up jobs
char cleanupJobsFile[1024];
if(clusterJobPrefix)
    safef(cleanupJobsFile, sizeof(cleanupJobsFile), "%s_cleanupJobList.txt", clusterJobPrefix);
else
    safef(cleanupJobsFile, sizeof(cleanupJobsFile), "cleanupJobList.txt");

fp = fopen(cleanupJobsFile, "a");
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", cleanupJobsFile);
fprintf(fp, "rm -f %s/data.svm\n", trainingDir);
for(split = 1; split <= splits; split++)
    {
    for(fold = 1; fold <=folds; fold++)
        {
        fprintf(fp, "rm -f %s/split%02d/fold%02d/data.svm",
            trainingDir,split,fold);
        fprintf(fp, " %s/split%02d/fold%02d/data.svm\n",
            validationDir,split,fold);
        }
    }
fclose(fp);
}


/****** WEKA (ARFF) splitting functions **********/
void fprint_WEKA_matrix(FILE *fp, matrix *A_ptr)
/*Modified matrix printing function for WEKA (ARFF) format*/
/*This will be similar to SVM format: val,val,val,class%sampleName*/
{
int i, a, j, k, class;
char tmpString[MAX_LABEL], classLabel[MAX_LABEL];
if(!A_ptr->labels)
    errAbort("ERROR: Trying to print to ARFF format with no labels. Exiting..\n");

fprintf(fp, "@RELATION\t\'MetadataVsData\'\n\n");
for(i = 0; i < A_ptr->cols; i++)
	fprintf(fp, "@ATTRIBUTE\t\"%s\"\treal\n", A_ptr->colLabels[i]);
fprintf(fp, "@ATTRIBUTE\t\'class\'\t{\'subgroup1\',\'subgroup2\'}\n\n");

fprintf(fp, "@DATA\n");
for(i = 0; i < A_ptr->rows; i++)
	{
    //print the data, comma separated
    for(a = 0; a < A_ptr->cols; a++)
		{
        if(A_ptr->graph[i][a] != NULL_FLAG)
            fprintf(fp, "%.6f,", A_ptr->graph[i][a]);
		else
			fprintf(fp, "?,");
	}
	//print the class label at the end of the line
	safef(tmpString,sizeof(tmpString), "%s", A_ptr->rowLabels[i]);
	k = 0;
	for(j=0; j < MAX_LABEL && tmpString[j] != ','; j++)
    	classLabel[k++] = tmpString[j];
	classLabel[k] = '\0';
	class = atoi(classLabel);
	if(class == 0)
    	fprintf(fp, "?");  //if it's an unlabeled case print a weka 'null'
	else if(class == -1)
    	fprintf(fp, "\'subgroup1\'");  //print the class
	else if(class == 1)
		fprintf(fp, "\'subgroup2\'");

	//at the end of line, print a comment marker then the sample label
    fprintf(fp, "%%");
	char *c = strchr(A_ptr->rowLabels[i],',');
	if(c != NULL)
		{
		c++;
		fprintf(fp, "%s", c);
		}
	else
		fprintf(fp, "NULL");

    fprintf(fp, "\n");
	}
}


void WEKAprintPresplitData(struct hash * config, matrix * data, matrix * metadata)
{
char targetFile[1024];
char * trainingDir = hashMustFindVal(config, "trainingDir");
safef(targetFile, sizeof(targetFile), "%s/data.arff", trainingDir);
FILE * fp = fopen(targetFile, "w");
if(!fp)
    errAbort("Error: couldn't write to the file %s", targetFile);
matrix * dataCpy = copy_matrix(data);
tagLabelsWithMetadata(dataCpy, metadata);
matrix * t_dataCpy = transpose(dataCpy);
fprint_WEKA_matrix(fp, t_dataCpy);
free_matrix(t_dataCpy);
free_matrix(dataCpy);
fclose(fp);
}


void WEKAprintSplits(struct hash * config, matrix * data, matrix * metadata, int split, int fold, struct slInt * validationList)
{
char targetFile[1024];
FILE * fp;
char * validationDir = hashMustFindVal(config, "validationDir");
char * trainingDir = hashMustFindVal(config, "trainingDir");

//print validation data
if(count_indices(validationList))
    {
    matrix * validationData = copy_matrix_subset(data, NULL, validationList);
    matrix * validationMetadata = copy_matrix_subset(metadata, NULL, validationList);
    tagLabelsWithMetadata(validationData, validationMetadata);
    free_matrix(validationMetadata);
    matrix * t_validationData = transpose(validationData);
    free_matrix(validationData);
    safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.arff", validationDir,split, fold);
    fp = fopen(targetFile, "w");
    if(fp == NULL)
        errAbort("Couldn't open file %s for writing.\n", targetFile);
    fprint_WEKA_matrix(fp, t_validationData);
    fclose(fp);
    free_matrix(t_validationData);
    }

//get list of training samples
struct slInt * trainingList = index_complement(data->cols, validationList);

//print training data
if(count_indices(trainingList))
    {
    matrix * trainingData = copy_matrix_subset(data, NULL, trainingList);
    matrix * trainingMetadata = copy_matrix_subset(metadata, NULL, trainingList);
    tagLabelsWithMetadata(trainingData, trainingMetadata);
    free_matrix(trainingMetadata);
    matrix * t_trainingData = transpose(trainingData);
    free_matrix(trainingData);
    safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.arff", trainingDir, split, fold);
    fp = fopen(targetFile, "w");
    if(fp == NULL)
        errAbort("Couldn't open file %s for writing.\n", targetFile);
    fprint_WEKA_matrix(fp, t_trainingData);
    fclose(fp);
    free_matrix(t_trainingData);
    }

//clean up
slFreeList(&trainingList);
}


void WEKAwriteClusterJobs(struct hash * config)
{
char * trainingDir = hashMustFindVal(config, "trainingDir");
char * modelDir = hashMustFindVal(config, "modelDir");
char * validationDir = hashMustFindVal(config, "validationDir");
char * parameters = hashMustFindVal(config, "parameters");
int fold, folds = foldsCountFromConfig(config);
int split, splits = splitsCountFromConfig(config);
char * classifiersRootDir = hashFindVal(config, "classifiersRootDir");
char classifierPath[1024];

if(classifiersRootDir)
    safef(classifierPath,sizeof(classifierPath), "%s/WEKAwrapper.sh", classifiersRootDir);
else
	safef(classifierPath,sizeof(classifierPath), "%s/WEKAwrapper.sh", CLUSTERROOTDIR);

//print the training jobs
char trainingJobsFile[1024];
char * clusterJobPrefix = hashFindVal(config, "clusterJobPrefix");
if(clusterJobPrefix)
    safef(trainingJobsFile, sizeof(trainingJobsFile), "%s_trainingJobList.txt", clusterJobPrefix);
else
    safef(trainingJobsFile, sizeof(trainingJobsFile), "trainingJobList.txt");
FILE * fp = fopen(trainingJobsFile, "a");

fprintf(fp, "%s %s,t=%s/data.arff,d=%s/model.weka %s/output.weka\n", classifierPath, parameters,trainingDir, modelDir, modelDir);
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", trainingJobsFile);
for(split = 1; split <= splits; split++)
	{
	for(fold = 1; fold <=folds; fold++)
	    {
	    fprintf(fp, "%s %s,t=%s/split%02d/fold%02d/data.arff,d=%s/split%02d/fold%02d/model.weka %s/split%02d/fold%02d/output.weka\n",
	                classifierPath, parameters, trainingDir, split, fold, modelDir, split, fold, modelDir, split, fold);
	    }
	}
fclose(fp);

//print the validation jobs
char validationJobsFile[1024];
if(clusterJobPrefix)
    safef(validationJobsFile, sizeof(validationJobsFile), "%s_validationJobList.txt", clusterJobPrefix);
else
    safef(validationJobsFile, sizeof(validationJobsFile), "validationJobList.txt");
fp = fopen(validationJobsFile, "a");
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", validationJobsFile);
fprintf(fp, "%s %s,T=%s/data.arff,l=%s/model.weka,p=0 %s/weka.training.results\n", classifierPath, parameters, trainingDir, modelDir, modelDir);
for(split = 1; split <= splits; split++)
	{
	for(fold = 1; fold <=folds; fold++)
	    {
		fprintf(fp, "%s %s,T=%s/split%02d/fold%02d/data.arff,l=%s/split%02d/fold%02d/model.weka,p=0 %s/split%02d/fold%02d/weka.training.results\n", 
			classifierPath, parameters, trainingDir, split, fold, modelDir, split, fold, modelDir, split, fold);
	    fprintf(fp, "%s %s,T=%s/split%02d/fold%02d/data.arff,l=%s/split%02d/fold%02d/model.weka,p=0 %s/split%02d/fold%02d/weka.validation.results\n", 
			classifierPath, parameters, validationDir, split, fold, modelDir, split, fold, modelDir, split, fold);
	    }
	}
fclose(fp);

//print clean up jobs
char cleanupJobsFile[1024];
if(clusterJobPrefix)
    safef(cleanupJobsFile, sizeof(cleanupJobsFile), "%s_cleanupJobList.txt", clusterJobPrefix);
else
    safef(cleanupJobsFile, sizeof(cleanupJobsFile), "cleanupJobList.txt");

fp = fopen(cleanupJobsFile, "a");
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", cleanupJobsFile);
fprintf(fp, "rm -f %s/data.arff\n", trainingDir);
for(split = 1; split <= splits; split++)
    {
    for(fold = 1; fold <=folds; fold++)
        {
        fprintf(fp, "rm -f %s/split%02d/fold%02d/data.arff",
            trainingDir,split,fold);
		fprintf(fp, " %s/split%02d/fold%02d/data.arff\n",
        	validationDir,split,fold);
        }
    }
fclose(fp);
}


/***** glmnet processing functions *******/
void glmnetwriteClusterJobs(struct hash * config)
{
char * trainingDir = hashMustFindVal(config, "trainingDir");
char * modelDir = hashMustFindVal(config, "modelDir");
char * validationDir = hashMustFindVal(config, "validationDir");
char * parameters = hashMustFindVal(config, "parameters");
int fold,folds = foldsCountFromConfig(config);
int split,splits = splitsCountFromConfig(config);
char * classifiersRootDir = hashFindVal(config, "classifiersRootDir");
char classifierPath[1024];
if(classifiersRootDir)
	safef(classifierPath, sizeof(classifierPath), "%s", classifiersRootDir);
else
	safef(classifierPath, sizeof(classifierPath), "%s", CLUSTERROOTDIR);
if(!parameters)
	errAbort("glmnet model specified without parameters. Exiting..\n");

//print the training jobs
char trainingJobsFile[1024];
char * clusterJobPrefix = hashFindVal(config, "clusterJobPrefix");

if(clusterJobPrefix)
    safef(trainingJobsFile, sizeof(trainingJobsFile), "%s_trainingJobList.txt", clusterJobPrefix);
else
    safef(trainingJobsFile, sizeof(trainingJobsFile), "trainingJobList.txt");

FILE * fp = fopen(trainingJobsFile, "a");
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", trainingJobsFile);

fprintf(fp, "%s/Rwrapper.sh %s/%s_train.R arg1=%s/data.tab,arg2=%s/metadata.tab,arg3=%s/model.RData %s/training.Rout\n",
	classifierPath,classifierPath, parameters, trainingDir, trainingDir, modelDir, modelDir);

for(split = 1; split <=splits; split++)
    {
    for(fold = 1; fold <= folds; fold++)
        {
        fprintf(fp, "%s/Rwrapper.sh %s/%s_train.R arg1=%s/split%02d/fold%02d/data.tab,arg2=%s/split%02d/fold%02d/metadata.tab,arg3=%s/split%02d/fold%02d/model.RData %s/split%02d/fold%02d/training.Rout\n",
            classifierPath,classifierPath, parameters, trainingDir, split, fold, trainingDir, split, fold, modelDir, split, fold, modelDir, split, fold);
        }
    }
fclose(fp);

//print the validation jobs
char validationJobsFile[1024];
if(clusterJobPrefix)
    safef(validationJobsFile, sizeof(validationJobsFile), "%s_validationJobList.txt", clusterJobPrefix);
else
    safef(validationJobsFile, sizeof(validationJobsFile), "validationJobList.txt");

fp = fopen(validationJobsFile, "a");
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", validationJobsFile);
fprintf(fp, "%s/Rwrapper.sh %s/%s_classify.R arg1=%s/data.tab,arg2=%s/model.RData,arg3=%s/%s.training.results %s/training.results.Rout\n",
        classifierPath, classifierPath, parameters, trainingDir, modelDir, modelDir, parameters, modelDir);
//extact coeffs from fully trained model while doing classification jobs
fprintf(fp, "%s/Rwrapper.sh %s/%s_extractCoefficients.R arg1=%s/model.RData,arg2=%s/model.coeffs %s/extractingCoeffs.Rout\n",
        classifierPath, classifierPath, parameters, modelDir, modelDir, modelDir);

for(split = 1; split <= splits; split++)
    {
    for(fold = 1; fold <=folds; fold++)
        {
        fprintf(fp, "%s/Rwrapper.sh %s/%s_classify.R arg1=%s/split%02d/fold%02d/data.tab,arg2=%s/split%02d/fold%02d/model.RData,arg3=%s/split%02d/fold%02d/%s.training.results %s/split%02d/fold%02d/training.results.Rout\n",
            classifierPath, classifierPath, parameters, trainingDir,split,fold, modelDir,split, fold, modelDir, split,fold, parameters,modelDir, split,fold);
        fprintf(fp, "%s/Rwrapper.sh %s/%s_classify.R arg1=%s/split%02d/fold%02d/data.tab,arg2=%s/split%02d/fold%02d/model.RData,arg3=%s/split%02d/fold%02d/%s.validation.results %s/split%02d/fold%02d/validation.results.Rout\n",
            classifierPath, classifierPath, parameters, validationDir, split, fold, modelDir,split, fold, modelDir,split, fold, parameters,modelDir,split, fold);
        }
    }
fclose(fp);

//print clean up jobs
char cleanupJobsFile[1024];
if(clusterJobPrefix)
    safef(cleanupJobsFile, sizeof(cleanupJobsFile), "%s_cleanupJobList.txt", clusterJobPrefix);
else
    safef(cleanupJobsFile, sizeof(cleanupJobsFile), "cleanupJobList.txt");

fp = fopen(cleanupJobsFile, "a");
if(fp == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", cleanupJobsFile);
fprintf(fp, "rm -f %s/data.tab %s/metadata.tab\n", trainingDir, trainingDir);
for(split = 1; split <= splits; split++)
    {
    for(fold = 1; fold <=folds; fold++)
        {
        fprintf(fp, "rm -f %s/split%02d/fold%02d/data.tab %s/split%02d/fold%02d/metadata.tab",
            trainingDir,split,fold, trainingDir,split,fold);
		fprintf(fp, " %s/split%02d/fold%02d/data.tab %s/split%02d/fold%02d/metadata.tab\n",
            validationDir,split,fold, validationDir,split,fold);
        }
    }
fclose(fp);
}

/*** Data splitting functions ****/
void splitDataFromBioInt(struct hash * config)
{
//read data in to internal matrix object
char * profile = hashMustFindVal(config, "profile"); 
char * db = hashMustFindVal(config, "db");
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char *tableName = hashMustFindVal(config, "tableName");
matrix * data = bioInt_fill_matrix(conn, tableName);
char * clinField = hashMustFindVal(config, "clinField");
matrix * metadata = bioInt_fill_KH(conn, data, clinField);
hFreeConn(&conn);
applyFilters(config, data, metadata);
writeSplits(config, data, metadata);
//clean up
free_matrix(data);
free_matrix(metadata);
}

void splitDataFromFile(struct hash * config)
{
//read data in to internal matrix object
char * dataFilepath = hashMustFindVal(config, "dataFilepath");
char * metadataFilepath = hashMustFindVal(config, "metadataFilepath");
FILE * dataFile, * metadataFile;
dataFile = fopen(dataFilepath, "r");
metadataFile = fopen(metadataFilepath, "r");
if(dataFile == NULL)
    errAbort("ERROR: Couldn't open the file \"%s\"\n", dataFilepath);
if(metadataFile == NULL)
    errAbort("ERROR: Couldn't open the file %s. Exiting...\n", metadataFilepath);

matrix * data = f_fill_matrix(dataFile, 1);
fclose(dataFile);
matrix * raw_metadata = f_fill_matrix(metadataFile, 1);
fclose(metadataFile);

matrix * metadata = copyMetadataByColLabels(data, raw_metadata);
free_matrix(raw_metadata);
applyFilters(config, data, metadata);
writeSplits(config, data, metadata);
//clean up
free_matrix(data);
free_matrix(metadata);
}

void splitDataFromFive3db(struct hash * config)
{
//read data in to internal matrix object
char * db = hashMustFindVal(config, "db");
struct sqlite3 *conn = NULL;
if(sqlite3_open(db, &conn) != SQLITE_OK)
	errAbort("Couldn't open connection to sqlite database %s", db);
char *tableName = hashMustFindVal(config, "tableName");
matrix * data = five3db_fill_matrix(conn, tableName);
char * clinField = hashMustFindVal(config, "clinField");
//HERE this is a hack to work with Steve's data. Uncomment the following when five3b has clinical data
//matrix * metadata = five3db_fill_KH(conn, data, clinField);
struct sqlConnection *conn2 = hAllocConnProfile("localDb", "bioInt");
matrix * metadata = bioInt_fill_KH(conn2, data, clinField);
hFreeConn(&conn2);
//Hack over

sqlite3_close(conn);
applyFilters(config, data, metadata);
writeSplits(config, data, metadata);
//clean up 
free_matrix(data);
free_matrix(metadata);
}

void applyFilters(struct hash *config, matrix * data, matrix * metadata)
{
if(hashFindVal(config, "excludeList"))
	{
	matrix * trimmedData = filterColumnsByExcludeList(config, data);
	free_matrix(data);
	data = trimmedData;
	matrix * trimmedMetadata = filterColumnsByExcludeList(config, metadata);
	free_matrix(metadata);
	metadata = trimmedMetadata;
	}

if(hashFindVal(config, "featureSelection") && 
	!sameString("None", hashFindVal(config, "featureSelection")))
    {
    matrix * trimmedData = featureSelection(config, data);
    free_matrix(data);
    data = trimmedData;
    }

if(hashFindVal(config, "dataDiscretizer") &&
    !sameString("None", hashFindVal(config, "dataDiscretizer")))
    {
    matrix * trimmedData = discretizeData(config, data);
	free_matrix(data);
	data = trimmedData;
    }

if(hashFindVal(config, "clinDiscretizer") &&
    !sameString("None", hashFindVal(config, "clinDiscretizer")))
    {
    matrix * trimmedData = discretizeMetadata(config, metadata);
	free_matrix(metadata);
	metadata = trimmedData;
    }
}

void writeSplits(struct hash *config, matrix * data, matrix * metadata)
{
//make data dirs if they don't already exist. 
char * trainingDir = hashMustFindVal(config, "trainingDir");
if(!dirExists(trainingDir))
	createDataDirs(config);

//print the original data to the trainingdir
printPresplitData(config, data, metadata);

//create a shuffled list of indices to walk over when selecting random splits
struct slInt *list = list_indices(metadata->cols);
int split, splits = splitsCountFromConfig(config);
int fold, folds = foldsCountFromConfig(config);

//create a place to save folds reports (may not be used)
matrix * foldsReport = init_matrix(splits, data->cols);
copy_matrix_labels(foldsReport, data, 2,2);
for(split = 1; split <= splits; split++)
	safef(foldsReport->rowLabels[(split-1)],sizeof(foldsReport->rowLabels[(split-1)]), "split%d", split);
foldsReport->labels = 1;
//iterate over splits and folds, peeling off samples into trainign and validation directories
for(split = 1; split <= splits; split++)
	{
	//shuffle up the samples based on fold order (lowest fold first)
	struct slInt *curr = NULL, *shuffledList = NULL;
	int usedSamples = 0;
	if(hashFindVal(config, "foldsDescriptionPath"))
		shuffledList = orderIndicesByDescription(config, data, split); 
	else
		shuffledList = seeded_shuffle_indices(list,split); 
	//iterate over folds, doing splitting.
	for(fold = 1; fold <= folds; fold++)
		{
		struct slInt *valCurr = NULL, *validationList = NULL;
		int itemsInFold = itemsInFoldFromConfig(config,data->cols, split, fold);
		int i;
		//advance the curr pointer past the samples used already
		curr = shuffledList;
		for(i = 0; i < usedSamples; i++)
			curr = curr->next;
		//copy the indices of the current validation samples
		for(i = 0; i < itemsInFold; i++)
			{
			if(!validationList)
				{
				validationList= slIntNew(curr->val);
				valCurr = validationList;
				}
			else
				{
				valCurr->next = slIntNew(curr->val);
				valCurr = valCurr->next;
				}
			curr = curr->next;
			}
		//update tracking how many samples have been used
		usedSamples += itemsInFold;
		printSplit(config, data, metadata, split, fold, validationList);
		
		//if folds reports are enabled, save them to matrix
		if(hashFindVal(config, "foldsReport"))
			{
			valCurr = validationList;
			for(i = 0; i < itemsInFold; i++)
				{
				foldsReport->graph[(split-1)][valCurr->val] = fold;
				valCurr = valCurr->next;
				}
			}
		slFreeList(&validationList);
		}
	slFreeList(&shuffledList);
	}

//if saving fold reports is enabled do it now
if(hashFindVal(config, "foldsReport"))
	{
	FILE * fp = fopen(hashMustFindVal(config, "foldsReport"), "w");
	if(!fp)
		errAbort("Couldn't open file %s for writing folds report to\n", (char*)hashMustFindVal(config, "foldsReport"));
	fprint_discreteMatrix(fp, foldsReport);
	fclose(fp);
	}

//clean up
slFreeList(&list);
free_matrix(foldsReport);
}

/***** generic bouncing functions to bounce from main setup routines to specific ones ***/
void printPresplitData(struct hash * config,matrix *  data,  matrix * metadata)
{
char * outputType = hashMustFindVal(config, "outputType");
if(sameString(outputType, "flatfiles"))
    flatfilesPrintPresplitData(config, data,metadata);
else if(sameString(outputType, "SVMlight"))
	{
    SVMprintPresplitData(config, data,metadata);
	}
else if(sameString(outputType, "WEKA"))
    WEKAprintPresplitData(config, data, metadata);
else
    errAbort("Unsupported output type in config.");
}

void printSplit(struct hash * config, matrix * data, matrix * metadata,int split, int fold, struct slInt * validationList)
/*Function to bounce the printing to the right function.*/
{
char * outputType = hashMustFindVal(config, "outputType");
//if(sameString(outputType, "NMFpredictor"))
//    NMFprintSplits(config,data,metadata, split, fold, validationList);
if(sameString(outputType, "SVMlight"))
    SVMprintSplits(config, data,metadata, split, fold, validationList);
else if(sameString(outputType, "flatfiles"))
    flatfilesPrintSplits(config, data, metadata, split, fold, validationList);
else if(sameString(outputType, "WEKA"))
    WEKAprintSplits(config, data, metadata, split,fold,validationList);
else
    errAbort("Unsupported output type in config.");
}

void splitData(struct hash * config)
/*Small function to run the right splitter given the input type*/
{
char * inputType = hashMustFindVal(config, "inputType");
if(sameString(inputType, "bioInt"))
    splitDataFromBioInt(config);
else if(sameString(inputType, "flatfiles"))
    splitDataFromFile(config);
else if(sameString(inputType, "five3db"))
	splitDataFromFive3db(config);
}

void writeClusterJobs(struct hash *config)
/*Small function to bounce the cluster job writer given the right output type*/
{
char * classifier = hashMustFindVal(config, "classifier");
if(sameString(classifier, "NMFpredictor"))
    NMFwriteClusterJobs(config);
else if(sameString(classifier, "SVMlight"))
    SVMwriteClusterJobs(config);
else if(sameString(classifier, "WEKA"))
    WEKAwriteClusterJobs(config);
else if(sameString(classifier, "glmnet"))
	glmnetwriteClusterJobs(config);
}

int dataExists(struct hash *config)
/* Checks if the directories listed in the config file exists already. If so splitting won't be rerun.*/
/* Limitation of this method is that if the user changes the number of folds and reruns, this won't re-make the data.
** Consider revisiing.*/
{
char * outputType = hashMustFindVal(config, "outputType");
char * trainingDir = hashMustFindVal(config, "trainingDir");

if(!dirExists(trainingDir))
    return 0;

char targetFile[1024];
if(sameString(outputType, "flatfiles"))
    {
    safef(targetFile, sizeof(targetFile), "%s/split01/fold01/data.tab",trainingDir);
    if(!fileExists(targetFile))
        return 0;
    safef(targetFile, sizeof(targetFile), "%s/split01/fold01/metadata.tab",trainingDir);
    if(!fileExists(targetFile))
        return 0;
    }
else if(sameString(outputType, "SVMlight"))
    {
    safef(targetFile, sizeof(targetFile), "%s/split01/fold01/data.svm",trainingDir);
    if(!fileExists(targetFile))
        return 0;
    }
else if(sameString(outputType, "WEKA"))
    {
    safef(targetFile, sizeof(targetFile), "%s/split01/fold01/data.arff",trainingDir);
    if(!fileExists(targetFile))
        return 0;
    }
else
    errAbort("Unsupported outputType field.\n");
return 1;
}


void createDataDirs(struct hash * config)
{
int fold, folds = foldsCountFromConfig(config);
int split, splits = splitsCountFromConfig(config);

char * trainingDir = hashMustFindVal(config, "trainingDir");
char * validationDir = hashMustFindVal(config, "validationDir");

makeDirsOnPath(trainingDir);
makeDirsOnPath(validationDir);

char targetDir[1024];
for(split = 1; split <= splits; split++)
    {
    for(fold = 1; fold <= folds; fold++)
        {
	    safef(targetDir,sizeof(targetDir),"%s/split%02d/fold%02d", trainingDir, split, fold);
		makeDirsOnPath(targetDir);
	    safef(targetDir,sizeof(targetDir), "%s/split%02d/fold%02d", validationDir, split, fold);
		makeDirsOnPath(targetDir);
		}
    }
}

void removeDataDirs(struct hash * config)
{
int fold, folds = foldsCountFromConfig(config);
int split, splits = splitsCountFromConfig(config);

char * trainingDir = hashMustFindVal(config, "trainingDir");
char * validationDir = hashMustFindVal(config, "validationDir");
char * outputType = hashMustFindVal(config, "outputType");

char targetDir[1024];
char targetFile[1024];
for(split = 1; split <= splits; split++)
    {
    for(fold = 1; fold <= folds; fold++)
        {
		if(sameString(outputType, "flatfiles"))
			{
			safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.tab", trainingDir, split, fold);
			remove(targetFile);
            safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/metadata.tab", trainingDir, split, fold);
            remove(targetFile);
            safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.tab", validationDir, split, fold);
            remove(targetFile);
            safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/metadata.tab", validationDir, split, fold);
            remove(targetFile);
			}
		else if(sameString(outputType, "SVMlight"))
			{
            safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.svm", trainingDir, split, fold);
            remove(targetFile);
            safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.svm", validationDir, split, fold);
            remove(targetFile);
			}
		else if(sameString(outputType, "WEKA"))
			{
            safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.arff", trainingDir, split, fold);
            remove(targetFile);
            safef(targetFile, sizeof(targetFile), "%s/split%02d/fold%02d/data.arff", validationDir, split, fold);
            remove(targetFile);
			}
	
        safef(targetDir,sizeof(targetDir),"%s/split%02d/fold%02d", trainingDir, split, fold);
        if(remove(targetDir) == -1)
            errAbort("ERROR: Coudln't remove directory %s. Exiting...\n", targetDir);
        safef(targetDir,sizeof(targetDir), "%s/split%02d/fold%02d", validationDir, split, fold);
        if(remove(targetDir) == -1)
            errAbort("ERROR: Coudln't remove directory %s. Exiting...\n", targetDir);
        }
    safef(targetDir,sizeof(targetDir),"%s/split%02d", trainingDir, split);
    if(remove(targetDir) == -1)
        errAbort("ERROR: Coudln't remove directory %s. Exiting...\n", targetDir);
    safef(targetDir,sizeof(targetDir), "%s/split%02d", validationDir, split);
    if(remove(targetDir) == -1)
        errAbort("ERROR: Coudln't remove directory %s. Exiting...\n", targetDir);
    }

if(sameString(outputType, "flatfiles"))
	{
	safef(targetFile, sizeof(targetFile), "%s/data.tab", trainingDir);
	remove(targetFile);
	safef(targetFile, sizeof(targetFile), "%s/metadata.tab", trainingDir);
	remove(targetFile);
    safef(targetFile, sizeof(targetFile), "%s/data.tab", validationDir);
    remove(targetFile);
    safef(targetFile, sizeof(targetFile), "%s/metadata.tab", validationDir);
    remove(targetFile);
	}
else if(sameString(outputType, "SVMlight"))
	{
	safef(targetFile, sizeof(targetFile), "%s/data.svm", trainingDir);
	remove(targetFile);
	safef(targetFile, sizeof(targetFile), "%s/data.svm", validationDir);
	remove(targetFile);
	}
else if(sameString(outputType, "WEKA"))
	{
	safef(targetFile, sizeof(targetFile), "%s/data.arff", trainingDir);
	remove(targetFile);
	safef(targetFile, sizeof(targetFile), "%s/data.arff", validationDir);
	remove(targetFile);
	}

if(remove(trainingDir) == -1)
    errAbort("ERROR: Coudln't remove directory %s. Exiting...\n", trainingDir);
if(remove(validationDir) == -1)
    errAbort("ERROR: Coudln't remove directory %s. Exiting...\n", validationDir);
}

void createModelDirs(struct hash * config)
{
char * modelDir = hashMustFindVal(config, "modelDir");
int fold, folds = foldsCountFromConfig(config);
int split,splits = splitsCountFromConfig(config);

makeDirsOnPath(modelDir);
//hack so command line programs can write here
chmod(modelDir, S_IRWXU | S_IRWXG | S_IRWXO );

char targetDir[1024];
for(split = 1; split <= splits; split++)
	{
    safef(targetDir, sizeof(targetDir),"%s/split%02d", modelDir,split);
	makeDirsOnPath(targetDir);
	chmod(targetDir, S_IRWXU | S_IRWXG | S_IRWXO );

	for(fold = 1; fold <= folds; fold++)
		{
        safef(targetDir,sizeof(targetDir),"%s/split%02d/fold%02d", modelDir, split, fold);
		makeDirsOnPath(targetDir);
    	chmod(targetDir, S_IRWXU | S_IRWXG | S_IRWXO );
		}
	}
}

matrix * flatfilesRecreateMetadata(struct hash * config)
/*Reads the first fold directories to recostruct the metadata that was predicted on*/
{
char * trainingDir = hashMustFindVal(config, "trainingDir");
char filename[1024];
safef(filename, sizeof(filename), "%s/split01/fold01/metadata.tab", trainingDir);
FILE * fp = fopen(filename , "r");
if(!fp)
    errAbort("Couldn't open model file %s", filename);
matrix * trMetadata = f_fill_matrix(fp, 1);
fclose(fp);

char * validationDir = hashMustFindVal(config, "validationDir");
safef(filename, sizeof(filename), "%s/split01/fold01/metadata.tab", validationDir);
fp = fopen(filename , "r");
if(!fp)
    errAbort("Couldn't open model file %s", filename);
matrix * valMetadata = f_fill_matrix(fp, 1);
matrix * metadata = append_matrices(trMetadata, valMetadata, 1);

free_matrix(trMetadata);
free_matrix(valMetadata);
fclose(fp);

return metadata;
}

matrix * SVMrecreateMetadata(struct hash *config)
{
char filename[1024];

char * trainingDir = hashMustFindVal(config, "trainingDir");
safef(filename, sizeof(filename), "%s/split01/fold01/data.svm", trainingDir);
matrix * trMetadata = SVMtoMetadataMatrix(filename);

char * validationDir = hashMustFindVal(config, "validationDir");
safef(filename, sizeof(filename), "%s/split01/fold01/data.svm", validationDir);
matrix * valMetadata = SVMtoMetadataMatrix(filename);
	
matrix * metadata = append_matrices(trMetadata, valMetadata, 1);

free_matrix(trMetadata);
free_matrix(valMetadata);

return metadata;		
}

matrix * WEKArecreateMetadata(struct hash *config)
{
char filename[1024];

char * trainingDir = hashMustFindVal(config, "trainingDir");
safef(filename, sizeof(filename), "%s/split01/fold01/data.arff", trainingDir);
matrix * trMetadata = WEKAtoMetadataMatrix(filename);

char * validationDir = hashMustFindVal(config, "validationDir");
safef(filename, sizeof(filename), "%s/split01/fold01/data.arff", validationDir);
matrix * valMetadata = WEKAtoMetadataMatrix(filename);

matrix * metadata = append_matrices(trMetadata, valMetadata, 1);

free_matrix(trMetadata);
free_matrix(valMetadata);

return metadata;
}

/**************** results writing functions ****************/
struct hash *readRaFile(char *fileName)
//reads the provied raFile into a hash
{
struct hash *hashOfHash = newHash(10);
struct hashEl *helList, *hel;
struct hash *raList = NULL, *ra;

raFoldIn(fileName, hashOfHash);
helList = hashElListHash(hashOfHash);
for (hel = helList; hel != NULL; hel = hel->next)
    {
    ra = hel->val;
    slAddHead(&raList, ra);
    hel->val = NULL;
    }
hashElFreeList(&helList);
hashFree(&hashOfHash);
return raList;
}

void printTasksRa(struct hash *config, char * file, int id)
{
int taskExists = 0;
int nextId = 1;
char * task = hashMustFindVal(config, "task");

//check the task doesn't already exist
if(fileExists(file))
	{
    struct hash *raHash, *raHashList = readRaFile(file);
    for (raHash = raHashList; raHash; raHash = raHash->next)
        {
        if( sameString("task", hashMustFindVal(raHash, "type")))
			{
            if(sameString(task, hashMustFindVal(raHash, "label")))
            	taskExists = 1;
        	nextId++;
			}
        }
    freeHash(&raHashList);
	}

if(id >= 0)
    nextId = id;

//if doesn't exist, append it
if(!taskExists)
	{
	FILE * fp = fopen(file, "a");	
	if(!fp)
		errAbort("Couldn't open tasks.ra for writing. Exiting..\n");
	fprintf(fp, "name\ttask%d\n",nextId);
	fprintf(fp, "label\t%s\n", task);
	fprintf(fp, "type\ttask\n\n");
	fclose(fp);
	}
}

void printClassifiersRa(struct hash *config, char * file, int id)
{
//construct a string for the parameters
char * classifier = hashMustFindVal(config, "classifier");
char * classifierParameters = hashMustFindVal(config, "parameters");

//check this classifier doesn't already exist.
int classifierExists = 0;
int nextId = 1;
if(fileExists(file))
	{	
	struct hash *raHash, *raHashList = readRaFile(file);
	for (raHash = raHashList; raHash; raHash = raHash->next)
        {
        if( sameString("classifier", hashMustFindVal(raHash, "type")))
			{
			if(	sameString(classifier, hashMustFindVal(raHash, "label")) &&
            	sameString(classifierParameters, hashMustFindVal(raHash, "parameters")))
            	classifierExists = 1;

        	nextId++;
			}
		}
    freeHash(&raHashList);
	}

if(id >= 0)
    nextId = id;

//if doesn't exist, append to file
if(!classifierExists)
    {
	FILE * fp = fopen(file, "a");
	if(!fp)
    	errAbort("Couldn't open %s for appending. \n", file);
    fprintf(fp, "name\tclassifier%d\n",nextId);
	fprintf(fp, "label\t%s\n", classifier);
    fprintf(fp, "parameters\t%s\n", classifierParameters);
    fprintf(fp, "type\tclassifier\n\n");
	fclose(fp);
	}
}

void printSubgroupsRa(struct hash * config, char * file, int id)
{
char * inputType = hashMustFindVal(config, "inputType");
char *clinDiscretizer = hashMustFindVal(config, "clinDiscretizer");

char label[MAX_LABEL];
char dataset[MAX_LABEL];
if(sameString(inputType, "bioInt") || sameString(inputType, "five3db"))
	{
	//make  astring from the current config's subgrouping description
	char *clinField = hashMustFindVal(config, "clinField");
	safef(label, sizeof(label), "%s %s split",clinField, clinDiscretizer);
	char *tableName = hashMustFindVal(config, "tableName");
    safef(dataset, sizeof(dataset), "%s", tableName);
	}
else if(sameString(inputType, "flatfiles"))
	{
	char *metadataFilepath = hashMustFindVal(config, "metadataFilepath");
	char *c = strrchr(metadataFilepath, '/');
    safef(label, sizeof(label), "%s %s split", c, clinDiscretizer);
    char *dataFilepath = hashMustFindVal(config, "dataFilepath");
    safef(dataset, sizeof(dataset), "%s", dataFilepath);
	}
else
	errAbort("Unsupported inputType when printing subgroups\n");

char parameters[1024];
if(sameString(clinDiscretizer, "thresholds"))
	{
    char * clinDiscretizerParameters = hashMustFindVal(config, "clinDiscretizerParameters");
    struct slName *paramList = slNameListFromComma(clinDiscretizerParameters);
    safef(parameters, sizeof(parameters), "low<%s,high>%s", paramList->name, paramList->next->name);
    slFreeList(&paramList);
	}
else if(sameString(clinDiscretizer, "classes"))
    {
    char * clinDiscretizerParameters = hashMustFindVal(config, "clinDiscretizerParameters");
    struct slName *paramList = slNameListFromComma(clinDiscretizerParameters);
    safef(parameters, sizeof(parameters), "low=%s,high=%s", paramList->name, paramList->next->name);
    slFreeList(&paramList);
    }
else if(sameString(clinDiscretizer, "expressions"))
    {
    char * clinDiscretizerParameters = hashMustFindVal(config, "clinDiscretizerParameters");
    struct slName *paramList = slNameListFromComma(clinDiscretizerParameters);
    safef(parameters, sizeof(parameters), "low%s,high%s", paramList->name, paramList->next->name);
    slFreeList(&paramList);
    }
else
	safef(parameters, sizeof(parameters), "NA");
//add on the excludeList path if it exists
if(hashFindVal(config, "excludeList") != NULL)
	{
	char tmp[1024];
	safef(tmp, sizeof(tmp), "%s,excludeList=%s", parameters, (char*)hashMustFindVal(config, "excludeList"));
	safef(parameters, sizeof(parameters), "%s", tmp);
	}

//check the file to see if the subgrouping already exists.
int subgroupingExists = 0;
int nextId = 1;
if(fileExists(file))
	{
	struct hash *raHash, *raHashList = readRaFile(file);
	for (raHash = raHashList; raHash; raHash = raHash->next)
    	{
		if(sameString("subgrouping", hashMustFindVal(raHash, "type")))
			{
			if(	sameString(label, hashMustFindVal(raHash, "label")) && 
				sameString(parameters, hashMustFindVal(raHash, "parameters")) &&
				sameString(dataset, hashMustFindVal(raHash, "dataset")))
        		subgroupingExists = 1;
			nextId++;
			}
    	}
	freeHash(&raHashList);
	}	

if(id >= 0)
    nextId = id;

if(!subgroupingExists)
	{
	//recreate the metadata matrix from files.
	char * outputType = hashMustFindVal(config, "outputType");
	matrix * metadata = NULL;
	if(sameString(outputType, "flatfiles"))
		metadata = flatfilesRecreateMetadata(config);
	else if(sameString(outputType, "SVMlight"))
		metadata = SVMrecreateMetadata(config);
	else if(sameString(outputType, "WEKA"))
		metadata = WEKArecreateMetadata(config);
	else
		errAbort("Unsupported outputType field.\n");
	
	//iterate over the matrix, assigning names to lists
	struct slName * subgroup1 = NULL, *subgroup2 = NULL;
	int i;
	for(i = 0; i < metadata->cols; i++)
		{
		if(metadata->graph[0][i] == 0)
			slNameAddHead(&subgroup1, metadata->colLabels[i]);
		else if(metadata->graph[0][i] == 1)
			slNameAddHead(&subgroup2, metadata->colLabels[i]);	
		}
	slReverse(&subgroup1);
	slReverse(&subgroup2);
	
	//append to subgroups file
	FILE * fp = fopen(file, "a");
	if(!fp)
	    errAbort("Couldn't open subgroups.ra for writing. Exiting..\n");
	fprintf(fp, "name\tsubgrouping%d\n", nextId);
	fprintf(fp, "type\tsubgrouping\n");
	fprintf(fp, "label\t%s\n", label);
	fprintf(fp, "parameters\t%s\n", parameters);
	fprintf(fp, "dataset\t%s\n", dataset);
	fprintf(fp, "subgroup1label\tLow\n");
	if(subgroup1 == NULL)
		fprintf(fp, "subgroup1\t,\n");
	else
		fprintf(fp, "subgroup1\t%s,\n", slNameListToString(subgroup1, ','));
	fprintf(fp, "subgroup2label\tHigh\n");
	if(subgroup2 == NULL)
		fprintf(fp, "subgroup2\t,\n\n");
	else
		fprintf(fp, "subgroup2\t%s,\n\n", slNameListToString(subgroup2, ','));
	fclose(fp);

	slFreeList(&subgroup1);
	slFreeList(&subgroup2);
	free_matrix(metadata);
	}
}

void printFeatureSelectionsRa(struct hash *config, char * file, int id)
{
//construct a string for the parameters
char * featureSelection = hashMustFindVal(config, "featureSelection");
char * parameters = hashMustFindVal(config, "featureSelectionParameters");
char * inputType = hashMustFindVal(config, "inputType");
char dataset[MAX_LABEL];
if(sameString(inputType, "bioInt") || sameString(inputType, "five3db"))
    {
    //make  astring from the current config's subgrouping description
    char *tableName = hashMustFindVal(config, "tableName");
    safef(dataset, sizeof(dataset), "%s", tableName);
    }
else if(sameString(inputType, "flatfiles"))
    {
    char *dataFilepath = hashMustFindVal(config, "dataFilepath");
    safef(dataset, sizeof(dataset), "%s", dataFilepath);
    }
else
    errAbort("Unsupported inputType when printing subgroups\n");

//check this classifier doesn't already exist.
int featureSelectionExists = 0;
int nextId = 1;
if(fileExists(file))
    {
    struct hash *raHash, *raHashList = readRaFile(file);
    for (raHash = raHashList; raHash; raHash = raHash->next)
        {
        if( sameString("featureSelection", hashMustFindVal(raHash, "type")))
            {
            if( sameString(featureSelection, hashMustFindVal(raHash, "label")) &&
                sameString(parameters, hashMustFindVal(raHash, "parameters")) &&
				sameString(dataset, hashMustFindVal(raHash, "dataset")))
                featureSelectionExists = 1;

            nextId++;
            }
        }
    freeHash(&raHashList);
    }

if(id >= 0)
    nextId = id;

//if doesn't exist, append to file
if(!featureSelectionExists)
    {
	int featureCount = featuresCountFromConfig(config);
	
    FILE * fp = fopen(file, "a");
    if(!fp)
        errAbort("Couldn't open %s for appending. \n", file);
    fprintf(fp, "name\tfeatureSelection%d\n",nextId);
    fprintf(fp, "type\tfeatureSelection\n");
    fprintf(fp, "label\t%s\n", featureSelection);
    fprintf(fp, "parameters\t%s\n", parameters);
	fprintf(fp, "dataset\t%s\n", dataset);
	fprintf(fp, "featureCount\t%d\n\n", featureCount);
    fclose(fp);
    }
}

void printTransformationsRa(struct hash *config, char * file, int id)
{
//construct a string for the parameters
char * transformation = hashMustFindVal(config, "dataDiscretizer");
char * transformationParameters = hashMustFindVal(config, "dataDiscretizerParameters");

//check this transformation doesn't already exist.
int transformationExists = 0;
int nextId = 1;
if(fileExists(file))
    {
    struct hash *raHash, *raHashList = readRaFile(file);
    for (raHash = raHashList; raHash; raHash = raHash->next)
        {
        if( sameString("transformation", hashMustFindVal(raHash, "type")))
            {
            if( sameString(transformation, hashMustFindVal(raHash, "label")) &&
                sameString(transformationParameters, hashMustFindVal(raHash, "parameters")))
                transformationExists = 1;

            nextId++;
            }
        }
    freeHash(&raHashList);
    }

if(id >= 0)
    nextId = id;

//if doesn't exist, append to file
if(!transformationExists)
    {
    FILE * fp = fopen(file, "a");
    if(!fp)
        errAbort("Couldn't open %s for appending. \n", file);
    fprintf(fp, "name\ttransformation%d\n",nextId);
    fprintf(fp, "label\t%s\n", transformation);
    fprintf(fp, "parameters\t%s\n", transformationParameters);
    fprintf(fp, "type\ttransformation\n\n");
    fclose(fp);
    }
}

void printJobsRa(struct hash * config,char * file, int id)
/*NB: Assumes job doesn't already exist*/
{
//calculate the necessary results. If they can't be found, quit
matrix * accuracies = getSampleAccuracy(config);
if(!accuracies)
    errAbort("Job couldn't provide accuracies. Exiting...\n");
matrix * predictionScores = getPredictionScores(config);
if(!predictionScores)
    errAbort("Model couldn't provide prediction scores. Exiting..\n");
//ensure they are in the same order
if(!matchedColLabels(accuracies, predictionScores))
    {
    matrix * tmp = matchOnColLabels(predictionScores, accuracies);
    free_matrix(predictionScores);
    predictionScores = tmp;
    }
//calc the top features. This only applies to linear models
matrix * coefficients = extractTopCoefficients(config, NUM_FEATURES); //get list of top genes

int i;
char * inputType = hashMustFindVal(config, "inputType");
char * modelDir = hashMustFindVal(config, "modelDir");
char * task = hashMustFindVal(config, "task");
char * clinDiscretizer = hashMustFindVal(config, "clinDiscretizer");
char * cv = hashMustFindVal(config, "crossValidation");
char * classifierType = hashMustFindVal(config, "classifier");
char * classifierParameters = hashMustFindVal(config, "parameters");
char * featureSelection = hashMustFindVal(config, "featureSelection");
char * featureSelectionParameters = hashMustFindVal(config, "featureSelectionParameters");
char * dataDiscretizer = hashMustFindVal(config, "dataDiscretizer");
char * dataDiscretizerParameters = hashMustFindVal(config, "dataDiscretizerParameters");

//construct a string for the dataset and subgrouping parameters
char subgroupingType[MAX_LABEL];
char dataset[MAX_LABEL];
if(sameString(inputType, "bioInt") || sameString(inputType, "five3db"))
    {
    //make  a string from the current config's subgrouping description
    char *clinField = hashMustFindVal(config, "clinField");
    safef(subgroupingType, sizeof(subgroupingType), "%s %s split",clinField, clinDiscretizer);
	char * tableName = hashMustFindVal(config, "tableName");
	safef(dataset, sizeof(dataset), "%s", tableName);
    }
else if(sameString(inputType, "flatfiles"))
    {
    char *metadataFilepath = hashMustFindVal(config, "metadataFilepath");
    char *c = strrchr(metadataFilepath, '/');
    safef(subgroupingType, sizeof(subgroupingType), "%s %s split", c, clinDiscretizer);
	char * dataFilepath = hashMustFindVal(config, "dataFilepath");
	c = strrchr(dataFilepath, '/');
	safef(dataset, sizeof(dataset), "%s", c);
    }
else
    errAbort("Unsupported inputType when printing subgroups\n");

char subgroupingParameters[1024];
if(sameString(clinDiscretizer, "thresholds"))
    {
    char * clinDiscretizerParameters = hashMustFindVal(config, "clinDiscretizerParameters");
    struct slName *paramList = slNameListFromComma(clinDiscretizerParameters);
    safef(subgroupingParameters, sizeof(subgroupingParameters), "low<%s,high>%s", paramList->name , paramList->next->name);
    slFreeList(&paramList);
    }
else if(sameString(clinDiscretizer, "classes"))
    {
    char * clinDiscretizerParameters = hashMustFindVal(config, "clinDiscretizerParameters");
    struct slName *paramList = slNameListFromComma(clinDiscretizerParameters);
    safef(subgroupingParameters, sizeof(subgroupingParameters), "low=%s,high=%s", paramList->name , paramList->next->name);
    slFreeList(&paramList);
    }
else if(sameString(clinDiscretizer, "expressions"))
    {
    char * clinDiscretizerParameters = hashMustFindVal(config, "clinDiscretizerParameters");
    struct slName *paramList = slNameListFromComma(clinDiscretizerParameters);
    safef(subgroupingParameters, sizeof(subgroupingParameters), "low%s,high%s", paramList->name , paramList->next->name);
    slFreeList(&paramList);
    }
else
    safef(subgroupingParameters, sizeof(subgroupingParameters), "NA");
//add on the excludeList path if it exists
if(hashFindVal(config, "excludeList") != NULL)
    {
    char tmp[1024];
    safef(tmp, sizeof(tmp), "%s,excludeList=%s", subgroupingParameters, (char*)hashMustFindVal(config, "excludeList"));
    safef(subgroupingParameters, sizeof(subgroupingParameters), "%s", tmp);
    }


int nextId = 1;
if(id >= 0)
    nextId = id;
else
	{
	if(fileExists(file))
		{
		struct hash *raHash, *raHashList = readRaFile(file);
	    for (raHash = raHashList; raHash; raHash = raHash->next)
	        {
	        if( sameString("job", hashMustFindVal(raHash, "type")))
	            nextId++;
	        }
	    freeHash(&raHashList);
		}
	}

//write the results to file
FILE * fp = fopen(file, "a");
if(!fp)
    errAbort("Couldn't open jobs.ra for writing.\n");
fprintf(fp, "name\tjob%d\n", nextId);
fprintf(fp, "type\tjob\n");
fprintf(fp, "task\t%s\n", task);
fprintf(fp, "dataset\t%s\n", dataset);
fprintf(fp, "subgrouping\t%s\n", subgroupingType);
fprintf(fp, "subgroupingParameters\t%s\n",subgroupingParameters);
fprintf(fp, "featureSelection\t%s\n", featureSelection);
fprintf(fp, "featureSelectionParameters\t%s\n", featureSelectionParameters);
fprintf(fp, "dataDiscretizer\t%s\n", dataDiscretizer);
fprintf(fp, "dataDiscretizerParameters\t%s\n", dataDiscretizerParameters);
fprintf(fp, "classifier\t%s\n", classifierType);
fprintf(fp, "classifierParameters\t%s\n", classifierParameters);
if(sameString(cv, "k-fold"))
	{
	if(hashFindVal(config, "foldMultiplier"))
        fprintf(fp, "accuracyType\tk-fold(k=%dx%d)\n", 
			atoi(hashMustFindVal(config, "folds")), 
			atoi(hashMustFindVal(config, "foldMultiplier")));
	else
		fprintf(fp, "accuracyType\tk-fold(k=%d)\n", atoi(hashMustFindVal(config, "folds")));
	}
else if(sameString(cv, "loo"))
	fprintf(fp, "accuracyType\tLOO\n");
else
	errAbort("Unknown crossValidation value\n");
fprintf(fp, "samples\t");

//Print accuracy results
for(i = 0; i < accuracies->cols; i++)
	fprintf(fp, "%s,", accuracies->colLabels[i]);
fprintf(fp, "\n");
fprintf(fp, "trainingAccuracies\t");
for(i = 0; i < accuracies->cols; i++)
	{
	if(accuracies->graph[0][i] == NULL_FLAG)
		fprintf(fp, "NULL,");
	else
		fprintf(fp, "%.4f,", accuracies->graph[0][i]);
	}
fprintf(fp, "\n");
fprintf(fp, "testingAccuracies\t");
for(i = 0; i < accuracies->cols; i++)
    {
    if(accuracies->graph[1][i] == NULL_FLAG)
        fprintf(fp, "NULL,");
    else
        fprintf(fp, "%.4f,", accuracies->graph[1][i]);
    }

//print prediction scores
fprintf(fp, "\npredictionScores\t");
for(i = 0; i < predictionScores->cols; i++)
    {
    if(predictionScores->graph[0][i] == NULL_FLAG)
        fprintf(fp, "NULL,");
    else
        fprintf(fp, "%.4f,", predictionScores->graph[0][i]);
    }

//Print top features, if they exist
fprintf(fp, "\ntopFeatures\t");
if(coefficients)
	{
	for(i = 0; i < coefficients->rows; i++)
		{
		if(countChars(coefficients->rowLabels[i], ','))
			stripChar(coefficients->rowLabels[i], ','); //replace comma with space, to avoid blob splitting error
		if(countChars(coefficients->rowLabels[i], '\''))
			stripChar(coefficients->rowLabels[i], '\'');//remove single quotes, to avoid sql insert breakage
		fprintf(fp, "%s,", coefficients->rowLabels[i]);
		}
	}
else
	fprintf(fp, "NULL");
fprintf(fp, "\ntopFeatureWeights\t");
if(coefficients)
	{
	for(i = 0; i < coefficients->rows; i++)
		fprintf(fp, "%.6f,",coefficients->graph[i][0]);
	free_matrix(coefficients);
	}
else
	fprintf(fp, "NULL");

//print the path to where the fully trained model file should be
if(sameString(classifierType, "NMFpredictor"))
	fprintf(fp, "\nmodelPath\t%s/model.tab", modelDir); 
else if(sameString(classifierType, "SVMlight"))
	fprintf(fp, "\nmodelPath\t%s/model.svm", modelDir);
else if(sameString(classifierType, "WEKA"))
	fprintf(fp, "\nmodelPath\t%s/model.weka", modelDir);
else if(sameString(classifierType, "glmnet"))
    fprintf(fp, "\nmodelPath\t%s/model.RData", modelDir);
else
	errAbort("Unsupported outputType when printing a path to model file\n");
fprintf(fp, "\n\n");
fclose(fp);

//clean up
free_matrix(accuracies);
free_matrix(predictionScores);
}

void printBackgroundResultsRa(struct hash * config, char * file, int id)
/*NB: Assumes job doesn't already exist*/
{
int nextId = 1;
if(id >= 0)
    nextId = id;
else
    {
    if(fileExists(file))
        {
        struct hash *raHash, *raHashList = readRaFile(file);
        for (raHash = raHashList; raHash; raHash = raHash->next)
            {
            if( sameString("background", hashMustFindVal(raHash, "type")))
                nextId++;
            }
        freeHash(&raHashList);
        }
    }

//write the results to file
FILE * fp = fopen(file, "a");
if(!fp)
    errAbort("Couldn't open jobs.ra for writing.\n");
fprintf(fp, "name\tbackground%d\n", nextId);
fprintf(fp, "type\tbackground\n");

char * profile = hashMustFindVal(config, "profile");
char * db = hashMustFindVal(config, "db");
char * modelPath = hashMustFindVal(config, "modelPath");
char * dataset = hashMustFindVal(config, "dataset");

//open a connection to corroborate info found in config
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];
safef(query, sizeof(query), "SELECT id FROM jobs WHERE modelPath=\'%s\'", modelPath);
fprintf(fp, "jobs_id\t%d\n", sqlNeedQuickNum(conn, query));

fprintf(fp, "data_table\t%s\n", dataset);
matrix * predictionScores = getPredictionScores(config);
if(!predictionScores)
    errAbort("Model couldn't provide prediction scores. Exiting..\n");
//print prediction scores
fprintf(fp, "predictionScores\t");
int i;
for(i = 0; i < predictionScores->cols; i++)
    {
    if(predictionScores->graph[0][i] == NULL_FLAG)
        fprintf(fp, "NULL,");
    else
        fprintf(fp, "%.16f,", predictionScores->graph[0][i]);
    }
fprintf(fp, "\n\n");
fclose(fp);
free_matrix(predictionScores);
}

int backgroundResultsExist(struct hash * config)
/*Returns true if the expected background results files exist.*/
{
char * classifier = hashMustFindVal(config, "classifier");
char * modelDir = hashMustFindVal(config, "modelDir");
char expectedFile[1024];
if(sameString(classifier, "NMFpredictor"))
	safef(expectedFile, sizeof(expectedFile), "%s/NMFpredictor.training.results",modelDir);
else if(sameString(classifier, "SVMlight"))
	safef(expectedFile, sizeof(expectedFile), "%s/svm.training.results", modelDir);
else if(sameString(classifier, "WEKA"))
	safef(expectedFile, sizeof(expectedFile), "%s/weka.training.results",modelDir);
else if(sameString(classifier, "glmnet"))
	{
	char * parameters = hashMustFindVal(config, "parameters");
    safef(expectedFile, sizeof(expectedFile), "%s/%s.training.results",modelDir, parameters);
	}
if(!fileExists(expectedFile))
	{
	fprintf(stderr, "WARNING: Expected file %s doesn't exist. Skipping printing these results\n", expectedFile);
	return 0;
	}
return 1;
}

int resultsExist(struct hash *config)
/*Returns true if the expected resuls files exist. Avoids atomic file writing issues*/
{
//catch if this is checking for background results, as opposed to normal classifier results
char * task = hashMustFindVal(config, "task");
if(sameString(task, "backgroundCalculation"))
	return backgroundResultsExist(config);
char * classifier = hashMustFindVal(config, "classifier");
char * modelDir = hashMustFindVal(config, "modelDir");
char resultsDir[1024];
char expectedFile[1024];
int fold, folds = foldsCountFromDataDir(config);
int split, splits = splitsCountFromDataDir(config);

safef(resultsDir, sizeof(resultsDir), "%s/split01/fold01", modelDir);
if(!dirExists(resultsDir))
	return 0;

for(split = 1; split <= splits; split++)
	{
	for(fold = 1; fold <= folds; fold++)
		{
		safef(resultsDir, sizeof(resultsDir), "%s/split%02d/fold%02d", modelDir, split, fold);
		if(!dirExists(resultsDir))
			return 0;
		if(sameString(classifier, "NMFpredictor"))
			safef(expectedFile, sizeof(expectedFile), "%s/NMFpredictor.validation.results",resultsDir);
		else if(sameString(classifier, "SVMlight"))
			safef(expectedFile, sizeof(expectedFile), "%s/svm.validation.results", resultsDir);	
		else if(sameString(classifier, "WEKA"))
			safef(expectedFile, sizeof(expectedFile), "%s/weka.validation.results",resultsDir);
		else if(sameString(classifier, "glmnet"))
			{
			char * parameters = hashMustFindVal(config, "parameters");
            safef(expectedFile, sizeof(expectedFile), "%s/%s.validation.results",resultsDir, parameters);
			}
		
		if(!fileExists(expectedFile))
			{
			fprintf(stderr, "WARNING: Expected file %s doesn't exist. Skipping printing these results\n", expectedFile);
			return 0;
			}
		}
	}
return 1;
}
