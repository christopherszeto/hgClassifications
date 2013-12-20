#include <ctype.h>
#include "common.h"
#include "options.h"
#include "linefile.h"
#include "jksql.h"
#include "hdb.h"
#include "hash.h"
#include "ra.h"
#include "dirent.h"
#include "../inc/MLmatrix.h"
#include "../inc/MLcrossValidation.h"
#include "../inc/MLreadConfig.h"
#include "../inc/MLio.h"
#include "../inc/MLfilters.h"

char * profile = "localDb";
char * db = "bioInt";

matrix * classify(matrix * data, int modelId)
{
matrix * result = NULL;
char * modelPath, *classifier, *parameters;
char * currDir = getCurrentDir();
//get info about this model from db
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];
safef(query, sizeof(query), "SELECT modelPath FROM jobs WHERE id=%d", modelId);
modelPath = sqlNeedQuickString(conn, query);
safef(query, sizeof(query), "SELECT classifiers.name FROM classifiers JOIN jobs ON (jobs.classifiers_id=classifiers.id) WHERE jobs.id=%d", modelId);
classifier = sqlNeedQuickString(conn, query);
safef(query, sizeof(query), "SELECT classifiers.parameters FROM classifiers JOIN jobs ON (jobs.classifiers_id=classifiers.id) WHERE jobs.id=%d", modelId);
parameters = sqlNeedQuickString(conn, query);

//get prediction scores
if(sameString(classifier, "NMFpredictor"))
    {
    matrix * data_cpy = NMFgetDataMatchedToModel(data, modelPath);
    result = applyNMFpredictorModel(data_cpy, currDir, modelPath);
    free_matrix(data_cpy);
    }
else if(sameString(classifier, "SVMlight"))
    {
    matrix * data_cpy = SVMgetDataMatchedToModel(data, modelPath);
    result = applySVMlightModel(data_cpy, currDir, modelPath);
    free_matrix(data_cpy);
    }
else if(sameString(classifier, "glmnet"))
	{
	char * userDataPrefix="/data/trash/MLuserSubmissions/tmp";
	matrix * data_cpy = glmnetgetDataMatchedToModel(data,userDataPrefix,modelPath,parameters);
	result = applyglmnetModel(data_cpy, userDataPrefix, modelPath, parameters);
	free_matrix(data_cpy);
	}
else if(sameString(classifier, "WEKA"))
    errAbort("ERROR: No functions in place to apply WEKA models yet.\n");
else
    errAbort("ERROR: Unrecongized model type\n");
//clean up
freeMem(modelPath);
freeMem(classifier);

return result;
}

void usage()
{
errAbort(	"Usage: ./MLclassify [tab file] [model id]\n");
}

int main(int argc, char *argv[])
/* Process command line. */
{
if(argc != 3) 
	usage();
FILE * fp = fopen(argv[1], "r");
if(!fp)
	errAbort("Couldn't open file %s\n", argv[1]);
matrix * data = f_fill_matrix(fp, 1);
matrix * results = classify(data, atoi(argv[2]));
fprint_matrix(stdout, results);

free_matrix(data);
free_matrix(results);
return 0;
}
