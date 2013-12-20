/* hgPredictorPrint */
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
#include "hgPredictorPrint.h"
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

char *db = "bioInt";
char *profile = "localDb";

void usage()
/* Explain usage and exit. */
{
errAbort(
  "hgPredictorPrint cgi\n"
  "usage:\n"
  "   hgPredictorPrint\n"
  );
}

/****** BEGIN HELPER FUNCTIONS *******/
double roundIt(double x, double n)
{ 
    x = floor( x * pow(10.0, n) + 0.5) / pow(10.0, n);
    return x;
}

int saveUserFileData()
/*Saves user data to a unique file path and return id of the file*/
{
char *userData = cartOptionalString(cart, ppUserData);
if(!userData)
    errAbort("%s is required", ppUserData);

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

int saveUserURLdata()
/*Saves data submitted through a URL*/
{
char * userDataURL = cartOptionalString(cart, ppUserDataURL);
if(!userDataURL)
    errAbort("%s is required", ppUserDataURL);
int fileId = getpid();

//make a system call to pull the data down here
char userDataFile[256];
safef(userDataFile, sizeof(userDataFile), "/data/trash/MLuserSubmissions/%d_data.tab", fileId);
char command[1024];
safef(command, sizeof(command), "wget --output-document=%s %s", userDataFile, userDataURL);
if(system(command) == -1)
    {
    fprintf(stderr, "ERROR: Couldn't download file %s\n", userDataURL);
    return -1;
    }
return fileId;
}

void getDataTypeList()
{
struct sqlConnection * conn =  hAllocConnProfile(profile, db);
char query[256];
safef(query, sizeof(query), "SELECT name FROM dataTypes");
struct slName * dt, * dtList = sqlQuickList(conn, query);
dt = dtList;
safef(query, sizeof(query), "SELECT id FROM dataTypes");
struct slInt * id, *idList = sqlQuickNumList(conn, query);
id = idList;
struct json *js = newJson();
struct json *dataType, *dataTypes = jsonAddContainerList(js, "dataTypes");
dataType = dataTypes;
for (dt = dtList, id=idList; dt && id; dt = dt->next, id=id->next)
    {
	jsonAddInt(dataType, "id", id->val);
    jsonAddString(dataType, "name", dt->name);
    if (dt->next)
       dataType = jsonAddContainerToList(&dataTypes);
    }
if (js)
   hPrintf("%s\n", js->print(js));
hFreeConn(&conn);
}

void getEstimatedTotalRunTime()
{
char * dataTypeIdString = cartOptionalString(cart, ppUserDataTypeId);
if(!dataTypeIdString)
    errAbort("%s is required", ppUserDataTypeId);
int dataTypeId = atoi(dataTypeIdString);
char * userMinAccGainString = cartOptionalString(cart, ppUserMinAccGain);
double userMinAccGain = 0;//default zero
if(userMinAccGainString != NULL)
	userMinAccGain = (atof(userMinAccGainString)/100);
struct sqlConnection * conn = hAllocConnProfile(profile, db);
struct json * js = newJson();
char query[1024];
safef(query, sizeof(query), "SELECT sum(estimatedRunTime) FROM backgroundModelScores "
							"JOIN jobs ON (backgroundModelScores.jobs_id=jobs.id) "
							"WHERE avgTestingAccuracyGain > %f AND estimatedRunTime < %d "
							"AND data_table LIKE \'%%_DATATYPE%d_%%\'",
							userMinAccGain, MAX_MODEL_RUNTIME, dataTypeId);
if(!sqlExists(conn, query))
	{
	jsonAddInt(js, "estimatedRunTime", -1);
	}
else
	{
	int totalSeconds = sqlQuickNum(conn, query);
	jsonAddInt(js, "estimatedRunTime", (totalSeconds+10)); //add some time for permutation processing
	}
if(js)
    hPrintf("%s\n", js->print(js));
hFreeConn(&conn);
}

void applyTopModels()
{
char * dataTypeIdString = cartOptionalString(cart, ppUserDataTypeId);
if(!dataTypeIdString)
    errAbort("%s is required", ppUserDataTypeId);
int dataTypeId = atoi(dataTypeIdString);
char * userMinAccGainString = cartOptionalString(cart, ppUserMinAccGain);
double userMinAccGain = 0;//default zero
if(userMinAccGainString)
	userMinAccGain = (atof(userMinAccGainString) / 100);

int i,j,fileId = -1;
char * userDataFilepath = cartOptionalString(cart, ppUserData);
char * userDataURLpath = cartOptionalString(cart, ppUserDataURL);
//if(strcmp(userDataFilepath, "") != 0)
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
char query[1024];
safef(query, sizeof(query), "SELECT jobs_id FROM backgroundModelScores "
                            "JOIN jobs ON (backgroundModelScores.jobs_id=jobs.id) "
                            "WHERE avgTestingAccuracyGain > %f AND estimatedRunTime < %d "
                            "AND data_table LIKE \'%%_DATATYPE%d_%%\'",
                            userMinAccGain, MAX_MODEL_RUNTIME, dataTypeId);
struct slInt * jid, * jobsIdList = sqlQuickNumList(conn, query);

//save the user data to a matrix here
char userDataPrefix[256];
safef(userDataPrefix, sizeof(userDataPrefix), "/data/trash/MLuserSubmissions/%d", fileId);
char userDataFile[256];
safef(userDataFile, sizeof(userDataFile), "%s_data.tab", userDataPrefix);
FILE * fp = fopen(userDataFile, "r");
if(!fp)
    errAbort("Couldn't open data file %s for reading.\n", userDataFile);
matrix * data = f_fill_matrix(fp, 1);
fclose(fp);

//Generate 3 permuted samples, and tack them on to the user data
matrix * permutations = permute_fill_matrix(data, DEFAULT_PERMUTATIONS); 
matrix * tmp = append_matrices(data, permutations, 1);
free_matrix(data);
free_matrix(permutations);
data = tmp;

//make a topmodels by samples matrix to save results in to
int modelCount = slCount(jobsIdList);
matrix * results = init_matrix(modelCount, data->cols);
copy_matrix_labels(results, data, 2,2);
matrix * featureUsage = init_matrix(modelCount, 1);

//apply the models, saving p-values to results matrix
int rowCount = 0, featureCount;
double modelMean, modelSd;
char * taskName= NULL, *modelPath = NULL, *classifier=NULL, *parameters=NULL;
matrix * thisResult = NULL;
for(jid = jobsIdList; jid; jid = jid->next)
	{
	fprintf(stderr, "Working on model %d\n", jid->val);
	//get info about this model from db
	safef(query, sizeof(query), "SELECT tasks.name FROM tasks JOIN jobs ON (jobs.tasks_id=tasks.id)  WHERE jobs.id=%d", jid->val);
	taskName = sqlNeedQuickString(conn, query);
	safef(query, sizeof(query), "SELECT modelPath FROM jobs WHERE id=%d", jid->val);
	modelPath = sqlNeedQuickString(conn, query);
	safef(query, sizeof(query), "SELECT mean FROM backgroundModelScores WHERE jobs_id=%d", jid->val);
	modelMean = sqlQuickDouble(conn, query);
	safef(query, sizeof(query), "SELECT sd FROM backgroundModelScores WHERE jobs_id=%d", jid->val);
	modelSd = sqlQuickDouble(conn, query);
	safef(query, sizeof(query), "SELECT classifiers.name FROM classifiers JOIN jobs ON (jobs.classifiers_id=classifiers.id) WHERE jobs.id=%d", jid->val);
	classifier = sqlNeedQuickString(conn, query);
	safef(query, sizeof(query), "SELECT classifiers.parameters FROM classifiers JOIN jobs ON (jobs.classifiers_id=classifiers.id) WHERE jobs.id=%d", jid->val);
	parameters = sqlNeedQuickString(conn, query);
	safef(query, sizeof(query), "SELECT featureSelections.featureCount FROM featureSelections JOIN jobs ON featureSelections.id=jobs.featureSelections_id WHERE jobs.id=%d", jid->val);
	featureCount = sqlNeedQuickNum(conn, query);
	
	//save name of task in the results matrix
	safef(results->rowLabels[rowCount], MAX_LABEL, "%s", cloneString(taskName));
	//get prediction scores
	thisResult = NULL;
	if(sameString(classifier, "NMFpredictor"))
    	{
		matrix * matchedData = NMFgetDataMatchedToModel(data, modelPath);
	    thisResult = applyNMFpredictorModel(matchedData, userDataPrefix, modelPath);
		featureUsage->graph[rowCount][0] = (double)countFeatureUsage(matchedData) * 100 / featureCount;
	    free_matrix(matchedData);
    	}
	else if(sameString(classifier, "SVMlight"))
    	{
		matrix * matchedData = SVMgetDataMatchedToModel(data, modelPath);
    	thisResult = applySVMlightModel(matchedData, userDataPrefix, modelPath);
        featureUsage->graph[rowCount][0] = (double)countFeatureUsage(matchedData) * 100 / featureCount;
	    free_matrix(matchedData);
   		}
    else if(sameString(classifier, "glmnet"))
        {
        matrix * matchedData = glmnetgetDataMatchedToModel(data,userDataPrefix,modelPath,parameters);
        thisResult = applyglmnetModel(matchedData, userDataPrefix, modelPath, parameters);
        featureUsage->graph[rowCount][0] = (double)countFeatureUsage(matchedData) * 100 / featureCount;
        free_matrix(matchedData);
        }
	else if(sameString(classifier, "WEKA"))
		{
    	errAbort("ERROR: No functions in place to apply WEKA models yet.\n");
		}
	else
		{
		errAbort("ERROR: Unrecongized model type\n");
		}
	//process results
	for(j = 0; j < thisResult->cols; j++)
		{
		if(modelSd != 0)
			results->graph[rowCount][j] = (thisResult->graph[0][j] - modelMean) / modelSd;
		else
			results->graph[rowCount][j] = NULL_FLAG;
		}
	//clean up
	freeMem(taskName);
	freeMem(modelPath);
	freeMem(classifier);
	if(thisResult)
		free_matrix(thisResult);
	rowCount++;
	fprintf(stderr, "%.2f%% done.\n", 100*((double)rowCount/modelCount));
	}

//convert results to a json and print it
struct json * js = newJson();
struct json * task, *tasks = jsonAddContainerList(js, "tasks");
struct json * zScore, *zScores = NULL;
task = tasks;
for(i = 0, jid=jobsIdList; (i < results->rows) && (jid !=NULL); i++, jid=jid->next)
	{
	jsonAddString(task, "taskName", cloneString(results->rowLabels[i]));
	jsonAddInt(task, "jobId", jid->val);
	jsonAddDouble(task, "featureUsage", featureUsage->graph[i][0]);
	zScores = jsonAddContainerList(task, "zScores");
	zScore = zScores;
	for(j = 0; j < results->cols; j++)
		{
		jsonAddString(zScore, "sampleName", cloneString(results->colLabels[j]));
		if(results->graph[i][j] != NULL_FLAG)
			{
			jsonAddDouble(zScore, "zScore", results->graph[i][j]);
			}
		if((j+1) < results->cols)
			{
			zScore = jsonAddContainerToList(&zScores);
			}
		}
	if((i+1) < results->rows)
		task = jsonAddContainerToList(&tasks);
	}
if(js)
	hPrintf("%s\n", js->print(js));
//clean up
safef(userDataFile, sizeof(userDataFile), "%s_data.tab", userDataPrefix);
unlink(userDataFile);
safef(userDataFile, sizeof(userDataFile), "%s.results", userDataPrefix);
unlink(userDataFile);
free_matrix(data);
free_matrix(results);
free_matrix(featureUsage);
slFreeList(&jobsIdList);
hFreeConn(&conn);
}

void loadTopModelDetails()
{
char * jobIdString = cartOptionalString(cart, ppJobId);
if(!jobIdString)
    errAbort("%s is required", ppJobId);
int jobId = atoi(jobIdString);
struct sqlConnection * conn = hAllocConnProfile(profile, db);
char query[256];
struct json * js = newJson();
//query out the info needed
safef(query, sizeof(query), "SELECT tasks.name FROM tasks JOIN jobs ON tasks.id=jobs.tasks_id WHERE jobs.id=%d", jobId);
char * task = sqlNeedQuickString(conn, query);
safef(query, sizeof(query), "SELECT classifiers.name FROM classifiers JOIN jobs ON classifiers.id=jobs.classifiers_id WHERE jobs.id=%d", jobId);
char * classifier = sqlNeedQuickString(conn, query);
safef(query, sizeof(query), "SELECT classifiers.parameters FROM classifiers JOIN jobs ON classifiers.id=jobs.classifiers_id WHERE jobs.id=%d", jobId);
char * classifierParameters = sqlNeedQuickString(conn, query);
char classifierString[256];
safef(classifierString, sizeof(classifierString), "Package:%s,Parameters:%s", classifier, classifierParameters);
if(sameString(profile, "paradigmKO"))
	safef(query, sizeof(query), "SELECT datasets.data_table FROM datasets JOIN jobs ON datasets.id=jobs.datasets_id WHERE jobs.id=%d", jobId);
else
	safef(query, sizeof(query), "SELECT datasets.name FROM datasets JOIN jobs ON datasets.id=jobs.datasets_id WHERE jobs.id=%d", jobId);
char * trainingDataset = sqlNeedQuickString(conn, query);
safef(query, sizeof(query), "SELECT subgroups.name FROM subgroups JOIN jobs ON subgroups.id=jobs.subgroups_id WHERE jobs.id=%d", jobId);
char * subgroup = sqlNeedQuickString(conn, query);
safef(query, sizeof(query), "SELECT subgroups.parameters FROM subgroups JOIN jobs ON subgroups.id=jobs.subgroups_id WHERE jobs.id=%d", jobId);
char * subgroupParameters = sqlNeedQuickString(conn, query);
char subgroupString[512];
safef(subgroupString, sizeof(subgroupString), "Method:%s,Parameters:%s", subgroup, subgroupParameters);
safef(query, sizeof(query), "SELECT featureSelections.name FROM featureSelections JOIN jobs ON featureSelections.id=jobs.featureSelections_id WHERE jobs.id=%d", jobId);
char * featureSelection = sqlNeedQuickString(conn, query);
safef(query, sizeof(query), "SELECT featureSelections.parameters FROM featureSelections JOIN jobs ON featureSelections.id=jobs.featureSelections_id WHERE jobs.id=%d", jobId);
char * featureSelectionParameters = sqlNeedQuickString(conn, query);
char featureSelectionString[256];
safef(featureSelectionString, sizeof(featureSelectionString), "Method:%s,Parameters:%s", featureSelection, featureSelectionParameters);
safef(query, sizeof(query), "SELECT transformations.name FROM transformations JOIN jobs ON transformations.id=jobs.transformations_id WHERE jobs.id=%d", jobId);
char * transformation = sqlNeedQuickString(conn, query);
safef(query, sizeof(query), "SELECT transformations.parameters FROM transformations JOIN jobs ON transformations.id=jobs.transformations_id WHERE jobs.id=%d", jobId);
char * transformationParameters = sqlNeedQuickString(conn, query);
char transformationString[256];
safef(transformationString, sizeof(transformationString), "Method:%s,Parameters:%s", transformation, transformationParameters);
safef(query, sizeof(query), "SELECT featureSelections.featureCount FROM featureSelections JOIN jobs ON featureSelections.id=jobs.featureSelections_id WHERE jobs.id=%d", jobId);
int featureCount = sqlNeedQuickNum(conn, query);
safef(query, sizeof(query), "SELECT avgTestingAccuracy FROM jobs WHERE id=%d", jobId);
double avgTestingAccuracy = sqlQuickDouble(conn, query); 
safef(query, sizeof(query), "SELECT avgTestingAccuracyGain FROM jobs WHERE id=%d", jobId);
double avgTestingAccuracyGain = sqlQuickDouble(conn, query);
safef(query, sizeof(query), "SELECT accuracyType FROM jobs WHERE id=%d", jobId);
char * accuracyType = sqlNeedQuickString(conn, query);
char host[256];
gethostname(host, 256);
char URL[1024];
safef(URL, sizeof(URL), "https://%s.soe.ucsc.edu/hgClassifications/view.html#mode=viewSamples&parameters=%s,%d", host, task, jobId); 
char * encodedURL = replaceChars(URL, " ", "%20");
encodedURL = replaceChars(encodedURL, ",", "%2C");

//add the info to json
jsonAddString(js, "task", task);
jsonAddString(js, "classifier", classifierString); 
jsonAddString(js, "trainingDataset", trainingDataset);
jsonAddString(js, "subgrouping", subgroupString);
jsonAddString(js, "featureSelection", featureSelectionString);
jsonAddString(js, "transformation", transformationString);
jsonAddInt(js, "featureCount", featureCount);
jsonAddDouble(js, "avgTestingAccuracy", avgTestingAccuracy);
jsonAddDouble(js, "avgTestingAccuracyGain", avgTestingAccuracyGain);
jsonAddString(js, "accuracyType", accuracyType);
jsonAddString(js, "URL", encodedURL);
if(js)
	hPrintf("%s\n", js->print(js));
//clean up
freeMem(task);
freeMem(classifier);
freeMem(classifierParameters);
freeMem(trainingDataset);
freeMem(featureSelection);
freeMem(featureSelectionParameters);
freeMem(subgroup);
freeMem(subgroupParameters);
freeMem(transformation);
freeMem(transformationParameters);
freeMem(accuracyType);
hFreeConn(&conn);
}

void dispatchRoutines()
/* Look at command variables in cart and figure out which
 * page to draw. */
{
/* retrieve cart variables, handle various modes */
char *mode = cartOptionalString(cart, ppMode);
if (!mode)
    errAbort("%s is required.", ppMode);

if(sameString(mode, "getDataTypeList"))
	getDataTypeList();
else if(sameString(mode, "getEstimatedTotalRunTime"))
	getEstimatedTotalRunTime();
else if (sameString(mode, "applyTopModels"))
    applyTopModels();
else if(sameString(mode, "loadTopModelDetails"))
	loadTopModelDetails();
else
	errAbort("Inccorect mode.\n");

cartRemovePrefix(cart, ppPrefix);
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
