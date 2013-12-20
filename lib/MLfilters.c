#include "../inc/MLmatrix.h"
#include "../inc/MLcrossValidation.h"
#include "../inc/MLreadConfig.h"
#include "../inc/MLio.h"
#include "../inc/MLfilters.h"

/*************** HELPER FUNCTIONS ***/

int compare_doubles (const void *a, const void *b){
const double *da = (const double *) a;
const double *db = (const double *) b;
return (*da > *db) - (*da < *db);
}

double calc_variance(double * array, int n)
{
if(n < 2)
	return 0;
double sum1=0, sum2=0, mean;
int i;
for(i = 0; i < n; i++)
	sum1 += array[i];
mean = sum1/n;
for(i = 0; i < n; i++)
	sum2 += (array[i] - mean)*(array[i] - mean);
return (sum2 / (n-1));
}

/************ END HELPER FUNCTIONS *****/

matrix * filterColumnsByExcludeList(struct hash * config, matrix * A_ptr)
//read list of samples to exclude from config, and remove those columns from A_ptr
{
if(!A_ptr->labels)
    errAbort("Error: Can't exclude samples from matrix that has no labels.\n");
//read the labels into an sll
char * excludeListFile = hashMustFindVal(config, "excludeList");
FILE * fp = fopen(excludeListFile, "r");
if(fp == NULL)
    errAbort("Couldn't open excludeList file %s\n", excludeListFile);
char line[A_ptr->cols * MAX_TOKEN];
fgets(line, sizeof(char) * A_ptr->cols * MAX_TOKEN, fp);
fclose(fp);
struct slName *excludeName, *excludeList = slNameListFromString(line, ',');
struct slInt *keepList = NULL;
int i, excludeFlag;
for(i = 0; i < A_ptr->cols; i++)
    {
    excludeFlag = 0;
    for(excludeName = excludeList; excludeName; excludeName = excludeName->next)
        {
        if(sameString(excludeName->name, A_ptr->colLabels[i]))
			{
            excludeFlag = 1;
			break;
			}
        }
    if(!excludeFlag)
        {
		slAddHead(&keepList,slIntNew(i));
        }
    }
slReverse(&keepList);
matrix * result =  copy_matrix_subset(A_ptr, NULL, keepList);
slFreeList(&keepList);
return result;
}

/******* FEATURE SELECTOR FUNCTIONS ****/
matrix * filterByVarianceRank(matrix * data, int n)
/*Return reduced matrix with top n features by variance on rows*/
{
struct slInt * curr = NULL, *keepList = NULL;
struct slDouble * var, * variances = NULL;
int row;

for(row = 0; row < data->rows; row++)
    {
    int numNonNAs = rowCountNonNA(data, row, NULL_FLAG);
    double * nonNAvals = rowToArray_NArm(data, row, NULL_FLAG);
    double thisVar = calc_variance(nonNAvals, numNonNAs);

	var = variances;
	curr=keepList;
    //if the new element is the largest seen yet, add it to the front of the list
	if((var == NULL) || (var->val <= thisVar))
		{
		slAddHead(&keepList, slIntNew(row));
		slAddHead(&variances, slDoubleNew(thisVar));
		}
	else
		{
		//otherwise iterate the list looking for if it fits in it. 
		while(var->next != NULL && (var->next->val > thisVar))
			{
			var = var->next;
			curr = curr->next;
			}

		struct slInt *tmp = curr->next;
		curr->next = slIntNew(row);
		curr->next->next = tmp;

		struct slDouble * tmpVar = var->next;
		var->next = slDoubleNew(thisVar);
		var->next->next = tmpVar;
		//clean up
		//slFreeList(&tmpVar);
		}

	//if the list has gotten too long, reduce it
	if(count_indices(keepList) > n)
		{
		curr = keepList;
		var = variances;
		int j = 0;
		while(curr->next->next && j < n)
			{
			curr = curr->next;
			var = var->next;
			j++;
			}
		slFreeList(&curr->next);
		slFreeList(&var->next);
		curr->next = NULL;
		var->next = NULL;
		}
	free(nonNAvals);
    }
if(keepList == NULL || count_indices(keepList) < 2)
	return NULL;
	
matrix * result = copy_matrix_subset(data, keepList, NULL);
slFreeList(&keepList);
slFreeList(&variances);
return result;
}

matrix * filterByVarianceScore(matrix * data, double minVariance)
/*Return matrix with only rows with variance greater than minVariance*/
{ 
struct slInt * curr = NULL, * keepList = NULL;
int row;
for(row = 0; row < data->rows; row++)
    {
    int n = rowCountNonNA(data, row, NULL_FLAG);
    double * nonNAvals = rowToArray_NArm(data, row, NULL_FLAG);
    double var = calc_variance(nonNAvals, n);
	if(var >= minVariance)
		{
		if(!keepList)
			{
			keepList = slIntNew(row);
			curr = keepList;
			}
		else
			{
			curr->next = slIntNew(row);
			curr = curr->next; 
			}
		}
	free(nonNAvals);
    }
if(keepList == NULL || count_indices(keepList) < 2)
	return NULL;

matrix * result =  copy_matrix_subset(data, keepList, NULL);
slFreeList(&keepList);
return result;
}

matrix * filterRowsByIncludeList(matrix * data, struct slName * includeList)
{
if(!data->labels)
    errAbort("Error: Can't include rows from matrix that has no labels.\n");
struct slInt *curr =  NULL, *keepList = NULL;
int i, includeFlag = 0;
struct slName * includeName;
for(i = 0; i < data->rows; i++)
    {
    includeFlag = 0;
    for(includeName = includeList; includeName; includeName = includeName->next)
        {
        if(sameString(includeName->name, data->rowLabels[i]))
            {
            includeFlag = 1;
            break;
            }
        }
    if(includeFlag)
        {
		if(!keepList)
			{
			keepList = slIntNew(i);
			curr = keepList;
			}
		else
			{
			curr->next = slIntNew(i);
			curr = curr->next;
			}
        }
    }
if(!keepList)
	return NULL;
matrix * result = copy_matrix_subset(data, keepList, NULL);
slFreeList(&keepList);
return result;
}

matrix * featureSelection(struct hash * config, matrix * data)
/*Function to bounce to correct feature select as per config instructions*/
{
matrix * result = NULL;
char * featureSelection = hashMustFindVal(config, "featureSelection");
if(sameString(featureSelection, "filterByVarianceRank"))
	{
	char * featureSelectionParameters = hashMustFindVal(config, "featureSelectionParameters");
	int n = atoi(featureSelectionParameters);
	result = filterByVarianceRank(data, n);
	}
else if(sameString(featureSelection, "filterByVarianceScore"))
	{
    char * featureSelectionParameters = hashMustFindVal(config, "featureSelectionParameters");
    double minVariance = atof(featureSelectionParameters);
    result = filterByVarianceScore(data, minVariance);
	}
else if(sameString(featureSelection, "filterByIncludeList"))
	{
	char * featureSelectionParameters = hashMustFindVal(config, "featureSelectionParameters");
	FILE * fp = fopen(featureSelectionParameters, "r");
	if(fp == NULL)
    	errAbort("Couldn't open feature list file %s\n", featureSelectionParameters);
	char line[data->rows * MAX_TOKEN];
	fgets(line, sizeof(char) * data->rows * MAX_TOKEN, fp);
	fclose(fp);
	struct slName *includeList = slNameListFromString(line, ',');
	result = filterRowsByIncludeList(data, includeList);
	slFreeList(&includeList);
	}
else
	{
	fprintf(stderr, "Feature selection method %s is unsupported.\n", featureSelection);
	return NULL;
	}
if(!result)
	{
	fprintf(stderr, "Feature selection couldn't be applied to data.\n");
	return NULL;
	}
return result;
}

/***** DISCRETIZER FUNCTIONS *******/
matrix * medianDiscretize(matrix * A_ptr){
	double median;
	int i, j, numVals;
	matrix * result = copy_matrix(A_ptr);
    for(i = 0; i < A_ptr->rows; i++){
		double * rowVals = rowToArray_NArm(A_ptr, i, NULL_FLAG);
		numVals = rowCountNonNA(A_ptr, i, NULL_FLAG);
		qsort(rowVals, numVals, sizeof(double), (int(*)(const void*,const void*))compare_doubles);
		if(numVals % 2  == 0)
			median = (rowVals[(int)(numVals/2)] + rowVals[(int)((numVals+1)/2)])/2;
		else
			median = rowVals[(int)((numVals)/2)];
        for(j = 0; j < A_ptr->cols; j++){
            if(A_ptr->graph[i][j] != NULL_FLAG){
				if(A_ptr->graph[i][j] < median)
                	result->graph[i][j] = 0;
            	else if(A_ptr->graph[i][j] >= median)
                	result->graph[i][j] = 1;
            	else
                	result->graph[i][j] = NULL_FLAG;
            }
        }
    }
	return result;
}

matrix * classDiscretize(matrix * A_ptr, double class1, double class2){
    int i,j;
	matrix * result = copy_matrix(A_ptr);
    for(i = 0; i < A_ptr->rows; i++){
        for(j = 0; j < A_ptr->cols; j++){
            if(A_ptr->graph[i][j] != NULL_FLAG){
                if(A_ptr->graph[i][j] == class1)
                    result->graph[i][j] = 0;
                else if(A_ptr->graph[i][j] == class2)
                    result->graph[i][j] = 1;
                else
                    result->graph[i][j] = NULL_FLAG;
            }
        }
    }
	return result;
}

matrix * expressionDiscretize(matrix * A_ptr, char * expression1, char * expression2){
	int i, j;
	matrix * result = copy_matrix(A_ptr);
	struct slName *word;
	for(i = 0; i < A_ptr->rows; i++){
		for(j = 0; j < A_ptr->cols; j++)
			result->graph[i][j] = NULL_FLAG;
	}
	struct slName *wordList = charSepToSlNames(expression1, '&');
	for(i = 0; i < A_ptr->rows; i++){
    	for(j = 0; j < A_ptr->cols; j++){
			if(A_ptr->graph[i][j] != NULL_FLAG){
				int matchesCriteria = 1;
			    for (word=wordList; word != NULL; word=word->next){
			        if(strpbrk(word->name, "!><=") != NULL){
						char * op = splitOffNonNumeric(word->name);
						double val = atof(skipToNumeric(word->name));
						if( (sameString(op, "<") && (A_ptr->graph[i][j] >= val)) ||
							(sameString(op, ">") && (A_ptr->graph[i][j] <= val)) ||
							(sameString(op, "==") && (A_ptr->graph[i][j] != val)) || 
							(sameString(op, ">=") && (A_ptr->graph[i][j] < val)) || 
                            (sameString(op, "<=") && (A_ptr->graph[i][j] > val)) ||
							(sameString(op, "!=") && (A_ptr->graph[i][j] == val))){
								{
								matchesCriteria = 0;
								break;
								}
						}
						freeMem(op);
					}else{
						fprintf(stderr, "Invalid format for expression in %s\n", word->name);
						return NULL;
					}
				}
				if(matchesCriteria)
					result->graph[i][j] = 0;
			}
		}
	}
    slFreeList(&wordList);
    wordList = charSepToSlNames(expression2, '&');
    for(i = 0; i < A_ptr->rows; i++){
        for(j = 0; j < A_ptr->cols; j++){
            if(A_ptr->graph[i][j] != NULL_FLAG){
                int matchesCriteria = 1;
                for (word=wordList; word != NULL; word=word->next){
                    if(strpbrk(word->name, "!><=") != NULL){
                        char * op = splitOffNonNumeric(word->name);
                        double val = atof(skipToNumeric(word->name));
                        if( (sameString(op, "<") && (A_ptr->graph[i][j] >= val)) ||
                            (sameString(op, ">") && (A_ptr->graph[i][j] <= val)) ||
                            (sameString(op, "==") && (A_ptr->graph[i][j] != val)) ||
                            (sameString(op, ">=") && (A_ptr->graph[i][j] < val)) ||
                            (sameString(op, "<=") && (A_ptr->graph[i][j] > val)) ||
                            (sameString(op, "!=") && (A_ptr->graph[i][j] == val))){
                                {
                                matchesCriteria = 0;
                                break;
                                }
                        }
						freeMem(op);
                    }else{
                        fprintf(stderr, "Invalid format for expression in %s\n", word->name);
						return NULL;
                    }
				}
                if(matchesCriteria)
            		result->graph[i][j] = 1;
            }
        }
    }
	//clean up
	slFreeList(&wordList);
	return result;
}

matrix * thresholdDiscretize(matrix * A_ptr, double low_threshold, double hi_threshold){
	int i,j;
	matrix * result = copy_matrix(A_ptr);
	for(i = 0; i < A_ptr->rows; i++){
		for(j = 0; j < A_ptr->cols; j++){
			if(A_ptr->graph[i][j] != NULL_FLAG){
				if(A_ptr->graph[i][j] <= low_threshold)
					result->graph[i][j] = 0;
				else if(A_ptr->graph[i][j] >= hi_threshold)
					result->graph[i][j] = 1;
				else
					result->graph[i][j] = NULL_FLAG;
			}
		}
	}
	return result;
}

matrix * percentDiscretize(matrix * A_ptr, double percent){
	matrix * result = copy_matrix(A_ptr);
    double *rowVals = NULL, *thresholds;
    int i,j,x, numVals;
	int numSplits = floor(100/percent);
	thresholds = (double *)malloc(numSplits*sizeof(double));
    for(i = 0; i < A_ptr->rows; i++){
       	rowVals = rowToArray_NArm(A_ptr, i, NULL_FLAG);
		numVals = rowCountNonNA(A_ptr, i, NULL_FLAG);
        qsort(rowVals, numVals, sizeof(double), (int(*)(const void*,const void*))compare_doubles);
		for(x = 0; x < numSplits; x++){
			thresholds[x] = rowVals[(numVals/numSplits)*(x+1)];	
		}
        free(rowVals);
        for(j = 0; j < A_ptr->cols; j++){
            if(A_ptr->graph[i][j] != NULL_FLAG){
				if(A_ptr->graph[i][j] > thresholds[numSplits])
					result->graph[i][j] = numSplits+1;
				else{
					for(x = 0; x < numSplits; x++){
						if(A_ptr->graph[i][j] <= thresholds[x])
							result->graph[i][j] = (x+1);
					}	
				}
            }
        }
    }
	free(thresholds);
	return result;
}

matrix * quartileDiscretize(matrix * A_ptr){
	matrix * result = copy_matrix(A_ptr);
	double *rowVals = NULL, upperQ, lowerQ;
    int i,j,numVals;

    for(i = 0; i < A_ptr->rows; i++){
        rowVals = rowToArray_NArm(A_ptr, i, NULL_FLAG);
        numVals = rowCountNonNA(A_ptr, i, NULL_FLAG);
        qsort(rowVals, numVals, sizeof(double), (int(*)(const void*,const void*))compare_doubles);
		upperQ = rowVals[(int)(numVals*3/4)];
		lowerQ = rowVals[(int)(numVals/4)];
        free(rowVals);
        for(j = 0; j < A_ptr->cols; j++){
            if(A_ptr->graph[i][j] != NULL_FLAG){
                if(A_ptr->graph[i][j] < lowerQ)
					result->graph[i][j] = 0;
                else if(A_ptr->graph[i][j] > upperQ)
                    result->graph[i][j] = 1;
				else
					result->graph[i][j] = NULL_FLAG;
            }
        }
    }
	return result;
}

matrix * signDiscretize(matrix * A_ptr){
	matrix * result = copy_matrix(A_ptr);
    int i,j;
    for(i = 0; i < A_ptr->rows; i++){
        for(j = 0; j < A_ptr->cols; j++){
            if(A_ptr->graph[i][j] != NULL_FLAG){
                if(A_ptr->graph[i][j] < 0)
                    result->graph[i][j] = 1;
				else if(A_ptr->graph[i][j] == 0)
					result->graph[i][j] = 2;
				else
                    result->graph[i][j] = 3;
            }
        }
    }
	return result;
}


matrix * discretizeData(struct hash * config, matrix * data)
/*Bouncing function to forward discretizer to the right function*/
{
matrix * result = NULL;
char * dataDiscretizer = hashMustFindVal(config, "dataDiscretizer");
if(sameString(dataDiscretizer, "median"))
    result = medianDiscretize(data);
else if(sameString(dataDiscretizer, "sign"))
    result = signDiscretize(data);
else if(sameString(dataDiscretizer, "quartile"))
    result = quartileDiscretize(data);
else if(sameString(dataDiscretizer, "percent"))
    {
	char * dataDiscretizerParameters = hashMustFindVal(config, "dataDiscretizerParameters");
    double dataPercentThreshold = atof(dataDiscretizerParameters);
    result = percentDiscretize(data, dataPercentThreshold);
    }
else{
   	fprintf(stderr, "Warning: dataDiscretizer value %s not defined or not supported.\n", dataDiscretizer);
	return NULL;
	}
if(!result)
	{
	fprintf(stderr, "Warning: Couldn't apply discretization to a matrix.\n");
	return NULL;
	}
return result;
}

matrix * discretizeMetadata(struct hash *config, matrix * metadata)
/*Bouncing function to forward discretizer to the right function*/
{
matrix * result = NULL;
char *clinDiscretizer = hashFindVal(config, "clinDiscretizer");
if(sameString(clinDiscretizer, "median"))
    result = medianDiscretize(metadata);
else if(sameString(clinDiscretizer, "sign"))
    result = signDiscretize(metadata);
else if(sameString(clinDiscretizer, "quartile"))
    result = quartileDiscretize(metadata);
else if(sameString(clinDiscretizer, "percent"))
    {
    double clinPercentThreshold = atof(hashMustFindVal(config, "clinDiscretizerParameters"));
    result = percentDiscretize(metadata, clinPercentThreshold);
    }
else if(sameString(clinDiscretizer, "thresholds"))
    {
	char * clinDiscretizerParameters = hashMustFindVal(config, "clinDiscretizerParameters");
	struct slName *param, *paramList = slNameListFromComma(clinDiscretizerParameters);
	param = paramList;
    double clinHiThreshold = atof(param->name);
	param = param->next;
    double clinLowThreshold = atof(param->name);
    result = thresholdDiscretize(metadata, clinLowThreshold, clinHiThreshold);
	//clean up
	slFreeList(&paramList);
    }
else if(sameString(clinDiscretizer, "classes"))
    {
    char * clinDiscretizerParameters = hashMustFindVal(config, "clinDiscretizerParameters");
    struct slName *param, *paramList = slNameListFromComma(clinDiscretizerParameters);
    param = paramList;
    int class1 = atoi(param->name);
    param = param->next;
    int class2 = atoi(param->name);;
    result = classDiscretize(metadata, class1, class2);
	//clean up
	slFreeList(&paramList);
    }
else if(sameString(clinDiscretizer, "expressions"))
    {
    char * clinDiscretizerParameters = hashMustFindVal(config, "clinDiscretizerParameters");
    struct slName *param, *paramList = slNameListFromComma(clinDiscretizerParameters);
    param = paramList;
    char * class1 = cloneString(param->name);
    param = param->next;
    char * class2 = cloneString(param->name);;
    result = expressionDiscretize(metadata, class1, class2);
	//clean up
	freeMem(class1);
	freeMem(class2);
    slFreeList(&paramList);
    }
else
	{
   	fprintf(stderr, "Warning: clinDiscretizer value %s not defined or not supported.", clinDiscretizer);
	return NULL;
	}
if(!result)
	{
	fprintf(stderr, "ERROR: Couldn't apply discretization to a matrix. Exiting...\n");
	return NULL;
	}
return result;
}
