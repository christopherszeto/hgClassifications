#include "../inc/MLmatrix.h"
#include "../inc/MLcrossValidation.h"
#include "../inc/MLreadConfig.h"
#include "../inc/MLextractCoefficients.h"
#include "../inc/MLio.h"
#include "../inc/MLfilters.h"
#include "../inc/MLsignificance.h"

int createBackgroundDirs(struct slInt *topModelIds)
{
struct slInt * currModelId = topModelIds;
char targetDir[1024];
while(currModelId)
	{
	safef(targetDir, sizeof(targetDir),"/data/trash/MLbackgroundAnalysis/model%d",currModelId->val);
	if(makeDir(targetDir) == -1)
		fprintf(stderr, "WARNING: %s couldn't be created or exists. Attempting to overwrite.\n", targetDir);
	chmod(targetDir, S_IRWXU | S_IRWXG | S_IRWXO );
	currModelId = currModelId->next;
	}
return 1;
}

matrix * NMFspoofDataMatchedToModel(matrix * data, char * modelPath)
{
FILE *fp = fopen(modelPath, "r");
if(!fp)
	{
	fprintf(stderr, "Couldn't open data file %s for reading.\n", modelPath);
	return NULL;
	}
matrix * model = f_fill_matrix(fp, 1);
fclose(fp);
struct slInt * rowList = list_indices(model->rows);
matrix * data_cpy = copy_matrix_subset(data, rowList, NULL);
data_cpy->labels=0;
copy_matrix_labels(data_cpy, model, 1,1);
//consider putting fake sample labels in here
data_cpy->labels=1;
free_matrix(model);
slFreeList(&rowList);
return data_cpy;
}


matrix * bioInt_fill_background_matrix(struct sqlConnection * conn, char *tableName)
/*Fills a data matrix from the bioInt database structure in tableName*/
{
//make some templates to save interim results to
int rows = bioInt_lineCount(conn, tableName);
int cols = bioInt_colCount(conn, tableName);
//make a place to save results
matrix * result = init_matrix(rows, cols);
//iterate samples
char query[256];
safef(query, sizeof(query), "SELECT DISTINCT sample_id FROM %s", tableName);
struct slInt *sampleId, *sampleIdList = sqlQuickNumList(conn, query);
int col = 0;
for(sampleId=sampleIdList; sampleId; sampleId=sampleId->next)
    {
    //save sample label
    safef(result->colLabels[col], MAX_LABEL, "%d", sampleId->val);

    //iterate resujlts, assigning them to the correct rows in result matrix
    safef(query, sizeof(query), "SELECT feature_id,val FROM %s "
                                "WHERE sample_id=%d ORDER BY feature_id",
                                tableName, sampleId->val);
    struct sqlResult * sr = sqlGetResult(conn, query);
	char **resultRow;
    int templateRow = 0;
    while((resultRow = sqlNextRow(sr)) != NULL)
        {
        if(sameString(resultRow[1], "NULL"))
            result->graph[templateRow++][col] = NULL_FLAG;
        else
            result->graph[templateRow++][col] = atof(resultRow[1]);
        }
    sqlFreeResult(&sr);
    col++;
    }
slFreeList(&sampleIdList);
result->labels=1;
return result;
}


matrix * glmnetspoofDataMatchedToModel(matrix * data, char * modelPath)
//assumes there's a .coeffs file listed in teh same place as RDATA file
{
char * coeffsPath = replaceChars(modelPath, "RData","coeffs");
FILE *fp = fopen(coeffsPath, "r");
if(!fp)
	{
	fprintf(stderr, "Couldn't open data file %s for reading.\n", coeffsPath);
	return NULL;
	}
matrix * coeffs = f_fill_matrix(fp, 1);
fclose(fp);
struct slInt * rowList = list_indices(coeffs->rows);
matrix * data_cpy = copy_matrix_subset(data, rowList, NULL);
data_cpy->labels=0;
copy_matrix_labels(data_cpy, coeffs, 1,1);
//consider putting fake sample labels in here
data_cpy->labels=1;
free_matrix(coeffs);
slFreeList(&rowList);
freeMem(coeffsPath);
return data_cpy;
}

int writeNMFbackgroundData(char * modelPath, char * target, matrix * data)
{
FILE *tmp = fopen(modelPath, "r");
if(!tmp)
errAbort("Couldn't open data file %s for reading.\n", modelPath);
matrix * model = f_fill_matrix(tmp, 1);
fclose(tmp);
matrix * data_cpy = copy_matrix(data);
data_cpy->labels=0;
copy_matrix_labels(data_cpy, model, 1,1);
data_cpy->labels=1;
free_matrix(model);

FILE * fp = fopen(target, "w");
if(!fp)
	errAbort("ERROR: Couldn't open file %s for writing\n", target);
shift_matrix(data_cpy);
fprint_matrix(fp, data_cpy);
fclose(fp);
//clean up
free_matrix(data_cpy);
return 1;
}

matrix * SVMspoofDataMatchedToModel(matrix * data, char * modelPath)
{
//read model file to last line,then get feature count
char * lastLine = NULL,* line = NULL;
FILE * fp = fopen(modelPath, "r");
if(!fp)
	{
    fprintf(stderr, "Couldn't open file %s for conversion to an SVMlight for", modelPath);
	return NULL;
	}
while((line = readLine(fp)) && line != NULL)
    lastLine = line;
int numFeatures = chopByWhiteRespectDoubleQuotes(lastLine, NULL, 0);
//reduce matrix to same length as model
struct slInt * rowList = list_indices(numFeatures);
matrix * data_cpy = copy_matrix_subset(data, rowList, NULL);
return data_cpy;
}

int writeSVMbackgroundData(char * target, matrix * data)
{
FILE * fp = fopen(target, "w");
if(!fp)
    errAbort("ERROR: Couldn't open file %s for writing\n", target);
matrix * fakeMetadata = init_matrix(1, data->cols);
int i;
for(i = 0; i < data->cols; i++)
    fakeMetadata->graph[0][i] = NULL_FLAG;
matrix * svm_dataCpy = matricesToSVM(data, fakeMetadata);
fprint_SVMlite_matrix(fp, svm_dataCpy);
free_matrix(svm_dataCpy);
fclose(fp);
return 1;
}

int writeWEKAbackgroundData(char * target, matrix * data)
{
FILE * fp = fopen(target, "w");
if(!fp)
    errAbort("Error: couldn't write to the file %s", target);
matrix * fakeMetadata = init_matrix(1, data->cols);
int i;
for(i = 0; i < data->cols; i++)
    fakeMetadata->graph[0][i] = NULL_FLAG;
for(i = 0; i < data->rows; i++)
	safef(data->rowLabels[i],MAX_LABEL, "feature%d", i);
data->labels=1;
matrix * dataCpy = copy_matrix(data);
tagLabelsWithMetadata(dataCpy, fakeMetadata);
matrix * t_dataCpy = transpose(dataCpy);
fprint_WEKA_matrix(fp, t_dataCpy);
free_matrix(t_dataCpy);
free_matrix(dataCpy);
free_matrix(fakeMetadata);
fclose(fp);
return 1;
}

int writeglmnetbackgroundData(char * modelPath, char * parameters, char * target, matrix * data)
/*Relies on writing temporary files. Consider revising*/
{
//run the extract coeffs R script to get a tmp file with gene names in it
char tmpCoeffsFile[1024], tmpOutFile[1024], command[2048];
safef(tmpCoeffsFile, sizeof(tmpCoeffsFile), "%s.coeffs", target);
safef(tmpOutFile, sizeof(tmpOutFile), "%s.Rout", target);
safef(command, sizeof(command),
	"%s/Rwrapper.sh %s/%s_extractCoefficients.R arg1=%s,arg2=%s %s",
	CLUSTERROOTDIR,CLUSTERROOTDIR,
	parameters, modelPath, tmpCoeffsFile, tmpOutFile);
system(command);
//opena nd read gene names from coeffs file into copy of data
FILE *tmp = fopen(tmpCoeffsFile, "r");
if(!tmp)
errAbort("Couldn't open data file %s for reading.\n", tmpCoeffsFile);
matrix * modelCoeffs = f_fill_matrix(tmp, 1);
fclose(tmp);
matrix * data_cpy = copy_matrix(data);
data_cpy->labels=0;
copy_matrix_labels(data_cpy, modelCoeffs, 1,1);
data_cpy->labels=1;
free_matrix(modelCoeffs);
//write copy to disk
FILE * fp = fopen(target, "w");
if(!fp)
    errAbort("ERROR: Couldn't open file %s for writing\n", target);
fprint_matrix(fp, data);
fclose(fp);
//clean up
free_matrix(data_cpy);
unlink(tmpCoeffsFile);
unlink(tmpOutFile);

return 1;
}

int writeBackgroundData(struct sqlConnection * conn, struct slInt *topModelIds)
{
char target[1024];
char query[256];
struct slInt * currModelId = topModelIds;
while(currModelId)
	{
	//get the latest background table name of the right data type
	safef(query, sizeof(query), "SELECT type_id FROM jobs,datasets WHERE jobs.id=%d AND jobs.datasets_id=datasets.id;", currModelId->val);
	int dataTypeId=sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "SELECT data_table FROM datasets WHERE data_table LIKE \"BACKGROUND_DATATYPE%d_%%\" ORDER BY created_date DESC LIMIT 1", dataTypeId);
	char * tableName = sqlNeedQuickString(conn, query);
	matrix * data = bioInt_fill_matrix(conn, tableName);
	//get model info
	safef(query, sizeof(query), "SELECT classifiers.name FROM classifiers JOIN jobs  ON (jobs.classifiers_id=classifiers.id) WHERE jobs.id=%d", currModelId->val);
	char * classifier = sqlQuickString(conn, query);
	safef(query, sizeof(query), "select featureCount from featureSelections join jobs on (jobs.featureSelections_id=featureSelections.id) where jobs.id=%d", currModelId->val);
	int M = sqlNeedQuickNum(conn, query);
	struct slInt * rowList = list_indices(M);
	matrix * data_cpy = copy_matrix_subset(data, rowList, NULL);
	slFreeList(&rowList);
	if(sameString(classifier, "NMFpredictor"))
		{
		safef(query, sizeof(query), "SELECT modelPath FROM jobs WHERE id=%d", currModelId->val);
		char * modelPath = sqlNeedQuickString(conn, query);
		safef(target, sizeof(target), "/data/trash/MLbackgroundAnalysis/model%d/data.tab", currModelId->val);
		writeNMFbackgroundData(modelPath, target, data_cpy);
		}
	else if(sameString(classifier, "SVMlight"))
		{
		safef(target, sizeof(target), "/data/trash/MLbackgroundAnalysis/model%d/data.svm", currModelId->val);
		writeSVMbackgroundData(target, data_cpy);
		}
	else if(sameString(classifier, "glmnet"))
		{
		safef(query, sizeof(query), "SELECT modelPath FROM jobs WHERE id=%d", currModelId->val);
        char * modelPath = sqlNeedQuickString(conn, query);
		safef(query, sizeof(query), "SELECT classifier.parameters FROM jobs INNER JOIN classifiers ON (jobs.classifiers_id=classifiers.id) WHERE jobs.id=%d", currModelId->val);
        char * parameters = sqlNeedQuickString(conn, query);
        safef(target, sizeof(target), "/data/trash/MLbackgroundAnalysis/model%d/data.tab", currModelId->val);
        writeglmnetbackgroundData(modelPath,parameters, target, data_cpy);
		}
	else if(sameString(classifier, "WEKA"))
		{
		safef(target, sizeof(target), "/data/trash/MLbackgroundAnalysis/model%d/data.arff", currModelId->val);
		writeWEKAbackgroundData(target, data_cpy);
		}
	free_matrix(data_cpy);
	currModelId = currModelId->next;
	free_matrix(data);
	}
return 1;
}

int writeBackgroundClusterJobs(struct sqlConnection * conn, struct slInt *topModelIds)
{
char query[512];
struct slInt * currModelId = topModelIds;

//open the jobs files you want to write
char applyModelFilename[1024], printResultsFilename[1024];
safef(applyModelFilename, sizeof(applyModelFilename), "/data/trash/MLbackgroundAnalysis/applyModelJobsList.txt");
FILE * applyModelFile = fopen(applyModelFilename, "a");
if(applyModelFile == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", applyModelFilename);
safef(printResultsFilename, sizeof(printResultsFilename), "/data/trash/MLbackgroundAnalysis/printResultsJobsList.txt");
FILE * printResultsFile = fopen(printResultsFilename, "a");
if(printResultsFile == NULL)
    errAbort("Couldn't open file %s for writing. Exiting...\n", printResultsFilename);

while(currModelId)
    {
    safef(query, sizeof(query), "SELECT classifiers.name FROM classifiers JOIN jobs  ON (jobs.classifiers_id=classifiers.id) WHERE jobs.id=%d", currModelId->val);
    char * classifier = sqlQuickString(conn, query);
    safef(query, sizeof(query), "SELECT classifiers.parameters FROM classifiers JOIN jobs  ON (jobs.classifiers_id=classifiers.id) WHERE jobs.id=%d", currModelId->val);
    char * parameters = sqlQuickString(conn, query);
	safef(query, sizeof(query), "SELECT modelPath FROM jobs WHERE id=%d", currModelId->val);
	char * modelPath = sqlQuickString(conn, query);
    //make sure the modelPAth has no parens, these will break command line processing
    if(strchr(modelPath, '(') || strchr(modelPath, ')'))
        {
        char * tmpModelPathCpy = replaceChars(modelPath, "(", "\\(");
        char * tmpModelPathCpy2 = replaceChars(tmpModelPathCpy, ")", "\\)");
        freeMem(modelPath);
        freeMem(tmpModelPathCpy);
        modelPath = tmpModelPathCpy2;
        }

	char targetDir[512];
	safef(targetDir, sizeof(targetDir), "/data/trash/MLbackgroundAnalysis/model%d", currModelId->val);

	//printn jobs that will apply each top model to the background dataset	
    if(sameString(classifier, "NMFpredictor"))
		{
		fprintf(applyModelFile, "%s/NMFpredictor_classify -rawCorrelations %s/data.tab %s %s/NMFpredictor.training.results\n",CLUSTERROOTDIR, targetDir, modelPath, targetDir);
		}
    else if(sameString(classifier, "SVMlight"))
		{
		fprintf(applyModelFile, "%s/SVMlightWrapper.sh svm_classify %s/data.svm %s %s/svm.training.results %s/model.log\n", CLUSTERROOTDIR, targetDir, modelPath, targetDir, targetDir);
		}
    else if(sameString(classifier, "WEKA"))
		{
		fprintf(applyModelFile, "%s/WEKAwrapper.sh %s,T=%s/data.arff,l=%s,p=0 %s/weka.training.results\n",CLUSTERROOTDIR, parameters, targetDir, modelPath, targetDir);
		}
	else if(sameString(classifier, "glmnet"))
		{
		fprintf(applyModelFile,
			"%s/Rwrapper.sh %s_classify.R arg1=%s/data.tab,arg2=%s,arg3=%s/%s.training.results %s/classifying.Rout\n",
			CLUSTERROOTDIR, parameters, targetDir, modelPath, targetDir, parameters, targetDir);
		}

	//print the jobs that will write results to ra
	fprintf(printResultsFile, "%s/MLwriteResultsToRa %s/model%d.cfg -backgrounds=/data/trash/MLbackgroundAnalysis/backgroundResults.ra\n",
		CLUSTERROOTDIR, targetDir,currModelId->val);
	
	//advance counters and pointers
	currModelId = currModelId->next;
    }
fclose(applyModelFile);
fclose(printResultsFile);

return 1;
}

int writeBackgroundConfigFiles(struct sqlConnection * conn, struct slInt *topModelIds)
{
char query[512];
struct slInt * currModelId = topModelIds;
char configFilename[1024];
int count = 1;
while(currModelId)
	{
	safef(configFilename, sizeof(configFilename), "/data/trash/MLbackgroundAnalysis/model%d/model%d.cfg", currModelId->val, currModelId->val);
	FILE * configFile = fopen(configFilename, "w");
	if(!configFile)
		errAbort("ERROR: Couldn't open %s for writing. Exiting...\n", configFilename);
	fprintf(configFile,"name\tbackgroundCalculation_%d\n", count);
	fprintf(configFile,"task\tbackgroundCalculation\n"); 
	//prediction score calculators use the modelDir and trainingDir info
	fprintf(configFile,"trainingDir\t/data/trash/MLbackgroundAnalysis/model%d\n", currModelId->val);
	fprintf(configFile,"modelDir\t/data/trash/MLbackgroundAnalysis/model%d\n", currModelId->val);
	//put in profile and db so you can open a connection from this config that points to this data
	fprintf(configFile, "profile\tlocalDb\n");
	fprintf(configFile, "db\tbioInt\n");
	//keep track of which background dataset is used here
	safef(query, sizeof(query), "SELECT type_id FROM jobs,datasets WHERE jobs.id=%d AND jobs.datasets_id=datasets.id;", currModelId->val);
	int dataTypeId = sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "SELECT data_table FROM datasets WHERE data_table LIKE \"BACKGROUND_DATATYPE%d_%%\" ORDER BY created_date LIMIT 1", dataTypeId);
	fprintf(configFile, "dataset\t%s\n", sqlNeedQuickString(conn, query));
	//use the modelPath to determine which model this is for - could use id but this is more robust with rebuilds
	safef(query, sizeof(query), "SELECT modelPath FROM jobs WHERE id=%d", currModelId->val);
	fprintf(configFile, "modelPath\t%s\n", sqlNeedQuickString(conn, query));
	safef(query, sizeof(query), "SELECT classifiers_id FROM jobs WHERE id=%d", currModelId->val);
	int classifierId = sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "SELECT name FROM classifiers WHERE id=%d", classifierId);
	fprintf(configFile,"classifier\t%s\n", sqlNeedQuickString(conn, query));
	fprintf(configFile,"outputType\t%s\n", sqlNeedQuickString(conn, query));
	safef(query, sizeof(query), "SELECT parameters FROM classifiers WHERE id=%d", classifierId);
	fprintf(configFile,"parameters\t%s\n", sqlNeedQuickString(conn, query));
	fclose(configFile);
	count++;
	currModelId = currModelId->next;
	}
return 1;
}

int printBackgroundModelResults(struct sqlConnection * conn, struct slInt * topModelIds)
{
//open backgroundResults.ra here
FILE * fp = fopen("/data/trash/MLbackgroundAnalysis/backgroundResults.ra", "w");
if(!fp)
	errAbort("ERROR: Couldn't open backgroundResults.ra for writing. Exiting...\n");
//run each model on a version of this background table
matrix * thisResult = NULL;
int rowCount = 0,j=0;
char backgroundDataPrefix[256];
struct slInt * jid = NULL;
time_t start, stop;
char * modelPath, *classifier, *classifierParameters;
char query[256];
for(jid = topModelIds; jid; jid=jid->next)
    {
    fprintf(stderr, "Applying model %d\n", jid->val);
	//get the latest background table name of the right data type
	safef(query, sizeof(query), "SELECT type_id FROM jobs,datasets WHERE jobs.id=%d AND jobs.datasets_id=datasets.id;", jid->val);
	int dataTypeId=sqlNeedQuickNum(conn, query);
	safef(query, sizeof(query), "SELECT data_table FROM datasets WHERE data_table LIKE \"BACKGROUND_DATATYPE%d_%%\" ORDER BY created_date DESC LIMIT 1", dataTypeId);
	char * tableName = sqlNeedQuickString(conn, query);
	matrix * data = bioInt_fill_background_matrix(conn, tableName);

	//start timer
	time(&start);
    //get info about this model from db
    safef(query, sizeof(query), "SELECT modelPath FROM jobs WHERE id=%d", jid->val);
   	modelPath = sqlNeedQuickString(conn, query);
    safef(query, sizeof(query), "SELECT classifiers.name FROM classifiers JOIN jobs ON (jobs.classifiers_id=classifiers.id) WHERE jobs.id=%d", jid->val);
    classifier = sqlNeedQuickString(conn, query);
	safef(query, sizeof(query), "SELECT classifiers.parameters FROM classifiers JOIN jobs ON (jobs.classifiers_id=classifiers.id) WHERE jobs.id=%d", jid->val);
    classifierParameters = sqlNeedQuickString(conn, query);
	safef(backgroundDataPrefix, sizeof(backgroundDataPrefix), "/data/trash/MLbackgroundAnalysis/model%d", jid->val);
    //get prediction scores
    thisResult = NULL;
    if(sameString(classifier, "NMFpredictor"))
		{
		matrix * data_cpy = NMFspoofDataMatchedToModel(data, modelPath);
		if(data_cpy == NULL)
			return 0; //no data file. Return without wriging.
        thisResult = applyNMFpredictorModel(data_cpy, backgroundDataPrefix, modelPath);
		free_matrix(data_cpy);
		}
    else if(sameString(classifier, "SVMlight"))
        {
		matrix * data_cpy = SVMspoofDataMatchedToModel(data, modelPath);
		if(data_cpy == NULL)
			return 0;
		thisResult = applySVMlightModel(data_cpy, backgroundDataPrefix, modelPath);
		free_matrix(data_cpy);
		}
	else if(sameString(classifier, "glmnet"))
		{
        matrix * data_cpy = glmnetspoofDataMatchedToModel(data, modelPath);
		if(data_cpy == NULL)
			return 0;
        thisResult = applyglmnetModel(data_cpy, backgroundDataPrefix, modelPath, classifierParameters);
        free_matrix(data_cpy);
		}
    else if(sameString(classifier, "WEKA"))
        errAbort("ERROR: No functions in place to apply WEKA models yet.\n");
    else
        errAbort("ERROR: Unrecongized model type\n");
	time(&stop);
    //process results
	fprintf(fp, "name\tbackground%d\n", jid->val);
	fprintf(fp, "type\tbackground\n");
	fprintf(fp, "data_table\t%s\n", tableName);
	fprintf(fp, "jobs_id\t%d\n", jid->val);
	fprintf(fp, "estimatedRunTime\t%.0f\n", difftime(stop,start));
	fprintf(fp, "predictionScores\t");
    for(j = 0; j < thisResult->cols; j++)
		fprintf(fp, "%.16f,", thisResult->graph[0][j]);
	fprintf(fp, "\n\n");
    //clean up
    freeMem(modelPath);
    freeMem(classifier);
    if(thisResult)
        free_matrix(thisResult);
    rowCount++;
	free_matrix(data);
	}
fclose(fp);
return 1;
}

struct slInt * listTopModelIds(struct sqlConnection * conn)
{
char query[512];
safef(query, sizeof(query), "SELECT DISTINCT(id) FROM dataTypes");
struct slInt * currDataTypeId, *dataTypeIds = sqlQuickNumList(conn, query);
currDataTypeId=dataTypeIds;
safef(query, sizeof(query), "SELECT DISTINCT(id) FROM tasks");
struct slInt * currTaskId, *taskIds = sqlQuickNumList(conn, query);
struct slInt * curr = NULL, * result = NULL;

while(currDataTypeId)
	{
//	safef(query, sizeof(query), "SELECT id FROM jobs WHERE tasks_id=%d ORDER BY avgTestingAccuracy DESC LIMIT 1", 
//		currTaskId->val);
	//TODO: The line above will grab any model, the line below grabs only svmlight t=0 and NMF better than majority classifier
	// ... Consider fixing weka processing
	currTaskId = taskIds;
	while(currTaskId)
		{
		safef(query, sizeof(query), "SELECT jobs.id FROM jobs JOIN datasets ON (jobs.datasets_id=datasets.id) INNER JOIN classifiers ON (jobs.classifiers_id=classifiers.id)  WHERE datasets.type_id=%d and tasks_id=%d AND classifiers.name != 'WEKA' AND avgTestingAccuracyGain > 0 ORDER BY avgTestingAccuracy DESC LIMIT 1", currDataTypeId->val, currTaskId->val);
		int thisTopModelId = sqlQuickNum(conn, query);
		if(!thisTopModelId)
			fprintf(stderr, "WARNING: Couldn't find a topmodel for task %d\n", currTaskId->val);
		else
			{
			if(!result)
				{
				result= newSlInt(thisTopModelId);
				curr = result;
				}
			else
				{
				curr->next = newSlInt(thisTopModelId);;
				curr = curr->next;
				}
			}
		currTaskId = currTaskId->next;
		}
	currDataTypeId = currDataTypeId->next;
	}
slFreeList(taskIds);
return result;
}

int saveBckgrndMatrixToBioInt(struct sqlConnection * conn,char * tableName, matrix * data)
{
fprintf(stderr, "Creating background table %s\n", tableName);

struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE %s (\n", tableName);
dyStringPrintf(dy, "sample_id int not null,\n");
dyStringPrintf(dy, "feature_id int  not null,\n");
dyStringPrintf(dy, "val float not null,\n");
dyStringPrintf(dy, "conf float not null,\n");
dyStringPrintf(dy, "KEY(feature_id, sample_id),\n");
dyStringPrintf(dy, "KEY(sample_id, feature_id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn,dy->string);
dyStringFree(&dy);

int col=0,currRow=0, lastPrintRow=0; 
char tmp[256];

fprintf(stderr, "Table created. Saving background to database..\n");
for(currRow = 0; currRow < data->rows; currRow++)
	{
	//fprintf(stderr, "%.2f%% saving completed\n", (double)(100*currRow)/data->rows);
	for(col = 0; col < data->cols; col++)
		{
		//calc the next entry
		safef(tmp, sizeof(tmp), "(%d,%d,%.8f,0)", (-col-1), (-currRow-1), data->graph[currRow][col]);

		if(currRow == 0 && col == 0)//if it's the first row, start it
			{
			dy = newDyString(20000);
            dyStringPrintf(dy, "INSERT INTO %s (sample_id, feature_id, val, conf) VALUES %s", tableName, tmp);
			}
		else if((currRow == (data->rows-1)) && (col == (data->cols-1))) //if it's the last one, print
			{
			dyStringPrintf(dy,",%s;\n", tmp);
            sqlUpdate(conn, dy->string);
            dyStringFree(&dy);
			}
		else if((strlen(tmp) + dy->stringSize) > dy->bufSize) //if you're about to run out of room, print
			{
        	dyStringPrintf(dy,";\n");
        	sqlUpdate(conn, dy->string);
        	dyStringFree(&dy);
        	dy = newDyString(20000);
        	dyStringPrintf(dy, "INSERT INTO %s (sample_id, feature_id, val, conf) VALUES %s", tableName, tmp);
        	lastPrintRow = currRow;
       		}
		else
			dyStringPrintf(dy, ",%s", tmp);
		}
	}
dyStringFree(&dy);
fprintf(stderr, "Done!\n");

fprintf(stderr, "Updating datasets table..\n");
safef(tmp, sizeof(tmp), "SELECT max(id) FROM datasets");
int nextId = sqlNeedQuickNum(conn, tmp) +1;
dy = newDyString(1024);
dyStringPrintf(dy, "INSERT INTO datasets (id, tissue_id, type_id, num_samples,name, data_table,created_date) ");
dyStringPrintf(dy, "VALUES (%d, 0,0,%d, \'%s\', \'%s\', NOW());",nextId,  BACKGROUND_SAMPLES,tableName, tableName);
sqlUpdate(conn, dy->string);
dyStringFree(&dy);
fprintf(stderr, "Done!\n");

return 1;
}

int saveBckgrndMatrixToParadigmKOdb(struct sqlConnection * conn,char * tableName, matrix * data)
{
fprintf(stderr, "Creating background table %s\n", tableName);

struct dyString *dy = newDyString(1024);
dyStringPrintf(dy, "CREATE TABLE %s (\n", tableName);
dyStringPrintf(dy, "sample_id int not null,\n");
dyStringPrintf(dy, "feature_id int  not null,\n");
dyStringPrintf(dy, "val float not null,\n");
dyStringPrintf(dy, "KEY(feature_id, sample_id),\n");
dyStringPrintf(dy, "KEY(sample_id, feature_id)\n");
dyStringPrintf(dy, ")\n");
sqlUpdate(conn,dy->string);
dyStringFree(&dy);

int col=0,currRow=0, lastPrintRow=0;
char tmp[256];

fprintf(stderr, "Table created. Saving background to database..\n");
for(currRow = 0; currRow < data->rows; currRow++)
    {
	if(currRow % 5 == 0)
    	fprintf(stderr, "%.2f%% saving completed\n", (double)(100*currRow)/data->rows);
    for(col = 0; col < data->cols; col++)
        {
        //calc the next entry
        safef(tmp, sizeof(tmp), "(%d,%d,%.8f)", (-col-1), (-currRow-1), data->graph[currRow][col]);

        if(currRow == 0 && col == 0)//if it's the first row, start it
            {
            dy = newDyString(20000);
            dyStringPrintf(dy, "INSERT INTO %s (sample_id, feature_id, val) VALUES %s", tableName, tmp);
            }
        else if((currRow == (data->rows-1)) && (col == (data->cols-1))) //if it's the last one, print
            {
            dyStringPrintf(dy,",%s;\n", tmp);
            sqlUpdate(conn, dy->string);
            dyStringFree(&dy);
            }
        else if((strlen(tmp) + dy->stringSize) > dy->bufSize) //if you're about to run out of room, print
            {
            dyStringPrintf(dy,";\n");
            sqlUpdate(conn, dy->string);
            dyStringFree(&dy);
            dy = newDyString(20000);
            dyStringPrintf(dy, "INSERT INTO %s (sample_id, feature_id, val) VALUES %s", tableName, tmp);
            lastPrintRow = currRow;
            }
        else
            dyStringPrintf(dy, ",%s", tmp);
        }
    }
dyStringFree(&dy);
fprintf(stderr, "Done!\n");

fprintf(stderr, "Updating datasets table..\n");
safef(tmp, sizeof(tmp), "SELECT max(id) FROM datasets");
int nextId = sqlNeedQuickNum(conn, tmp) +1;
dy = newDyString(1024);
dyStringPrintf(dy, "INSERT INTO datasets (id,num_samples, data_table, created_date) ");
dyStringPrintf(dy, "VALUES (%d,%d,\'%s\', NOW());",nextId,  BACKGROUND_SAMPLES, tableName);
sqlUpdate(conn, dy->string);
dyStringFree(&dy);
fprintf(stderr, "Done!\n");

return 1;
}



int buildBackgroundTables(char * profile, char * db)
{
struct sqlConnection *conn = hAllocConnProfile(profile, db);

//make a hash of dataset ids and the number of samples in each
char query[256];
safef(query, sizeof(query), "SELECT DISTINCT(type_id) FROM datasets WHERE type_id != 0;");
struct slInt *currDataType, *dataTypes =  sqlQuickNumList(conn, query);
currDataType = dataTypes;
while(currDataType){
	safef(query, sizeof(query), "SELECT id,num_samples FROM datasets where type_id=%d and data_table not like \"BACKGROUND_%%\";", currDataType->val);
	struct hash *sampleCountHash = sqlQuickHash(conn, query);
	struct hashEl *hel;
	struct hashCookie cookie;
	
	//find total number of samples currently in db
	safef(query, sizeof(query), "SELECT sum(num_samples) FROM datasets where type_id=%d and data_table not like \"BACKGROUND_%%\";", currDataType->val);
	int N = sqlQuickNum(conn, query);
	
	//Fill in columns of resulting background table
	matrix * result = init_matrix(BACKGROUND_GENES, BACKGROUND_SAMPLES);
	int i,j, datasetId, randSample, countSum= 0;
	srand(5);//seed, but in the same way - helps with reproducibility
	fprintf(stderr,"Selecting randomized values of data type %d..\n", currDataType->val);
	for(j = 0; j < BACKGROUND_SAMPLES; j++)
		{
		if(j % 5 == 0)
			fprintf(stderr, "%.2f %% selection completed\n", (double)(100*j)/BACKGROUND_SAMPLES);
		randSample = (int)rand()%N;
		countSum = 0;
		cookie = hashFirst(sampleCountHash);
		while ((hel = hashNext(&cookie)) != NULL)
		{
			int thisCount=atoi(hel->val);
			if((countSum+thisCount) > randSample)
				break;
			else
				countSum += thisCount;
		}
		datasetId = atoi(hel->name);
		safef(query, sizeof(query), "SELECT data_table FROM datasets WHERE id=%d;", datasetId);
		char * datasetName = sqlNeedQuickString(conn, query);
		
		int totalRandValues = 0;
		while(totalRandValues < BACKGROUND_GENES)
			{
			safef(query, sizeof(query), "SELECT val FROM %s ORDER BY RAND() LIMIT %d;" ,datasetName , BACKGROUND_GENES);
		    struct slDouble *currRandVal, *randValues = sqlQuickDoubleList(conn, query);
		    currRandVal = randValues;
		
			for(i = 0; i < BACKGROUND_GENES && currRandVal!= NULL; i++)
				{
				result->graph[i][j] = currRandVal->val;
				totalRandValues++;
				currRandVal = currRandVal->next;
				}
			slFreeList(&randValues);
			}
		}
	fprintf(stderr, "Done!\n");
	//find the correct place to save to db and dooo it.
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	char currDate[256];
	strftime(currDate,sizeof(currDate),"%x",timeinfo);
	char backgroundTableName[256];
	safef(backgroundTableName, sizeof(backgroundTableName), "BACKGROUND_DATATYPE%d_%s", currDataType->val, currDate);
	memSwapChar(backgroundTableName, sizeof(backgroundTableName), '/', '_');
	if(sameString(profile, "paradigmKO"))
		saveBckgrndMatrixToParadigmKOdb(conn, backgroundTableName, result);
	else
		saveBckgrndMatrixToBioInt(conn, backgroundTableName, result);
	free_matrix(result);
	currDataType = currDataType->next;
	}
return 1;
}

