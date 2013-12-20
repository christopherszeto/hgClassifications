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
if(max == NULL_FLAG)
	errAbort("Error: Couldn't find max in all null matrix!\n");
return max;
}

matrix * NMFpredictor_project(matrix * data, matrix * bases, struct hash *parameters)
/*Returns the model that trained best on data*/
{
//make sure data is positive
shift_matrix(data);

//set options from parameters, or to defaults
int seeds = atoi(hashOptionalVal(parameters, "s", "30"));
int max_reps= atoi(hashOptionalVal(parameters, "r", "100"));
double w_resistance = atof(hashOptionalVal(parameters, "wr", "0.1"));
double h_resistance = atof(hashOptionalVal(parameters, "hr", "1"));
int verboseOutput = 0, scaleW = 0;
if(hashFindVal(parameters, "v"))
	verboseOutput=1;
if(hashFindVal(parameters, "sw"))
	scaleW = 1;
double percentComplete = 0;

//iterate over seeds, learning a new model each time
int seed;
float bestResidual = NULL_FLAG;
matrix * bestProjection = NULL;
for(seed = 0; seed < seeds; seed++)
	{
	//make a random seed
    matrix * projection = sr_fill_matrix(seed, bases->cols, data->cols, 1, bases->cols); 
    if(scaleW)
    	scale_matrix(bases, 2, 1);
	
	//calc the initial WH and the residual
	matrix * dataApprox = init_matrix(bases->rows, projection->cols);
	mult_matrices(bases,projection,dataApprox);

	double currResidual = calc_residual(data,dataApprox);
	double prevResidual = 0;
	double dResidual = 0;
	double cutoff = 0;

	//Learn until convergence or until you've done max_reps
	int i;
    for(i = 0; i < (max_reps-1) && dResidual >= cutoff; i++)
		{
		if(verboseOutput)
			{
			percentComplete = (double)(seed*max_reps+i)*100 / (seeds * max_reps);
			printProgress(percentComplete);
			}

        if(w_resistance)
			{
            updateW(data, bases, projection, dataApprox, w_resistance);
            if(scaleW)
				scale_matrix(bases, 2, 1);
        	}
        if(h_resistance)
			{
            updateH(data, bases, projection, dataApprox, h_resistance);
        	}

        prevResidual = currResidual;
		currResidual = calc_residual(data,dataApprox);
        dResidual = currResidual - prevResidual;
		if(i == 0)
			{
			cutoff = dResidual/100;
			}
    	}

	//if this is the best projection so far, cache the projection matrix
	if(bestResidual == NULL_FLAG)
		{
		bestProjection = copy_matrix(projection);
		bestResidual = currResidual;
		}
	else if(currResidual > bestResidual)
		{
		free_matrix(bestProjection);
		bestProjection = copy_matrix(projection);
		}

	//clean up
	free_matrix(projection);
	}

//put the labels on projection, and print it
bestProjection->labels=0;
copy_matrix_labels(bestProjection, data, 2,2);
copy_matrix_labels(bestProjection, bases, 1,2);
bestProjection->labels=1;

return bestProjection;
}

void usage()
{ 
	errAbort("Usage: NMFpredictor_train [OPTIONS] data_file bases_file output_file\n"
			"NMFpredictor V0.1 help:\nThis program convolutes a given matrix into two matrices.\n"
			"Convolution of V, such that V = (WH) where V is probes vs. tissues (from chip data),\n"
			"H is sample weights (from clusters; pseudoclasses), and W is the bases; metagenes vs clusters\n"
			"OPTIONS:\n"
			"-s=[int]\tHow many seeds (models) to test (default = 30)\n"
			"-r=[int]\tMax number of reps to iterate learning (default = 100)\n"
			"-wr=[float]\tW matrix resistance coefficient (defailt = 0.1)\n"
			"-hr=[float]\tH matrix resistance coefficeint (default = 1)\n"
			"-sw\tFlag to enable scaling W matrix to a vote table after each round of learning. Default off\n"
			"-v\tFlag to enable verbose output - including a percentage-complete counter. Default off\n");
}

int main(int argc, char * argv[])
{
struct hash *parameters = optionParseIntoHash(&argc, argv, 0);

FILE * fp = fopen(argv[1], "r");
matrix * data = NULL, *bases = NULL; 
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
	bases = f_fill_matrix(fp, 1);
else
	{
    fprintf(stderr, "Couldn't open file %s\n", argv[2]);
    usage();
	}
fclose(fp);

matrix * bestProjection = NMFpredictor_project(data, bases, parameters);

fp = fopen(argv[3], "w");
if(fp != NULL)
	fprint_matrix(fp, bestProjection);
else
	{
    fprintf(stderr, "Couldn't open file %s\n", argv[3]);
    usage();
	}
fclose(fp);

//clean up
free_matrix(data);
free_matrix(bases);
free_matrix(bestProjection);
freeHash(&parameters);
return 0;
}
