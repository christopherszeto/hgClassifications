#include "common.h"
#include "linefile.h"
#include "hash.h"
#include "jksql.h"
#include "hPrint.h"
#include "hdb.h"
#include "ra.h"
#include "../inc/classificationTables.h"
#include "../inc/MLio.h"

/******* Utility functions start here ******/
void createJobsTable(struct sqlConnection * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE jobs (\n");
dyStringPrintf(dy, "id int unsigned auto_increment not null,\n");
dyStringPrintf(dy, "datasets_id int unsigned not null,\n");
dyStringPrintf(dy, "tasks_id int unsigned not null,\n");
dyStringPrintf(dy, "classifiers_id int unsigned not null,\n");
dyStringPrintf(dy, "featureSelections_id int unsigned not null,\n");
dyStringPrintf(dy, "transformations_id int unsigned not null,\n");
dyStringPrintf(dy, "subgroups_id int unsigned not null,\n");
dyStringPrintf(dy, "avgTestingAccuracy float not null,\n");
dyStringPrintf(dy, "avgTestingAccuracyGain float not null,\n");
dyStringPrintf(dy, "accuracyType varchar(255) not null,\n");
dyStringPrintf(dy, "features longblob,\n");
dyStringPrintf(dy, "weights longblob,\n");
dyStringPrintf(dy, "modelPath longblob,\n");
dyStringPrintf(dy, "PRIMARY KEY(id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn,dy->string);
dyStringFree(&dy);
}

void createTasksTable(struct sqlConnection * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE tasks (\n");
dyStringPrintf(dy, "id int unsigned auto_increment not null,\n");
dyStringPrintf(dy, "name varchar(255) not null,\n");
dyStringPrintf(dy, "PRIMARY KEY(id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn,dy->string);
dyStringFree(&dy);
}

void createClassifiersTable(struct sqlConnection * conn){
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE classifiers (\n");
dyStringPrintf(dy, "id int unsigned auto_increment not null,\n");
dyStringPrintf(dy, "name varchar(255) not null,\n");
dyStringPrintf(dy, "parameters varchar(255) not null,\n");
dyStringPrintf(dy, "PRIMARY KEY(id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn,dy->string);
dyStringFree(&dy);
}

void createClassifierResultsTable(struct sqlConnection * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE classifierResults (\n");
dyStringPrintf(dy, "id int unsigned auto_increment not null,\n");
dyStringPrintf(dy, "tasks_id int unsigned not null,\n");
dyStringPrintf(dy, "jobs_id int unsigned not null,\n");
dyStringPrintf(dy, "sample_id int unsigned not null,\n");
dyStringPrintf(dy, "trainingAccuracy float,\n");
dyStringPrintf(dy, "testingAccuracy float,\n");
dyStringPrintf(dy, "predictionScore float,\n");
dyStringPrintf(dy, "PRIMARY KEY(id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn,dy->string);
dyStringFree(&dy);
}

void createFeatureSelectionsTable(struct sqlConnection * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE featureSelections (\n");
dyStringPrintf(dy, "id int unsigned not null,\n");
dyStringPrintf(dy, "name varchar(255) not null,\n");
dyStringPrintf(dy, "parameters varchar(255) not null,\n");
dyStringPrintf(dy, "datasets_id int unsigned not null,\n");
dyStringPrintf(dy, "featureCount int unsigned not null,\n");
dyStringPrintf(dy, "PRIMARY KEY(id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn,dy->string); 
dyStringFree(&dy); 
}

void createTransformationsTable(struct sqlConnection * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE transformations (\n");
dyStringPrintf(dy, "id int unsigned not null,\n");
dyStringPrintf(dy, "name varchar(255) not null,\n");
dyStringPrintf(dy, "parameters varchar(255) not null,\n");
dyStringPrintf(dy, "PRIMARY KEY(id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn,dy->string);
dyStringFree(&dy);
}

void createSubgroupsTable(struct sqlConnection * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE subgroups (\n");
dyStringPrintf(dy, "id int unsigned auto_increment not null,\n");
dyStringPrintf(dy, "name varchar(255) not null,\n");
dyStringPrintf(dy, "parameters varchar(255) not null,\n");
dyStringPrintf(dy, "datasets_id int unsigned not null,\n");
dyStringPrintf(dy, "PRIMARY KEY(id)\n");
dyStringPrintf(dy, ");\n");
sqlUpdate(conn,dy->string);
dyStringFree(&dy);
}

void createSamplesInSubgroupsTable(struct sqlConnection * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE samplesInSubgroups (\n");
dyStringPrintf(dy, "id int unsigned auto_increment not null,\n");
dyStringPrintf(dy, "subgroups_id int unsigned not null,\n");
dyStringPrintf(dy, "subgroup int unsigned not null,\n");
dyStringPrintf(dy, "sample_id int unsigned not null,\n");
dyStringPrintf(dy, "PRIMARY KEY(id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn,dy->string);
dyStringFree(&dy);
}

void createBackgroundModelScoresTable(struct sqlConnection * conn)
{
struct dyString * dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE backgroundModelScores (\n");
dyStringPrintf(dy, "id int unsigned not null,\n"); 
dyStringPrintf(dy, "jobs_id int unsigned not null,\n");
dyStringPrintf(dy, "data_table varchar(255) not null,\n");
dyStringPrintf(dy, "max float not null,\n");
dyStringPrintf(dy, "mean float not null,\n");
dyStringPrintf(dy, "median float not null,\n");
dyStringPrintf(dy, "sd float not null,\n");
dyStringPrintf(dy, "min float not null,\n");
dyStringPrintf(dy, "estimatedRunTime int unsigned not null,\n");
dyStringPrintf(dy, "PRIMARY KEY(id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn, dy->string);
dyStringFree(&dy);
}                                                                                                             

void setupTable(struct sqlConnection *conn, char *tableName, int dropTables)
/*Generic setupTable function to fire specific creators
* Drops tables if specified. */
{
uglyTime(NULL);
fprintf(stderr, "Setting up %s table...\n", tableName);
if (sqlTableExists(conn, tableName) && dropTables)
    {
    fprintf(stderr, "Table %s already exists in db, dropping...\n", tableName);
	sqlDropTable(conn, tableName);
	}
if (!sqlTableExists(conn, tableName))
    {
    fprintf(stderr, "Creating table %s...\n", tableName);
	if(sameString(tableName, "jobs"))
		createJobsTable(conn);
	else if(sameString(tableName, "tasks"))
		createTasksTable(conn);
    else if(sameString(tableName, "classifiers"))
        createClassifiersTable(conn);
    else if(sameString(tableName, "classifierResults"))
        createClassifierResultsTable(conn);
    else if(sameString(tableName, "featureSelections"))
        createFeatureSelectionsTable(conn);
    else if(sameString(tableName, "transformations"))
        createTransformationsTable(conn);
	else if(sameString(tableName, "subgroups"))
        createSubgroupsTable(conn);
	else if(sameString(tableName, "samplesInSubgroups"))
        createSamplesInSubgroupsTable(conn);
	else if(sameString(tableName , "backgroundModelScores"))
		createBackgroundModelScoresTable(conn);
    }
uglyTime("Time");
}

/**** Ra Entry parsers **********/
void saveTaskRa(struct sqlConnection * conn, struct hash * raHash)
{
fprintf(stderr, "Saving task...\n");
char * name = (char *)hashMustFindVal(raHash, "label");
char query[512];
safef(query, sizeof(query),"select id from tasks where name=\'%s\'", name);
if(!sqlExists(conn, query))
	{
	struct tasks *task;
	AllocVar(task);
	safef(query, sizeof(query), "select max(id) from tasks");
	task->id = sqlQuickNum(conn, query)+1;
	task->name = cloneString(name);
	tasksSaveToDb(conn, task, "tasks", 1024);
	tasksFree(&task);
	}
else
	{
	fprintf(stderr, "Warning: Duplicate entries for task %s\n", name);
	}
}

void saveSubgroupingRa(struct sqlConnection * conn, struct hash * raHash)
/*TODO: Add an iterator that'll allow adding more than two subgroups*/
{
fprintf(stderr, "Saving subgrouping ...\n");
//parse subgroup info
char * name = (char *)hashMustFindVal(raHash, "label");
char * parameters = (char *)hashMustFindVal(raHash, "parameters");

int subgroups_id;
char query[512];
safef(query, sizeof(query), "select id from datasets where data_table=\'%s\'",
    (char *)hashMustFindVal(raHash, "dataset"));
int datasets_id = sqlQuickNum(conn, query);
safef(query, sizeof(query),"select id from subgroups where name=\'%s\' and parameters=\'%s\' and datasets_id=\'%d\'",
	name, parameters, datasets_id);
if(!sqlExists(conn, query))
    {
	struct subgroups *sgd;
	AllocVar(sgd);
	sgd->id = sqlTableSize(conn, "subgroups")+1;
	subgroups_id = sgd->id;
	sgd->name=cloneString(name);
	sgd->parameters=cloneString(parameters);
	sgd->datasets_id=datasets_id;
	subgroupsSaveToDb(conn, sgd, "subgroups", 1024);
	subgroupsFree(&sgd);

	//parse samples in subgroups
	fprintf(stderr, "Saving samples in subgroups ...\n");
	struct slName *sample;
	int subgroup,subgroups = 2; //TODO:hardcoded for now. Should be set in raHash/
	char subgroupLabel[256];
	for(subgroup = 1; subgroup <= subgroups; subgroup++)
		{
		safef(subgroupLabel, sizeof(subgroupLabel), "subgroup%d", subgroup);
		char * subgroupString = (char *)hashMustFindVal(raHash, subgroupLabel);
		struct slName *subgroupSamples = slNameListFromComma(subgroupString);
		for(sample=subgroupSamples; sample != NULL; sample=sample->next)
			{
			if(!sameString(sample->name, ""))
				{
		 		struct samplesInSubgroups * sg;
		    	AllocVar(sg);
		    	sg->id = sqlTableSize(conn, "samplesInSubgroups")+1;
				sg->subgroups_id = subgroups_id;
				sg->subgroup = subgroup;
		    	safef(query, sizeof(query), "select id from samples where name=\'%s\' and dataset_id=\'%d\' limit 1", sample->name, datasets_id);
		    	sg->sample_id = sqlNeedQuickNum(conn, query);
		   		samplesInSubgroupsSaveToDb(conn, sg, "samplesInSubgroups", 1024);
		    	samplesInSubgroupsFree(&sg);
				}
			}
		}
	}
else
	{
	fprintf(stderr, "Warning: Duplicate entry for subgrouping %s %s %d", name, parameters, datasets_id);
	}
}

void saveClassifierRa(struct sqlConnection * conn, struct hash * raHash)
{
fprintf(stderr, "Saving classifier ...\n");
char * name = (char *)hashMustFindVal(raHash, "label");
char * parameters = (char *)hashMustFindVal(raHash, "parameters");
char query[512];
safef(query, sizeof(query), "select id from classifiers where name=\'%s\' and parameters=\'%s\'",
    name, parameters);
if(!sqlExists(conn, query))
    {
    struct classifiers * cl;
    AllocVar(cl);
	cl->id = sqlTableSize(conn, "classifiers")+1;
    cl->name=cloneString(name);
    cl->parameters=cloneString(parameters);
    classifiersSaveToDb(conn, cl, "classifiers", 1024);
    classifiersFree(&cl);
    }
else
    {
    fprintf(stderr, "Warning: Duplicate classifiers entry %s,%s", name, parameters);
    }
}

void saveFeatureSelectionRa(struct sqlConnection * conn, struct hash * raHash)
{
printf("Saving feature selection...\n");
char * name = (char *)hashMustFindVal(raHash, "label");
char * parameters = (char *) hashMustFindVal(raHash, "parameters");
char * dataset = (char *) hashMustFindVal(raHash, "dataset");
int featureCount = atoi(hashMustFindVal(raHash, "featureCount"));

char query[512];
safef(query, sizeof(query), "SELECT id FROM datasets WHERE data_table =\'%s\'", dataset);
int datasets_id = sqlQuickNum(conn, query);
safef(query, sizeof(query), "select id from featureSelections where name=\'%s\' and parameters=\'%s\' and datasets_id=%d", 
	name, parameters, datasets_id);
if(!sqlExists(conn, query))
	{
	struct featureSelections * fs;
	AllocVar(fs);
	fs->id=sqlTableSize(conn, "featureSelections")+1;
	fs->name=cloneString(name);
	fs->parameters=cloneString(parameters);
	fs->datasets_id=datasets_id;
	fs->featureCount=featureCount;
	featureSelectionsSaveToDb(conn, fs, "featureSelections", 1024);
	featureSelectionsFree(&fs);
	}
else
	{
	fprintf(stderr, "WARNING: Trying to duplicate featureSelections entry %s,%s", name, parameters);
	}
}

void saveTransformationRa(struct sqlConnection * conn, struct hash * raHash)
{
fprintf(stderr, "Saving transformation ...\n");
char * name = (char *)hashMustFindVal(raHash, "label");
char * parameters = (char *)hashMustFindVal(raHash, "parameters");
char query[512];
safef(query, sizeof(query), "select id from transformations where name=\'%s\' and parameters=\'%s\'",
    name, parameters);
if(!sqlExists(conn, query))
    {
    struct transformations * cl;
    AllocVar(cl);
    cl->id = sqlTableSize(conn, "transformations")+1;
    cl->name=cloneString(name);
    cl->parameters=cloneString(parameters);
    transformationsSaveToDb(conn, cl, "transformations", 1024);
    transformationsFree(&cl);
    }
else
    {
    fprintf(stderr, "Warning: Duplicate transformations entry %s,%s", name, parameters);
    }
}


void saveJobRa(struct sqlConnection * conn, struct hash * raHash)
{
char query[512];
//save task to table, if it doesnt exist
safef(query, sizeof(query), "select id from tasks where name=\'%s\'", (char *)hashMustFindVal(raHash, "task"));
int tasks_id;
if(!sqlExists(conn, query))
	{
	struct tasks * task;
	AllocVar(task);
	task->id = sqlTableSize(conn, "tasks")+1;
	tasks_id = task->id; //save this for classifierResults saving
	task->name = cloneString( (char *)hashMustFindVal(raHash, "task"));
	tasksSaveToDb(conn, task, "tasks", 1024);
    tasksFree(&task);
	}
else
	tasks_id = sqlNeedQuickNum(conn, query);

//save to classierResults table
int jobs_id = sqlTableSize(conn, "jobs")+1;
fprintf(stderr, "Saving classifierResults...\n");
char * samplesString = (char *)hashMustFindVal(raHash, "samples");
struct slName *sl, *sampleList = slNameListFromComma(samplesString);
char * trainingAccString = (char *)hashMustFindVal(raHash, "trainingAccuracies");
struct slName *tr, *trainingAccList = slNameListFromComma(trainingAccString); //despite being double these have to be strings because of NULL
char * testingAccString = (char *)hashMustFindVal(raHash, "testingAccuracies");
struct slName *te,*testingAccList = slNameListFromComma(testingAccString);
char * predScoresString = (char *)hashMustFindVal(raHash, "predictionScores");
struct slName *ps,*predScoresList = slNameListFromComma(predScoresString);

double avgTeAcc=0, teDenom=0;
char *dataset = (char *)hashMustFindVal(raHash, "dataset");
safef(query, sizeof(query), "SELECT id FROM datasets WHERE data_table =\'%s\'", dataset);
int dataset_id = sqlQuickNum(conn, query);
int allNull = 1; //check for all nulls - these break saving the job
for(sl=sampleList,tr=trainingAccList,te=testingAccList, ps=predScoresList; sl && te && tr && ps; sl=sl->next,tr=tr->next,te=te->next, ps=ps->next)
	{
	if(!sameString(sl->name, ""))
		{	
		int valid = 1;
		int id,sample_id;
		id = sqlTableSize(conn, "classifierResults")+1;
		safef(query, sizeof(query),"SELECT id FROM samples WHERE name=\'%s\' AND dataset_id=%d", sl->name, dataset_id);
		sample_id = sqlNeedQuickNum(conn, query);
	
		struct dyString *dy = newDyString(1024);
		dyStringPrintf(dy, "INSERT INTO classifierResults (id,tasks_id,jobs_id,sample_id,trainingAccuracy,testingAccuracy,predictionScore) VALUES (");
		dyStringPrintf(dy, "%d,", id);
		dyStringPrintf(dy, "%d,", tasks_id);
		dyStringPrintf(dy, "%d,", jobs_id);
		dyStringPrintf(dy, "%d", sample_id);
		if(!sameString(tr->name, "NULL") && !sameString(te->name, "NULL") && !sameString(ps->name, "NULL"))
	   		{
			double trAcc = atof(tr->name);
			double teAcc = atof(te->name);
			double predScore = atof(ps->name);
			dyStringPrintf(dy, ",%.4f,%.4f,%.4f)", trAcc, teAcc, predScore);
			avgTeAcc += teAcc;
			teDenom++;
			allNull = 0;
	    	}
		else
			valid = 0;
	
		//save the classifierResult string to table
		if(valid)
			sqlUpdate(conn, dy->string);
		dyStringFree(&dy);
		}
	}
	
//save to jobs table
fprintf(stderr, "Saving job %s...\n", (char*)hashMustFindVal(raHash, "name"));
if(!allNull)
	{
	struct jobs * job;
	AllocVar(job);
	job->id = jobs_id;
	safef(query, sizeof(query), "select id from datasets where data_table=\'%s\'",
	    (char *)hashMustFindVal(raHash, "dataset"));
	job->datasets_id = sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "select id from tasks where name=\'%s\'",
	    (char *)hashMustFindVal(raHash, "task"));
	job->tasks_id = sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "select id from classifiers where name=\'%s\' and parameters=\'%s\'",
	    (char *)hashMustFindVal(raHash, "classifier"), (char *)hashMustFindVal(raHash, "classifierParameters"));
	job->classifiers_id = sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "select id from featureSelections where name=\'%s\' and parameters=\'%s\' and datasets_id=%d",
	    (char *)hashMustFindVal(raHash, "featureSelection"), (char *)hashMustFindVal(raHash, "featureSelectionParameters"), job->datasets_id);
	job->featureSelections_id = sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "select id from transformations where name=\'%s\' and parameters=\'%s\'",
	    (char *)hashMustFindVal(raHash, "dataDiscretizer"), (char *)hashMustFindVal(raHash, "dataDiscretizerParameters"));
	job->transformations_id = sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "select id from subgroups where name=\'%s\' and parameters=\'%s\' and datasets_id='%d'",
	    (char *)hashMustFindVal(raHash, "subgrouping"), (char *)hashMustFindVal(raHash, "subgroupingParameters"), job->datasets_id);
	job->subgroups_id = sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "SELECT count(*) FROM samplesInSubgroups WHERE subgroups_id=%d AND subgroup=1",
		job->subgroups_id);
	int n1 = sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "SELECT count(*) FROM samplesInSubgroups WHERE subgroups_id=%d AND subgroup=2",
	    job->subgroups_id);
	int n2 = sqlNeedQuickNum(conn, query);
	double majorityClassifierAcc = 0;
	if(n1 > n2)
		majorityClassifierAcc = (double)n1/(n1+n2);
	else
		majorityClassifierAcc = (double)n2/(n1+n2);
	if(teDenom == 0)
		job->avgTestingAccuracy = 0; //catch divide-by-zero.. This may unfairly discriminate against all-null splits..
	else
		job->avgTestingAccuracy = avgTeAcc/teDenom;
	job->avgTestingAccuracyGain = job->avgTestingAccuracy - majorityClassifierAcc;
	job->accuracyType = cloneString((char *)hashMustFindVal(raHash, "accuracyType"));
	job->features = cloneString((char *)hashMustFindVal(raHash, "topFeatures"));
	job->weights = cloneString((char *)hashMustFindVal(raHash, "topFeatureWeights"));
	job->modelPath = cloneString((char *)hashMustFindVal(raHash, "modelPath"));
	jobsSaveToDb(conn,job,"jobs",1024); 
	jobsFree(&job);
	}
}

/**********************/
void populateClassificationTables(char * profile, char *db, char *raFile, int dropTables)
{
struct sqlConnection *conn = hAllocConnProfile(profile, db);

//check all the tables are in place (or replace them)
setupTable(conn, "jobs", dropTables);
setupTable(conn, "tasks", dropTables);
setupTable(conn, "classifiers", dropTables);
setupTable(conn, "classifierResults", dropTables);
setupTable(conn, "featureSelections", dropTables);
setupTable(conn, "transformations", dropTables);
setupTable(conn, "subgroups", dropTables);
setupTable(conn, "samplesInSubgroups", dropTables);

// read ra file and populate tables with that info
struct hash *raHash, *raHashList = readRaFile(raFile);

for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
	if(sameString(type, "task"))
		saveTaskRa(conn, raHash);	
	}
for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
	if(sameString(type, "subgrouping"))
		saveSubgroupingRa(conn, raHash);
	}
for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
    if(sameString(type, "featureSelection"))
        saveFeatureSelectionRa(conn, raHash);
    }
for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
    if(sameString(type, "transformation"))
        saveTransformationRa(conn, raHash);
    }
for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
	if(sameString(type, "classifier"))
		saveClassifierRa(conn, raHash);
	}
for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
	if(sameString(type, "job"))
		saveJobRa(conn, raHash);
	}

//clean up
hashFree(&raHash);
hashFree(&raHashList);
hFreeConn(&conn);
}
