#include <ctype.h>
#include "common.h"
#include "options.h"
#include "linefile.h"
#include "ra.h"
#include "hash.h"
#include "../inc/MLmatrix.h"
#include "../inc/MLcrossValidation.h"
#include "../inc/MLreadConfig.h"
#include "../inc/MLio.h"
#include "../inc/MLfilters.h"
#include "../inc/MLextractCoefficients.h"

void usage()
{
errAbort(   "Usage: MLreportFolds [config_file]\n");
}

int main(int argc, char *argv[])
{
if(argc < 2) 
	usage();
char * configFile = argv[1];
struct hash * config = raReadSingle(configFile);

matrix * data = NULL;
if(sameString("bioInt", hashMustFindVal(config, "inputType")))
	{
	char * profile = hashMustFindVal(config, "profile"); //TODO: make this optional so if it's not set it defaults
	char * db = hashMustFindVal(config, "db");
	struct sqlConnection *conn = hAllocConnProfile(profile, db);
	char *tableName = hashMustFindVal(config, "tableName");
	data = bioInt_fill_matrix(conn, tableName);
	hFreeConn(&conn);
	}
else if(sameString("flatfiles", hashMustFindVal(config, "inputType")))
	{
	char * dataFilepath = hashMustFindVal(config, "dataFilepath");
	FILE * dataFile;
	dataFile = fopen(dataFilepath, "r");
	if(dataFile == NULL)
   		errAbort("ERROR: Couldn't open the file \"%s\"\n", dataFilepath);
	data = f_fill_matrix(dataFile, 1);
	fclose(dataFile);
	}
else
	errAbort("Unsupported input type");

int itemsInFold = -1;
int folds = foldsCountFromConfig(config);
char * cv = hashMustFindVal(config, "crossValidation");
if(sameString("k-fold", cv))
    itemsInFold = floor(data->cols/folds);
else if(sameString("loo", cv))
    itemsInFold = 1;
if(folds == -1 || itemsInFold == -1)
    errAbort("Couldn't assign folds or itemsInFold\n");

if(hashFindVal(config, "excludeList"))
    {
    matrix * trimmedData = filterColumnsByExcludeList(config, data);
    free_matrix(data);
    data = trimmedData;
    }

struct slInt *list = list_indices(data->cols); 

int split, splits = splitsCountFromConfig(config);
matrix * foldReports = NULL;
for(split = 1; split <= splits; split++)
	{
	struct slInt *shuffledList = seeded_shuffle_indices(list, split);

	matrix * tmp = reportFolds(config, data, folds,itemsInFold, shuffledList);
	if(split == 1)
		{
		foldReports = copy_matrix(tmp);
		}
	else
		{
		matrix * tmp2 = append_matrices(foldReports, tmp, 2);
		free_matrix(foldReports);
		foldReports = copy_matrix(tmp2);
		}
	slFreeList(&shuffledList);
	}
fprint_discreteMatrix(stdout, foldReports);

free_matrix(foldReports);
freeHash(&config);
slFreeList(&list);
return 0;
}
