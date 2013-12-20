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
#include <pthread.h>

#define MAX_THREADS 10

/*Prototypes*/
struct thread_args{
	struct hash * config;
	matrix * data;
	matrix * metadata;
	char *currDir;
	int taskCounter;
	int datasetCounter;
	int clinDiscretizerCounter;
	int dataDiscretizerCounter;
	int featureSelectionCounter;
	int modelNum;
};

/* Default globals */
char *db = "bioInt";
char *profile = "localDb";
char * clusterRootDir = "/data/home/common/topmodel_tools";
char * excludeList = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int touchedModels = 0; //count how many models you've tried to process, for locking purposes;
int totalModels = 0; //count how many models there are overall to process

/* Option handling */
static struct optionSpec options[] = {
    {"tasks", OPTION_STRING},
    {"datasets", OPTION_STRING},
    {"featureSelections", OPTION_STRING},
    {"classifiers", OPTION_STRING},
    {"clinDiscretizers", OPTION_STRING},
    {"dataDiscretizers", OPTION_STRING},
    {"profile", OPTION_STRING},
	{"crossValidation", OPTION_STRING},
    {"db", OPTION_STRING},
	{"clusterRootDir", OPTION_STRING},
    {"excludeList", OPTION_STRING},
    {NULL, 0}
};

void usage()
{
errAbort(	"Usage: ./MLbatchSetup [options]\n"
			"-tasks=[filename]\n"
			"-datasets=[filename]\n"
            "-featureSelections=[filename]\n"
            "-classifiers=[filename]\n"
            "-clinDiscretizers=[filename]\n"
            "-dataDiscretizers=[filename]\n"
			"-crossValidation=[filename]\n"
            "-profile=[string]\n"
            "-db=[string]\n"
			"-clusterRootDir=[filepath]\n"
			"-excludeList=[filepath]\n"
		);
}

void printMLconfigFile(struct hash *config, char *configFile)
{
FILE * fp = mustOpen(configFile, "w");
char * raString = hashToRaString(config);
fprintf(fp, "%s", raString);
freeMem(raString);
fclose(fp);
}

int trainingDataExists(struct hash *config)
{
char * outputType = hashMustFindVal(config, "outputType");
char * trainingDir = hashMustFindVal(config, "trainingDir");
char targetFile[2048];
if(sameString(outputType, "flatfiles"))
	safef(targetFile, sizeof(targetFile), "%s/data.tab", trainingDir);
else if(sameString(outputType, "SVMlight"))
    safef(targetFile, sizeof(targetFile), "%s/data.svm", trainingDir);
else if(sameString(outputType, "WEKA"))
    safef(targetFile, sizeof(targetFile), "%s/data.arff", trainingDir);
else
	errAbort("Unsupported output type %s. Exiting...\n", outputType);
if(fileExists(targetFile))
	return 1;
return 0;
}

void spoofTrainingData(struct hash * config)
{
char * outputType = hashMustFindVal(config, "outputType");
char * trainingDir = hashMustFindVal(config, "trainingDir");
char targetFile[2048];
if(sameString(outputType, "flatfiles"))
    safef(targetFile, sizeof(targetFile), "%s/data.tab", trainingDir);
else if(sameString(outputType, "SVMlight"))
    safef(targetFile, sizeof(targetFile), "%s/data.svm", trainingDir);
else if(sameString(outputType, "WEKA"))
    safef(targetFile, sizeof(targetFile), "%s/data.arff", trainingDir);
else
    errAbort("Unsupported classifier type %s. Exiting...\n", outputType);
createDataDirs(config);
FILE * fp = fopen(targetFile, "w");
if(!fp)
	errAbort("Couldn't open %s for writing.", targetFile);
fclose(fp);
}

void * threadSafeWrite(void * threadArgs)
{
//extract args out of void* parameter
struct thread_args *args;
args = (struct thread_args *) threadArgs;  /* type cast to a pointer to thdata */
struct hash * config = args->config;
matrix * data = args->data;
matrix * metadata = args->metadata;
char * currDir = args->currDir;
char * clinField = hashMustFindVal(config, "clinField");
if(!currDir || !clinField)
	errAbort("One of the strings in threadSafeWrite is null\n");
//make a directory-name-safe version of clinField
char * clinFieldDir = replaceChars(clinField, " ", "_");
int taskCounter = args->taskCounter;
int datasetCounter = args->datasetCounter;
int clinDiscretizerCounter = args->clinDiscretizerCounter;
int dataDiscretizerCounter = args->dataDiscretizerCounter;
int featureSelectionCounter = args->featureSelectionCounter;
int modelNum = args->modelNum;
int trDataExists = 1;
//save some paths to config
char targetDir[2048], targetFile[2048], foldReport[2048], configFile[2048];
//these variables are one-use, otherwise they will not work properly
char taskName[200], trainingDir[2048], validationDir[2048], modelDir[2048];
safef(taskName, sizeof(taskName), "%s_%d", clinFieldDir, taskCounter);
hashAdd(config, "name", taskName);
safef(modelDir, sizeof(modelDir), "%s/models/%s/dataset%d_model%d",
    currDir, clinFieldDir, datasetCounter, modelNum);
hashAdd(config, "modelDir", modelDir);
safef(trainingDir, sizeof(trainingDir),
    "%s/data/%s/dataset%d_clinDiscretizer%d_dataDiscretizer%d_featureSelection%d/trainingDir",
    currDir,clinFieldDir,datasetCounter,clinDiscretizerCounter,dataDiscretizerCounter,featureSelectionCounter);
hashAdd(config, "trainingDir", trainingDir);
safef(validationDir, sizeof(validationDir),
    "%s/data/%s/dataset%d_clinDiscretizer%d_dataDiscretizer%d_featureSelection%d/validationDir",
    currDir,clinFieldDir,datasetCounter,clinDiscretizerCounter,dataDiscretizerCounter,featureSelectionCounter);
hashAdd(config, "validationDir", validationDir);
safef(foldReport, sizeof(foldReport), "%s/models/%s/dataset%d_model%d.foldReport",
    currDir,clinFieldDir,datasetCounter, modelNum);
hashAdd(config, "foldsReport", foldReport);
//wait for lock, then create directories with race conditions
while(pthread_mutex_lock(&mutex) != 0)
    usleep(1);
safef(targetDir, sizeof(targetDir),
	"%s/data/%s/dataset%d_clinDiscretizer%d_dataDiscretizer%d_featureSelection%d",
	currDir,clinFieldDir,datasetCounter,clinDiscretizerCounter,dataDiscretizerCounter,featureSelectionCounter);
makeDirsOnPath(targetDir);
safef(targetDir, sizeof(targetDir), "%s/models/%s", currDir, clinFieldDir);
makeDirsOnPath(targetDir);
//while you have lock, check for training data existence. If not there, save the space for this thread to create it
if(!trainingDataExists(config))
	{
	spoofTrainingData(config);
	trDataExists = 0;
	}
createModelDirs(config);
pthread_mutex_unlock(&mutex);
//do stuff without lock
safef(configFile, sizeof(configFile), "%s/models/%s/dataset%d_model%d.cfg", currDir, clinFieldDir, datasetCounter, modelNum);
printMLconfigFile(config, configFile);
if(!trDataExists) //this thread has been flagged to write some data
	{
	writeSplits(config, data, metadata);
	}
//get lock again
while(pthread_mutex_lock(&mutex) != 0)
	usleep(1);
writeClusterJobs(config);
safef(targetFile, sizeof(targetFile), "%s/printJobsRa.txt", currDir);
FILE *printJobsRa_fp = mustOpen(targetFile, "a");
fprintf(printJobsRa_fp, "%s/MLwriteResultsToRa %s/models/%s/dataset%d_model%d.cfg -jobs=%s/models/%s/.dataset%d_model%d.ra -id=%d\n", clusterRootDir, currDir, clinFieldDir, datasetCounter, modelNum, currDir, clinFieldDir, datasetCounter, modelNum, modelNum);
fclose(printJobsRa_fp);
safef(targetFile, sizeof(targetFile), "%s/wrapupJobs.txt", currDir);
FILE *wrapupJobs_fp = mustOpen(targetFile, "a");
fprintf(wrapupJobs_fp, "%s/MLwriteResultsToRa %s/models/%s/dataset%d_model%d.cfg -tasks=%s/results.ra -subgroups=%s/results.ra -featureSelections=%s/results.ra -transformations=%s/results.ra -classifiers=%s/results.ra\n", clusterRootDir, currDir, clinFieldDir, datasetCounter, modelNum, currDir, currDir, currDir, currDir, currDir);
fclose(wrapupJobs_fp);
//update progress markers
touchedModels++;
printf("%.3f%% complete\n", ((float)touchedModels/totalModels)*100);
//release lock
pthread_mutex_unlock(&mutex);
//clean up mem
freeMem(clinFieldDir);
return NULL;
}

struct hash * copy_hash(struct hash * ptr)
{
struct hash * result = newHash(0);
struct hashEl *hel;
struct hashCookie cookie = hashFirst(ptr);
while ((hel = hashNext(&cookie)) != NULL)
	hashAdd(result, hel->name, hel->val);
return result;
}

int MLbatchSetup()
{
//make a threadpool for splitting data in parallel
pthread_t threads[MAX_THREADS];
struct thread_args threadArgs[MAX_THREADS];
//make a container to store config variables in
struct hash * config = newHash(0);
//set options if they exist
if(optionExists("profile"))
    profile = optionVal("profile", profile);
if(optionExists("db"))
    db = optionVal("db", db);
if(optionExists("clusterRootDir"))
    clusterRootDir = optionVal("clusterRootDir", clusterRootDir);
if(optionExists("excludeList"))
	excludeList = optionVal("excludeList", excludeList);
//open a db connection
struct sqlConnection * conn = hAllocConnProfile(profile, db);
//get the current dir, to use as a root
char * currDir = getCurrentDir();
if(startsWith("/cluster", currDir)) //cluster dir screws up paths. Remove
	currDir = (skipBeyondDelimit((currDir+1), '/') - 1);
//make sure you have files from command line
char * tasksFilename = NULL, * datasetsFilename = NULL, *featureSelectionsFilename = NULL, *classifiersFilename = NULL;
char * clinDiscretizersFilename = NULL, * dataDiscretizersFilename = NULL, *crossValidationFilename = NULL;
if(optionExists("tasks"))
	tasksFilename = optionVal("tasks", tasksFilename);
if(optionExists("datasets"))
    datasetsFilename = optionVal("datasets", datasetsFilename);
if(optionExists("featureSelections"))
    featureSelectionsFilename = optionVal("featureSelections", featureSelectionsFilename);
if(optionExists("classifiers"))
    classifiersFilename = optionVal("classifiers", classifiersFilename);
if(optionExists("clinDiscretizers"))
    clinDiscretizersFilename = optionVal("clinDiscretizers", clinDiscretizersFilename);
if(optionExists("dataDiscretizers"))
    dataDiscretizersFilename = optionVal("dataDiscretizers", dataDiscretizersFilename);
if(optionExists("crossValidation"))
    crossValidationFilename = optionVal("crossValidation", crossValidationFilename);
if(	!tasksFilename || !datasetsFilename || !featureSelectionsFilename || !classifiersFilename || !clinDiscretizersFilename || 
	!dataDiscretizersFilename || !crossValidationFilename)
	{
	fprintf(stderr, "ERROR: Necessary file missing.\n");
	usage();
	}
//open files
FILE *tasks_fp = mustOpen(tasksFilename, "r");
FILE *datasets_fp = mustOpen(datasetsFilename, "r");
FILE *featureSelections_fp = mustOpen(featureSelectionsFilename, "r");
FILE *classifiers_fp = mustOpen(classifiersFilename, "r");
FILE *clinDiscretizers_fp = mustOpen(clinDiscretizersFilename, "r");
FILE *dataDiscretizers_fp = mustOpen(dataDiscretizersFilename, "r");
FILE *crossValidation_fp = mustOpen(crossValidationFilename, "r");
//calculate how many models we're about to build
int numTasks = lineCount(tasks_fp);
int numDatasets = lineCount(datasets_fp);
int numFeatureSelections = lineCount(featureSelections_fp);
int numClassifiers= lineCount(classifiers_fp);
int numClinDiscretizers = lineCount(clinDiscretizers_fp);
int numDataDiscretizers = lineCount(dataDiscretizers_fp);
totalModels =   numTasks * numDatasets * numFeatureSelections *
                    numClassifiers * numClinDiscretizers * numDataDiscretizers;
//read parameters from files in to lists
struct slName * slNamePtr=NULL;
struct slName * clinFields = NULL, *currClinField = NULL;
struct slName * tasks = NULL, *currTask = NULL;
struct slName * datasets = NULL, *currDataset = NULL;
struct slName * featureSelections = NULL, *currFeatureSelection = NULL;
struct slName * featureSelectionParameters = NULL, *currFeatureSelectionParameters = NULL;
struct slName * classifiers = NULL, *currClassifier = NULL;
struct slName * classifierParameters = NULL, *currClassifierParameters = NULL;
struct slName * clinDiscretizers = NULL, *currClinDiscretizer = NULL;
struct slName * clinDiscretizerParameters = NULL, *currClinDiscretizerParameters = NULL;
struct slName * dataDiscretizers = NULL, *currDataDiscretizer = NULL;
struct slName * dataDiscretizerParameters = NULL, *currDataDiscretizerParameters = NULL;

char * fileLine;
//process tasks file
while((fileLine = readLine(tasks_fp)) && fileLine != NULL)
	{
	slNamePtr = slNameListFromString(fileLine, '\t');
	if(!clinFields)
		clinFields = newSlName(slNamePtr->name);
	else
		slNameAddTail(&clinFields, slNamePtr->name);
	if(slNamePtr->next != NULL)
		{
		if(!tasks)
			tasks = newSlName(slNamePtr->next->name);
		else
			slNameAddTail(&tasks, slNamePtr->next->name);
		}
	else
		{
		if(!tasks)
			tasks = newSlName(slNamePtr->name);
		else
			slNameAddTail(&tasks, slNamePtr->name);
		}
	slFreeList(&slNamePtr);
	freeMem(fileLine);
	}
currClinField = clinFields;
currTask = tasks;
//process datasets file
while((fileLine = readLine(datasets_fp)) && fileLine != NULL)
	{
	if(!datasets)
		datasets = newSlName(fileLine);
	else
    	slNameAddTail(&datasets, fileLine);
	freeMem(fileLine);
	}
currDataset = datasets;
//process feature selections file
while((fileLine = readLine(featureSelections_fp)) && fileLine != NULL)
	{
    slNamePtr = slNameListFromString(fileLine, '\t');
	if(!featureSelections || !featureSelectionParameters)
		{
		featureSelections = newSlName(slNamePtr->name);
		featureSelectionParameters = newSlName(slNamePtr->next->name);
		}
	else
		{
    	slNameAddTail(&featureSelections, slNamePtr->name);
    	slNameAddTail(&featureSelectionParameters, slNamePtr->next->name);
		}
	freeMem(fileLine);
    slFreeList(&slNamePtr);
	}
currFeatureSelection = featureSelections;
currFeatureSelectionParameters = featureSelectionParameters;
//process classifiers file
while((fileLine = readLine(classifiers_fp)) && fileLine != NULL)
	{
    slNamePtr = slNameListFromString(fileLine, '\t');
    if(!classifiers || !classifierParameters)
        {
        classifiers = newSlName(slNamePtr->name);
        classifierParameters = newSlName(slNamePtr->next->name);
        }
    else
        {
        slNameAddTail(&classifiers, slNamePtr->name);
        slNameAddTail(&classifierParameters, slNamePtr->next->name);
        }
	freeMem(fileLine);
    slFreeList(&slNamePtr);
	}
currClassifier = classifiers;
currClassifierParameters = classifierParameters;
//process clinDisc file
while((fileLine = readLine(clinDiscretizers_fp)) && fileLine != NULL)
	{
    slNamePtr = slNameListFromString(fileLine, '\t');
    if(!clinDiscretizers || !clinDiscretizerParameters)
        {
        clinDiscretizers = newSlName(slNamePtr->name);
        clinDiscretizerParameters = newSlName(slNamePtr->next->name);
        }
    else
        {
        slNameAddTail(&clinDiscretizers, slNamePtr->name);
        slNameAddTail(&clinDiscretizerParameters, slNamePtr->next->name);
        }
	freeMem(fileLine);
    slFreeList(&slNamePtr);
	}
currClinDiscretizer = clinDiscretizers;
currClinDiscretizerParameters = clinDiscretizerParameters;
//process data discretizer file
while((fileLine = readLine(dataDiscretizers_fp)) && fileLine != NULL)
	{
    slNamePtr = slNameListFromString(fileLine, '\t');
    if(!dataDiscretizers || !dataDiscretizerParameters)
        {
        dataDiscretizers = newSlName(slNamePtr->name);
        dataDiscretizerParameters = newSlName(slNamePtr->next->name);
        }
    else
        {
        slNameAddTail(&dataDiscretizers, slNamePtr->name);
        slNameAddTail(&dataDiscretizerParameters, slNamePtr->next->name);
        }
	freeMem(fileLine);
    slFreeList(&slNamePtr);
	}
currDataDiscretizer = dataDiscretizers;
currDataDiscretizerParameters = dataDiscretizerParameters;
//process crossvaliidation file
fileLine = readLine(crossValidation_fp);
if(!fileLine)
    errAbort("No cross validation scheme found");
slNamePtr = slNameListFromString(fileLine, '\t');
freeMem(fileLine);
hashReplace(config, "crossValidation", cloneString(slNamePtr->name));
if(sameString(hashMustFindVal(config, "crossValidation"), "k-fold"))
    {
    hashReplace(config, "folds", cloneString(slNamePtr->next->name));
    if(slNamePtr->next->next->name)
        hashReplace(config, "foldMultiplier", cloneString(slNamePtr->next->next->name));
    }
slFreeList(&slNamePtr);
//close file handles
fclose(tasks_fp);
fclose(datasets_fp);
fclose(featureSelections_fp);
fclose(classifiers_fp);
fclose(clinDiscretizers_fp);
fclose(dataDiscretizers_fp);
//make space to save various versions of data and metadata
matrix * prefilteringData = NULL, *data = NULL, *filteredData = NULL, *discretizedData = NULL;
matrix * presubgroupingMetadata = NULL, *filteredMetadata = NULL, *subgroupedMetadata = NULL;
//set counters
int datasetCounter=1,featureSelectionCounter=1,dataDiscretizerCounter=1,taskCounter=1,clinDiscretizerCounter=1, modelNum=1;

//save variables in config that won't change between rounds
hashReplace(config, "inputType", "bioInt");
hashReplace(config, "profile", profile);
hashReplace(config, "db", db);
if(optionExists("excludeList"))
	hashReplace(config, "excludeList", excludeList);
//iterate over all combinatorial parameters, writing as you go
while(currDataset)
	{
	hashReplace(config, "tableName", currDataset->name);
	prefilteringData = bioInt_fill_matrix(conn, hashMustFindVal(config, "tableName"));
	data = NULL;
	if(excludeList)
    	data = filterColumnsByExcludeList(config, prefilteringData);
	else
		data = copy_matrix(prefilteringData);
	if(!data)
		{
        touchedModels += numFeatureSelections*numDataDiscretizers*numTasks*numClinDiscretizers*numClassifiers;
		continue;
		}
	featureSelectionCounter=1;
	while( currFeatureSelection && currFeatureSelectionParameters )
		{
		hashReplace(config, "featureSelection", currFeatureSelection->name);
		hashReplace(config, "featureSelectionParameters", currFeatureSelectionParameters->name);
		if(!sameString("None", hashMustFindVal(config, "featureSelection")))
    		filteredData = featureSelection(config, data);
		else
			filteredData = copy_matrix(data);
		if(!filteredData)
			{
			touchedModels += numDataDiscretizers*numTasks*numClinDiscretizers*numClassifiers;
			continue;
			}
		dataDiscretizerCounter =1;
		while( currDataDiscretizer && currDataDiscretizerParameters )
			{
			hashReplace(config, "dataDiscretizer", currDataDiscretizer->name);
			hashReplace(config, "dataDiscretizerParameters", currDataDiscretizerParameters->name);
			if(hashFindVal(config, "dataDiscretizer") && !sameString("None", hashFindVal(config, "dataDiscretizer")))
   				discretizedData  = discretizeData(config, filteredData);
			else
				discretizedData = copy_matrix(filteredData);
			if(!discretizedData)
				{
				touchedModels += numTasks*numClinDiscretizers*numClassifiers;
				continue;
				}
			taskCounter = 1;
			while( currTask && currClinField )
				{
				hashReplace(config, "task", currTask->name);
				hashReplace(config, "clinField", currClinField->name);
               	presubgroupingMetadata = bioInt_fill_KH(conn, discretizedData, currClinField->name);
              	if(hashFindVal(config, "excludeList"))
                    filteredMetadata = filterColumnsByExcludeList(config, presubgroupingMetadata);
				else
					filteredMetadata = copy_matrix(presubgroupingMetadata);
				if(!filteredMetadata)
					{
					touchedModels += numClinDiscretizers*numClassifiers;
					continue;
					}
				clinDiscretizerCounter=1;
				while( currClinDiscretizer && currClinDiscretizerParameters )
					{
					hashReplace(config, "clinDiscretizer", currClinDiscretizer->name);
					hashReplace(config, "clinDiscretizerParameters", currClinDiscretizerParameters->name);
					if(hashFindVal(config, "clinDiscretizer") && !sameString("None", hashFindVal(config, "clinDiscretizer")))
    					subgroupedMetadata = discretizeMetadata(config, filteredMetadata);
					else
						subgroupedMetadata = copy_matrix(filteredMetadata);
					if(!subgroupedMetadata)
						{
						touchedModels += numClassifiers;
						continue;
						}
					int i = 0;
					while(i < numClassifiers)
						{
						//create a thread per model
						int desiredThreads = MAX_THREADS;
						int threadCounter = 0;
						if(desiredThreads > (numClassifiers - i))
							desiredThreads = (numClassifiers - i);
						for(threadCounter = 0; threadCounter < desiredThreads; i++, threadCounter++)
							{
							hashReplace(config, "classifier", currClassifier->name);
							hashReplace(config, "parameters", currClassifierParameters->name);
							if(sameString(currClassifier->name,"NMFpredictor") || sameString(currClassifier->name, "glmnet"))
								hashReplace(config, "outputType", "flatfiles");
							else if(sameString(currClassifier->name, "SVMlight"))
								hashReplace(config, "outputType", "SVMlight");
							else if(sameString(currClassifier->name, "WEKA"))
								hashReplace(config, "outputType", "WEKA");
							else
								errAbort("Unsupported classifier specified\n");
							//create a struct with full copies of all thread arguments
							threadArgs[threadCounter].config = copy_hash(config);
							threadArgs[threadCounter].data = copy_matrix(discretizedData);
							threadArgs[threadCounter].metadata = copy_matrix(subgroupedMetadata);
							threadArgs[threadCounter].currDir = currDir;
							threadArgs[threadCounter].taskCounter = taskCounter;
							threadArgs[threadCounter].datasetCounter = datasetCounter;
                            threadArgs[threadCounter].clinDiscretizerCounter = clinDiscretizerCounter;
                            threadArgs[threadCounter].dataDiscretizerCounter = dataDiscretizerCounter;
                            threadArgs[threadCounter].featureSelectionCounter = featureSelectionCounter;
                            threadArgs[threadCounter].datasetCounter = datasetCounter;
							threadArgs[threadCounter].modelNum = modelNum;
							pthread_create(&threads[threadCounter], NULL, threadSafeWrite,(void *)&threadArgs[threadCounter]); 
							//advance counters/pointers
							currClassifier = currClassifier->next;
							currClassifierParameters = currClassifierParameters->next;
							modelNum++;
							}//end thread creating loop
                		for(threadCounter = 0; threadCounter < desiredThreads; threadCounter++)
							{
							//bind main to thread, then free resources
                        	pthread_join(threads[threadCounter], NULL);
                            freeHash(&threadArgs[threadCounter].config); 
                            free_matrix(threadArgs[threadCounter].data);
                            free_matrix(threadArgs[threadCounter].metadata);
							}//end thread joining loop
						}//end foreach classifier
					//clean up subgrouping level data
					free_matrix(subgroupedMetadata);
					//advane counters/pointers
					currClinDiscretizer = currClinDiscretizer->next;
					currClinDiscretizerParameters = currClinDiscretizerParameters->next;	
					clinDiscretizerCounter++;
					//rewind to begining of list of classifiers, advance counters
					currClassifier=classifiers;
					currClassifierParameters= classifierParameters;
					}//end foreach clinDiscretizer
				//clean up
				free_matrix(filteredMetadata);
				free_matrix(presubgroupingMetadata);
				//advance counters/pointers
				currClinField = currClinField->next;
				currTask = currTask->next;
				taskCounter++;
				//rewind to beginning of list of clinDiscretizers, advance counter
				currClinDiscretizer = clinDiscretizers;
				currClinDiscretizerParameters = clinDiscretizerParameters;
				}//end foreach task
			//clean up discretization level data
			free_matrix(discretizedData);
			//advance counters/pointers
			currDataDiscretizer = currDataDiscretizer->next;
			currDataDiscretizerParameters = currDataDiscretizerParameters->next;
			dataDiscretizerCounter++;
			//rewind to beginning of tasks
			currTask = tasks;
			currClinField = clinFields;
			}//end foreach dataDiscretizer
		//clean up feature selection level data
		free_matrix(filteredData);
		//advance pointers/counters
		currFeatureSelection = currFeatureSelection->next;
		currFeatureSelectionParameters = currFeatureSelectionParameters->next;
		featureSelectionCounter++;
		//rewind to begining of data discretizers
		currDataDiscretizer = dataDiscretizers;
		currDataDiscretizerParameters = dataDiscretizerParameters;
		}//end foreach featureSelection
	//clean up dataset level data
	free_matrix(data);
	free_matrix(prefilteringData);
	//avance pointers/counters
	currDataset = currDataset->next;
	datasetCounter++;
	//reset to beginning of feature selections file, advance counter
	currFeatureSelection = featureSelections;
	currFeatureSelectionParameters = featureSelectionParameters;
	}//end foreach dataset

//wait for all threads to have completed, then add finishing up jobs to wrapup file
int waitTime=0, acceptableWaitTime=300;//5min
while((touchedModels < totalModels) && (waitTime < acceptableWaitTime))
	{
	printf("%d/%d models have been processed. Waiting for last writes to finish...\n", touchedModels, totalModels);
	sleep(1);
	waitTime++;
	}
printf("100%% complete. %d/%d models were skipped.\n", (totalModels - (modelNum-1)), totalModels);
char targetFile[1024];
safef(targetFile, sizeof(targetFile), "%s/wrapupJobs.txt", currDir);
FILE *wrapupJobs_fp = mustOpen(targetFile, "a");
fprintf(wrapupJobs_fp, "cat %s/models/*/.*.ra >> results.ra\n", currDir);
fprintf(wrapupJobs_fp, "rm %s/models/*/.*.ra", currDir);
fclose(wrapupJobs_fp);

//clean up
slFreeList(&tasks);
slFreeList(&clinFields);
slFreeList(&classifiers);
slFreeList(&classifierParameters);
slFreeList(&datasets);
slFreeList(&clinDiscretizers);
slFreeList(&clinDiscretizerParameters);
slFreeList(&featureSelections);
slFreeList(&featureSelectionParameters);
slFreeList(&dataDiscretizers);
slFreeList(&dataDiscretizerParameters);
freeMem(hashMustFindVal(config, "crossValidation"));
if(hashFindVal(config, "folds"))
	freeMem(hashMustFindVal(config, "folds"));
if(hashFindVal(config, "foldMultiplier"))
    freeMem(hashMustFindVal(config, "foldMultiplier"));
freeHash(&config);
//close connection
hFreeConn(&conn);

return 0;
}

int main(int argc, char *argv[])
{
optionInit(&argc, argv, options);
MLbatchSetup();
return 0;
}
