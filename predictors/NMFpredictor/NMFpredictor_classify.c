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

static struct optionSpec options[] = {
    {"rawCorrelations", OPTION_BOOLEAN},
    {NULL, 0},
};

boolean rawCorrelations = FALSE;

matrix * NMFpredictor_classify(char * dataFile, char * modelFile)
/*Returns predictions for each column in data*/
{
FILE * fp = fopen(dataFile, "r");
matrix * data = NULL, *model = NULL;
if(fp != NULL)
    data = f_fill_matrix(fp, 1);
else
    errAbort("Couldn't open file %s\n", dataFile);
fclose(fp);
shift_matrix(data);

fp = fopen(modelFile, "r");
if(fp != NULL)
    model = f_fill_matrix(fp, 1);
else
    errAbort("Couldn't open file %s\n", modelFile);
fclose(fp);

if(!matchedRowLabels(data, model))
	{
	matrix * tmp = matchOnRowLabels(data, model);
	free_matrix(data);
	data = tmp;
	}
matrix * result = NULL;
if(rawCorrelations)
	{
	result = populateCorrMatrix_NArm(data, model, NULL_FLAG);
	result->labels= 0;
	copy_matrix_labels(result, data, 2,2);
	result->labels= 0;
	copy_matrix_labels(result, model, 1,2);
	result->labels=1;
	}
else
	result = NMFpopulateClassPredictionsMatrix(data, model);

free_matrix(data);
free_matrix(model);
return result;
}

void usage()
{ 
	errAbort(	"Usage: NMFpredictor_classify [OPTIONS] data_file model_file output_file\n"
				"OPTIONS:\n"
				"-rawCorrelations\tPrint correlations for each class in model rather than index of predicted class\n");
}

int main(int argc, char * argv[])
{
optionInit(&argc, argv, options);
if(argc != 4)
	usage();

if(optionExists("rawCorrelations"))
    rawCorrelations= TRUE;

matrix * result = NMFpredictor_classify(argv[1], argv[2]);

FILE * fp = fopen(argv[3], "w");
if(fp == NULL)
	errAbort("couldn't open %s for writing\n", argv[3]);
fprint_matrix(fp, result);
fclose(fp);

return 0;
}
