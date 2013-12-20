#include "common.h"
#include "hui.h"
#include "web.h"
#include "htmshell.h"
#include "json.h"
#include "hgClassifications.h"
#include "../inc/classificationTables.h"
#include "../inc/populateClassificationTables.h"
#include "../inc/MLmatrix.h"
#include "../inc/MLcrossValidation.h"
#include "../inc/MLio.h"
#include "../inc/MLreadConfig.h"
#include "../inc/MLfilters.h"
#include "../inc/MLextractCoefficients.h"

/* ---- Necessary prototypes ----*/
void hghDoUsualHttp();

/* ---- Global variables. ---- */
struct cart *cart;	         /* This holds cgi and other variables between clicks. */
struct hash *oldVars;	         /* Old cart hash. */

char *db = "paradigmko";
char *profile = "paradigmKO";

void usage()
/* Explain usage and exit. */
{
errAbort(
  "hgClassifications cgi\n"
  "usage:\n"
  "   hgClassifications\n"
  );
}

/****** BEGIN HELPER FUNCTIONS *******/
double roundIt(double x, double n)
{ 
    x = floor( x * pow(10.0, n) + 0.5) / pow(10.0, n);
    return x;
}

static int featureWeightCmp(const void *va, const void *vb)
/*Compare based on abs weight of features in slPair*/
{
const struct slPair *a = *((struct slPair **)va);
const struct slPair *b = *((struct slPair **)vb);
if(fabs(atof(a->val)) > fabs(atof(b->val)))
	return 1;
return -1;
}

struct jobs *getJobsByTaskId(struct sqlConnection *conn, int tasks_id)
{
char query[256];
safef(query, sizeof(query),
	"select * from jobs where tasks_id = %d", tasks_id);
return jobsLoadByQuery(conn, query);
}

char * getStringFromTableById(struct sqlConnection *conn, char *field, char *table, int id)
{
char query[256];
safef(query, sizeof(query),
	"select %s from %s where id=%d", field, table, id);
return sqlQuickString(conn, query);
}

void loadTasks()
{
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];
safef(query, sizeof(query), "select * from tasks;");
struct tasks *ta, *taList = tasksLoadByQuery(conn, query);
struct json *js = newJson();
struct json *task, *tasks = jsonAddContainerList(js, "tasks");
task = tasks;
for (ta = taList; ta; ta = ta->next)
	{
	jsonAddInt(task, "id", ta->id);
	safef(query, sizeof(query), "select longLabel from features where name='%s'", ta->name);
	char * longLabel = sqlQuickNonemptyString(conn,query);
	if(longLabel != NULL)
		jsonAddString(task, "name", longLabel);
	else
		jsonAddString(task, "name", ta->name);

   	if (ta->next)
       task = jsonAddContainerToList(&tasks);
	}
if (js)
   hPrintf("%s\n", js->print(js));
hFreeConn(&conn);
}

void loadTaskDetails()
{
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];
char *taskIdString = cartOptionalString(cart, clTaskId);
if (!clTaskId)
	errAbort("%s is required", clTaskId);
int taskId = atoi(taskIdString);

//createt the json that will be added to and returned
struct json *js = newJson();
struct json *job, *jobs = jsonAddContainerList(js, "jobs");
job = jobs;

jsonAddInt(js, "id", taskId);
//Quuery out a list of jobs and iterate over
struct jobs *j, *jList = getJobsByTaskId(conn, taskId);
for (j = jList; j; j = j->next)
	{
	jsonAddInt(job, "id", j->id);
	jsonAddString(job, "dataset",
	     getStringFromTableById(conn, "name", "datasets", j->datasets_id));
	jsonAddString(job, "classifier", 
		getStringFromTableById(conn, "name", "classifiers", j->classifiers_id));
	jsonAddString(job, "classifierParameters", 
		getStringFromTableById(conn, "parameters", "classifiers", j->classifiers_id));
	jsonAddString(job, "featureSelection", 
		getStringFromTableById(conn, "name", "featureSelections", j->featureSelections_id));
	jsonAddString(job, "featureSelectionParameters", 
	    getStringFromTableById(conn, "parameters", "featureSelections", j->featureSelections_id));
	jsonAddString(job, "M", 
		getStringFromTableById(conn, "featureCount", "featureSelections", j->featureSelections_id));
    jsonAddString(job, "transformation",
        getStringFromTableById(conn, "name", "transformations", j->transformations_id));
    jsonAddString(job, "transformationParameters",
        getStringFromTableById(conn, "parameters", "transformations", j->transformations_id));
	jsonAddString(job, "subgrouping", 
		getStringFromTableById(conn, "name", "subgroups", j->subgroups_id));
	jsonAddString(job, "subgroupingParameters",
	    getStringFromTableById(conn, "parameters", "subgroups", j->subgroups_id));
	safef(query, sizeof(query), "SELECT count(*) FROM samplesInSubgroups WHERE subgroups_id=%d", j->subgroups_id);
	int N = sqlQuickNum(conn, query);
	jsonAddInt(job, "N", N);
	safef(query, sizeof(query), "SELECT avgTestingAccuracy FROM jobs WHERE id=%d;", j->id);
	double avgAcc = sqlQuickDouble(conn, query)*100;
	jsonAddDouble(job, "accuracy", avgAcc);
    safef(query, sizeof(query), "SELECT avgTestingAccuracyGain FROM jobs WHERE id=%d;", j->id);
	double avgAccGain = sqlQuickDouble(conn, query)*100;
	jsonAddDouble(job, "accuracyGain", avgAccGain);
	jsonAddString(job, "accuracyType", j->accuracyType);
	safef(query, sizeof(query), "SELECT count(*) FROM jobs WHERE id=%d AND features != 'NULL';", j->id);
	if(sqlQuickNum(conn, query))
		jsonAddInt(job, "featuresAvailable", 1);
	else
		jsonAddInt(job, "featuresAvailable", 0);

	if (j->next)
   		job = jsonAddContainerToList(&jobs);
	}
if (js)
   hPrintf("%s\n", js->print(js));
hFreeConn(&conn);
}

void summarizeFeatures()
/*Create a JSON object of samples*/
{
char *taskId = cartOptionalString(cart, clTaskId);
if (!taskId)
    errAbort("%s is required", clTaskId);
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];
//make a list of features by iterating jobs in this task
struct slPair *f, *fs = NULL;
safef(query, sizeof(query), "SELECT features,weights,avgTestingAccuracy FROM jobs WHERE tasks_id=%s",taskId);
struct sqlResult * featuresAndAccResults = sqlMustGetResult(conn, query);
char ** queryResultRow;
while((queryResultRow = sqlNextRow(featuresAndAccResults)) && (queryResultRow != NULL))
	{
	char * featureListString = queryResultRow[0];
	if(!sameString(featureListString, "NULL"))
		{
		char * weightListString = queryResultRow[1];
		struct slName * feature, *featureList = slNameListFromComma(featureListString);
		struct slName * weight, *weightList = slNameListFromComma(weightListString);
		weight = weightList;
		double acc = atof(queryResultRow[2]);
		for(feature = featureList; feature; feature = feature->next)
			{
			//if it exists in the list add the new val, otherwise add it to list
			if((f = slPairFind(fs, feature->name)) != NULL)
				{
				char tmp[256];
				safef(tmp, sizeof(tmp), "%.4f", (atof(weight->name) *acc) + atof(f->val));
				f->val = cloneString(tmp);
				}
			else
				{
				char tmp[256];
				safef(tmp, sizeof(tmp), "%.4f", (atof(weight->name)*acc));
				slPairAdd(&fs, feature->name, cloneString(tmp));
				}
			weight = weight->next;
			}
			slFreeList(&featureList);
			slFreeList(&weightList);
		}
	}
sqlFreeResult(&featuresAndAccResults);
//sort the list descending abs val
slSort(&fs, featureWeightCmp);
slReverse(&fs);

//make json of top 200
struct json *js = newJson();
struct json * feature, * features = jsonAddContainerList(js, "features");
feature = features;

jsonAddString(js, "taskId", taskId);
int count = 0;
for(f = fs; f && count < 200; f=f->next)
	{
	//add to json
    jsonAddString(feature, "feature", f->name);
    jsonAddDouble(feature, "weight", atof(f->val));

	count++;
    if(f->next && count < 200)
        feature = jsonAddContainerToList(&features);
	}
slPairFreeList(&fs);
if (js)
    hPrintf("%s\n", js->print(js));
hFreeConn(&conn);
}

void viewFeatures()
/*Create a JSON object of top features*/
{
char *jobId = cartOptionalString(cart, clJobId);
if (!jobId)
    errAbort("%s is required", clJobId);

struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];
safef(query, sizeof(query), "SELECT features FROM jobs WHERE id = %s;", jobId);
char * featuresString = sqlNeedQuickString(conn, query);
struct slName *f,*fs = slNameListFromComma(featuresString);
safef(query, sizeof(query), "SELECT weights FROM jobs WHERE id = %s;", jobId);
char * weightsString = sqlNeedQuickString(conn, query);
struct slName *w, *ws = slNameListFromComma(weightsString);
w = ws;

//Set up a "features" container for the JSON
struct json *js = newJson();
struct json *feature, *features = jsonAddContainerList(js, "features");
feature = features;

//tag in the task name (for linking to the cancer browser)
safef(query, sizeof(query), "SELECT tasks_id FROM jobs WHERE id=%s", jobId);
int tasks_id= sqlQuickNum(conn, query);
safef(query, sizeof(query), "SELECT name FROM tasks WHERE id=%d", tasks_id);
jsonAddString(js, "task", sqlQuickString(conn, query));

//give a rank estimate of this model (for naming purposes)
//safef(query, sizeof(query), "SELECT count(*) FROM jobs WHERE tasks_id=%d AND avgTestingAccuracyGain > (SELECT avgTestingAccuracyGain FROM jobs WHERE id=%s)", tasks_id, jobId);
//int rank = sqlQuickNum(conn, query)+1;
//jsonAddInt(js, "rank", rank);

//tag in the dataset name (for linking to cancer browser)
safef(query, sizeof(query), "SELECT datasets_id FROM jobs WHERE id=%s", jobId);
int datasets_id = sqlQuickNum(conn, query);
if(sameString(profile, "paradigmKO")){
    safef(query, sizeof(query), "SELECT name FROM datasets WHERE id=%d", datasets_id);
}else{
	safef(query, sizeof(query), "SELECT data_table FROM datasets WHERE id=%d", datasets_id);
}
jsonAddString(js, "dataset", sqlQuickString(conn, query));

//get feature counts
safef(query, sizeof(query),	"SELECT featureCount FROM featureSelections JOIN "
							"jobs ON featureSelections.id=jobs.featureSelections_id "
							"WHERE jobs.id=%s", jobId);
jsonAddInt(js, "featureCount", sqlQuickNum(conn, query));

//iterate over the slName list, saving the key-value pairs to json
for (f = fs; f || w; f = f->next)
 	{
	if((f && !w) || (w && !f))
		errAbort("job %s has uneven number of weights to features reported.\n", jobId);

    jsonAddString(feature, "feature", f->name);
    jsonAddDouble(feature, "weight", atof(w->name));

    if(f->next)
        feature = jsonAddContainerToList(&features);

	w = w->next;
	}
if (js)
	hPrintf("%s\n", js->print(js));
hFreeConn(&conn);
}

void summarizeSamples()
/*Create a JSON object of samples*/
{
char *taskId = cartOptionalString(cart, clTaskId);
if (!taskId)
    errAbort("%s is required", clTaskId);
//create json to save results in
struct json *js = newJson();
jsonAddString(js, "taskId", taskId);
struct json *sample, *samples = jsonAddContainerList(js, "samples");
sample = samples;
//the following is hardcoded to two subgroups to speed things up. TODO: generalize this
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[1024];
safef(query, sizeof(query), "SELECT samples.name,avgTrainingAcc,avgTestingAcc,IFNULL(sg1count,0),IFNULL(sg2count,0) "
							"FROM (SELECT sample_id,avg(trainingAccuracy) as avgTrainingAcc,avg(testingAccuracy) as avgTestingAcc "
							"FROM classifierResults WHERE tasks_id=%s GROUP BY sample_id) AS tbl1 "
							"LEFT OUTER JOIN (SELECT sample_id,count(*) as sg1count FROM samplesInSubgroups "
							"JOIN jobs ON (jobs.subgroups_id=samplesInSubgroups.subgroups_id) "
							"WHERE subgroup=1 AND tasks_id=%s GROUP BY sample_id) AS tbl2 "
							"ON (tbl1.sample_id=tbl2.sample_id) LEFT OUTER JOIN (SELECT sample_id,count(*) AS sg2count "
							"FROM samplesInSubgroups JOIN jobs ON (jobs.subgroups_id=samplesInSubgroups.subgroups_id) "
							"WHERE subgroup=2 AND tasks_id=%s GROUP BY sample_id) AS tbl3 "
							"ON (tbl1.sample_id=tbl3.sample_id) JOIN samples ON (samples.id=tbl1.sample_id) "
							"GROUP BY samples.name", taskId, taskId, taskId);
fprintf(stderr, "%s\n", query);
struct  sqlResult * sampleAccuracies = sqlGetResult(conn, query);
//start iterators of results
char ** accuracyResultRow = sqlNextRow(sampleAccuracies);
while(accuracyResultRow)
	{
	jsonAddString(sample, "name", accuracyResultRow[0]);
	jsonAddDouble(sample, "trainingValue", 100*atof(accuracyResultRow[1]));
	jsonAddDouble(sample, "testingValue", 100*atof(accuracyResultRow[2]));
	//save the counts for each subgroup
	struct json *sgCount, *sgCountList = jsonAddContainerList(sample, "subgroupCounts");
	sgCount = sgCountList;
	jsonAddInt(sgCount, "subgroup", 1);
	jsonAddInt(sgCount, "count", atoi(accuracyResultRow[3]));
	sgCount = jsonAddContainerToList(&sgCountList);
	jsonAddInt(sgCount, "subgroup", 2);
    jsonAddInt(sgCount, "count", atoi(accuracyResultRow[4]));
	//advance row pointers
	accuracyResultRow = sqlNextRow(sampleAccuracies);
    if(accuracyResultRow)
    	sample = jsonAddContainerToList(&samples);
	}
if(js)
   hPrintf("%s\n", js->print(js));
//clean up
sqlFreeResult(&sampleAccuracies);
hFreeConn(&conn);
}

void viewSamples()
/*Create a JSON object of the job data*/
{
char *jobId = cartOptionalString(cart, clJobId);
if (!jobId)
    errAbort("%s is required", clJobId);

struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[2048];
struct json *js = newJson();
struct json *subgroup, *subgroups = jsonAddContainerList(js, "subgroups");
subgroup = subgroups;
safef(query, sizeof(query), "SELECT subgroups_id FROM jobs WHERE id=%s",jobId);
int subgroupsId = sqlNeedQuickNum(conn, query);
safef(query, sizeof(query), "SELECT DISTINCT subgroup FROM samplesInSubgroups "
							"WHERE subgroups_id=%d", subgroupsId);
struct slInt *sg, * sgList = sqlQuickNumList(conn, query);
for(sg = sgList; sg; sg=sg->next)
	{
	//query what you need
//FIX THIS
	safef(query, sizeof(query), "SELECT COUNT(*) FROM samplesInSubgroups WHERE subgroups_id=%d AND subgroup=%d", subgroupsId, sg->val);
	int samplesInSubgroup = sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "SELECT samples.name,trainingAccuracy,testingAccuracy "
								"FROM (SELECT jobs_id,sample_id,trainingAccuracy,testingAccuracy FROM classifierResults "
								"WHERE jobs_id=%s) AS tbl1 JOIN (SELECT sample_id,subgroup FROM samplesInSubgroups "
								"WHERE subgroups_id=%d and subgroup=%d) AS tbl2 ON (tbl1.sample_id=tbl2.sample_id) "
								"JOIN samples ON (samples.id=tbl1.sample_id) GROUP BY samples.name",
								jobId,subgroupsId,sg->val);
	struct sqlResult * samplesInSubgroupList =  sqlMustGetResult(conn, query);
	if(!samplesInSubgroupList)
		errAbort("Couldn't get query results\n");
	int ommitted = 0; //relic: Consider removing.
 	//create json container for this set of samples
    struct json *sample = NULL, *samples = NULL;
    samples = jsonAddContainerList(subgroup, "samples");
    sample = samples;
	//iterate over rows(i.e. samples in this subgroup) adding them to subgroup json container
	char ** resultRow = sqlNextRow(samplesInSubgroupList);
	while(resultRow != NULL)
		{
		jsonAddString(sample, "name", resultRow[0]);
		jsonAddDouble(sample, "trainingAccuracy", 100*atof(resultRow[1]));
		jsonAddDouble(sample, "testingAccuracy", 100*atof(resultRow[2]));
		resultRow = sqlNextRow(samplesInSubgroupList);
		if(resultRow)
			sample = jsonAddContainerToList(&samples);
		}
	jsonAddInt(subgroup, "subgroup", sg->val);
	jsonAddInt(subgroup, "count", samplesInSubgroup);
    jsonAddInt(subgroup, "ommittedSamples", ommitted);
	if(sg->next)
		subgroup = jsonAddContainerToList(&subgroups);
    sqlFreeResult(&samplesInSubgroupList);
	}
if (js)
   hPrintf("%s\n", js->print(js));
slFreeList(&sgList);
hFreeConn(&conn);
}

void summarizeClassifiers()
/*Create a JSON object of classifiers stats (high, median, low)*/
{
char *taskId = cartOptionalString(cart, clTaskId);
if (!taskId)
    errAbort("%s is required", clTaskId);

struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];

safef(query, sizeof(query), "SELECT DISTINCT classifiers_id FROM jobs WHERE tasks_id=%s", taskId);
struct slInt * classifierId, *classifierIdList = sqlQuickNumList(conn, query);

struct json *js = newJson();
struct json *classifier, *classifiers = jsonAddContainerList(js, "classifiers");
classifier = classifiers;

jsonAddString(js, "taskId", taskId);
for(classifierId = classifierIdList; classifierId; classifierId=classifierId->next)
	{
	safef(query, sizeof(query), "SELECT name FROM classifiers WHERE id=%d", classifierId->val);
	jsonAddString(classifier, "name", sqlNeedQuickString(conn, query));
    safef(query, sizeof(query), "SELECT parameters FROM classifiers WHERE id=%d", classifierId->val);
    jsonAddString(classifier, "parameters", sqlNeedQuickString(conn, query));

	safef(query, sizeof(query), "SELECT id FROM jobs WHERE tasks_id=%s AND classifiers_id=%d", taskId, classifierId->val);
	struct slInt *jobId, *jobIdList = sqlQuickNumList(conn, query);
	struct slDouble *avgAccs = NULL;
	struct slDouble *avgAccGains = NULL;
	for(jobId = jobIdList; jobId; jobId=jobId->next)
    	{
 		safef(query, sizeof(query), "SELECT avgTestingAccuracy FROM jobs WHERE id=%d", jobId->val);
		struct slDouble * tmp = slDoubleNew(sqlQuickDouble(conn, query));
		slSafeAddHead(&avgAccs, tmp);
	
		safef(query, sizeof(query), "SELECT avgTestingAccuracyGain FROM jobs WHERE id=%d", jobId->val);
        struct slDouble * tmp2 = slDoubleNew(sqlQuickDouble(conn, query));
        slSafeAddHead(&avgAccGains, tmp2);
		}
	double min, q1, median, q3, max;
	slDoubleBoxWhiskerCalc(avgAccs, &min, &q1, &median, &q3, &max);
	jsonAddDouble(classifier, "min", min*100);
    jsonAddDouble(classifier, "median", median*100);
    jsonAddDouble(classifier, "max", max*100);
	
	slDoubleBoxWhiskerCalc(avgAccGains, &min, &q1, &median, &q3, &max);
    jsonAddDouble(classifier, "minGain", min*100);
    jsonAddDouble(classifier, "medianGain", median*100);
    jsonAddDouble(classifier, "maxGain", max*100);

    if(classifierId->next)
        classifier = jsonAddContainerToList(&classifiers);
	slFreeList(&jobIdList);
	}
if (js)
   hPrintf("%s\n", js->print(js));
slFreeList(&classifierIdList);
hFreeConn(&conn);
}

void summarizeFeatureSelections()
/*Create a JSON object of classifiers stats (high, median, low)*/
{
char *taskId = cartOptionalString(cart, clTaskId);
if (!taskId)
    errAbort("%s is required", clTaskId);

struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];

safef(query, sizeof(query), "SELECT DISTINCT featureSelections_id FROM jobs WHERE tasks_id=%s", taskId);
struct slInt * featureSelectionId, *featureSelectionIdList = sqlQuickNumList(conn, query);

struct json *js = newJson();
struct json *featureSelection, *featureSelections = jsonAddContainerList(js, "featureSelections");
featureSelection = featureSelections;

jsonAddString(js, "taskId", taskId);
for(featureSelectionId = featureSelectionIdList; featureSelectionId; featureSelectionId=featureSelectionId->next)
    {
    safef(query, sizeof(query), "SELECT name FROM featureSelections WHERE id=%d", featureSelectionId->val);
    jsonAddString(featureSelection, "name", sqlNeedQuickString(conn, query));
    safef(query, sizeof(query), "SELECT parameters FROM featureSelections WHERE id=%d", featureSelectionId->val);
    jsonAddString(featureSelection, "parameters", sqlNeedQuickString(conn, query));
    safef(query, sizeof(query), "SELECT datasets_id FROM featureSelections WHERE id=%d", featureSelectionId->val);
	int datasetsId = sqlNeedQuickNum(conn, query);
	if(sameString(profile, "paradigmKO")){
    	safef(query, sizeof(query), "SELECT name FROM datasets WHERE id=%d", datasetsId);
	}else{
    	safef(query, sizeof(query), "SELECT data_table FROM datasets WHERE id=%d", datasetsId);
	}
	jsonAddString(featureSelection, "dataset", sqlNeedQuickString(conn, query)); 

    safef(query, sizeof(query), "SELECT id FROM jobs WHERE tasks_id=%s AND featureSelections_id=%d AND datasets_id=%d", taskId, featureSelectionId->val, datasetsId);
    struct slInt *jobId, *jobIdList = sqlQuickNumList(conn, query);
    struct slDouble *avgAccs = NULL;
    struct slDouble *avgAccGains = NULL;
    for(jobId = jobIdList; jobId; jobId=jobId->next)
        {
        safef(query, sizeof(query), "SELECT avgTestingAccuracy FROM jobs WHERE id=%d", jobId->val);
        struct slDouble * tmp = slDoubleNew(sqlQuickDouble(conn, query));
        slSafeAddHead(&avgAccs, tmp);

        safef(query, sizeof(query), "SELECT avgTestingAccuracyGain FROM jobs WHERE id=%d", jobId->val);
        struct slDouble * tmp2 = slDoubleNew(sqlQuickDouble(conn, query));
        slSafeAddHead(&avgAccGains, tmp2);
        }
    double min, q1, median, q3, max;
    slDoubleBoxWhiskerCalc(avgAccs, &min, &q1, &median, &q3, &max);
    jsonAddDouble(featureSelection, "min", min*100);
    jsonAddDouble(featureSelection, "median", median*100);
    jsonAddDouble(featureSelection, "max", max*100);

    slDoubleBoxWhiskerCalc(avgAccGains, &min, &q1, &median, &q3, &max);
    jsonAddDouble(featureSelection, "minGain", min*100);
    jsonAddDouble(featureSelection, "medianGain", median*100);
    jsonAddDouble(featureSelection, "maxGain", max*100);

    if(featureSelectionId->next)
        featureSelection = jsonAddContainerToList(&featureSelections);
    slFreeList(&jobIdList);
    }
if (js)
   hPrintf("%s\n", js->print(js));
slFreeList(&featureSelectionIdList);
hFreeConn(&conn);
}

void summarizeDatasets()
/*Create a JSON object of datasets stats (high, median, low)*/
{
char *taskId = cartOptionalString(cart, clTaskId);
if (!taskId)
    errAbort("%s is required", clTaskId);

struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];

safef(query, sizeof(query), "SELECT DISTINCT datasets_id FROM jobs WHERE tasks_id=%s", taskId);
struct slInt * datasetId, *datasetIdList = sqlQuickNumList(conn, query);

struct json *js = newJson();
struct json *dataset, *datasets = jsonAddContainerList(js, "datasets");
dataset = datasets;
jsonAddString(js, "taskId", taskId);
for(datasetId = datasetIdList; datasetId; datasetId=datasetId->next)
    {
	if(sameString(profile, "paradigmKO")){
   		safef(query, sizeof(query), "SELECT name FROM datasets WHERE id=%d", datasetId->val);
	}else{
    	safef(query, sizeof(query), "SELECT data_table FROM datasets WHERE id=%d", datasetId->val);
	}
    jsonAddString(dataset, "name", sqlNeedQuickString(conn, query));

    safef(query, sizeof(query), "SELECT id FROM jobs WHERE tasks_id=%s AND datasets_id=%d", taskId, datasetId->val);
    struct slInt *jobId, *jobIdList = sqlQuickNumList(conn, query);
    struct slDouble *avgAccs = NULL;
	struct slDouble *avgAccGains = NULL;
    for(jobId = jobIdList; jobId; jobId=jobId->next)
        {
        safef(query, sizeof(query), "SELECT avgTestingAccuracy FROM jobs WHERE id=%d", jobId->val);
        struct slDouble * tmp = slDoubleNew(sqlQuickDouble(conn, query));
        slSafeAddHead(&avgAccs, tmp);
	
		safef(query, sizeof(query), "SELECT avgTestingAccuracyGain FROM jobs WHERE id=%d", jobId->val);
        struct slDouble * tmp2 = slDoubleNew(sqlQuickDouble(conn, query));
        slSafeAddHead(&avgAccGains, tmp2);
        }
    double min, q1, median, q3, max;
    slDoubleBoxWhiskerCalc(avgAccs, &min, &q1, &median, &q3, &max);
    jsonAddDouble(dataset, "min", min*100);
    jsonAddDouble(dataset, "median", median*100);
    jsonAddDouble(dataset, "max", max*100);

    slDoubleBoxWhiskerCalc(avgAccGains, &min, &q1, &median, &q3, &max);
    jsonAddDouble(dataset, "minGain", min*100);
    jsonAddDouble(dataset, "medianGain", median*100);
    jsonAddDouble(dataset, "maxGain", max*100);

    if(datasetId->next)
        dataset = jsonAddContainerToList(&datasets);
    slFreeList(&jobIdList);
    }
if (js)
   hPrintf("%s\n", js->print(js));
slFreeList(&datasetIdList);
hFreeConn(&conn);
}

void summarizeSubgroups()
/*Create a JSON object of subgroups stats (high, median, low)*/
{
char *taskId = cartOptionalString(cart, clTaskId);
if (!taskId)
    errAbort("%s is required", clTaskId);

struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];

safef(query, sizeof(query), "SELECT DISTINCT CONCAT(name, '\t', parameters) FROM subgroups "
							"JOIN jobs ON (jobs.subgroups_id=subgroups.id) "
							"WHERE tasks_id=%s", taskId);
struct slName * sg, *sgList = sqlQuickList(conn, query);
struct json *js = newJson();
struct json *subgroup, *subgroups = jsonAddContainerList(js, "subgroups");
subgroup = subgroups;
jsonAddString(js, "taskId", taskId);
for(sg = sgList; sg; sg=sg->next)
    {	
	struct slName * tokenizedName = slNameListFromString(sg->name, '\t');
    jsonAddString(subgroup, "name", slNameListToString(tokenizedName, ' '));
	safef(query, sizeof(query), "SELECT avgTestingAccuracy FROM jobs JOIN subgroups "
								"ON (subgroups.id=jobs.subgroups_id) WHERE subgroups.name=\'%s\' "
								"AND subgroups.parameters=\'%s\' AND jobs.tasks_id=%s",
								tokenizedName->name, tokenizedName->next->name, taskId);
	struct slDouble *avgAccs = sqlQuickDoubleList(conn, query);
    double min, q1, median, q3, max;
    slDoubleBoxWhiskerCalc(avgAccs, &min, &q1, &median, &q3, &max);
    jsonAddDouble(subgroup, "min", min*100);
    jsonAddDouble(subgroup, "median", median*100);
    jsonAddDouble(subgroup, "max", max*100);
    safef(query, sizeof(query), "SELECT avgTestingAccuracyGain FROM jobs JOIN subgroups "
                                "ON (subgroups.id=jobs.subgroups_id) WHERE subgroups.name=\'%s\' "
                                "AND subgroups.parameters=\'%s\' AND jobs.tasks_id=%s",
								tokenizedName->name, tokenizedName->next->name, taskId);
    struct slDouble *avgAccGains = sqlQuickDoubleList(conn, query);
    slDoubleBoxWhiskerCalc(avgAccGains, &min, &q1, &median, &q3, &max);
    jsonAddDouble(subgroup, "minGain", min*100);
    jsonAddDouble(subgroup, "medianGain", median*100);
    jsonAddDouble(subgroup, "maxGain", max*100);

    if(sg->next)
        subgroup = jsonAddContainerToList(&subgroups);
	//clean up
	slFreeList(&avgAccGains);
	slFreeList(&avgAccs);
	slFreeList(&tokenizedName);
    }
if (js)
   hPrintf("%s\n", js->print(js));
slFreeList(&sgList);
hFreeConn(&conn);
}


void overviewTasks()
/*Create a JSON object of datasets stats (high, median, low)*/
{
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];

safef(query, sizeof(query), "SELECT DISTINCT id FROM tasks");
struct slInt * taskId, *taskIdList = sqlQuickNumList(conn, query);

struct json *js = newJson();
struct json *task, *tasks = jsonAddContainerList(js, "tasks");
task = tasks;

for(taskId = taskIdList; taskId; taskId=taskId->next)
    {
	safef(query, sizeof(query), "SELECT avgTestingAccuracy FROM jobs WHERE tasks_id=%d", taskId->val);
    struct slDouble * avgAccs = sqlQuickDoubleList(conn, query);
	if(avgAccs)
		{
	    safef(query, sizeof(query), "SELECT name FROM tasks WHERE id=%d", taskId->val);
	    char * taskName = sqlNeedQuickString(conn, query);
	
	    safef(query, sizeof(query), "select longLabel from features where name='%s'", taskName);
	    char * longLabel = sqlQuickNonemptyString(conn,query);
	    if(longLabel != NULL)
	        jsonAddString(task, "name", longLabel);
	    else
	        jsonAddString(task, "name", taskName);
		safef(query, sizeof(query), "SELECT avgTestingAccuracy FROM jobs WHERE tasks_id=%d", taskId->val);
		struct slDouble * avgAccs = sqlQuickDoubleList(conn, query);
	    double min, q1, median, q3, max;
	    slDoubleBoxWhiskerCalc(avgAccs, &min, &q1, &median, &q3, &max);
	    jsonAddDouble(task, "min", min*100);
	    jsonAddDouble(task, "median", median*100);
	    jsonAddDouble(task, "max", max*100);
		
	    safef(query, sizeof(query), "SELECT avgTestingAccuracyGain FROM jobs WHERE tasks_id=%d", taskId->val);
	    struct slDouble * avgAccGains = sqlQuickDoubleList(conn, query);
	    slDoubleBoxWhiskerCalc(avgAccGains, &min, &q1, &median, &q3, &max);
	    jsonAddDouble(task, "minGain", min*100);
	    jsonAddDouble(task, "medianGain", median*100);
	    jsonAddDouble(task, "maxGain", max*100);
	    if(taskId->next)
	        task = jsonAddContainerToList(&tasks);
		}
    }
if (js)
   hPrintf("%s\n", js->print(js));
slFreeList(&taskIdList);
hFreeConn(&conn);
}

void overviewClassifiers()
/*Create a JSON object of datasets stats (high, median, low)*/
{
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];

safef(query, sizeof(query), "SELECT DISTINCT id FROM classifiers");
struct slInt * classifierId, *classifierIdList = sqlQuickNumList(conn, query);

struct json *js = newJson();
struct json *classifier, *classifiers = jsonAddContainerList(js, "classifiers");
classifier = classifiers;

for(classifierId = classifierIdList; classifierId; classifierId=classifierId->next)
    {
	safef(query, sizeof(query), "SELECT avgTestingAccuracy FROM jobs WHERE classifiers_id=%d", classifierId->val);
    struct slDouble * avgAccs = sqlQuickDoubleList(conn, query);
	if(avgAccs)
		{
	    safef(query, sizeof(query), "SELECT name FROM classifiers WHERE id=%d", classifierId->val);
	    jsonAddString(classifier, "name", sqlNeedQuickString(conn, query));
	    safef(query, sizeof(query), "SELECT parameters FROM classifiers WHERE id=%d", classifierId->val);
	    jsonAddString(classifier, "parameters", sqlNeedQuickString(conn, query));
	
		double min, q1, median, q3, max;
		slDoubleBoxWhiskerCalc(avgAccs, &min, &q1, &median, &q3, &max);
		jsonAddDouble(classifier, "min", min*100);
		jsonAddDouble(classifier, "median", median*100);
		jsonAddDouble(classifier, "max", max*100);
		
		safef(query, sizeof(query), "SELECT avgTestingAccuracyGain FROM jobs WHERE classifiers_id=%d", classifierId->val);
	    struct slDouble * avgAccGains = sqlQuickDoubleList(conn, query);
	    slDoubleBoxWhiskerCalc(avgAccGains, &min, &q1, &median, &q3, &max);
	    jsonAddDouble(classifier, "minGain", min*100);
	    jsonAddDouble(classifier, "medianGain", median*100);
	    jsonAddDouble(classifier, "maxGain", max*100);

	    if(classifierId->next)
    	    classifier = jsonAddContainerToList(&classifiers);
		}
    }
if (js)
   hPrintf("%s\n", js->print(js));
slFreeList(&classifierIdList);
hFreeConn(&conn);
}

int saveUserURLdata()
/*Saves data submitted through a URL*/
{
char * userDataURL = cartOptionalString(cart, clUserDataURL);
if(!userDataURL)
	errAbort("%s is required", clUserDataURL);
int fileId = getpid();

//make a system call to pull the data down here
char userDataFile[256];
safef(userDataFile, sizeof(userDataFile), "/data/trash/MLuserSubmissions/%d_data.tab", fileId);
char command[1024];
safef(command, sizeof(command), "wget --output-file=%s %s", userDataFile, userDataURL);
if(system(command) == -1)
	{
	fprintf(stderr, "ERROR: Couldn't download file %s\n", userDataURL);
	return -1;
	}
return fileId;
}

int saveUserFileData()
/*Saves user data to a unique file path and return id of the file*/
{
char *userData = cartOptionalString(cart, clUserData);
if(!userData)
    errAbort("%s is required", clUserData);

int fileId = getpid();

//write user submitted data to file
char userDataPrefix[256];
safef(userDataPrefix, sizeof(userDataPrefix), "/data/trash/MLuserSubmissions/%d", fileId);
char userDataFile[256];
safef(userDataFile, sizeof(userDataFile), "%s_data.tab", userDataPrefix);
FILE * fp = fopen(userDataFile, "w");
if(!fp)
	{
    fprintf(stderr, "Error: Couldn't open file %s for writing user data to.\n", userDataFile);
	return -1;
	}	
fprintf(fp,"%s", userData);
fclose(fp);

return fileId;
}


void applyModel()
{
//save the user data
char *jobIdString = cartOptionalString(cart, clJobId);
if (!jobIdString)
    errAbort("%s is required", clJobId);
int jobId = atoi(jobIdString);

int fileId = -1;
char * userDataFilepath = cartOptionalString(cart, clUserData);
char * userDataURLpath = cartOptionalString(cart, clUserDataURL);
//if(strcmp(userDataFilepath,"") != 0)
//    fileId = saveUserFileData();
//else if(strcmp(userDataURLpath,"") != 0)
//    fileId = saveUserURLdata();
if(userDataFilepath != NULL)
    fileId = saveUserFileData();
else if(userDataURLpath != NULL)
    fileId = saveUserURLdata();
else
    errAbort("ERROR: Call to save data but no input data exists.");
if(fileId == -1)
    errAbort("ERROR: Couldn't save user data\n"); //TODO: Send this message back to the front end

//Get details of the classifier used to determine which model to apply
struct sqlConnection *conn = hAllocConnProfile(profile, db);
char query[256];
safef(query, sizeof(query), "SELECT modelPath FROM jobs WHERE id=%d", jobId);
char * modelPath = sqlNeedQuickString(conn, query);
safef(query, sizeof(query), "SELECT classifiers_id FROM jobs WHERE id=%d", jobId);
int classifiersId = sqlNeedQuickNum(conn, query);
safef(query, sizeof(query), "SELECT name FROM classifiers WHERE id=%d", classifiersId);
char * classifier = sqlNeedQuickString(conn, query);
//save the number of features in this model to compare feature usage to
safef(query, sizeof(query), "SELECT featureSelections_id FROM jobs WHERE id=%d", jobId);
int featureSelectionsId = sqlNeedQuickNum(conn, query);
safef(query, sizeof(query), "SELECT featureCount FROM featureSelections where id=%d", featureSelectionsId);
int modelFeatures = sqlNeedQuickNum(conn, query);
int matchedFeatures = 0;

//Create strings that pointn to the user data file
char userDataPrefix[256];
safef(userDataPrefix, sizeof(userDataPrefix), "/data/trash/MLuserSubmissions/%d", fileId);
char userDataFile[256];
safef(userDataFile, sizeof(userDataFile), "%s_data.tab", userDataPrefix);

//save user data in a matrix
FILE * fp = fopen(userDataFile, "r");
if(!fp)
    errAbort("Couldn't open data file %s for reading.\n",userDataFile);;
matrix * data = f_fill_matrix(fp, 1);
fclose(fp);

//run command line programs to do classification
matrix * results = NULL;
if(sameString(classifier, "NMFpredictor"))
    {
    matrix * matchedData = NMFgetDataMatchedToModel(data, modelPath);
    results = applyNMFpredictorModel(matchedData, userDataPrefix, modelPath);
    matchedFeatures = countFeatureUsage(matchedData);
    free_matrix(matchedData);
    }
else if(sameString(classifier, "SVMlight"))
    {
    matrix * matchedData = SVMgetDataMatchedToModel(data, modelPath);
    results = applySVMlightModel(matchedData, userDataPrefix, modelPath);
    matchedFeatures = countFeatureUsage(matchedData);
    free_matrix(matchedData);
    }
else if(sameString(classifier, "glmnet"))
	{
	safef(query, sizeof(query), "SELECT parameters FROM classifiers WHERE id=%d", classifiersId);
	char * parameters = sqlNeedQuickString(conn, query);
	matrix * matchedData = glmnetgetDataMatchedToModel(data, userDataPrefix, modelPath, parameters);
	results = applyglmnetModel(matchedData, userDataPrefix, modelPath, parameters);
	matchedFeatures = countFeatureUsage(matchedData);
	}
else if(sameString(classifier, "WEKA"))
    errAbort("ERROR: No functions in place to apply WEKA models yet.");

//create a json of the samples and their predicted values
struct json *js = newJson();
if(!modelFeatures || !matchedFeatures)
    errAbort("Couldn't get a feature count out of user submitted data. Exiting..\n");
jsonAddInt(js, "modelFeatures", modelFeatures);
jsonAddInt(js, "matchedFeatures", matchedFeatures);
struct json *sample, *samples = jsonAddContainerList(js, "samples");
sample = samples;
int i;
for(i = 0; i < results->cols; i++)
    {
    jsonAddString(sample, "name", cloneString(results->colLabels[i]));
	jsonAddInt(sample, "userSample", 1);
    jsonAddDouble(sample, "predictedClass", results->graph[0][i]);
    if(i < results->cols)
        sample = jsonAddContainerToList(&samples);
    }
//make a list of classifierResults from this job, and add to JSON as a 'background'.
safef(query, sizeof(query), "SELECT * FROM classifierResults WHERE jobs_id=%d;", jobId);
struct classifierResults *cr, *crList = classifierResultsLoadByQuery(conn, query);

//if the cohort is too large, display just the top and bottom
if(slCount(crList) > 100)
	{
	jsonAddInt(js, "largeCohort", 1); 
	double min = crList->predictionScore;
	double max = crList->predictionScore;
	for(cr = crList; cr; cr=cr->next)
		{
		if(cr->predictionScore > max)
			max = cr->predictionScore;

		if(cr->predictionScore < min)
			min = cr->predictionScore;
		}
	jsonAddString(sample, "name", "trainingMin"); 
	jsonAddInt(sample, "userSample", 0);
	jsonAddDouble(sample, "predictedClass", min); 
	
	sample = jsonAddContainerToList(&samples);
	jsonAddString(sample, "name", "trainingMax"); 
	jsonAddInt(sample, "userSample", 0);
	jsonAddDouble(sample, "predictedClass", max); 
	}
else
	{
	jsonAddInt(js, "largeCohort", 0);
	for(cr = crList; cr; cr=cr->next)
		{
		jsonAddString(sample, "name", getStringFromTableById(conn, "patient_name", "samples", cr->sample_id));
		jsonAddInt(sample, "userSample", 0);
		jsonAddDouble(sample, "predictedClass", cr->predictionScore);
	    if(cr->next)
	        sample = jsonAddContainerToList(&samples);
		}
	}
if (js)
   hPrintf("%s\n", js->print(js));

//clean up 
safef(userDataFile, sizeof(userDataFile), "%s_data.tab", userDataPrefix);
unlink(userDataFile);
safef(userDataFile, sizeof(userDataFile), "%s.results", userDataPrefix);
unlink(userDataFile);

hFreeConn(&conn);
}

/****** END HELPER FUNCTIONS *******/

void dispatchRoutines()
/* Look at command variables in cart and figure out which
 * page to draw. */
{
/* retrieve cart variables, handle various modes */
char *mode = cartOptionalString(cart, clMode);
if (!mode)
	errAbort("%s is required.", clMode);

if (sameString(mode, "loadTasks"))
	loadTasks();
else if(sameString(mode, "loadTaskDetails"))
	loadTaskDetails();
else if(sameString(mode, "viewSamples"))
    viewSamples();
else if(sameString(mode, "viewFeatures"))
	viewFeatures();
else if(sameString(mode, "summarizeFeatures"))
	summarizeFeatures();
else if(sameString(mode, "summarizeSamples"))
	summarizeSamples();
else if(sameString(mode, "summarizeClassifiers"))
    summarizeClassifiers();
else if(sameString(mode, "summarizeFeatureSelections"))
    summarizeFeatureSelections();
else if(sameString(mode, "summarizeDatasets"))
    summarizeDatasets();
else if(sameString(mode, "summarizeSubgroups"))
    summarizeSubgroups();
else if(sameString(mode, "overviewTasks"))
    overviewTasks();
else if(sameString(mode, "overviewClassifiers"))
    overviewClassifiers();
else if(sameString(mode, "applyModel"))
	applyModel();
else
	errAbort("Incorrect mode = %s", mode);

cartRemovePrefix(cart, clPrefix);
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

