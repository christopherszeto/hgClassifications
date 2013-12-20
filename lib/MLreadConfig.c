#include "../inc/MLmatrix.h"
#include "../inc/MLcrossValidation.h"
#include "../inc/MLreadConfig.h"
#include "../inc/MLio.h"
#include "portable.h"

struct hash * parseParametersFromConfig(struct hash * config)
/*Grabs the parameters entry out of config and makes a hash of the csv entries with structure [field1]=[val],[field2]=[val] etc.*/
/*If the parameter string doesn't match this format, returns NULL*/
{
char * parameterString = hashMustFindVal(config, "parameters");
if(strchr(parameterString, '=') == NULL)
	return NULL;
struct slName * param,* paramList = slNameListFromComma(parameterString);
struct hash * result = hashNew(0);
for(param = paramList; param != NULL; param = param->next)
	{
	if(strchr(param->name, '='))
		{
		char *name, *val;
		char * tok = strtok(param->name, "=");
		name = cloneString(tok);
		tok = strtok(NULL, "=");
		val = cloneString(tok);
		hashAdd(result, name, val);
		}
	else
		{
		errAbort("Error: No value associated with parameter in string %s",param->name);
		}
	}
return result;
}

int foldsCountFromConfig(struct hash *config)
{
char * cv = hashMustFindVal(config, "crossValidation");
char * inputType = hashMustFindVal(config, "inputType");

//if there's a folds description, use that
if(hashFindVal(config, "foldsDescriptionPath"))
	return foldsCountFromFoldsDescription(config);

int folds = -1;
if(sameString("k-fold",cv))
    {
    folds = atoi(hashMustFindVal(config, "folds")) ;
    }
else if(sameString("loo", cv))
    {
	if(sameString(inputType, "bioInt"))
		{
		char * profile = hashMustFindVal(config, "profile");
		char * db = hashMustFindVal(config, "db");
    	char * tableName = hashMustFindVal(config, "tableName");
        struct sqlConnection *conn = hAllocConnProfile(profile, db); 
    	folds = bioInt_colCount(conn, tableName);
		if(hashFindVal(config, "excludeList")!= NULL)
			{
			//read the labels into an sll
			char * excludeListFile = hashMustFindVal(config, "excludeList");
			FILE * fp = fopen(excludeListFile, "r");
			if(fp == NULL)
			    errAbort("Couldn't open excludeList file %s\n", excludeListFile);
			char line[1024];
			fgets(line, sizeof(line), fp);
			fclose(fp);
			struct slName *excludeName, *excludeList = slNameListFromString(line, ',');
			excludeName = excludeList;
			char query[256];

			//get teh dataset id for qurying against
			safef(query, sizeof(query), "SELECT id FROM datasets WHERE data_table =\'%s\'", tableName);
			int dataset_id=sqlQuickNum(conn, query);
	
			//take a count of how many samples should be excluded
			int excluded = 0;
			while(excludeName)
				{
				safef(query, sizeof(query), "SELECT COUNT(*) FROM samples WHERE name=\'%s\' AND dataset_id=%d", 
					excludeName->name, dataset_id);
				if(sqlQuickNum(conn, query))
					excluded++;
				excludeName=excludeName->next;
				}
			folds -= excluded;
			slFreeList(&excludeList);
			}
   		hFreeConn(&conn);
		}
	else if(sameString(inputType, "flatfiles"))
		{
    	char * dataFilepath = hashMustFindVal(config, "dataFilepath");
    	FILE * fp = fopen(dataFilepath,"r");
    	if(fp == NULL)
        	errAbort("Couldn't open file %s\n", dataFilepath);
    	folds = colCount(fp);
    	fclose(fp);     
    	}   
	}
if(folds == -1)
    {
    errAbort("Couldn't assign folds for modelDir building\n");
    }
return folds;
}

int splitsCountFromConfig(struct hash *config)
{
//if there's a folds description, use that
if(hashFindVal(config, "foldsDescriptionPath"))
    return splitsCountFromFoldsDescription(config);

if(hashFindVal(config, "foldMultiplier") != NULL)
	return atoi(hashMustFindVal(config, "foldMultiplier"));
else
	return 1;
}

int foldsCountFromDataDir(struct hash *config)
{
char splitDir[1024];
safef(splitDir, sizeof(splitDir), "%s/split01", (char*)hashMustFindVal(config, "trainingDir"));
struct slName *fold,* foldDirs = listDir(splitDir, "fold*");
fold = foldDirs;
int folds = 0;
for(fold = foldDirs; fold != NULL; fold = fold->next)
	folds++;
return folds;
}

int splitsCountFromDataDir(struct hash *config)
{
char * trainingDir = hashMustFindVal(config, "trainingDir");
struct slName *split,* splitDirs = listDir(trainingDir, "split*");
split = splitDirs;
int splits = 0;
for(split = splitDirs; split != NULL; split = split->next)
    splits++;
return splits;
}


int featuresCountFromConfig(struct hash *config)
{
char * trainingDir = hashMustFindVal(config, "trainingDir");
char * classifierType = hashMustFindVal(config, "classifier");
char file[1024];
int count =-1;
if(sameString(classifierType,"NMFpredictor") || sameString(classifierType, "glmnet"))
	{
	safef(file, sizeof(file), "%s/split01/fold01/data.tab", trainingDir);
	FILE * fp = fopen(file, "r");
	if(fp == NULL)
		errAbort("Couldn't open file %s while trying to count features.\n", file);
	count = lineCount(fp) - 1; //one labeling line
	fclose(fp);
	}
else if(sameString(classifierType, "SVMlight"))
	{
    safef(file, sizeof(file), "%s/split01/fold01/data.svm", trainingDir);
    FILE * fp = fopen(file, "r");
    if(fp == NULL)
        errAbort("Couldn't open file %s while trying to count features.\n", file);
    char * line = readLine(fp);
	fclose(fp);
	count = countChars(line, ' ');
	}
else if(sameString(classifierType, "WEKA"))
	{
    safef(file, sizeof(file), "%s/split01/fold01/data.arff", trainingDir);
    FILE * fp = fopen(file, "r");
	if(fp == NULL)
        errAbort("Couldn't open file %s while trying to count features.\n", file);
	count = 0;
	char * line;
	while( (line = readLine(fp)) && line != NULL)
		{
		if(strstr(line, "@ATTRIBUTE"))
			count++;
		}
	count--;//one labeling line
	fclose(fp);
	}

if(count == -1)
	errAbort("ERROR: Couldn't count features in data associated with config.\n");

return count;
}

matrix * foldsDescriptionFromConfig(struct hash * config)
{
char * foldsDescriptionPath = hashMustFindVal(config, "foldsDescriptionPath");	
FILE * fp = fopen(foldsDescriptionPath, "r");
if(fp == NULL)
	errAbort("ERROR: Couldn't open foldsDescriptionPath file %s for reading\n", foldsDescriptionPath);
matrix *  foldsDescription = f_fill_matrix(fp, 1);
fclose(fp);
return foldsDescription;
}

int splitsCountFromFoldsDescription(struct hash * config)
{
char * foldsDescriptionPath = hashMustFindVal(config, "foldsDescriptionPath");
FILE * fp = fopen(foldsDescriptionPath, "r");
if(fp == NULL)
    errAbort("ERROR: Couldn't open foldsDescriptionPath file %s for reading\n", foldsDescriptionPath);
int result = lineCount(fp);
result--;
fclose(fp);
return result;
}

int foldsCountFromFoldsDescription(struct hash * config)
{
matrix * fd = foldsDescriptionFromConfig(config);
int i, j;
double max = fd->graph[0][0];
for(i = 0; i < fd->rows; i++)
	{
	for(j = 0; j < fd->cols; j++)
		{
		if(fd->graph[i][j] != NULL_FLAG && fd->graph[i][j] > max)
			max = fd->graph[i][j];
		}
	}
free_matrix(fd);
return max;
}

int itemsInFoldFromConfig(struct hash * config,int n, int split, int fold)
{
int result = 0;
if(hashFindVal(config, "foldsDescriptionPath"))
	{
	matrix * fd = foldsDescriptionFromConfig(config);
	int i;
	for(i = 0; i < fd->cols; i++)
		{
		if(fd->graph[(split-1)][i] == fold)
			result++;
		}
	free_matrix(fd);
	}
else
	{
	int folds = foldsCountFromConfig(config);
	if(fold == folds)//make sure if it's the last fold it gets everything, even if it's uneven
		result = n - ((folds - 1) * floor(n/folds));
	else //otherwise assign equally to all folds
		result = floor(n/folds);
	}
return result;
}

