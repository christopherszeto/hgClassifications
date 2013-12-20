#include "common.h"
#include "linefile.h"
#include "hash.h"
#include "options.h"
#include "jksql.h"
#include "hPrint.h"
#include "hdb.h"
#include "ra.h"
#include "../inc/classificationTables.h"
#include "../inc/populateClassificationTables.h"
#include "../inc/MLmatrix.h" 
#include "../inc/MLcrossValidation.h" 
#include "../inc/MLreadConfig.h" 
#include "../inc/MLio.h"
#include "../inc/MLextractCoefficients.h"
#include "../inc/MLfilters.h"
#include "../inc/sqlite3.h"

void usage()
/* Explain usage and exit. */
{
errAbort(
  "loadBackgroundResults\n"
  "   loadBackgroundResults [OPTIONS] db raFile\n"
  "options:\n"
  "   -drop   Drop/recreate background results table\n"
  "   -profile=localDb   handles alternate db profiles\n"
  "\n"
  );
}

//default options
boolean dropTables = FALSE;   // If true, any table that should be dropped/recreated will be
char *profile = "localDb";
static struct optionSpec options[] = {
    {"drop", OPTION_BOOLEAN},
    {"profile", OPTION_STRING},
    {NULL, 0},
};


void saveBackgroundRa(struct sqlConnection * conn, struct hash * raHash)
{
int bms_id = sqlTableSize(conn, "backgroundModelScores")+1;
fprintf(stderr, "Saving background model scores...\n");
char * predScoresString = (char *)hashMustFindVal(raHash, "predictionScores");
char *data_table = (char *)hashMustFindVal(raHash, "data_table");
char *jobsIdString = (char *)hashMustFindVal(raHash, "jobs_id");
int jobs_id = atoi(jobsIdString);
int estimatedRunTime = atoi((char *)hashMustFindVal(raHash, "estimatedRunTime"));

//make sure the background table refered to exists. If not, don't save.
if(!sqlTableExists(conn,data_table))
	{
	fprintf(stderr,"ERROR: Entry for %d refers to background table that doesn't exist. Skipping...\n", jobs_id);
	return;
	}

//get the vital stats from the list of prediction scores in the ra
struct slName *ps,*predScoresList = slNameListFromComma(predScoresString);
ps = predScoresList;
double min = NULL_FLAG, mean=0, sd=0,  median = NULL_FLAG, max = NULL_FLAG;
int length = slCount(predScoresList);
double predScores[length];
double val;
int currIx = 0;

//terate through twice, calculating everything necessary.
for(ps = predScoresList; (ps != NULL) && (currIx < length); ps = ps->next)
	{
	val = atof(ps->name);
	if(val != NULL_FLAG)
		{
		predScores[currIx++] = val;
		mean += val;
		//if this is the first one, set the min and max to the first val
		if(min == NULL_FLAG)
			{
			min = val;
			max = val;
			}
		else
			{
			if(min > val)
				min = val;
			if(max < val)
				max = val;
			}
		}
	}
int nonNullLength = currIx;
mean = (mean/nonNullLength);
int medianIx = (int)(nonNullLength/2);
currIx = 0;
for( ps = predScoresList; (ps != NULL) && (currIx < length); ps= ps->next)
	{
	val = atof(ps->name);
	if(val != NULL_FLAG)
		{
		sd += ((val - mean)*(val-mean));
		if(currIx == medianIx)
			median = val;
		currIx++;
		}
	}
sd = sqrt(sd/nonNullLength);
if(min == NULL_FLAG || max == NULL_FLAG || median == NULL_FLAG || mean == NULL_FLAG || sd == NULL_FLAG)
	errAbort("Missing value needed for saving background values. Exiting...\n");

//save to jobs table
struct backgroundModelScores * bms;
AllocVar(bms);
bms->id = bms_id;
bms->jobs_id=jobs_id;
bms->data_table = cloneString(data_table);
bms->min = min;
bms->max = max;
bms->sd = sd;
bms->mean = mean;
bms->median = median;
bms->estimatedRunTime = estimatedRunTime;
backgroundModelScoresSaveToDb(conn,bms,"backgroundModelScores",1024);

//clean up
slFreeList(&predScoresList);
backgroundModelScoresFree(&bms);
}

void populateBackgroundTables(char * profile, char *db, char *raFile, int dropTables)
{
struct hash *raHash, *raHashList = readRaFile(raFile);
struct sqlConnection *conn = hAllocConnProfile(profile, db);
setupTable(conn, "backgroundModelScores", dropTables);
for (raHash = raHashList; raHash; raHash = raHash->next)
	{
	char * type = hashMustFindVal(raHash, "type");
    if(!sameString(type, "background"))
		errAbort("ERROR: Trying to save background model scores from non-background ra. Exiting...\n");
	saveBackgroundRa(conn, raHash);
	}
//clean up
hashFree(&raHash);
hashFree(&raHashList);
hFreeConn(&conn);
}

int main(int argc, char *argv[])
/* Process command line. */
{
optionInit(&argc, argv, options);
if (argc != 3)
    usage();

dropTables = FALSE;
if (optionExists("drop"))
    dropTables = TRUE;
if(optionExists("profile"))
    profile = optionVal("profile", profile);

populateBackgroundTables(profile, argv[1], argv[2], dropTables);

return 0;
}
