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

static struct optionSpec options[] = {
   {"tasks", OPTION_STRING},
   {"classifiers", OPTION_STRING},
   {"subgroups", OPTION_STRING},
   {"featureSelections", OPTION_STRING},
   {"transformations", OPTION_STRING},
   {"jobs", OPTION_STRING},
   {"backgrounds", OPTION_STRING},
   {"id", OPTION_INT},
   {NULL, 0},
};
int id = -1;

void usage()
{
errAbort(   "Usage: MLwriteResultsToRa [options] [config_file]\n"
            "config_file will point to the results of the run you want to write to ra.\n"
			"various .ra files are where the results are stored.\n"
			"Options:\n"
			"-tasks=[filename]\tWrite tasks ra entries in filename\n"
			"-classifiers=[filename]\tWrite classifiers ra entries in filename\n"
			"-subgroups=[filename]\tWrite subgroups ra entries in filename\n"
			"-featureSelections=[filename]\tWrite feature selector ra entries in filename\n"
			"-transformations=[filename]\tWrite data transformations ra entries in filename\n"
			"-jobs=[filename]\tWrite jobs ra entries in filename\n"
			"-backgrounds=[filename]\tWrite background scores in filename\n"
			"-id=[int]\tNumber to tag on to names to make sure they are unique\n"
			"NB: The optional filenames can be the same filename, writing will be atomic. If a\n"
			"filename is not specified for a type that type of ra entry will not be written.\n\n");
}

int main(int argc, char *argv[])
{
optionInit(&argc, argv, options);
if(argc < 2) 
	usage();
char * configFile = argv[1];
struct hash * config = raReadSingle(configFile);
printf("Processing %s\n", configFile);

if(optionExists("id"))
	id = atof(optionVal("id", NULL));

char * outputFilename = NULL;
if(resultsExist(config))
	{
	if(optionExists("tasks"))
		{
    	outputFilename = optionVal("tasks", outputFilename);
		printTasksRa(config, outputFilename, id);
		}
	if(optionExists("classifiers"))
		{
        outputFilename = optionVal("classifiers", outputFilename);
		printClassifiersRa(config, outputFilename, id);
		}
	if(optionExists("subgroups"))
		{
        outputFilename = optionVal("subgroups", outputFilename);
		printSubgroupsRa(config, outputFilename, id);
		}
    if(optionExists("featureSelections"))
        {
        outputFilename = optionVal("featureSelections", outputFilename);
        printFeatureSelectionsRa(config, outputFilename, id);
        }
    if(optionExists("transformations"))
        {
        outputFilename = optionVal("transformations", outputFilename);
        printTransformationsRa(config, outputFilename, id);
        }
	if(optionExists("jobs"))
		{
        outputFilename = optionVal("jobs", outputFilename);
		printJobsRa(config, outputFilename, id);
		}
	if(optionExists("backgrounds"))
		{
		outputFilename = optionVal("backgrounds", outputFilename);
		printBackgroundResultsRa(config, outputFilename, id);
		}
	}
else
	fprintf(stderr, "WARNING: Results for %s do not exist. Skipping writing results to file.\n", configFile);

freeHash(&config);
return 0;
}
