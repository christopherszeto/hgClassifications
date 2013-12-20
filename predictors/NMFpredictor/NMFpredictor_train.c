#include <ctype.h>
#include "common.h"
#include "options.h"
#include "hash.h"
#include "linefile.h"
#include "ra.h"
#include "NMF.h"
#include "../../inc/MLmatrix.h"
#include "../../inc/MLcrossValidation.h"
#include "../../inc/MLio.h"
#include "../../inc/MLreadConfig.h"
#include "../../inc/MLextractCoefficients.h"
#include "../../inc/MLfilters.h"

matrix * createPseudoclassedMatrix(matrix * metadata)
/*Small hack function to make a pseudclassed two-row matrix from a set of binary values in metadata*/
{
double pseudoclassMaxVal = 10000;
double pseudoclassMinVal = 1000;

matrix * A_ptr = init_matrix(2,metadata->cols);
safef(A_ptr->rowLabels[0],MAX_LABEL,  "Low");
safef(A_ptr->rowLabels[1],MAX_LABEL, "High");

int i = 0;
for(i = 0; i < A_ptr->cols; i++)
    {   
    safef(A_ptr->colLabels[i], MAX_LABEL, "%s", metadata->colLabels[i]);
    if(metadata->graph[0][i] == 0)
        {
        A_ptr->graph[0][i] = pseudoclassMaxVal;
        A_ptr->graph[1][i] = pseudoclassMinVal;
        }
    else if(metadata->graph[0][i] == 1)
        {
           A_ptr->graph[0][i] = pseudoclassMinVal;
           A_ptr->graph[1][i] = pseudoclassMaxVal;
        }
    else
        {
        A_ptr->graph[0][i] = A_ptr->graph[1][i] = (((pseudoclassMaxVal - pseudoclassMinVal)/2) + pseudoclassMinVal);
        }
    }
return A_ptr;
}

int matrixMax(matrix * m)
{
int i, j;
double max = m->graph[0][0];
for(i = 0; i < m->rows; i++)
	{
	for(j = 0; j < m->cols; j++)
		{
		if(m->graph[i][j] != NULL_FLAG && m->graph[i][j] >= max)
			max = m->graph[i][j];
		}
	}
if(max == -99999)
	errAbort("Error: Couldn't find max in all null matrix!\n");
return max;
}

matrix * NMFpredictor_train(matrix * data, matrix * metadata, struct hash *parameters)
/*Returns the model that trained best on data*/
{
//make sure data is positive
shift_matrix(data);
//fill a pseudoclasses matrix from known values for training
matrix * pseudoclasses = createPseudoclassedMatrix(metadata);

//set options from parameters, or to defaults
int seeds = hashIntValDefault(parameters, "s", 30);
int max_reps= hashIntValDefault(parameters, "r", 100);
double w_resistance = 1, h_resistance =0.1, training_proportion = 0.8;
if(hashFindVal(parameters, "wr"))
	w_resistance = atof(hashMustFindVal(parameters, "wr"));
if(hashFindVal(parameters, "hr"))
    h_resistance = atof(hashMustFindVal(parameters, "hr"));
if(hashFindVal(parameters, "tp"))
    training_proportion = atof(hashMustFindVal(parameters, "tp"));
int sparsity_constraint = 0;
if(hashFindVal(parameters, "sp"))
	sparsity_constraint = 1;

//iterate over seeds, learning a new model each time
int seed;
float bestAcc = -1;
matrix * bestModel = NULL;
for(seed = 0; seed < seeds; seed++)
	{
	//Pick random indices to train on
	struct slInt * trainingList = seeded_random_indices(data->cols, training_proportion, 1); //pick a training list
    matrix * trData = copy_matrix_subset(data, NULL, trainingList);  //set the training V matrix
	matrix * trPseudo = copy_matrix_subset(pseudoclasses, NULL, trainingList); //set the training H matrix

	//make a random seed
    matrix * model = sr_fill_matrix(seed, data->rows, 2, 0.00001, 1); //consider using matrixMax:this will break on non-represented max
    scale_matrix(model, 2, 1);
	
	//calc the initial WH and the residual
	matrix * dataApprox = init_matrix(data->rows, trPseudo->cols);
	mult_matrices(model,trPseudo,dataApprox);
	double currResidual = calc_residual(trData,dataApprox);
	double prevResidual = 0;
	double dResidual = 0;
	double cutoff = 0;

	//Learn until convergence or until you've done max_reps
	int i;
    for(i = 0; i < (max_reps-1) && dResidual >= cutoff; i++)
		{
        if(w_resistance)
			{
            updateW(trData, model, trPseudo, dataApprox, w_resistance);
            scale_matrix(model, 2, 1);
			if(sparsity_constraint)
				constrainSparsity(model);
        	}
        if(h_resistance)
			{
            updateH(trData, model, trPseudo, dataApprox, h_resistance);
        	}
        prevResidual = currResidual;
		currResidual = calc_residual(trData,dataApprox);
        dResidual = currResidual - prevResidual;
		if(i == 0)
			{
			cutoff = dResidual/100;
			}
    	}

	//test
	matrix * predictions =  NMFpopulateClassPredictionsMatrix(data, model);
	matrix * accuracies = NMFpopulateAccuracyMatrix(predictions,metadata,trainingList);

	//if this is the best model so far, cache the model
	char accString[256];
	safef(accString, sizeof(accString), "%02d.%02d", (int)avgTestingAccuracy(accuracies), (int)avgTrainingAccuracy(accuracies));
	float acc = atof(accString);
	if(bestAcc == -1)
		{
		bestModel = copy_matrix(model);
		bestAcc = acc;
		}
	else if(acc > bestAcc)
		{
		free_matrix(bestModel);
		bestModel = copy_matrix(model);
		}

	//clean up
    free_matrix(dataApprox);
	free_matrix(trPseudo);
	free_matrix(trData);
	free_matrix(predictions);
	free_matrix(model);
	free_matrix(accuracies);
	slFreeList(&trainingList);
	}
//put the labels on model, and print it to modelDir
bestModel->labels=0;
copy_matrix_labels(bestModel, data, 1, 1);
bestModel->labels=0;
copy_matrix_labels(bestModel, pseudoclasses, 2,1);
bestModel->labels=1;

//clean up local matrices
free_matrix(pseudoclasses);

return bestModel;
}

void usage()
{ 
	errAbort("Usage: NMFpredictor_train [OPTIONS] data_file metadata_file output_file\n"
			"NMFpredictor V0.1 help:\nThis program deconvolutes a given matrix into two matrices.\n"
			"Deconvolution of V, such that V = (WH) where V is probes vs. tissues (from chip data),\n"
			"H is tissues vs. transcripts (from clusters; pseudoclasses), and W is unknown; metagenes vs clusteras\n"
			"OPTIONS:\n"
			"-s=[int]\tHow many seeds (models) to test (default = 30)\n"
			"-r=[int]\tMax number of reps to iterate learning (default = 100)\n"
			"-tp=[float]\tProportion of data to use for training model, vs. selecting best model (default = 0.8)\n"
			"-wr=[float]\tW matrix resistance coefficient (defailt = 1)\n"
			"-hr=[float]\tH matrix resistance coefficeint (default = 0.1)\n"
			"-sp\tConstrain sparsity in model\n");
}

int main(int argc, char * argv[])
{
struct hash *parameters = optionParseIntoHash(&argc, argv, 0);

FILE * fp = fopen(argv[1], "r");
matrix * data = NULL, *metadata = NULL; 
if(fp != NULL)
	data = f_fill_matrix(fp, 1);
else
	{
	fprintf(stderr, "Couldn't open file %s\n", argv[1]);
	usage();
	}
fclose(fp);
fp = fopen(argv[2], "r");
if(fp != NULL)
	metadata = f_fill_matrix(fp, 1);
else
	{
    fprintf(stderr, "Couldn't open file %s\n", argv[2]);
    usage();
	}
fclose(fp);

matrix * bestModel = NMFpredictor_train(data, metadata, parameters);

fp = fopen(argv[3], "w");
if(fp != NULL)
	fprint_matrix(fp, bestModel);
else
	{
    fprintf(stderr, "Couldn't open file %s\n", argv[3]);
    usage();
	}
fclose(fp);

//clean up
free_matrix(data);
free_matrix(metadata);
free_matrix(bestModel);
freeHash(&parameters);
return 0;
}
