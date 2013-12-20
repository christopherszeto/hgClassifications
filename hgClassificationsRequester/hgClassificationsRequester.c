/* hgClassificationsRequest */
#include "common.h"
#include "bed.h"
#include "cart.h"
#include "hash.h"
#include "hCommon.h"
#include "hdb.h"
#include "hPrint.h"
#include "htmshell.h"
#include "hui.h"
#include "web.h"
#include "json.h"
#include "ra.h"
#include "hash.h"
#include "hgClassificationsRequester.h"
#include "../inc/classificationTables.h"
#include "../inc/populateClassificationTables.h"
#include "../inc/MLmatrix.h"
#include "../inc/MLcrossValidation.h"
#include "../inc/MLio.h"
#include "../inc/MLreadConfig.h"
#include "../inc/MLfilters.h"
#include "../inc/MLextractCoefficients.h" 

/* ---- Global variables. ---- */
struct cart *cart;           /* This holds cgi and other variables between clicks. */
struct hash *oldVars;            /* Old cart hash. */

char *db = "bioInt";
char *profile = "localDb";

void usage()
/* Explain usage and exit. */
{
errAbort(
  "hgClassificationsRequest cgi\n"
  "usage:\n"
  "   hgClassificationsRequest\n"
  );
}

void listDatasets()
{
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];
safef(query, sizeof(query), "select data_table from datasets;");
struct slName * d, * ds = sqlQuickList(conn, query);
struct json *js = newJson();
struct json *dataset,*datasets = jsonAddContainerList(js, "datasets");
dataset=datasets;

for(d = ds; d; d = d->next)
	{
	jsonAddString(dataset, "dataset", d->name);
	if(d->next)
		dataset = jsonAddContainerToList(&datasets);
	}
if (js)
   hPrintf("%s\n", js->print(js));
hFreeConn(&conn);
slFreeList(&ds);
}

void listTasks()
/*TODO: Check that you can use the shortLabel to get back to a distinct task to run.*/
/*Or have to revert to using name rather than shortLabel when writing value*/
{
char * dataset = cartOptionalString(cart, rqDataset);
if(!dataset)
    errAbort("%s is required", rqDataset);

struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];
safef(query, sizeof(query), "select id from datasets where data_table=\'%s\'", dataset);
int dataset_id = sqlNeedQuickNum(conn, query);
safef(query, sizeof(query), "select distinct feature_id from clinicalData "
							"join samples on (clinicalData.sample_id=samples.id) where dataset_id=%d;", dataset_id);
struct slInt *fId, * fIds = sqlQuickNumList(conn, query);
struct json *js = newJson();
struct json *task,*tasks = jsonAddContainerList(js, "tasks");
task = tasks;
for(fId = fIds; fId; fId = fId->next)
    {
    safef(query, sizeof(query), "select name from features where id=%d", fId->val);
    char * taskName = sqlNeedQuickString(conn, query);
    jsonAddString(task, "task", taskName);

	safef(query, sizeof(query), "select shortLabel from features where id=%d", fId->val);
	char * label = sqlNeedQuickString(conn, query);
    jsonAddString(task, "label", label);
    if(fId->next)
        task = jsonAddContainerToList(&tasks);
    }
if (js)
   hPrintf("%s\n", js->print(js));
hFreeConn(&conn);
slFreeList(&fIds);
}


void viewSubgroups()
{
char * dataset = cartOptionalString(cart, rqDataset);
char * task = cartOptionalString(cart, rqTask);
char * clinDiscretizer = cartOptionalString(cart, rqClinDiscretizer);
char * clinDiscretizerParameters = cartOptionalString(cart, rqClinDiscretizerParameters);
if(!dataset)
    errAbort("%s is required", rqDataset);
if(!task)
    errAbort("%s is required", rqTask);
if(!clinDiscretizer)
    errAbort("%s is required", rqClinDiscretizer);
if(!clinDiscretizerParameters)
    errAbort("%s is required", rqClinDiscretizerParameters);

struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[1024];
char binName[256];

struct json *js = newJson();
struct json *bin,*bins = jsonAddContainerList(js, "bins");
bin=bins;

safef(query, sizeof(query), "SELECT id FROM features WHERE name=\'%s\'", task);
int featureId = sqlNeedQuickNum(conn, query);

//check if it's a coded feature. If so, set teh bins to the vals coded for and make the names appropriately
safef(query, sizeof(query), "SELECT COUNT(DISTINCT clinicalData.code) FROM clinicalData JOIN %s ON "
            "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.feature_id=%d AND "
			"clinicalData.code NOT LIKE \'(null)\'", dataset,dataset,featureId);
if(sqlQuickNum(conn, query) != 0)
	{
	safef(query, sizeof(query), "SELECT DISTINCT(val) FROM clinicalData WHERE feature_id=%d", featureId);
	struct slInt * uv, *uniqVals = sqlQuickNumList(conn, query);
	for(uv=uniqVals; uv; uv=uv->next)
		{
		//Make a string for the value
		safef(query, sizeof(query), "SELECT DISTINCT(code) FROM clinicalData WHERE feature_id=%d AND val=%d",
			featureId, uv->val);
		char * code = sqlNeedQuickString(conn, query);
		safef(binName, sizeof(binName), "%s(%d)", code, uv->val);
		jsonAddString(bin, "name", binName);

		//find the count for this val
		safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
			"(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val=%d AND clinicalData.feature_id=%d",
			dataset,dataset,dataset,uv->val, featureId);
		jsonAddInt(bin, "value", sqlNeedQuickNum(conn, query));
	
	    if(uv->next)
    	    bin = jsonAddContainerToList(&bins);
		}
	}
//if it's not a coded feature, calc 10 splits and fill those bins
else
	{
	safef(query, sizeof(query), "SELECT MIN(val) FROM clinicalData WHERE feature_id=%d",
		featureId);
	double min = sqlQuickDouble(conn, query);
	safef(query, sizeof(query), "SELECT MAX(val) FROM clinicalData WHERE feature_id=%d",
        featureId);
	double max = sqlQuickDouble(conn, query);
	double step = (max-min)/9;
	double minVal;
	int i;
	for(i = 0, minVal = min; i < 10; i++, minVal=minVal+step)
		{
		safef(binName, sizeof(binName), "%.2f-%.2f", minVal, (minVal+step));
		jsonAddString(bin, "name", binName);
	
		//find the count in this bin
		safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
            "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val >= %f AND clinicalData.val < %f "
			" AND clinicalData.feature_id=%d", dataset, dataset, dataset,minVal, (minVal+step), featureId);
		jsonAddInt(bin, "value", sqlNeedQuickNum(conn, query));
	
		if((i+1) < 10)
        	bin = jsonAddContainerToList(&bins);
		}
	}

//add counts of subgroup1 and subgroup2 to the js
if(sameString(clinDiscretizer, "None"))
	{
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val = 0 "
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, featureId);
    jsonAddInt(js, "subgroup1count", sqlNeedQuickNum(conn, query));
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val = 1 "
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, featureId);
    jsonAddInt(js, "subgroup2count", sqlNeedQuickNum(conn, query));
	}
else if(sameString(clinDiscretizer, "median"))
	{
    safef(query, sizeof(query), "SELECT val FROM clinicalData WHERE feature_id=%d ORDER BY val", featureId);
	struct slDouble *v,* valList = sqlQuickDoubleList(conn, query);
	v = valList;
	int medianIx = (int)(slCount(valList) / 2);
	int i;
	for(i = 0; i < medianIx; i++)
		v=v->next;
	double medianVal = v->val;
	safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val <= %f"
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, medianVal, featureId);
    jsonAddInt(js, "subgroup1count", sqlNeedQuickNum(conn, query));
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val > %f"
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, medianVal, featureId);
    jsonAddInt(js, "subgroup2count", sqlNeedQuickNum(conn, query));
	}
else if(sameString(clinDiscretizer, "quartile"))
	{
    safef(query, sizeof(query), "SELECT val FROM clinicalData WHERE feature_id=%d ORDER BY val", featureId);
    struct slDouble *v,* valList = sqlQuickDoubleList(conn, query);
    v = valList;
    int lowerQix = (int)(slCount(valList) / 4);
	int upperQix = (int)(slCount(valList)*3 / 4);
	int i;
    for(i = 0; i < lowerQix; i++)
        v=v->next;
    double lowerQ = v->val;
	for(i = i; i < upperQix; i++)
		v=v->next;
	double upperQ = v->val;
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val <= %f"
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, lowerQ, featureId);
    jsonAddInt(js, "subgroup1count", sqlNeedQuickNum(conn, query));
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val > %f"
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, upperQ, featureId);
    jsonAddInt(js, "subgroup2count", sqlNeedQuickNum(conn, query));
    }
else if(sameString(clinDiscretizer, "thresholds"))
	{
    struct slName * thresholds = slNameListFromComma(clinDiscretizerParameters);
    double threshold1 = atof(thresholds->name);
    double threshold2 = atof(thresholds->next->name);
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val <= %f "
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, threshold1, featureId);
    jsonAddInt(js, "subgroup1count", sqlNeedQuickNum(conn, query));
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val >= %f "
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, threshold2, featureId);
    jsonAddInt(js, "subgroup2count", sqlNeedQuickNum(conn, query));
	}
else if(sameString(clinDiscretizer, "classes"))
	{
	struct slName * classes = slNameListFromComma(clinDiscretizerParameters);
    int class1 = atoi(classes->name);
    int class2 = atoi(classes->next->name);
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val = %d "
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, class1, featureId);
    jsonAddInt(js, "subgroup1count", sqlNeedQuickNum(conn, query));
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val = %d "
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, class2, featureId);
    jsonAddInt(js, "subgroup2count", sqlNeedQuickNum(conn, query));
	}
else if(sameString(clinDiscretizer, "expressions"))
	{
	struct slName * expressions = slNameListFromComma(clinDiscretizerParameters);
	char * expression1 = expressions->name;
	char * expression2 = expressions->next->name;
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val%s "
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, expression1, featureId);
    jsonAddInt(js, "subgroup1count", sqlNeedQuickNum(conn, query));
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT %s.sample_id) FROM clinicalData JOIN %s ON "
        "(%s.sample_id=clinicalData.sample_id) WHERE clinicalData.val%s"
        " AND clinicalData.feature_id=%d", dataset, dataset, dataset, expression2, featureId);
    jsonAddInt(js, "subgroup2count", sqlNeedQuickNum(conn, query));
	}

//print results
if (js)
   hPrintf("%s\n", js->print(js));
}


void createUsrconfigTable(struct sqlConnection *conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE usrConfig (\n");
dyStringPrintf(dy, "id int unsigned not null,\n");
dyStringPrintf(dy, "profile varchar(255) not null,\n");
dyStringPrintf(dy, "db varchar(255) not null,\n");
dyStringPrintf(dy, "classifiersRootDir varchar(255) not null,\n");
dyStringPrintf(dy, "clusterJobPrefix varchar(255) not null,\n");
dyStringPrintf(dy, "task varchar(255) not null,\n");
dyStringPrintf(dy, "inputType varchar(255) not null,\n");
dyStringPrintf(dy, "dataTableName varchar(255) not null,\n");
dyStringPrintf(dy, "clinField varchar(255) not null,\n");
dyStringPrintf(dy, "dataFilepath varchar(255) not null,\n");
dyStringPrintf(dy, "metadataFilepath varchar(255) not null,\n");
dyStringPrintf(dy, "trainingDir varchar(255) not null,\n");
dyStringPrintf(dy, "validationDir varchar(255) not null,\n");
dyStringPrintf(dy, "modelDir varchar(255) not null,\n");
dyStringPrintf(dy, "crossValidation varchar(255) not null,\n");
dyStringPrintf(dy, "folds int unsigned not null,\n");
dyStringPrintf(dy, "foldMultiplier int unsigned not null,\n");
dyStringPrintf(dy, "classifier varchar(255) not null,\n");
dyStringPrintf(dy, "outputType varchar(255) not null,\n");
dyStringPrintf(dy, "parameters varchar(255) not null,\n");
dyStringPrintf(dy, "dataDiscretizer varchar(255) not null,\n");
dyStringPrintf(dy, "dataDiscretizerParameters varchar(255) not null,\n");
dyStringPrintf(dy, "clinDiscretizer varchar(255) not null,\n");
dyStringPrintf(dy, "clinDiscretizerParameters varchar(255) not null,\n");
dyStringPrintf(dy, "featureSelection varchar(255) not null,\n");
dyStringPrintf(dy, "featureSelectionParameters varchar(255) not null,\n");
dyStringPrintf(dy, "modelPath varchar(255) not null,\n");
dyStringPrintf(dy, "PRIMARY KEY(id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn,dy->string);
dyStringFree(&dy);
}


void createRequestQueueTable(struct sqlConnection *conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE requestQueue (\n");
dyStringPrintf(dy, "id int unsigned not null,\n");
dyStringPrintf(dy, "usrConfig_id int unsigned not null,\n");
dyStringPrintf(dy, "status varchar(255) not null,\n");
dyStringPrintf(dy, "estimatedRunTime double not null,\n");
dyStringPrintf(dy, "submissionTime varchar(255) not null,\n");
dyStringPrintf(dy, "runBeginTime varchar(255) not null,\n");
dyStringPrintf(dy, "runEndTime varchar(255) not null,\n");
dyStringPrintf(dy, "PRIMARY KEY(id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn,dy->string);
dyStringFree(&dy);
}


char * writeConfig(int id)
{
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char configFilepath[1024], query[256];
safef(configFilepath, sizeof(configFilepath), "/data/trash/MLuserRequests/model%d.cfg", id);
FILE *fp = fopen(configFilepath, "w");
if(!fp)
	errAbort("Couldn't open file %s for writing a config file to.\n", configFilepath);

fprintf(fp, "name\tmodel%d\n", id);
safef(query, sizeof(query), "SELECT profile FROM usrConfig WHERE id=%d", id);
fprintf(fp, "profile\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT db FROM usrConfig WHERE id=%d", id);
fprintf(fp, "db\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT clusterJobPrefix FROM usrConfig WHERE id=%d", id);
fprintf(fp, "clusterJobPrefix\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT classifiersRootDir FROM usrConfig WHERE id=%d", id);
fprintf(fp, "classifiersRootDir\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT task FROM usrConfig WHERE id=%d", id);
fprintf(fp, "task\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT inputType FROM usrConfig WHERE id=%d", id);
fprintf(fp, "inputType\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT dataTableName FROM usrConfig WHERE id=%d", id);
fprintf(fp, "tableName\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT clinField FROM usrConfig WHERE id=%d", id);
fprintf(fp, "clinField\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT dataFilepath FROM usrConfig WHERE id=%d", id);
fprintf(fp, "dataFilepath\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT metadataFilepath FROM usrConfig WHERE id=%d", id);
fprintf(fp, "metadataFilepath\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT trainingDir FROM usrConfig WHERE id=%d", id);
fprintf(fp, "trainingDir\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT validationDir FROM usrConfig WHERE id=%d", id);
fprintf(fp, "validationDir\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT modelDir FROM usrConfig WHERE id=%d", id);
fprintf(fp, "modelDir\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT crossValidation FROM usrConfig WHERE id=%d", id);
fprintf(fp, "crossValidation\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT folds FROM usrConfig WHERE id=%d", id);
fprintf(fp, "folds\t%d\n", sqlNeedQuickNum(conn, query));
safef(query, sizeof(query), "SELECT foldMultiplier FROM usrConfig WHERE id=%d", id);
fprintf(fp, "foldMultiplier\t%d\n", sqlNeedQuickNum(conn, query));
safef(query, sizeof(query), "SELECT classifier FROM usrConfig WHERE id=%d", id);
fprintf(fp, "classifier\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT outputType FROM usrConfig WHERE id=%d", id);
fprintf(fp, "outputType\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT parameters FROM usrConfig WHERE id=%d", id);
fprintf(fp, "parameters\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT dataDiscretizer FROM usrConfig WHERE id=%d", id);
fprintf(fp, "dataDiscretizer\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT dataDiscretizerParameters FROM usrConfig WHERE id=%d", id);
fprintf(fp, "dataDiscretizerParameters\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT clinDiscretizer FROM usrConfig WHERE id=%d", id);
fprintf(fp, "clinDiscretizer\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT clinDiscretizerParameters FROM usrConfig WHERE id=%d", id);
fprintf(fp, "clinDiscretizerParameters\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT featureSelection FROM usrConfig WHERE id=%d", id);
fprintf(fp, "featureSelection\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT featureSelectionParameters FROM usrConfig WHERE id=%d", id);
fprintf(fp, "featureSelectionParameters\t%s\n", sqlNeedQuickString(conn, query));
safef(query, sizeof(query), "SELECT modelPath FROM usrConfig WHERE id=%d", id);
fprintf(fp, "modelPath\t%s\n", sqlNeedQuickString(conn, query));
fclose(fp);

return cloneString(configFilepath);
}

void viewSubmissionStatus()
{
char * requestId = cartOptionalString(cart, rqRequestId);
if(!requestId)
    errAbort("%s is required", rqRequestId);

struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];
safef(query, sizeof(query), "SELECT status FROM requestQueue WHERE id=%s", requestId);
char * status = sqlNeedQuickString(conn, query);

struct json *js = newJson();
jsonAddString(js, "requestId", requestId);
jsonAddString(js, "status", status);
safef(query, sizeof(query), "SELECT estimatedRunTime FROM requestQueue WHERE id=%s", requestId);
jsonAddDouble(js, "estimatedRunTime", sqlQuickNum(conn, query));
//calc the elapsed time
if(sameString(status, "Unsubmitted"))
    {
    safef(query, sizeof(query), "SELECT TIME_TO_SEC(TIMEDIFF(NOW(), submissionTime)) FROM requestQueue WHERE id=%s", requestId);
    jsonAddInt(js, "elapsedTime", sqlNeedQuickNum(conn, query));
    }
else if(sameString(status, "Processing"))
    {
    safef(query, sizeof(query), "SELECT TIME_TO_SEC(TIMEDIFF(NOW(), runBeginTime)) FROM requestQueue WHERE id=%s", requestId);
    jsonAddInt(js, "elapsedTime", sqlNeedQuickNum(conn, query));
    }
else if(sameString(status, "Processed"))
    {
    safef(query, sizeof(query), "SELECT TIME_TO_SEC(TIMEDIFF(runEndTime, submissionTime)) FROM requestQueue WHERE id=%s", requestId);
    jsonAddInt(js, "elapsedTime", sqlNeedQuickNum(conn, query));
    }
else if(sameString(status, "Error"))
    {
    safef(query, sizeof(query), "SELECT TIME_TO_SEC(TIMEDIFF(NOW(),submissionTime)) FROM requestQueue WHERE id=%s", requestId);
    jsonAddInt(js, "elapsedTime", sqlNeedQuickNum(conn, query));
	}
if (js)
   hPrintf("%s\n", js->print(js));
hFreeConn(&conn);
}

void runRequest()
/*Attempt to run the job from the queue with id*/
{
char * requestId = cartOptionalString(cart, rqRequestId);
if(!requestId)
    errAbort("%s is required", rqRequestId);
int id = atoi(requestId);
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];
safef(query, sizeof(query), "UPDATE requestQueue SET status=\'Processing\', runBeginTime=NOW() WHERE id=%d", id);
sqlUpdate(conn, query);
safef(query, sizeof(query), "SELECT usrConfig_id FROM requestQueue WHERE id=%d", id);
int usrConfig_id = sqlNeedQuickNum(conn, query);
char * configFilepath = writeConfig(usrConfig_id);
struct hash * config = raReadSingle(configFilepath);

//run MLsetup (as functions here)
if(!dataExists(config))
    splitData(config);
createModelDirs(config);
writeClusterJobs(config);

//run the "cluster jobs" from system calls here
char *clusterJobPrefix = (char*)hashMustFindVal(config, "clusterJobPrefix");
char clusterJobFile[1024];
safef(clusterJobFile, sizeof(clusterJobFile), "%s_trainingJobList.txt", clusterJobPrefix);
FILE * fp = fopen(clusterJobFile, "r");
if(!fp)
	errAbort("Couldn't open cluster job file %s for reading\n", clusterJobFile);
char * line;
while((line = readLine(fp)) && line != NULL)
	system(line);
fclose(fp);
safef(clusterJobFile, sizeof(clusterJobFile), "%s_validationJobList.txt", clusterJobPrefix);
fp = fopen(clusterJobFile, "r");
if(!fp)
    errAbort("Couldn't open cluster job file %s for reading\n", clusterJobFile);
while((line = readLine(fp)) && line != NULL)
    system(line);
fclose(fp);

//run MLwriteResultsToRa (as functions here)
char outputRa[1024];
safef(outputRa, sizeof(outputRa), "%s_results.ra", clusterJobPrefix);
printTasksRa(config, outputRa, usrConfig_id);
printClassifiersRa(config, outputRa, usrConfig_id);
printSubgroupsRa(config, outputRa, usrConfig_id);
printTasksRa(config, outputRa, usrConfig_id);
printFeatureSelectionsRa(config, outputRa, usrConfig_id);
printTransformationsRa(config, outputRa, usrConfig_id);
printJobsRa(config, outputRa, usrConfig_id);

//Run loadClassificationsTables (as function here)
populateClassificationTables(profile, db, outputRa, 0);

//set status to processed
safef(query, sizeof(query), "UPDATE requestQueue SET status=\'Processed\', runEndTime=NOW() WHERE id=%d", id);
sqlUpdate(conn, query);

//clean up
removeDataDirs(config);
safef(clusterJobFile, sizeof(clusterJobFile), "%s_trainingJobList.txt", clusterJobPrefix);
remove(clusterJobFile);
safef(clusterJobFile, sizeof(clusterJobFile), "%s_validationJobList.txt", clusterJobPrefix);
remove(clusterJobFile);
safef(clusterJobFile, sizeof(clusterJobFile), "%s_results.ra", clusterJobPrefix);
remove(clusterJobFile);
freeHash(&config);
hFreeConn(&conn);

viewSubmissionStatus();
}

void submitRequest()
{
struct sqlConnection *conn = hAllocConnProfile(profile, db);
if(!sqlTableExists(conn, "requestQueue"))
	createRequestQueueTable(conn);
if(!sqlTableExists(conn, "usrConfig"))
	createUsrconfigTable(conn);

int rq_id = sqlQuickNum(conn, "SELECT MAX(id) FROM requestQueue;")+1;
int cfg_id = sqlQuickNum(conn, "SELECT MAX(id) FROM usrConfig;")+1;

//creat and save the config to db
struct usrConfig * cfg;
AllocVar(cfg);
cfg->id = cfg_id;
cfg->profile = cloneString(cartOptionalString(cart, rqProfile));
cfg->db = cloneString(cartOptionalString(cart, rqDb));
cfg->classifiersRootDir=cloneString("/hive/users/cszeto/bin");
char tmp[256];
safef(tmp, sizeof(tmp), "/data/trash/MLuserRequests/model%d", cfg_id);
cfg->clusterJobPrefix=cloneString(tmp);
cfg->task = cloneString(cartOptionalString(cart, rqTask));
cfg->inputType = cloneString(cartOptionalString(cart, rqInputType));
cfg->dataTableName = cloneString(cartOptionalString(cart, rqTableName));
cfg->clinField = cloneString(cartOptionalString(cart, rqClinField));
cfg->dataFilepath = cloneString(cartOptionalString(cart, rqDataFilepath));
cfg->metadataFilepath = cloneString(cartOptionalString(cart, rqMetadataFilepath));
safef(tmp, sizeof(tmp), "/data/trash/MLuserRequests/model%d_trainingDir", cfg_id);
cfg->trainingDir = cloneString(tmp);
safef(tmp, sizeof(tmp), "/data/trash/MLuserRequests/model%d_validationDir", cfg_id);
cfg->validationDir = cloneString(tmp);
safef(tmp, sizeof(tmp), "/data/trash/MLuserRequests/model%d_modelDir", cfg_id);
cfg->modelDir = cloneString(tmp);
cfg->crossValidation = cloneString(cartOptionalString(cart, rqCrossValidation));
int folds = atoi(cartOptionalString(cart, rqFolds));
cfg->folds = folds;
int foldMultiplier = atoi(cartOptionalString(cart, rqFoldMultiplier));
cfg->foldMultiplier = foldMultiplier;
cfg->classifier = cloneString(cartOptionalString(cart, rqClassifier));
cfg->outputType = cloneString(cartOptionalString(cart, rqOutputType));
cfg->parameters = cloneString(cartOptionalString(cart, rqParameters));
cfg->dataDiscretizer = cloneString(cartOptionalString(cart, rqDataDiscretizer));
cfg->dataDiscretizerParameters = cloneString(cartOptionalString(cart, rqDataDiscretizerParameters));
cfg->clinDiscretizer = cloneString(cartOptionalString(cart, rqClinDiscretizer));
cfg->clinDiscretizerParameters = cloneString(cartOptionalString(cart, rqClinDiscretizerParameters));
cfg->featureSelection = cloneString(cartOptionalString(cart, rqFeatureSelection));
cfg->featureSelectionParameters = cloneString(cartOptionalString(cart, rqFeatureSelectionParameters));
if(sameString(cfg->classifier, "NMFpredictor") || sameString(cfg->classifier, "glmnet"))
	safef(tmp, sizeof(tmp), "/data/trash/MLuserRequests/model%d_modelDir/model.tab", cfg_id);
else if(sameString(cfg->classifier, "SVMlight"))
    safef(tmp, sizeof(tmp), "/data/trash/MLuserRequests/model%d_modelDir/model.svm", cfg_id);
else if(sameString(cfg->classifier, "WEKA"))
    safef(tmp, sizeof(tmp), "/data/trash/MLuserRequests/model%d_modelDir/model.weka", cfg_id);
else
	safef(tmp, sizeof(tmp), "NULL");
cfg->modelPath = cloneString(tmp);
usrConfigSaveToDbEscaped(conn, cfg, "usrConfig", 2048);
usrConfigFree(&cfg);

//Get a time estimate on this job
char query[256];
safef(query, sizeof(query), "SELECT id FROM datasets WHERE data_table='%s'",
    cartOptionalString(cart, rqTableName));
int dataset_id=sqlQuickNum(conn, query);
safef(query, sizeof(query), "SELECT COUNT(DISTINCT patient_id) FROM samples WHERE dataset_id=%d;", dataset_id);
int samples = sqlQuickNum(conn, query);
int features = 0;
if(sameString(cartOptionalString(cart, rqFeatureSelection), "None"))
	{
    safef(query, sizeof(query), "SELECT COUNT(DISTINCT feature_id) FROM %s",
        cartOptionalString(cart, rqTableName));
    features = sqlQuickNum(conn, query);
	}
else
	{
    features = 1000; //hack - most of the time there'll be about 1000. Consider revising
	}
double estimatedRunTime = (samples * features * folds * foldMultiplier) / 12000;
//create and save the request to db
struct requestQueue * request;
AllocVar(request);
request->id=rq_id;
request->usrConfig_id=cfg_id;
request->status = cloneString("Unsubmitted");
request->estimatedRunTime = estimatedRunTime;
request->submissionTime = cloneString(sqlQuickString(conn, "SELECT NOW()"));
request->runBeginTime = cloneString("");
request->runEndTime = cloneString("");
requestQueueSaveToDbEscaped(conn, request, "requestQueue", 2048);
requestQueueFree(&request);
//return a json of the id for this request
struct json *js = newJson();
jsonAddInt(js, "requestId", rq_id);
jsonAddDouble(js, "estimatedRunTime", estimatedRunTime);
if (js)
   hPrintf("%s\n", js->print(js));
//clean up
hFreeConn(&conn);
}

void dispatchRoutines()
/* Look at command variables in cart and figure out which
 * page to draw. */
{
/* retrieve cart variables, handle various modes */
char *mode = cartOptionalString(cart, rqMode);
if (!mode)
    errAbort("%s is required.", rqMode);

if (sameString(mode, "listDatasets"))
    listDatasets();
else if(sameString(mode, "listTasks"))
    listTasks();
else if(sameString(mode, "viewSubgroups"))
	viewSubgroups();
else if(sameString(mode, "submitRequest"))
	submitRequest();
else if(sameString(mode, "runRequest"))
	runRequest();
else if(sameString(mode, "viewSubmissionStatus"))
	viewSubmissionStatus();
else
    errAbort("Incorrect mode = %s", mode);

cartRemovePrefix(cart, rqPrefix);
}

void hghDoUsualHttp()
/* Wrap html page dispatcher with code that writes out
 * HTTP header and write cart back to database. */
{
cartWriteCookie(cart, hUserCookie());
printf("Content-Type:application/x-javascript\r\n\r\n");

/* Dispatch other pages, that actually want to write HTML. */
cartWarnCatcher(dispatchRoutines, cart, jsonEarlyWarningHandler);
cartCheckout(&cart);
}

char *excludeVars[] = {"Submit", "submit", NULL};

int main(int argc, char *argv[])
/* Process command line. */
{
htmlPushEarlyHandlers();
cgiSpoof(&argc, argv);
htmlSetStyle(htmlStyleUndecoratedLink);
cart = cartForSession(hUserCookie(), excludeVars, oldVars);

hghDoUsualHttp();
return 0;
}

