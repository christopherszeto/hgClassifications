#include <ctype.h>
#include "common.h"
#include "options.h"
#include "linefile.h"
#include "ra.h"
#include "hash.h"
#include "../inc/MLmatrix.h"
#include "../inc/MLcrossValidation.h"
#include "../inc/MLio.h"

/* Default globals */
char *db = "bioInt";
char *profile = "localDb";

/* Option handling */
static struct optionSpec options[] = {
	{"classifier", OPTION_STRING},
	{"classifierParameters", OPTION_STRING},
	{"modelPath", OPTION_STRING},
	{"output", OPTION_STRING},
	{"model", OPTION_INT},
	{"five3db", OPTION_STRING},
	{"tmpFilePrefix", OPTION_STRING},
    {"profile", OPTION_STRING},
    {"db", OPTION_STRING},
	{NULL, 0},
};

/* helper functions */
matrix * compareKOs(char * preDataFilename, char *postDataFilename, char * classifier, char *classifierParameters, char * modelPath, char * tmpFilePrefix)
{
//make a place to save results
matrix * preKOscores = NULL, *postKOscores = NULL;
//open files and save to matrices
FILE * fp = fopen(preDataFilename, "r");
if(!fp)
    errAbort("Couldn't open data file %s for reading.\n", preDataFilename);
matrix * preKOdata = f_fill_matrix(fp, 1);
fclose(fp);
fp = fopen(postDataFilename, "r");
if(!fp)
    errAbort("Couldn't open data file %s for reading.\n", postDataFilename);
matrix * postKOdata = f_fill_matrix(fp, 1);
fclose(fp);
//apply the predictors
if(sameString(classifier, "NMFpredictor"))
    {
    matrix * matchedPreKOdata = NMFgetDataMatchedToModel(preKOdata, modelPath);
    preKOscores = applyNMFpredictorModel(matchedPreKOdata, tmpFilePrefix, modelPath);
    matrix * matchedPostKOdata = NMFgetDataMatchedToModel(postKOdata, modelPath);
    postKOscores = applyNMFpredictorModel(matchedPostKOdata, tmpFilePrefix, modelPath);
    free_matrix(matchedPreKOdata);
    free_matrix(matchedPostKOdata);
    }
else if(sameString(classifier, "SVMlight"))
    {
    matrix * matchedPreKOdata = SVMgetDataMatchedToModel(preKOdata, modelPath);
    preKOscores = applySVMlightModel(matchedPreKOdata, tmpFilePrefix, modelPath);
    matrix * matchedPostKOdata = SVMgetDataMatchedToModel(postKOdata, modelPath);
    postKOscores = applySVMlightModel(matchedPostKOdata, tmpFilePrefix, modelPath);
	free_matrix(matchedPreKOdata);
	free_matrix(matchedPostKOdata);
    }
else if(sameString(classifier,"glmnet"))
	{
	matrix * matchedPreKOdata = glmnetgetDataMatchedToModel(preKOdata, tmpFilePrefix, modelPath, classifierParameters);
    preKOscores = applyglmnetModel(matchedPreKOdata, tmpFilePrefix, modelPath, classifierParameters);
    matrix * matchedPostKOdata = glmnetgetDataMatchedToModel(postKOdata, tmpFilePrefix, modelPath, classifierParameters);
    postKOscores = applyglmnetModel(matchedPostKOdata, tmpFilePrefix, modelPath, classifierParameters);
    free_matrix(matchedPreKOdata);
    free_matrix(matchedPostKOdata);
	}
else if(sameString(classifier, "WEKA"))
    errAbort("ERROR: No functions in place to apply WEKA models yet.");
//clean up data not in use
free_matrix(preKOdata);
free_matrix(postKOdata);
//match columns and find the delta
if(!matchedColLabels(preKOscores,postKOscores))
    {
    matrix * tmp = matchOnColLabels(postKOscores, preKOscores);
    free_matrix(postKOscores);
    postKOscores = tmp;
    }
matrix * result= append_matrices(preKOscores, postKOscores, 2);
result->labels=0;
safef(result->rowLabels[0], MAX_LABEL, "PreKOscore");
safef(result->rowLabels[1], MAX_LABEL, "PostKOscore");
copy_matrix_labels(result, preKOscores, 2, 2);
result->labels=1;

return result;
}

/**** some database interaction code that should probably be moved elsewhere ****/
char * bioInt_classifierTypeFromJobId(struct sqlConnection *conn, int jobId)
{
char query[256];
safef(query, sizeof(query),	"SELECT name FROM jobs,classifiers WHERE jobs.id=%d AND "
							"jobs.classifiers_id=classifiers.id;", jobId);
char * result = sqlNeedQuickString(conn, query);
return result;
}

char * bioInt_classifierParametersFromJobId(struct sqlConnection *conn, int jobId)
{
char query[256];
safef(query, sizeof(query), "SELECT parameters FROM jobs,classifiers WHERE jobs.id=%d AND "
                            "jobs.classifiers_id=classifiers.id;", jobId);
char * result = sqlNeedQuickString(conn, query);
return result;
}

char * bioInt_modelPathFromJobId(struct sqlConnection * conn, int jobId)
{
char query[256];
safef(query, sizeof(query), "SELECT modelPath FROM jobs WHERE id=%d;", jobId);
char * result = sqlNeedQuickString(conn, query);
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

char * five3db_classifierTypeFromJobId(sqlite3 *conn, int jobId)
{
char query[256];
safef(query, sizeof(query), "SELECT name FROM jobs,classifiers WHERE jobs.id=%d AND "
                            "jobs.classifiers_id=classifiers.id;", jobId);
char * result = sqliteQuickString(conn, query);
return result;
}

char * five3db_classifierParametersFromJobId(sqlite3 *conn, int jobId)
{
char query[256];
safef(query, sizeof(query), "SELECT parameters FROM jobs,classifiers WHERE jobs.id=%d AND "
                            "jobs.classifiers_id=classifiers.id;", jobId);
char * result = sqliteQuickString(conn, query);
return result;
}

char * five3db_modelPathFromJobId(sqlite3 * conn, int jobId)
{
char query[256];
safef(query, sizeof(query), "SELECT modelPath FROM jobs WHERE id=%d;", jobId);
char * result = sqliteQuickString(conn, query);
return result;
}

/***************/
void usage()
{
errAbort(   "Usage: MLcompareKOs [OPTIONS] [preKOfile] [postKOfile]\n"
			"OPTIONS:\n"
			"-tmpFilePrefix=[string]\tPrefix to tag the flatfiles written for excution to. Make sure it's writable.\n"
			"-output=[file]\tWhere to write results of comparison. Default is stdout.\n"
			"Necessary for pulling models from file:\n"
			"-modelPath=[file]\tPAth to the model you want to use\n"
			"-classifier=[string]\tType of classifier in modelPath\n"
			"-classifierParameters=[string]\tNecessary for glmnet models to determine type of classifier\t"
			"Nessary for pulling models from db:\n"
			"-model=[id]\tId of the model to apply to compare these files.\n"
			"Otional for pulling models from db:\n"
			"-db=[string]\tDatabase name to use if not bioInt and not five3db sqlite database\n"
			"-five3db=[file]\tWhere to go to find an sqlite3 db, and use that instead of bioInt\n");
}

int main(int argc, char *argv[])
{
optionInit(&argc, argv, options);
if(argc < 2) 
	usage();
char * preDataFilename = argv[1];
char * postDataFilename = argv[2];
char * outputFilename = NULL;
char * classifier = NULL, *classifierParameters=NULL, *modelPath = NULL;
int jobId = atoi(optionVal("model", "-1"));
if(optionExists("profile"))
	profile = optionVal("profile", profile);;
if(optionExists("db"))
	db = optionVal("db", db);
if(!jobId)
	{
	fprintf(stderr, "ERROR: Incorrect model number\n");
	usage();
	}
//get the model path, classifier, classifier parameters, from db or from command line.
if(optionExists("model"))
	{
	if(optionExists("five3db"))
		{
		struct sqlite3 *conn = NULL;
	    char * five3db = NULL;
		five3db = optionVal("five3db", five3db);
		if(sqlite3_open(five3db, &conn) != SQLITE_OK)
	    	errAbort("Couldn't open connection to sqlite database %s", five3db);
		classifier = five3db_classifierTypeFromJobId(conn, jobId);
		classifierParameters = five3db_classifierParametersFromJobId(conn, jobId);
		modelPath = five3db_modelPathFromJobId(conn, jobId);
		sqlite3_close(conn);
		}
	else
		{
		struct sqlConnection *conn = hAllocConnProfile(profile, db);
		classifier = bioInt_classifierTypeFromJobId(conn, jobId);
		classifierParameters = bioInt_classifierParametersFromJobId(conn, jobId);
		modelPath = bioInt_modelPathFromJobId(conn, jobId);
		hFreeConn(&conn);
		}
	}
else if(optionExists("modelPath") && optionExists("classifier") && optionExists("classifierParameters"))
	{
	modelPath = optionVal("modelPath", NULL);
	classifier = optionVal("classifier", NULL);
	classifierParameters = optionVal("classifierParameters", NULL);
	}
else
	{
	fprintf(stderr, "ERROR: Missing necessary command line options\n");
	usage();
	}

char tmpFilePrefix[512];
if(optionExists("tmpFilePrefix"))
	safef(tmpFilePrefix, sizeof(tmpFilePrefix),"%s", optionVal("tmpFilePrefix", tmpFilePrefix));
else
	safef(tmpFilePrefix, sizeof(tmpFilePrefix), "%s",getcwd(NULL, 0));
matrix * deltas = compareKOs(preDataFilename, postDataFilename, classifier, classifierParameters, modelPath, tmpFilePrefix);
if(optionExists("output"))
    {
    outputFilename = optionVal("output", outputFilename);
    FILE * fp = fopen(outputFilename, "w");
    if(fp ==  NULL)
        errAbort("Couldn't open %s for writing. Exiting..\n", outputFilename);
	else
		{
		fprint_matrix(fp, deltas);
		fclose(fp);
		}
    }
else
	fprint_matrix(stdout, deltas);
return 0;
}
