#include "../inc/MLmatrix.h"
#include "../inc/MLcrossValidation.h"
#include "../inc/MLreadConfig.h"
#include "../inc/MLextractCoefficients.h"
#include "../inc/MLio.h"
#include "../inc/MLfilters.h"
#include "portable.h"

struct slInt * seeded_random_indices(int max, double proportion, int seed)
{
int i;
int numToPick = floor(max*proportion);
struct slInt *curr = NULL, *list = NULL, *shuffledList = NULL;
for(i = 0; i < max; i++)
	{
	if(!list)
		{
		list = slIntNew(i);
		curr= list;
		}
	else
		{
		curr->next = slIntNew(i);
		curr = curr->next;
		}
	}
shuffledList = seeded_shuffle_indices(list, seed);
slFreeList(&list);
curr = shuffledList;
for(i = 0; i < (numToPick-1); i++)
    curr = curr->next;
slFreeList(curr->next);
curr->next =NULL;
return shuffledList;
}

struct slInt * random_indices(int max, double proportion)
{
int i;
int numToPick = floor(max*proportion);
struct slInt *curr = NULL, *list = NULL, *shuffledList = NULL;
for(i = 0; i < max; i++)
    {
    if(!list)
        {
        list=slIntNew(i);
        curr= list;
        }
    else
        {
        curr->next=slIntNew(i);
        curr = curr->next;
        }
    }
shuffledList = shuffle_indices(list);
slFreeList(&list);
curr = shuffledList;
for(i = 0; i < (numToPick-1); i++)
    curr = curr->next;
slFreeList(curr->next);
curr->next =NULL;
return shuffledList;
}

struct slInt * index_complement(int max, struct slInt * list)
{
struct slInt * listCurr = list;
struct slInt * result = NULL;
struct slInt * resultCurr = NULL;
int i = 0;

//init complementary array as all complementary
int complementary[max];
for(i = 0; i < max; i++)
	complementary[i] = 1;

//set flags in complementary array where list contains the index
while(listCurr != NULL)
	{
	complementary[listCurr->val] = 0;	
	listCurr=listCurr->next;
	}

//iterate over complementary array, creating list from items flagged as complements
for(i = 0; i < max; i++)
	{
	if(complementary[i])
		{
		if(!result)
			{
			result=slIntNew(i);
			resultCurr = result;
			}
		else
			{
			resultCurr->next = slIntNew(i);
			resultCurr = resultCurr->next;
			}
		}
	}
return result;
}

int indexInList(int index, struct slInt * indexList)
{
if(slIntFind(indexList, index))
	return 1;
return 0;
}


int count_indices(struct slInt * list)
{
return slCount(list);
}

int max_index(struct slInt * list)
{
struct slInt * curr = list;
int result = -1;
while(curr)
    {
	if(curr->val > result)
		result = curr->val;
    curr=curr->next;
    }
return result;
}


void fprint_indices(FILE *fp, struct slInt *positions)
{
struct slInt * curr = positions;
while(curr!=NULL)
	{
	fprintf(fp,"%d\t",curr->val);
	curr=curr->next;
	}
fprintf(fp,"\n");
}

struct slInt * list_indices(int n)
{
struct slInt * result = NULL, * curr = NULL;
int i;
for(i = 0; i < n; i++)
	{
	if(!result)
		{
		result = slIntNew(i);
		curr=result;
		}
	else
		{
		curr->next = slIntNew(i);
		curr = curr->next;
		}
	}
return result;
}	

struct slInt * copy_list(struct slInt * list)
{
struct slInt * result = NULL;
struct slInt * curr = NULL;
struct slInt * listCurr = list;
while(listCurr)
	{
    if(!result)
        {
		result = slIntNew(listCurr->val);
        curr = result;
        }
	else
		{
		curr->next = slIntNew(listCurr->val);
		curr = curr->next;
		}
	listCurr = listCurr->next;
	}
return result;
}

struct slInt * shuffle_indices(struct slInt *list)
{
int length = 0, currIx = 0, swapIx = 0, i = 0,j=0;
struct slInt * tmp, * currPrev,  * curr, *swapPrev, *swap, *result = copy_list(list);
length=count_indices(result);
srand(time(NULL));
for(currIx = 0; currIx < length; currIx++)
    {
    currPrev = swapPrev = NULL;
    curr = swap = result;

    //iterate a pointer to the node to be swapped
    for(i = 0; i < (currIx-1); i++)
        {
        currPrev = curr;
        curr = curr->next;
        }

    //pick an index to swap this node with and iterate a pointer to the node before it
    swapIx = (rand() % length + 1);
    for(j = 0; j < (swapIx - 1); j++)
        {
        swapPrev = swap;
        swap = swap->next;
        }

    //swap nodes
    if(currPrev)
        currPrev->next = swap;
    else
        result = swap;
    if(swapPrev)
        swapPrev->next = curr;
    else
        result = curr;
    tmp = curr->next;
    curr->next = swap->next;
    swap->next = tmp;
    }
return result;
}

struct slInt * seeded_shuffle_indices(struct slInt *list, int seed)
{
int length = 0, currIx = 0, swapIx = 0, i = 0,j=0;
struct slInt * tmp, * currPrev,  * curr, *swapPrev, *swap, *result = copy_list(list);
length=count_indices(result);
srand(seed);
for(currIx = 0; currIx < length; currIx++)
	{
	currPrev = swapPrev = NULL;
	curr = swap = result;

	//iterate a pointer to the node to be swapped
	for(i = 0; i < (currIx-1); i++)
		{
		currPrev = curr;
		curr = curr->next;
		}
		
	//pick an index to swap this node with and iterate a pointer to the node before it
    swapIx = (rand() % length + 1);
	for(j = 0; j < (swapIx - 1); j++)
		{
		swapPrev = swap;	
		swap = swap->next;
		}

	//swap nodes
	if(currPrev)
		currPrev->next = swap;
	else
		result = swap;
	if(swapPrev)
		swapPrev->next = curr;
	else
		result = curr;
	tmp = curr->next;
	curr->next = swap->next;
	swap->next = tmp;
	}	
return result;
}

struct slInt * orderIndicesByDescription(struct hash * config, matrix * data, int split)
{
matrix * fd = foldsDescriptionFromConfig(config);
matrix * orderedFd = matchOnColLabels(fd, data);
int fold, folds = foldsCountFromFoldsDescription(config);
if(orderedFd->cols != data->cols)
	errAbort("ERROR: Columns differ for data and foldDescription file. Unsupported behaviour.\n");
struct slInt *curr = NULL, *result = NULL;
for(fold = 1; fold <= folds; fold++)
	{	
	int j;
	for(j = 0; j < orderedFd->cols; j++)
		{
		if(orderedFd->graph[(split-1)][j] == NULL_FLAG)
			errAbort("ERROR: fold description didn't include an entry for sample %s in split %d", orderedFd->colLabels[j], split);
		if(orderedFd->graph[(split-1)][j] == fold)
			{
			if(!result)
        		{
				result = slIntNew(j);
				curr = result;
				}
			else
				{
        		curr->next = slIntNew(j);
        		curr = curr->next;
        		}
    		}
		}
	}
return result;
}

matrix * copy_matrix_subset(matrix *m, struct slInt * desiredRows, struct slInt * desiredCols)
{
int i = 0, j = 0;
//if desiredRows or desiredCols is NULL that means grab them all.
struct slInt *rows = NULL, *cols = NULL;
if(desiredRows == NULL)
	rows = list_indices(m->rows);
else
	rows = desiredRows;
if(desiredCols == NULL)
	cols = list_indices(m->cols);
else
	cols = desiredCols;
struct slInt * curr_row = rows;
struct slInt * curr_col = cols;
int subsetRows = count_indices(rows);
int subsetCols = count_indices(cols);
matrix * result = init_matrix(subsetRows, subsetCols);

//if the source matrix has labels, make space for them in the result matrix
if(m->labels)
	{
	//copy labels	
	i = 0;
	while(curr_row != NULL)
		{
		safef(result->rowLabels[i++],MAX_LABEL, "%s", m->rowLabels[curr_row->val]);
		curr_row = curr_row->next;
		}
	curr_row = rows;
	i = 0;
	while(curr_col != NULL)
		{
        safef(result->colLabels[i++],MAX_LABEL,"%s", m->colLabels[curr_col->val]);
		curr_col = curr_col->next;
		}
	curr_col = cols;
	result->labels=1;
	}
//run over your selected rows and cols, copying from source matrix
i=0;i=0;
while(curr_row != NULL)
	{
	while(curr_col != NULL)
		{
		result->graph[i][j++] = m->graph[curr_row->val][curr_col->val];
		curr_col = curr_col->next;
		}
	i++;
	j = 0;
	curr_col = cols;
	curr_row = curr_row->next;
	}
if(desiredRows == NULL)
	slFreeList(&rows);
if(desiredCols == NULL)
	slFreeList(&cols);
return result;
}

int * copy_array_subset(struct slInt *positions, int * array)
{
int *result = malloc(count_indices(positions)*sizeof(int));
struct slInt * curr = positions;
int i = 0;
if(result == NULL)
	errAbort("ERROR: Coudldn't assign memory to array subset. Exiting..\n");
while(curr != NULL)
	{
	result[i++] = array[curr->val];
	curr=curr->next;
	}
return result;
}

struct slInt * listTopN(matrix * A_ptr, int n, int direction, int ix)
/*Make a list of the top features in a row or column of matrix. Direction: 1=rows, 2=cols*/
{
//make an empty list, and a pointer to it.
struct slInt *list = NULL, *curr = NULL;
int i, j;

if(direction == 1)
	{
	//start the list by adding the first element (non null) in matrix
	j = 0;
	while(A_ptr->graph[j][ix] == NULL_FLAG && j < A_ptr->rows)
		j++;
    list=slIntNew(j);
	curr = list;

	for(i = (j+1); i < A_ptr->rows; i++)
		{
		curr = list;
		//if the new element is the largest seen yet, add it to the front of the list
		if(A_ptr->graph[i][ix] != NULL_FLAG  && fabs(A_ptr->graph[i][ix]) > fabs(A_ptr->graph[curr->val][ix]))
			{
			slAddHead(&list, slIntNew(i));
			}
		else
			{
			//otherwise iterate the list looking for if it fits in it. 
			while(curr->next != NULL && (A_ptr->graph[i][ix] != NULL_FLAG && fabs(A_ptr->graph[i][ix]) < fabs(A_ptr->graph[curr->next->val][ix])))
				curr = curr->next;

			struct slInt *tmp = curr->next;
			curr->next = slIntNew(i);
			curr->next->next = tmp;
			}

		//if the list has gotten too long, reduce it
		if(count_indices(list) > n)
			{
			curr = list;
			j = 0;
			while(curr->next->next && j < n)
				{
				curr = curr->next;
				j++;
				}
			slFreeList(&curr->next);
			curr->next = NULL;
			}
		}
	}
else if(direction == 2)
	{
    //start the list by adding the first element in matrix
    j = 0;
    while(A_ptr->graph[ix][j] == NULL_FLAG && j < A_ptr->rows)
        j++;
    curr->val = j;

    for(i = (j+1); i < A_ptr->cols; i++)
        {
        curr = list;
        //if the new element is the largest seen yet, add it to the front of the list
        if(A_ptr->graph[ix][i] != NULL_FLAG  && fabs(A_ptr->graph[ix][i]) > fabs(A_ptr->graph[ix][curr->val]))
            {
			slAddHead(&list, slIntNew(i));
            }
        else
            {
            //otherwise iterate the list looking for if it fits in it. 
            while(curr->next != NULL && (A_ptr->graph[ix][i] != NULL_FLAG && fabs(A_ptr->graph[ix][i]) < fabs(A_ptr->graph[ix][curr->next->val])))
                curr = curr->next;

            struct slInt *tmp = curr->next;
            curr->next = slIntNew(i);
            curr->next->next = tmp;
            }

        //if the list has gotten too long, reduce it
        if(count_indices(list) > n)
            {
            curr = list;
            j = 0;
            while(curr->next->next && j < n)
                {
                curr = curr->next;
                j++;
                }
            slFreeList(&curr->next);
            curr->next = NULL;
            }
        }
	}
else
	errAbort("When listing top N an invalid direction was supplied.\n");
return list;
}


/******* Generic correlation functions *******/
double mean(double *a, int n)
{
int i;
double sum = 0;
for(i = 0; i < n; i++)
	sum += a[i];
return (sum/n);
}

double sd(double * a, double mu, int n)
{
int i;
double sum = 0, dev = 0;
for(i = 0; i < n; i++)
	{
	dev = (mu - a[i]);
	sum += (dev*dev);
	}
sum = sqrt(sum/(n-1));
return sum;
}

double mean_NArm(double *a, int n, double null_val)
{
int i;
double sum = 0, count = 0;
for(i = 0; i < n; i++){
	if(a[i] != null_val)
		{
		sum += a[i];
		count++;
		}
	}
return (sum/count);
}

double sd_NArm(double * a, double mu, int n, double null_val)
{
int i;
double sum = 0, dev = 0, count = 0;
for(i = 0; i < n; i++)
	{
	if(a[i] != null_val)
		{
		dev = (mu - a[i]);
		sum += (dev*dev);
		count++;
		}
	}
return sqrt(sum/(count-1));
}

double pearson(double *a, double *b, int n){
        double sumX = 0;
        double sumY = 0;
        double sumXsq = 0;
        double sumYsq = 0;
        double sumXY = 0;
        double r = 0;
        int i = 0;
        for(i = 0; i < n; i++){
        	sumX += a[i];
        	sumY += b[i];
        	sumXsq += a[i]*a[i];
        	sumYsq += b[i]*b[i];
        	sumXY += a[i]*b[i];
        }

        r = ( n*sumXY - sumX*sumY )  /  ( sqrt(n*sumXsq - sumX*sumX)  *   sqrt(n*sumYsq - sumY*sumY) );
        return r;
}

double pearson_NArm(double *a, double *b, int n, double null_val){
	double sumX = 0;
	double sumY = 0;
	double sumXsq = 0;
	double sumYsq = 0;
	double sumXY = 0;
	double count = 0;	
	double r = 0;
	int i = 0;
	for(i = 0; i < n; i++){
		if(a[i] != null_val && b[i] != null_val){
			sumX += a[i];
			sumY += b[i];
			sumXsq += a[i]*a[i];
			sumYsq += b[i]*b[i];
			sumXY += a[i]*b[i];
			count++;
		}
	}
	
	r = ( count*sumXY - sumX*sumY )  /  ( sqrt(count*sumXsq - sumX*sumX)  *   sqrt(count*sumYsq - sumY*sumY) );
	return r;
}

double dotProd(double *a, double *b, int n){
    double sumXsq = 0;
    double sumYsq = 0;
    double sumXY = 0;
    double r = 0;
    int i = 0;
    for(i = 0; i < n; i++){
        sumXsq += a[i]*a[i];
        sumYsq += b[i]*b[i];
        sumXY += a[i]*b[i];
    }
    if(sumYsq == 0 || sumXsq == 0)
        return NULL_FLAG;
    r = ( sumXY )  /  ( sqrt(sumXsq) * sqrt(sumYsq) );
    return r;
}

double dotProd_NArm(double *a, double *b, int n, double null_val){
    double sumXsq = 0;
    double sumYsq = 0;
    double sumXY = 0;
    double r = 0;
    int i = 0;
	
    for(i = 0; i < n; i++){
        if(a[i] != null_val && b[i] != null_val){
            sumXsq += a[i]*a[i];
            sumYsq += b[i]*b[i];
            sumXY += a[i]*b[i];
        }
    }
	if(sumYsq == 0 || sumXsq == 0)
		return NULL_FLAG;
    r = ( sumXY )  /  ( sqrt(sumXsq) * sqrt(sumYsq) );
    return r;
}


double euclidean_NArm(double *a, double *b, int n, double null_val){
	int i = 0;
	double sum = 0;
	for(i = 0; i < n; i++){
                if(a[i] != null_val && b[i] != null_val)
			sum += (a[i] - b[i])*(a[i] - b[i]);
	}	
	return sqrt(sum);
}

/**** matrix correlation functions *******/

/*populate the correlation matrix*/
matrix * populateCorrMatrix(matrix *V, matrix *W){
    matrix * correlations = init_matrix(W->cols, V->cols); 
    double a[V->rows];
    double b[V->rows];
    int x, y, i;

    //find correlations for each sample against each class
    for(x = 0; x < correlations->cols; x++){ //for each sample
            for(y = 0; y < correlations->rows; y++){ //for each class
                    for(i = 0; i < W->rows; i++){
                            a[i] = W->graph[i][y]; //set a as the predictor column
                            b[i] = V->graph[i][x]; //set b as the sample's column
                    }
                    correlations->graph[y][x] = dotProd(a,b,V->rows);
            }
    }
	return correlations;
}

matrix * populateCorrMatrix_NArm(matrix *V, matrix *W, double null_val){
        matrix * correlations = init_matrix(W->cols, V->cols); //classes by samples
        double a[V->rows];
        double b[V->rows];
        int x, y, i;
	
        //find correlations for each sample against each class
        for(x = 0; x < correlations->cols; x++){ //for each sample
                for(y = 0; y < correlations->rows; y++){ //for each class
                        for(i = 0; i < W->rows; i++){
                                a[i] = W->graph[i][y]; //set a as the predictor column
                                b[i] = V->graph[i][x]; //set b as the sample's column
                        }
                        correlations->graph[y][x] = dotProd_NArm(a,b,V->rows, null_val);
                }
        }
        return correlations;
}

double avgTrainingAccuracy(matrix * accuracies){
    int i, count = 0;
    double sum = 0;
    for(i = 0; i < accuracies->cols; i++){
        if(accuracies->graph[0][i] != NULL_FLAG){
            sum += accuracies->graph[0][i];
            count++;
        }
    }
    if(count == 0)
        return 0;
    return (sum/count);
}

double avgTestingAccuracy(matrix * accuracies){
    int i, count= 0;
    double sum = 0;
    for(i = 0; i < accuracies->cols; i++){
        if(accuracies->graph[1][i] != NULL_FLAG){
            sum+= accuracies->graph[1][i];
            count++;
        }
    }
    if(count == 0)
        return 0;
    return (sum/count);
}

/******* NMFspecific functions *******/

/* Populates a matrix with training and testing accuracies. Assumes disjoint folds*/
matrix * NMFpopulateAccuracyMatrix(matrix * predictions, matrix *metadata, struct slInt * trainingList)
{
int i, j;

//make a place to store results
matrix * result = init_matrix(2, metadata->cols);
safef(result->rowLabels[0], MAX_LABEL, "trainingAccuracies");
safef(result->rowLabels[1], MAX_LABEL, "testingAccuracies");
for(i = 0; i < metadata->cols; i++)
    safef(result->colLabels[i], MAX_LABEL, "%s", metadata->colLabels[i]);
result->labels=1;

//iterate over each sample, recording accuacies
struct slInt * testList = index_complement(metadata->cols,trainingList);
for(j = 0; j < predictions->cols; j++)
	{
    int resultRow = -1;
    if(indexInList(j, trainingList))
        resultRow = 0;
    else if(indexInList(j, testList))
        resultRow = 1;

    if(metadata->graph[0][j] == NULL_FLAG)
        result->graph[resultRow][j] = NULL_FLAG; // O_o
    else if(metadata->graph[0][j] == predictions->graph[0][j])
        result->graph[resultRow][j] = 1; //yay!
    else
        result->graph[resultRow][j] = 0; //boo.
	}
return result;
}

matrix * NMFgetSampleAccuracy(struct hash *config)
/*Read all the folds and calculate training and testing accuracies from best models*/
{
char * trainingDir = hashMustFindVal(config, "trainingDir");
char * validationDir = hashMustFindVal(config, "validationDir");
char * modelDir = hashMustFindVal(config, "modelDir");
int fold, folds = foldsCountFromDataDir(config);
int split, splits  = splitsCountFromDataDir(config);

matrix * accuracies = NULL;
char filename[256];
FILE * fp;
for(split = 1; split <= splits; split++)
    {
    for(fold = 1; fold <= folds; fold++)
        {
        //cat togetehr the training and validation KH values and record which were used to train
        safef(filename, sizeof(filename), "%s/split%02d/fold%02d/metadata.tab", trainingDir, split, fold);
        fp = fopen(filename, "r");
        if(fp == NULL)
            errAbort("Couldn't open file %s\n", filename);
        matrix * trMetadata = f_fill_matrix(fp, 1);
        fclose(fp);

        safef(filename, sizeof(filename), "%s/split%02d/fold%02d/metadata.tab", validationDir, split, fold);
        fp = fopen(filename, "r");
        if(fp == NULL)
            errAbort("Couldn't open file %s\n", filename);
        matrix * valMetadata = f_fill_matrix(fp, 1);
        fclose(fp);

        struct slInt * trainingList = list_indices(trMetadata->cols);
        matrix * metadata = append_matrices(trMetadata, valMetadata, 1);

        safef(filename, sizeof(filename), "%s/split%02d/fold%02d/NMFpredictor.training.results", modelDir, split, fold);
        fp = fopen(filename , "r");
        if(!fp)
            errAbort("Couldn't open training results file %s", filename);
        matrix * trainingPred = f_fill_matrix(fp, 1);
        fclose(fp);

        safef(filename, sizeof(filename), "%s/split%02d/fold%02d/NMFpredictor.validation.results", modelDir, split, fold);
        fp = fopen(filename , "r");
        if(!fp)
            errAbort("Couldn't open validation results file %s", filename);
        matrix * valPred = f_fill_matrix(fp, 1);
        fclose(fp);

        //calc the accuracy by sample
        matrix * predictions = append_matrices(trainingPred, valPred, 1);
        matrix * accuraciesInFold = NMFpopulateAccuracyMatrix(predictions, metadata, trainingList);
        //add the accuracies to the running totals
        if(split == 1 && fold == 1)
            accuracies = copy_matrix(accuraciesInFold);
        else
            add_matrices_by_colLabel(accuracies, accuraciesInFold);
		//clean up
        free_matrix(trainingPred);
        free_matrix(valPred);
        free_matrix(predictions);
        free_matrix(trMetadata);
        free_matrix(valMetadata);
        free_matrix(metadata);
        free_matrix(accuraciesInFold);
        }
    }
//normalize accuracies over number of splits and folds
int i;
for(i = 0; i < accuracies->cols; i++)
    {
    if(accuracies->graph[0][i] != NULL_FLAG)
        accuracies->graph[0][i] = (accuracies->graph[0][i] / ((folds-1) * splits));
    if(accuracies->graph[1][i] != NULL_FLAG)
        accuracies->graph[1][i] = (accuracies->graph[1][i] / (1 * splits));
    }
return accuracies;
}


matrix * NMFpopulateClassPredictionsMatrix(matrix * data, matrix * model)
/*Returns predictions for each column in data*/
{
//create a one-row matrix to hold predicted class ixs
matrix * result = init_matrix(1, data->cols);
safef(result->rowLabels[0], MAX_LABEL, "prediction");
copy_matrix_labels(result, data, 2,2);
result->labels=1;

//make sure data is positive
shift_matrix(data);

//calc correlations for each class, then assign each sample to highest correlated class
matrix * correlations = populateCorrMatrix_NArm(data, model, NULL_FLAG);
float max;
int maxIx, i, j;
for(j = 0; j < correlations->cols; j++)
    {
    maxIx = 0;
    max = correlations->graph[0][j];
    for(i = 1; i < correlations->rows; i++)
        {
        if(correlations->graph[i][j] > max)
            {
            max = correlations->graph[i][j];
            maxIx = i;
            }
        }
    result->graph[0][j] = maxIx;
    }
return result;
}

matrix * NMFpopulatePredictionsMatrix(char * resultsFile)
/*Gives a coefficient per sample of prediction strength - ONLY FOR BINARY CLASSIFIERS*/
{
FILE * fp = fopen(resultsFile, "r");
if(!fp)
	errAbort("Couldnt open results file %s for reading\n", resultsFile);
matrix * tmp = f_fill_matrix(fp, 1);
fclose(fp);

if(tmp->rows != 2)
	errAbort("ERROR: Prediction population fof NMF currently only supports 2 classes.\n");

if(!tmp->labels)
    errAbort("Trying to obtain prediction values from a matrix with no labels.");

matrix * result = init_matrix(1, tmp->cols);
int i;
for(i = 0; i < tmp->cols; i++)
    {
	if(tmp->graph[0][i] == 0)
		result->graph[0][i] = log2(tmp->graph[1][i] / MIN_RANDOM_VAL); //catch divide by zero error
    else if(tmp->graph[0][i] != NULL_FLAG && tmp->graph[1][i] != NULL_FLAG)
		result->graph[0][i] = log2(tmp->graph[1][i] / tmp->graph[0][i]);
    else
        result->graph[0][i] = NULL_FLAG;
    }
safef(result->rowLabels[0], MAX_LABEL, "Prediction");
copy_matrix_labels(result, tmp, 2,2);
result->labels=1;
free_matrix(tmp);
return result;
}

/************* SVM specific functions **********/
matrix * SVMpopulateAccuracyMatrix(matrix * predictions, matrix * metadata, struct slInt * trainingList)
{
int i, j;

//make a place to store results
matrix * result = init_matrix(2, metadata->cols);
safef(result->rowLabels[0], MAX_LABEL, "trainingAccuracies");
safef(result->rowLabels[1], MAX_LABEL, "testingAccuracies");
for(i = 0; i < metadata->cols; i++)
    safef(result->colLabels[i], MAX_LABEL, "%s", metadata->colLabels[i]);
result->labels=1;

//iterate over each sample, recording accuacies
struct slInt * testList = index_complement(metadata->cols,trainingList);
for(j = 0; j < predictions->cols; j++)
	{
    int resultRow = -1;
    if(indexInList(j, trainingList))
        resultRow = 0;
    else if(indexInList(j, testList))
        resultRow = 1;

	if(metadata->graph[0][j] == NULL_FLAG)
        result->graph[resultRow][j] = NULL_FLAG; // O_o
    else if((metadata->graph[0][j] == 0 && predictions->graph[0][j] < 0) || 
			(metadata->graph[0][j] == 1 && predictions->graph[0][j] > 0))
        result->graph[resultRow][j] = 1; //yay!
    else
        result->graph[resultRow][j] = 0; //boo.
	}
return result;
}


matrix * SVMpopulatePredictionsMatrix(char * resultsFile)
/*Returns an unlabeled matrix of SVM prediction values*/
{
FILE * fp = fopen(resultsFile , "r");
if(!fp)
    errAbort("Couldn't open results file %s", resultsFile);
matrix * tmp1 = f_fill_matrix(fp, 0);
fclose(fp);

matrix * predictions = transpose(tmp1);
free_matrix(tmp1);
return predictions;
}

matrix * SVMgetSampleAccuracy(struct hash *config)
/*Read all the folds and calculate training and testing accuracies from best models*/
{
char * trainingDir = hashMustFindVal(config, "trainingDir");
char * validationDir = hashMustFindVal(config, "validationDir");
char * modelDir = hashMustFindVal(config, "modelDir");
int fold, folds = foldsCountFromDataDir(config);
int split, splits = splitsCountFromDataDir(config);

matrix * accuracies = NULL;
char filename[1024];
for(split = 1; split <= splits; split++)
	{
	for(fold = 1; fold <= folds; fold++)
		{
		//cat togetehr the training and validation KH values and record which were used to train
		safef(filename, sizeof(filename), "%s/split%02d/fold%02d/data.svm", trainingDir, split, fold);
	    matrix * trMetadata = SVMtoMetadataMatrix(filename);
	
		safef(filename, sizeof(filename), "%s/split%02d/fold%02d/data.svm", validationDir, split, fold);
	    matrix * valMetadata = SVMtoMetadataMatrix(filename);
	    struct slInt * trainingList = list_indices(trMetadata->cols);
		matrix * metadata = append_matrices(trMetadata, valMetadata, 1);
	
		//cat together the guesses from SVM
		safef(filename, sizeof(filename), "%s/split%02d/fold%02d/svm.training.results", modelDir, split, fold);
		matrix * trainingPred = SVMpopulatePredictionsMatrix(filename);
		safef(trainingPred->rowLabels[0],MAX_LABEL, "Prediction");
		copy_matrix_labels(trainingPred, trMetadata, 2,2);
		trainingPred->labels=1;

	    safef(filename, sizeof(filename), "%s/split%02d/fold%02d/svm.validation.results", modelDir, split, fold);
		matrix * testingPred = SVMpopulatePredictionsMatrix(filename);
		safef(testingPred->rowLabels[0], MAX_LABEL, "Prediction");
		copy_matrix_labels(testingPred, valMetadata, 2,2);
		testingPred->labels=1;

	    matrix * predictions = append_matrices(trainingPred, testingPred, 1);
		//get accuracies
		matrix * accuraciesInFold = SVMpopulateAccuracyMatrix(predictions, metadata, trainingList);
		
		//add the accuracies to the running totals
		if(split == 1 && fold == 1)
			accuracies = copy_matrix(accuraciesInFold);
		else
		    add_matrices_by_colLabel(accuracies, accuraciesInFold);
		
		//clean up
	    free_matrix(trMetadata);
	    free_matrix(valMetadata);
		free_matrix(metadata);
	    free_matrix(trainingPred);
	    free_matrix(testingPred);
		free_matrix(predictions);
		free_matrix(accuraciesInFold);
		slFreeList(&trainingList);
		}
	}

//normalize accuracies over number of splits and folds
int i;
for(i = 0; i < accuracies->cols; i++)
    {
    if(accuracies->graph[0][i] != NULL_FLAG)
        accuracies->graph[0][i] = (accuracies->graph[0][i] / ((folds-1) * splits));
    if(accuracies->graph[1][i] != NULL_FLAG)
        accuracies->graph[1][i] = (accuracies->graph[1][i] / (1 * splits));
    }
return accuracies;
}


/**** WEKA specific functions *********/
matrix * WEKApopulateAccuracyMatrix(struct hash * config, int split, int fold)
{
char * trainingDir = hashMustFindVal(config, "trainingDir");
char * validationDir = hashMustFindVal(config, "validationDir");
char * modelDir = hashMustFindVal(config, "modelDir");
char filename[256];

//cat togetehr the training and validation KH values and record which were used to train
safef(filename, sizeof(filename), "%s/split%02d/fold%02d/data.arff", trainingDir, split, fold);
matrix * trMetadata = WEKAtoMetadataMatrix(filename);
safef(filename, sizeof(filename), "%s/split%02d/fold%02d/data.arff", validationDir, split, fold);
matrix * valMetadata = WEKAtoMetadataMatrix(filename);
matrix * metadata = append_matrices(trMetadata, valMetadata, 1);
struct slInt * trainingList = list_indices(trMetadata->cols);
	
//create a labeled matrix for results to be stored in
matrix * result = init_matrix(2, metadata->cols);
safef(result->rowLabels[0], MAX_LABEL, "trainingAccuracies");
safef(result->rowLabels[1], MAX_LABEL, "testingAccuracies");
copy_matrix_labels(result, metadata, 2,2);
result->labels=1;

//read the results from file
safef(filename, sizeof(filename), "%s/split%02d/fold%02d/weka.training.results", modelDir, split, fold);
FILE * fp = fopen(filename, "r");
if(fp == NULL)
	errAbort("Couldn't open %s for reading.", filename);
//advance the cursor to where data starts
char * line;
while( (line = readLine(fp)) && line != NULL)
	{
	if(strstr(line, "inst#") != NULL)
		break;
	}
//read each result and save to results matrix
int i;
for(i = 0; i < trMetadata->cols && (line = readLine(fp)) != NULL; i++)
	{
	if(strstr(line, ":?") == NULL)
		{
		if(strstr(line, " + ") == NULL)
			result->graph[0][i] = 1;
		else
			result->graph[0][i] = 0;
		}
	}

safef(filename, sizeof(filename), "%s/split%02d/fold%02d/weka.validation.results", modelDir, split, fold);
fp = fopen(filename, "r");
if(fp == NULL)
    errAbort("Couldn't open %s for reading.", filename);
//advance the cursor to where data starts
while( (line = readLine(fp)) && line != NULL)
    {
    if(strstr(line, "inst#") != NULL)
        break;
    }
//read each result and save to results matrix
for(i = i; i < result->cols && (line = readLine(fp)) != NULL; i++)
    {
    if(strstr(line, ":?") == NULL)
        {
        if(strstr(line, " + ") == NULL)
            result->graph[1][i] = 1;
        else
            result->graph[1][i] = 0;
        }
    }


free_matrix(trMetadata);
free_matrix(valMetadata);
free_matrix(metadata);
slFreeList(&trainingList);

return result;
}


matrix * WEKAgetSampleAccuracy(struct hash * config)
{
int fold, folds = foldsCountFromDataDir(config);
int split, splits = splitsCountFromDataDir(config);

matrix * accuracies = NULL;
for(split = 1; split <= splits; split++)
	{
	for(fold = 1; fold <= folds; fold++)
	    {
		matrix * accuraciesInFold = WEKApopulateAccuracyMatrix(config,split, fold);

	    //add the accuracies to the running totals
	    if(split == 1 && fold == 1) 
	        accuracies = copy_matrix(accuraciesInFold);
	    else
	        add_matrices_by_colLabel(accuracies, accuraciesInFold);
	
	    //clean up
	    free_matrix(accuraciesInFold);
		}
	}
//normalize accuracies over number of splits and folds
int i;
for(i = 0; i < accuracies->cols; i++)
    {
    if(accuracies->graph[0][i] != NULL_FLAG)
        accuracies->graph[0][i] = (accuracies->graph[0][i] / ((folds-1) * splits));
    if(accuracies->graph[1][i] != NULL_FLAG)
        accuracies->graph[1][i] = (accuracies->graph[1][i] / (1 * splits));
    }
return accuracies;
}

matrix * WEKApopulatePredictionsMatrix(struct hash * config)
{
//quick hack to get proper labes from trianing data. TODO: add a labeling scheme to WEKAwrapper for output
char * trainingDir = hashMustFindVal(config, "trainingDir");
char * modelDir = hashMustFindVal(config, "modelDir");
char filename[1024];
safef(filename, sizeof(filename), "%s/data.arff", trainingDir);
matrix * labelTemplate = WEKAtoMetadataMatrix(filename);
safef(filename, sizeof(filename), "%s/weka.training.results", modelDir);
FILE * fp = fopen(filename, "r");
if(fp == NULL)
    errAbort("Couldn't open %s for reading.", filename);

//read the number of data lines  by advancing the cursor to where data starts, then counting lines
char * line;
while( (line = readLine(fp)) && line != NULL)
    {
    if(strstr(line, "inst#") != NULL)
        break;
    }
int samples = 0;
while( (line = readLine(fp)) && line != NULL && !sameString(line, ""))
	samples++;
if(samples == 0)
	return NULL;//catch where the model doesn't return values because of all null known vals
rewind(fp);

//advance the cursor again
while( (line = readLine(fp)) && line != NULL)
    {
    if(strstr(line, "inst#") != NULL)
        break;
    }

//create target for results
matrix * result = init_matrix(1, samples);
safef(result->rowLabels[0], MAX_LABEL, "prediction");
copy_matrix_labels(result, labelTemplate, 2,2);
result->labels=1;
free_matrix(labelTemplate);

//read each result and save to results matrix
int i;
for(i = 0; i < result->cols && (line = readLine(fp)) != NULL; i++)
    {
    if(strstr(line, ":?") == NULL)
		{
		if(strstr(line, "subgroup 1:subgroup"))
			result->graph[0][i] = 0-atof(lastWordInLine(line));
		else if(strstr(line, "subgroup 2:subgroup"))
			result->graph[0][i] = atof(lastWordInLine(line));
		else
			errAbort("ERROR: Coudln't find a proper class assignment in your WEKA results file.\n");
		}
    }
fclose(fp);
return result;
}


/**** glmnet specific functions *****/
matrix * glmnetpopulateAccuracyMatrix(matrix * predictions, matrix * metadata, struct slInt * trainingList)
{
int i, j;

//make a place to store results
matrix * result = init_matrix(2, metadata->cols);
safef(result->rowLabels[0], MAX_LABEL, "trainingAccuracies");
safef(result->rowLabels[1], MAX_LABEL, "testingAccuracies");
for(i = 0; i < metadata->cols; i++)
    safef(result->colLabels[i], MAX_LABEL, "%s", metadata->colLabels[i]);
result->labels=1;

//iterate over each sample, recording accuacies
struct slInt * testList = index_complement(metadata->cols,trainingList);
for(j = 0; j < predictions->cols; j++)
    {
    int resultRow = -1;
    if(indexInList(j, trainingList))
        resultRow = 0;
    else if(indexInList(j, testList))
        resultRow = 1;

    if(metadata->graph[0][j] == NULL_FLAG)
        result->graph[resultRow][j] = NULL_FLAG; // O_o
    else if((metadata->graph[0][j] == 0 && predictions->graph[0][j] < 0) ||
            (metadata->graph[0][j] == 1 && predictions->graph[0][j] > 0))
        result->graph[resultRow][j] = 1; //yay!
    else
        result->graph[resultRow][j] = 0; //boo.
    }
return result;
}

matrix * glmnetgetSampleAccuracy(struct hash *config)
/*Read all the folds and calculate training and testing accuracies from best models*/
{
char * trainingDir = hashMustFindVal(config, "trainingDir");
char * validationDir = hashMustFindVal(config, "validationDir");
char * modelDir = hashMustFindVal(config, "modelDir");
char * parameters = hashMustFindVal(config, "parameters");
int fold, folds = foldsCountFromDataDir(config);
int split, splits  = splitsCountFromDataDir(config);

matrix * accuracies = NULL;
char filename[256];
FILE * fp;
for(split = 1; split <= splits; split++)
    {
    for(fold = 1; fold <= folds; fold++)
        {
        //cat togetehr the training and validation KH values and record which were used to train
        safef(filename, sizeof(filename), "%s/split%02d/fold%02d/metadata.tab", trainingDir, split, fold);
        fp = fopen(filename, "r");
        if(fp == NULL)
            errAbort("Couldn't open file %s\n", filename);
        matrix * trMetadata = f_fill_matrix(fp, 1);
        fclose(fp);

        safef(filename, sizeof(filename), "%s/split%02d/fold%02d/metadata.tab", validationDir, split, fold);
        fp = fopen(filename, "r");
        if(fp == NULL)
            errAbort("Couldn't open file %s\n", filename);
        matrix * valMetadata = f_fill_matrix(fp, 1);
        fclose(fp);

        struct slInt * trainingList = list_indices(trMetadata->cols);
        matrix * metadata = append_matrices(trMetadata, valMetadata, 1);

        safef(filename, sizeof(filename), "%s/split%02d/fold%02d/%s.training.results", modelDir, split, fold, parameters);
        fp = fopen(filename , "r");
        if(!fp)
            errAbort("Couldn't open training results file %s", filename);
        matrix * trainingPred = f_fill_matrix(fp, 1);
        fclose(fp);

        safef(filename, sizeof(filename), "%s/split%02d/fold%02d/%s.validation.results", modelDir, split, fold, parameters);
        fp = fopen(filename , "r");
        if(!fp)
            errAbort("Couldn't open validation results file %s", filename);
        matrix * valPred = f_fill_matrix(fp, 1);
        fclose(fp);

        //calc the accuracy by sample
        matrix * tmp = append_matrices(trainingPred, valPred, 2);
		matrix * predictions = transpose(tmp);
		free_matrix(tmp);
        matrix * accuraciesInFold = glmnetpopulateAccuracyMatrix(predictions, metadata, trainingList); //just use NMF code here..

        //add the accuracies to the running totals
        if(split == 1 && fold == 1)
            accuracies = copy_matrix(accuraciesInFold);
        else
            add_matrices_by_colLabel(accuracies, accuraciesInFold);

        free_matrix(trainingPred);
        free_matrix(valPred);
        free_matrix(predictions);
        free_matrix(trMetadata);
        free_matrix(valMetadata);
        free_matrix(metadata);
        free_matrix(accuraciesInFold);
        }
    }

//normalize accuracies over number of splits and folds
int i;
for(i = 0; i < accuracies->cols; i++)
    {
    if(accuracies->graph[0][i] != NULL_FLAG)
        accuracies->graph[0][i] = (accuracies->graph[0][i] / ((folds-1) * splits));
    if(accuracies->graph[1][i] != NULL_FLAG)
        accuracies->graph[1][i] = (accuracies->graph[1][i] / (1 * splits));
    }
return accuracies;
}

matrix * glmnetpopulatePredictionsMatrix(char * resultsFile)
/*Gives a coefficient per sample of prediction strength - ONLY FOR BINARY CLASSIFIERS*/
{
FILE * fp = fopen(resultsFile, "r");
if(!fp)
    errAbort("Couldnt open results file %s for reading\n", resultsFile);
matrix * tmp = f_fill_matrix(fp, 1);
fclose(fp);

if(!tmp->labels)
    errAbort("Trying to obtain prediction values from a matrix with no labels.");

//transpose results
matrix * result = transpose(tmp);
free_matrix(tmp);

return result;
}


/**** Generic bouncing functions *****/
matrix * getSampleAccuracy(struct hash * config)
/*Function to bounce tot he right sample accuracy calculator based on settings in config*/
{
char * classifier = hashMustFindVal(config, "classifier");
if(sameString(classifier, "NMFpredictor"))
    return NMFgetSampleAccuracy(config);
else if(sameString(classifier, "SVMlight"))
    return SVMgetSampleAccuracy(config);
else if(sameString(classifier, "WEKA"))
    return WEKAgetSampleAccuracy(config);
else if(sameString(classifier, "glmnet"))
	return glmnetgetSampleAccuracy(config);
else
    errAbort("Unsupported classifier type in config.\n");
return NULL;
}

matrix * getPredictionScores(struct hash * config)
/*Function to bounce tot he right sample accuracy calculator based on settings in config*/
//HERE TODO configure the prediction functions to return already labeled matrices
{
char * classifier = hashMustFindVal(config, "classifier");
char * modelDir = hashMustFindVal(config, "modelDir");
char * trainingDir = hashMustFindVal(config, "trainingDir");
char filename[1024];
if(sameString(classifier, "NMFpredictor"))
	{
	safef(filename, sizeof(filename), "%s/NMFpredictor.training.results", modelDir);
    return NMFpopulatePredictionsMatrix(filename);
	}
else if(sameString(classifier, "SVMlight"))
	{
    safef(filename, sizeof(filename), "%s/svm.training.results", modelDir);
    matrix * result= SVMpopulatePredictionsMatrix(filename);
	safef(filename, sizeof(filename), "%s/data.svm", trainingDir);
	matrix * labelTemplate = SVMtoMetadataMatrix(filename);
	safef(result->rowLabels[0], MAX_LABEL, "Prediction");
	copy_matrix_labels(result, labelTemplate, 2,2);	
	result->labels=1;
	free_matrix(labelTemplate);
	return result;
	}
else if(sameString(classifier, "WEKA"))
	{
    return WEKApopulatePredictionsMatrix(config);
	}
else if(sameString(classifier, "glmnet"))
	{
	char * parameters = hashMustFindVal(config, "parameters");
    safef(filename, sizeof(filename), "%s/%s.training.results", modelDir, parameters);
	return glmnetpopulatePredictionsMatrix(filename);
	}
else
    errAbort("Unsupported classifier type in config.\n");
return NULL;
}

