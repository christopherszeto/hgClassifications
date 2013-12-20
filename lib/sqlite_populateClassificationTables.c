#include "common.h"
#include "linefile.h"
#include "hash.h"
#include "hPrint.h"
#include "hdb.h"
#include "ra.h"
#include "../inc/classificationTables.h"
#include "../inc/MLio.h"
#include "../inc/sqlite3.h"

/******* Utility functions start here ******/
int sqliteExists(sqlite3 * conn, char * query)
{
struct sqlite3_stmt *stmt = NULL;
sqlite3_prepare(conn, query, -1, &stmt, 0);
sqlite3_step(stmt);
char * result = (char *)sqlite3_column_text(stmt, 0);
if(sqlite3_finalize(stmt) != SQLITE_OK)
    errAbort("ERROR: sqlite command %s failed\n", query);
if(result == NULL || sameString(result, ""))
	return 0;
return 1;
}


void sqliteUpdate(sqlite3 *conn, char * query)
{
if(sqlite3_exec(conn, query, NULL, NULL, NULL) != SQLITE_OK)
    errAbort("ERROR: sqlite command %s failed\n", query);
}

int sqliteTableExists(struct sqlite3 * conn, char * tableName)
{
char query[1024];
safef(query, sizeof(query), "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='%s';", tableName);
struct sqlite3_stmt *stmt = NULL;
sqlite3_prepare(conn, query, -1, &stmt, 0);
sqlite3_step(stmt);
int result = sqlite3_column_int(stmt, 0);
if(sqlite3_finalize(stmt) != SQLITE_OK)
    errAbort("ERROR: sqlite command %s failed\n", query);
return result;
}

void sqliteDropTable(struct sqlite3 *conn, char *tableName)
{
char query[256];
safef(query, sizeof(query), "DROP TABLE IF EXISTS %s", tableName);
sqliteUpdate(conn, query);
}

int sqliteTableSize(struct sqlite3 *conn, char * tableName)
{
char query[256];
safef(query, sizeof(query), "SELECT COUNT(*) FROM %s;", tableName);
struct sqlite3_stmt *stmt = NULL;
sqlite3_prepare(conn, query, -1, &stmt, 0);
sqlite3_step(stmt);
int result = sqlite3_column_int(stmt, 0); 
if(sqlite3_finalize(stmt) != SQLITE_OK)
    errAbort("ERROR: sqlite command %s failed\n", query);
return result;
}

int sqliteQuickNum(sqlite3 *conn, char * query)
{
struct sqlite3_stmt *stmt = NULL;
sqlite3_prepare(conn, query, -1, &stmt, 0);
sqlite3_step(stmt);
int result = sqlite3_column_int(stmt, 0);
if(sqlite3_finalize(stmt) != SQLITE_OK)
    errAbort("ERROR: sqlite command %s failed\n", query);
return result;
}

double sqliteQuickDouble(sqlite3 *conn, char * query)
{
struct sqlite3_stmt *stmt = NULL;
sqlite3_prepare(conn, query, -1, &stmt, 0);
sqlite3_step(stmt);
double result = sqlite3_column_double(stmt, 0);
if(sqlite3_finalize(stmt) != SQLITE_OK)
    errAbort("ERROR: sqlite command %s failed\n", query);
return result;
}

char * sqliteQuickString(sqlite3 *conn, char * query)
{
struct sqlite3_stmt *stmt = NULL;
sqlite3_prepare(conn, query, -1, &stmt, 0);
sqlite3_step(stmt);
char * result = cloneString((char *)sqlite3_column_text(stmt, 0));
if(sqlite3_finalize(stmt) != SQLITE_OK)
    errAbort("ERROR: sqlite command %s failed\n", query);
return result;
}

/***** five3db create table statements *****/
void sqlite_createJobsTable(struct sqlite3 * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE jobs (\n");
dyStringPrintf(dy, "\tid INTEGER PRIMARY KEY,\n");
dyStringPrintf(dy, "\tdatasets_id INTEGER,\n");
dyStringPrintf(dy, "\ttasks_id INTEGER,\n");
dyStringPrintf(dy, "\tclassifiers_id INTEGER,\n");
dyStringPrintf(dy, "\tfeatureSelections_id INTEGER,\n");
dyStringPrintf(dy, "\ttransformations_id INTEGER,\n");
dyStringPrintf(dy, "\tsubgroups_id INTEGER,\n");
dyStringPrintf(dy, "\tavgTestingAccuracy DOUBLE,\n");
dyStringPrintf(dy, "\tavgTestingAccuracyGain DOUBLE,\n");
dyStringPrintf(dy, "\taccuracyType VARCHAR(255),\n");
dyStringPrintf(dy, "\tfeatures BLOB,\n");
dyStringPrintf(dy, "\tweights BLOB,\n");
dyStringPrintf(dy, "\tmodelPath BLOB\n");
dyStringPrintf(dy, ")\n");
sqliteUpdate(conn,dy->string);
dyStringFree(&dy);
}

void sqlite_createTasksTable(struct sqlite3 * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE tasks (\n");
dyStringPrintf(dy, "\tid INTEGER PRIMARY KEY,\n");
dyStringPrintf(dy, "\tname VARCHAR(255)\n");
dyStringPrintf(dy, ")\n");
sqliteUpdate(conn,dy->string);
dyStringFree(&dy);
}

void sqlite_createClassifiersTable(struct sqlite3 * conn){
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE classifiers (\n");
dyStringPrintf(dy, "\tid INTEGER PRIMARY KEY,\n");
dyStringPrintf(dy, "\tname VARCHAR(255),\n");
dyStringPrintf(dy, "\tparameters VARCHAR(255)\n");
dyStringPrintf(dy, ")\n");
sqliteUpdate(conn,dy->string);
dyStringFree(&dy);
}

void sqlite_createClassifierResultsTable(struct sqlite3 * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE classifierResults (\n");
dyStringPrintf(dy, "\tid INTEGER PRIMARY KEY,\n");
dyStringPrintf(dy, "\ttasks_id INTEGER,\n");
dyStringPrintf(dy, "\tjobs_id INTEGER,\n");
dyStringPrintf(dy, "\tsample_id INTEGER,\n");
dyStringPrintf(dy, "\ttrainingAccuracy DOUBLE,\n");
dyStringPrintf(dy, "\ttestingAccuracy DOUBLE,\n");
dyStringPrintf(dy, "\tpredictionScore DOUBLE\n");
dyStringPrintf(dy, ")\n");
sqliteUpdate(conn,dy->string);
dyStringFree(&dy);
}

void sqlite_createFeatureSelectionsTable(struct sqlite3 * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE featureSelections (\n");
dyStringPrintf(dy, "\tid INTEGER PRIMARY KEY,\n");
dyStringPrintf(dy, "\tname VARCHAR(255),\n");
dyStringPrintf(dy, "\tparameters VARCHAR(255),\n");
dyStringPrintf(dy, "\tdatasets_id INTEGER,\n");
dyStringPrintf(dy, "\tfeatureCount INTEGER\n");
dyStringPrintf(dy, ")\n");
sqliteUpdate(conn,dy->string);
dyStringFree(&dy); 
}

void sqlite_createTransformationsTable(struct sqlite3 * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE transformations (\n");
dyStringPrintf(dy, "\tid INTEGER PRIMARY KEY,\n");
dyStringPrintf(dy, "\tname VARCHAR(255),\n");
dyStringPrintf(dy, "\tparameters VARCHAR(255)\n");
dyStringPrintf(dy, ")\n");
sqliteUpdate(conn,dy->string);
dyStringFree(&dy);
}

void sqlite_createSubgroupsTable(struct sqlite3 * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE subgroups (\n");
dyStringPrintf(dy, "\tid INTEGER PRIMARY KEY,\n");
dyStringPrintf(dy, "\tname VARCHAR(255),\n");
dyStringPrintf(dy, "\tparameters VARCHAR(255),\n");
dyStringPrintf(dy, "\tdatasets_id INTEGER\n");
dyStringPrintf(dy, ");\n");
sqliteUpdate(conn,dy->string);
dyStringFree(&dy);
}

void sqlite_createSamplesInSubgroupsTable(struct sqlite3 * conn)
{
struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE samplesInSubgroups (\n");
dyStringPrintf(dy, "\tid INTEGER PRIMARY KEY,\n");
dyStringPrintf(dy, "\tsubgroups_id INTEGER,\n");
dyStringPrintf(dy, "\tsubgroup INTEGER,\n");
dyStringPrintf(dy, "\tsample_id INTEGER\n");
dyStringPrintf(dy, ")\n");
sqliteUpdate(conn,dy->string);
dyStringFree(&dy);
}

void sqlite_setupTable(struct sqlite3 * conn, char *tableName, int dropTables)
/*Generic setupTable function to fire specific creators
* Drops tables if specified. */
{
uglyTime(NULL);
fprintf(stderr, "Setting up %s table...\n", tableName);
if (sqliteTableExists(conn, tableName) && dropTables)
    {
    fprintf(stderr, "Table %s already exists in db, dropping...\n", tableName);
	sqliteDropTable(conn, tableName);
	}
if (!sqliteTableExists(conn, tableName))
    {
    fprintf(stderr, "Creating table %s...\n", tableName);
	if(sameString(tableName, "jobs"))
		sqlite_createJobsTable(conn);
	else if(sameString(tableName, "tasks"))
		sqlite_createTasksTable(conn);
    else if(sameString(tableName, "classifiers"))
        sqlite_createClassifiersTable(conn);
    else if(sameString(tableName, "classifierResults"))
        sqlite_createClassifierResultsTable(conn);
    else if(sameString(tableName, "featureSelections"))
        sqlite_createFeatureSelectionsTable(conn);
    else if(sameString(tableName, "transformations"))
        sqlite_createTransformationsTable(conn);
	else if(sameString(tableName, "subgroups"))
        sqlite_createSubgroupsTable(conn);
	else if(sameString(tableName, "samplesInSubgroups"))
        sqlite_createSamplesInSubgroupsTable(conn);
    }
uglyTime("Time");
}

/**** Ra Entry parsers **********/
void sqlite_saveTaskRa(struct sqlite3 * conn, struct hash * raHash)
{
fprintf(stderr, "Saving task...\n");
char * name = (char *)hashMustFindVal(raHash, "label");
char query[512];
safef(query, sizeof(query),"SELECT id FROM tasks WHERE name=\'%s\'", name);
if(!sqliteExists(conn, query))
	{
	struct tasks *task;
	AllocVar(task);
	task->id = sqliteTableSize(conn, "tasks")+1;
	task->name = cloneString(name);
    struct dyString *dy = newDyString(1024);
    dyStringPrintf(dy, "INSERT INTO tasks (id,name) VALUES (");
    dyStringPrintf(dy, "%d,", task->id);
    dyStringPrintf(dy, "'%s');", task->name);
	sqliteUpdate(conn, dy->string);
	tasksFree(&task);
	dyStringFree(&dy);
	}
else
	{
	fprintf(stderr, "Warning: Duplicate entries for task %s\n", name);
	}
}

void sqlite_saveSubgroupingRa(struct sqlite3 * conn, struct hash * raHash)
/*TODO: Add an iterator that'll allow adding more than two subgroups*/
{
fprintf(stderr, "Saving subgrouping ...\n");
//parse subgroup info
char * name = (char *)hashMustFindVal(raHash, "label");
char * parameters = (char *)hashMustFindVal(raHash, "parameters");

int subgroups_id;
char query[512];
safef(query, sizeof(query), "SELECT id FROM data_files WHERE name=\'%s\'",
    (char *)hashMustFindVal(raHash, "dataset"));
int datasets_id = sqliteQuickNum(conn, query);
safef(query, sizeof(query),"SELECT id FROM subgroups WHERE name=\'%s\' AND parameters=\'%s\' AND datasets_id=\'%d\'",
	name, parameters, datasets_id);
if(!sqliteExists(conn, query))
    {
	struct subgroups *sgd;
	AllocVar(sgd);
	sgd->id = sqliteTableSize(conn, "subgroups")+1;
	subgroups_id = sgd->id;
	sgd->name=cloneString(name);
	sgd->parameters=cloneString(parameters);
	sgd->datasets_id=datasets_id;
	
	struct dyString *dy = newDyString(1024);
    dyStringPrintf(dy, "INSERT INTO subgroups (id,name, parameters, datasets_id) VALUES (");
    dyStringPrintf(dy, "%d,", sgd->id);
    dyStringPrintf(dy, "'%s',", sgd->name);
	dyStringPrintf(dy, "'%s',", sgd->parameters);
	dyStringPrintf(dy, "%d);", sgd->datasets_id);
    sqliteUpdate(conn, dy->string);
	dyStringFree(&dy);
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
		    	sg->id = sqliteTableSize(conn, "samplesInSubgroups")+1;
				sg->subgroups_id = subgroups_id;
				sg->subgroup = subgroup;
		    	safef(query, sizeof(query), 
					"SELECT id FROM samples WHERE name=\'%s\' LIMIT 1", //AND dataset_id=\'%d\' LIMIT 1", 
					sample->name);//, datasets_id);
		    	sg->sample_id = sqliteQuickNum(conn, query);
				struct dyString *dy = newDyString(1024);
				dyStringPrintf(dy, "INSERT INTO samplesInSubgroups (id,subgroups_id,subgroup, sample_id) VALUES (");
				dyStringPrintf(dy, "%d,", sg->id);
				dyStringPrintf(dy, "%d,", sg->subgroups_id);
				dyStringPrintf(dy, "%d,", sg->subgroup);
				dyStringPrintf(dy, "%d);", sg->sample_id);
				sqliteUpdate(conn, dy->string);
		    	samplesInSubgroupsFree(&sg);
				dyStringFree(&dy);
				}
			}
		}
	}
else
	{
	fprintf(stderr, "Warning: Duplicate entry for subgrouping %s %s %d", name, parameters, datasets_id);
	}
}

void sqlite_saveClassifierRa(struct sqlite3 * conn, struct hash * raHash)
{
fprintf(stderr, "Saving classifier ...\n");
char * name = (char *)hashMustFindVal(raHash, "label");
char * parameters = (char *)hashMustFindVal(raHash, "parameters");
char query[512];
safef(query, sizeof(query), "SELECT id FROM classifiers WHERE name=\'%s\' AND parameters=\'%s\'",
    name, parameters);
if(!sqliteExists(conn, query))
    {
    struct classifiers * cl;
    AllocVar(cl);
	cl->id = sqliteTableSize(conn, "classifiers")+1;
    cl->name=cloneString(name);
    cl->parameters=cloneString(parameters);
	struct dyString *dy = newDyString(1024);
	dyStringPrintf(dy, "INSERT INTO classifiers (id, name, parameters) VALUES (");
	dyStringPrintf(dy, "%d,", cl->id);
	dyStringPrintf(dy, "'%s',", cl->name);
	dyStringPrintf(dy, "'%s');", cl->parameters);
	sqliteUpdate(conn, dy->string);
	dyStringFree(&dy);
    classifiersFree(&cl);
    }
else
    {
    fprintf(stderr, "Warning: Duplicate classifiers entry %s,%s", name, parameters);
    }
}

void sqlite_saveFeatureSelectionRa(struct sqlite3 * conn, struct hash * raHash)
{
printf("Saving feature selection...\n");
char * name = (char *)hashMustFindVal(raHash, "label");
char * parameters = (char *) hashMustFindVal(raHash, "parameters");
char * dataset = (char *) hashMustFindVal(raHash, "dataset");
int featureCount = atoi(hashMustFindVal(raHash, "featureCount"));

char query[512];
safef(query, sizeof(query), "SELECT id FROM data_files WHERE name=\'%s\'", dataset);
int datasets_id = sqliteQuickNum(conn, query);
safef(query, sizeof(query), "SELECT id FROM featureSelections WHERE name=\'%s\' AND parameters=\'%s\' AND datasets_id=%d", 
	name, parameters, datasets_id);
if(!sqliteExists(conn, query))
	{
	struct featureSelections * fs;
	AllocVar(fs);
	fs->id=sqliteTableSize(conn, "featureSelections")+1;
	fs->name=cloneString(name);
	fs->parameters=cloneString(parameters);
	fs->datasets_id=datasets_id;
	fs->featureCount=featureCount;
	struct dyString *dy = newDyString(1024);
	dyStringPrintf(dy, "INSERT INTO featureSelections (id,name,parameters, datasets_id,featureCount) VALUES (");
	dyStringPrintf(dy, "%d,", fs->id);
	dyStringPrintf(dy, "'%s',", fs->name);
	dyStringPrintf(dy, "'%s',", fs->parameters);
	dyStringPrintf(dy, "%d,",fs->datasets_id);
	dyStringPrintf(dy, "%d);",fs->featureCount);
	sqliteUpdate(conn, dy->string);
	dyStringFree(&dy);
	featureSelectionsFree(&fs);
	}
else
	{
	fprintf(stderr, "WARNING: Trying to duplicate featureSelections entry %s,%s", name, parameters);
	}
}

void sqlite_saveTransformationRa(struct sqlite3 * conn, struct hash * raHash)
{
fprintf(stderr, "Saving transformation ...\n");
char * name = (char *)hashMustFindVal(raHash, "label");
char * parameters = (char *)hashMustFindVal(raHash, "parameters");
char query[512];
safef(query, sizeof(query), "SELECT id FROM transformations WHERE name=\'%s\' AND parameters=\'%s\'",
    name, parameters);
if(!sqliteExists(conn, query))
    {
    struct transformations * cl;
    AllocVar(cl);
    cl->id = sqliteTableSize(conn, "transformations")+1;
    cl->name=cloneString(name);
    cl->parameters=cloneString(parameters);
	struct dyString *dy = newDyString(1024);
    dyStringPrintf(dy, "INSERT INTO transformations (id,name,parameters) VALUES (");
    dyStringPrintf(dy, "%d,", cl->id);
    dyStringPrintf(dy, "'%s',", cl->name);
    dyStringPrintf(dy, "'%s');", cl->parameters);
    sqliteUpdate(conn, dy->string);
    dyStringFree(&dy);
    transformationsFree(&cl);
    }
else
    {
    fprintf(stderr, "Warning: Duplicate transformations entry %s,%s", name, parameters);
    }
}


void sqlite_saveJobRa(struct sqlite3 * conn, struct hash * raHash)
{
char query[512];
//save task to table, if it doesnt exist
safef(query, sizeof(query), "SELECT id FROM tasks WHERE name=\'%s\'", (char *)hashMustFindVal(raHash, "task"));
int tasks_id;
if(!sqliteExists(conn, query))
	{
	struct tasks * task;
	AllocVar(task);
	task->id = sqliteTableSize(conn, "tasks")+1;
	tasks_id = task->id; //save this for classifierResults saving
	task->name = cloneString( (char *)hashMustFindVal(raHash, "task"));
	struct dyString *dy = newDyString(1024);
    dyStringPrintf(dy, "INSERT INTO tasks (id,name) VALUES (");
    dyStringPrintf(dy, "%d,", task->id);
    dyStringPrintf(dy, "'%s');", task->name);
    sqliteUpdate(conn, dy->string);
    dyStringFree(&dy);
    tasksFree(&task);
	}
else
	tasks_id = sqliteQuickNum(conn, query);

//save to classierResults table
int jobs_id = sqliteTableSize(conn, "jobs")+1;
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
for(sl=sampleList,tr=trainingAccList,te=testingAccList, ps=predScoresList; sl && te && tr && ps; sl=sl->next,tr=tr->next,te=te->next, ps=ps->next)
	{
	if(!sameString(sl->name, ""))
		{	
		int valid = 1;
		int id,sample_id;
		id = sqliteTableSize(conn, "classifierResults")+1;
		safef(query, sizeof(query),"SELECT id FROM samples WHERE name=\'%s\'", sl->name);//COULD BE ERRORS HERE; ha to remove dataset_id from this query for five3db
		sample_id = sqliteQuickNum(conn, query);
	
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
	    	}
		else
			valid = 0;
	
		//save the classifierResult string to table
		if(valid)
			sqliteUpdate(conn, dy->string);
		dyStringFree(&dy);
		}
	}
	
//save to jobs table
fprintf(stderr, "Saving job...\n");
struct jobs * job;
AllocVar(job);
job->id = jobs_id;
safef(query, sizeof(query), "SELECT id FROM data_files WHERE name=\'%s\'",
    (char *)hashMustFindVal(raHash, "dataset"));
job->datasets_id = sqliteQuickNum(conn, query);
safef(query, sizeof(query), "SELECT id FROM tasks WHERE name=\'%s\'",
    (char *)hashMustFindVal(raHash, "task"));
job->tasks_id = sqliteQuickNum(conn, query);
safef(query, sizeof(query), "SELECT id FROM classifiers WHERE name=\'%s\' AND parameters=\'%s\'",
    (char *)hashMustFindVal(raHash, "classifier"), (char *)hashMustFindVal(raHash, "classifierParameters"));
job->classifiers_id = sqliteQuickNum(conn, query);
safef(query, sizeof(query), "SELECT id FROM featureSelections WHERE name=\'%s\' AND parameters=\'%s\' AND datasets_id=%d",
    (char *)hashMustFindVal(raHash, "featureSelection"), (char *)hashMustFindVal(raHash, "featureSelectionParameters"), job->datasets_id);
job->featureSelections_id = sqliteQuickNum(conn, query);
safef(query, sizeof(query), "SELECT id FROM transformations WHERE name=\'%s\' AND parameters=\'%s\'",
    (char *)hashMustFindVal(raHash, "dataDiscretizer"), (char *)hashMustFindVal(raHash, "dataDiscretizerParameters"));
job->transformations_id = sqliteQuickNum(conn, query);
safef(query, sizeof(query), "SELECT id FROM subgroups WHERE name=\'%s\' AND parameters=\'%s\' AND datasets_id='%d'",
    (char *)hashMustFindVal(raHash, "subgrouping"), (char *)hashMustFindVal(raHash, "subgroupingParameters"), job->datasets_id);
job->subgroups_id = sqliteQuickNum(conn, query);
safef(query, sizeof(query), "SELECT count(*) FROM samplesInSubgroups WHERE subgroups_id=%d AND subgroup=1",
	job->subgroups_id);
int n1 = sqliteQuickNum(conn, query);
safef(query, sizeof(query), "SELECT count(*) FROM samplesInSubgroups WHERE subgroups_id=%d AND subgroup=2",
    job->subgroups_id);
int n2 = sqliteQuickNum(conn, query);
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

struct dyString *dy = newDyString(4096);
dyStringPrintf(dy, "INSERT INTO jobs (id,datasets_id, tasks_id,classifiers_id,featureSelections_id,transformations_id,subgroups_id,avgTestingAccuracy,avgTestingAccuracyGain,accuracyType,features,weights,modelPath) VALUES ("),
dyStringPrintf(dy, "%d,",job->id);
dyStringPrintf(dy, "%d,",job->datasets_id);
dyStringPrintf(dy, "%d,",job->tasks_id);
dyStringPrintf(dy, "%d,",job->classifiers_id);
dyStringPrintf(dy, "%d,",job->featureSelections_id);
dyStringPrintf(dy, "%d,",job->transformations_id);
dyStringPrintf(dy, "%d,",job->subgroups_id);
dyStringPrintf(dy, "%f,",job->avgTestingAccuracy);
dyStringPrintf(dy, "%f,",job->avgTestingAccuracyGain);
dyStringPrintf(dy, "'%s',",job->accuracyType);
dyStringPrintf(dy, "'%s',",job->features);
dyStringPrintf(dy, "'%s',",job->weights);
dyStringPrintf(dy, "'%s');",job->modelPath);
sqliteUpdate(conn, dy->string);
dyStringFree(&dy);
jobsFree(&job);
}

/**********************/
void sqlite_populateClassificationTables(char *db, char *raFile, int dropTables)
{
struct sqlite3 *conn = NULL;
if(sqlite3_open(db, &conn) != SQLITE_OK)
    errAbort("Couldn't open connection to sqlite database %s", db);

//check all the tables are in place (or replace them)
sqlite_setupTable(conn, "jobs", dropTables);
sqlite_setupTable(conn, "tasks", dropTables);
sqlite_setupTable(conn, "classifiers", dropTables);
sqlite_setupTable(conn, "classifierResults", dropTables);
sqlite_setupTable(conn, "featureSelections", dropTables);
sqlite_setupTable(conn, "transformations", dropTables);
sqlite_setupTable(conn, "subgroups", dropTables);
sqlite_setupTable(conn, "samplesInSubgroups", dropTables);

// read ra file and populate tables with that info
struct hash *raHash, *raHashList = readRaFile(raFile);

for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
	if(sameString(type, "task"))
		sqlite_saveTaskRa(conn, raHash);	
	}
for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
	if(sameString(type, "subgrouping"))
		sqlite_saveSubgroupingRa(conn, raHash);
	}
for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
    if(sameString(type, "featureSelection"))
        sqlite_saveFeatureSelectionRa(conn, raHash);
    }
for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
    if(sameString(type, "transformation"))
        sqlite_saveTransformationRa(conn, raHash);
    }
for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
	if(sameString(type, "classifier"))
		sqlite_saveClassifierRa(conn, raHash);
	}
for (raHash = raHashList; raHash; raHash = raHash->next)
    {
    char *type = (char *)hashMustFindVal(raHash, "type");
	if(sameString(type, "job"))
		sqlite_saveJobRa(conn, raHash);
	}

//clean up
hashFree(&raHash);
hashFree(&raHashList);
sqlite3_close(conn);
}
