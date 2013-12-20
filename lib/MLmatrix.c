#include "../inc/MLmatrix.h"
#include "../inc/MLcrossValidation.h"
#include "../inc/MLreadConfig.h"
#include "../inc/MLio.h"
#include "../inc/sqlite3.h"

matrix * init_matrix(int rows, int cols){
    int i, j;
    matrix * A_ptr = (matrix*)needMem(sizeof(matrix));
   	A_ptr->rows = rows;
    A_ptr->cols = cols;
	//make space for values
    A_ptr->graph = needMem(A_ptr->rows*sizeof(double));
    if(A_ptr->graph == NULL){
    	fprintf(stderr, "ERROR: Couldn't assign memory to a matrix. Exiting..\n");
        exit(1);
    }
    for(i = 0; i < rows; i++ ){
        A_ptr->graph[i] = needMem(A_ptr->cols*sizeof(double));
    }
    for(i = 0; i < A_ptr->rows; i++){
        for(j = 0; j < A_ptr->cols; j++){
            A_ptr->graph[i][j] = NULL_FLAG;
        }
    }
	//make space for labels
	A_ptr->labels = 0;
	A_ptr->rowLabels = (char **)needMem(A_ptr->rows*sizeof(char **));
	for(j = 0; j < A_ptr->rows; j++){
		A_ptr->rowLabels[j] = (char *)needMem(MAX_LABEL*sizeof(char *));
	}
    A_ptr->colLabels = (char **)needMem(A_ptr->cols*sizeof(char **));
	for(j = 0; j < A_ptr->cols; j++){
		A_ptr->colLabels[j] = (char *)needMem(MAX_LABEL*sizeof(char *));
	}
	return A_ptr;
}

matrix * copy_matrix(matrix *A_ptr){
	int i = 0, j = 0;
	matrix * result = init_matrix(A_ptr->rows, A_ptr->cols);
	for(i =0; i < A_ptr->rows; i++){
        for(j = 0; j < A_ptr->cols; j++){
            result->graph[i][j] = A_ptr->graph[i][j];
        }
    }
	if(A_ptr->labels){
		for(i = 0; i < A_ptr->rows; i++){
			safef(result->rowLabels[i], MAX_LABEL, "%s", A_ptr->rowLabels[i]);
		}	
		for(i = 0; i < A_ptr->cols; i++){
			safef(result->colLabels[i],MAX_LABEL, "%s", A_ptr->colLabels[i]);
		}
		result->labels=1;
	}
    return result;
}

void copy_matrix_labels(matrix *target, matrix *template, int targetDirection, int templateDirection){
	int i =0;
	if(target->labels){
		fprintf(stderr, "ERROR: Trying to save labels in a matrix with labels already. Unsupported behaviour.\n");
		exit(1);
	}
	//copy row  labels
	if(templateDirection == 1){
		//.. to row labels
		if(targetDirection == 1){
				if(template->rows != target->rows){
					fprintf(stderr, "ERROR: Trying to copy rows to incompatibly sized matrix. Exiting...\n");
					exit(1);
				}
				for(i = 0; i < target->rows; i++){
					safef(target->rowLabels[i],MAX_LABEL, "%s", template->rowLabels[i]);
				}
		//... to col labels
		}else{
                if(template->rows != target->cols){
                    fprintf(stderr, "ERROR: Trying to copy rows to incompatibly sized matrix. Exiting...\n");
                    exit(1);
                }
                for(i = 0; i < target->cols; i++){
                    safef(target->colLabels[i],MAX_LABEL, "%s", template->rowLabels[i]);
                }
		}
	//copy col labels
	}else if(templateDirection == 2){
		//..to col labels
		if(targetDirection == 2){
				if(template->cols != target->cols){
					fprintf(stderr, "ERROR: Trying to copy cols to incompatibly sized matrix. Exiting...\n");
					exit(1);
				}
				for(i = 0; i < target->cols; i++){
					safef(target->colLabels[i],MAX_LABEL, "%s", template->colLabels[i]);
				}
		//... to row labels
		}else{
                if(template->cols != target->rows){
                    fprintf(stderr, "ERROR: Trying to copy cols to incompatibly sized matrix. Exiting...\n");
                    exit(1);
                }
                for(i = 0; i < target->rows; i++){
                    safef(target->rowLabels[i],MAX_LABEL, "%s", template->colLabels[i]);
                }
		}
	}
	target->labels=1;
}

int matchedRowLabels(matrix * A_ptr,matrix * B_ptr){
	if(A_ptr->rows != B_ptr->rows || !A_ptr->labels || !B_ptr->labels)
		return 0;

	int i;
	for(i = 0; i < A_ptr->rows; i++){
		if(!sameString(A_ptr->rowLabels[i], B_ptr->rowLabels[i]))
			return 0;
	}
	return 1;
}

int matchedColLabels(matrix * A_ptr,matrix * B_ptr){
    if(A_ptr->cols != B_ptr->cols || !A_ptr->labels || !B_ptr->labels)
        return 0;

    int i;
    for(i = 0; i < A_ptr->cols; i++){
        if(!sameString(A_ptr->colLabels[i], B_ptr->colLabels[i]))
            return 0;
    }
    return 1;
}


/*remove all rows from A_ptr that don't appear in B_ptr*/
matrix * matchOnRowLabels(matrix *  A_ptr, matrix * B_ptr){
	//set a space to save results to
	matrix * result = init_matrix(B_ptr->rows, A_ptr->cols);
	copy_matrix_labels(result, B_ptr, 1,1);
	result->labels=0;
	copy_matrix_labels(result, A_ptr, 2,2);
	result->labels=1;

	//create an index lookup for all entries in A
	int i,j;
	struct hash * A_ixs = hashNew(0);
	for(i = 0; i < A_ptr->rows; i++)
		hashAddInt(A_ixs, A_ptr->rowLabels[i], i);

	//iterate over B, copying rows that match in A, or NULLs if none exist.
	for(i = 0; i < B_ptr->rows; i++){
		int ix = hashIntValDefault(A_ixs, B_ptr->rowLabels[i], -1);
		if(ix != -1){
			for(j = 0; j < A_ptr->cols; j++)
				result->graph[i][j] = A_ptr->graph[ix][j];
		}else{
            for(j = 0; j < A_ptr->cols; j++)
                result->graph[i][j] = NULL_FLAG;
		}
	}
		
	return result;
}

/*remove all cols from A_ptr that don't appear in B_ptr*/
matrix * matchOnColLabels(matrix *  A_ptr, matrix * B_ptr){
    //set a space to save results to
    matrix * result = init_matrix(A_ptr->rows, B_ptr->cols);
    copy_matrix_labels(result, A_ptr, 1,1);
	result->labels=0;
    copy_matrix_labels(result, B_ptr, 2,2);
	result->labels=1;

    //create an index lookup for all entries in A
    int i,j;
    struct hash * A_ixs = hashNew(0);
    for(i = 0; i < A_ptr->cols; i++)
        hashAddInt(A_ixs, A_ptr->colLabels[i], i);

    //iterate over B, copying cols that match in A, or NULLs if none exist.
    for(j = 0; j < B_ptr->cols; j++){
        int ix = hashIntValDefault(A_ixs, B_ptr->colLabels[j], -1);
        if(ix != -1){
            for(i = 0; i < A_ptr->rows; i++)
                result->graph[i][j] = A_ptr->graph[i][ix];
        }else{
            for(i = 0; i < A_ptr->rows; i++)
                result->graph[i][j] = NULL_FLAG;
        }
    }
    return result;
}


void mult_matrices(matrix *A_ptr, matrix *B_ptr, matrix * AB_ptr){
	int x, y, r, n;

	if( A_ptr->cols != B_ptr->rows || AB_ptr->rows != A_ptr->rows || AB_ptr->cols != B_ptr->cols){
        	printf( "ERROR: Dimensions of matrices being multiplied are imcompatible\n");
        	exit(1);
    	}
    	for(x= 0; x < AB_ptr->rows; x++){
        	for(y=0; y < AB_ptr->cols; y++){
            		AB_ptr->graph[x][y] = 0;
        	}
    	}

    	n = A_ptr->cols;
    	for(x = 0; x < AB_ptr->rows; x++ ){
        	for(y = 0; y < AB_ptr->cols; y++ ){
            		for(r = 0; r < n; r++){
				if(A_ptr->graph[x][r] != NULL_FLAG && B_ptr->graph[r][y] != NULL_FLAG)
                			AB_ptr->graph[x][y] += A_ptr->graph[x][r]*B_ptr->graph[r][y];
            		}
        	}
    	}
}

void add_matrices(matrix *A_ptr, matrix *B_ptr, matrix *AB_ptr){
	if(A_ptr->rows != B_ptr->rows || A_ptr->rows != AB_ptr->rows || A_ptr->cols != B_ptr->cols || A_ptr->cols != AB_ptr->cols){
		fprintf(stderr,"ERROR: Trying to add matrices with incompatible dimensions. Exiting...\n");
		exit(1);
	}
	int i, j;
	for(i = 0; i < AB_ptr->rows; i++){
		for(j = 0; j < AB_ptr->cols; j++){
			if(A_ptr->graph[i][j] == NULL_FLAG)
				AB_ptr->graph[i][j] = B_ptr->graph[i][j];
			else if(B_ptr->graph[i][j] == NULL_FLAG)
				AB_ptr->graph[i][j] = A_ptr->graph[i][j];
			else
				AB_ptr->graph[i][j] = A_ptr->graph[i][j] + B_ptr->graph[i][j];
		}
	}
}

void subtract_matrices(matrix *A_ptr, matrix *B_ptr, matrix *AB_ptr){
    if(A_ptr->rows != B_ptr->rows || A_ptr->rows != AB_ptr->rows || A_ptr->cols != B_ptr->cols || A_ptr->cols != AB_ptr->cols){
        fprintf(stderr,"ERROR: Trying to add matrices with incompatible dimensions. Exiting...\n");
        exit(1);
    }
    int i, j;
	
    for(i = 0; i < AB_ptr->rows; i++){
        for(j = 0; j < AB_ptr->cols; j++){
            if(A_ptr->graph[i][j] == NULL_FLAG)
                AB_ptr->graph[i][j] = NULL_FLAG;
            else if(B_ptr->graph[i][j] == NULL_FLAG)
                AB_ptr->graph[i][j] = NULL_FLAG;
            else
                AB_ptr->graph[i][j] = A_ptr->graph[i][j] - B_ptr->graph[i][j];
        }
    }
}

void delta_matrices(matrix *A_ptr, matrix *B_ptr, matrix *AB_ptr){
    if(A_ptr->rows != B_ptr->rows || A_ptr->rows != AB_ptr->rows || A_ptr->cols != B_ptr->cols || A_ptr->cols != AB_ptr->cols){
        fprintf(stderr,"ERROR: Trying to add matrices with incompatible dimensions. Exiting...\n");
        exit(1);
    }
    int i, j;
    for(i = 0; i < AB_ptr->rows; i++){
        for(j = 0; j < AB_ptr->cols; j++){
            if(A_ptr->graph[i][j] == NULL_FLAG)
                AB_ptr->graph[i][j] = NULL_FLAG;
            else if(B_ptr->graph[i][j] == NULL_FLAG)
                AB_ptr->graph[i][j] = NULL_FLAG;
            else{
				if(B_ptr->graph[i][j] < A_ptr->graph[i][j])
					AB_ptr->graph[i][j] = A_ptr->graph[i][j] - B_ptr->graph[i][j];
				else
					AB_ptr->graph[i][j] = B_ptr->graph[i][j] - A_ptr->graph[i][j];
			}
        }
    }
}


void add_matrices_by_colLabel(matrix * A_ptr, matrix * B_ptr){
	if(A_ptr->rows != B_ptr->rows || A_ptr->cols != B_ptr->cols)
        errAbort("ERROR: Trying to add matrices by column label with incompatible dimensions. Exiting...\n");

    int Acol, Bcol, i;
	for(Acol = 0; Acol < A_ptr->cols; Acol++){
		for(Bcol = 0; Bcol <= B_ptr->cols; Bcol++){
			if(Bcol == B_ptr->cols)
				errAbort("Trying to add matrices with a mismatched label. Exiting.\n");
			else if(sameString(A_ptr->colLabels[Acol], B_ptr->colLabels[Bcol]))
				break;
		}

    	for(i = 0; i < A_ptr->rows; i++){
            if(A_ptr->graph[i][Acol] == NULL_FLAG)
                A_ptr->graph[i][Acol] = B_ptr->graph[i][Bcol];
            else if(B_ptr->graph[i][Bcol] == NULL_FLAG)
                A_ptr->graph[i][Acol] = A_ptr->graph[i][Acol];
            else
                A_ptr->graph[i][Acol] = A_ptr->graph[i][Acol] + B_ptr->graph[i][Bcol];
		}
    }
}	


matrix * append_matrices(matrix * A_ptr, matrix * B_ptr, int direction){
	matrix * result = NULL;
	int row = 0, col = 0, templateRow= 0, templateCol = 0;
	if(direction == 1){
		if(A_ptr->rows != B_ptr->rows)
			errAbort("Incompatible dimensions for appending two matrices\n");
		result = init_matrix(A_ptr->rows, A_ptr->cols+B_ptr->cols);
		if(A_ptr->labels &&  B_ptr->labels)
			copy_matrix_labels(result, A_ptr, 1,1);
		for(row = 0; row < result->rows; row++){
			for(col = 0; col < A_ptr->cols; col++){
				result->graph[row][col] = A_ptr->graph[row][col];
				if(A_ptr->labels)	
					safef(result->colLabels[col], MAX_LABEL, "%s", A_ptr->colLabels[col]);
			}
			templateCol = 0;
			for(col = col; col < result->cols && templateCol < B_ptr->cols; col++, templateCol++){
				result->graph[row][col] = B_ptr->graph[row][templateCol];
				if(B_ptr->labels)
                    safef(result->colLabels[col], MAX_LABEL, "%s", B_ptr->colLabels[templateCol]);
			}
		}
        if(B_ptr->labels && A_ptr->labels)
            result->labels=1;
	}else if(direction == 2){
        if(A_ptr->cols != B_ptr->cols)
            errAbort("Incompatible dimensions for appending two matrices\n");
        result = init_matrix(A_ptr->rows+B_ptr->rows, A_ptr->cols);

        for(row = 0,templateRow = 0; row < A_ptr->rows && templateRow < A_ptr->rows; templateRow++){
			if(A_ptr->labels)
				safef(result->rowLabels[row], MAX_LABEL, "%s", A_ptr->rowLabels[templateRow]);
            for(col = 0, templateCol = 0; col < result->cols; col++){
				if(A_ptr->labels && row == 0)
					safef(result->colLabels[col], MAX_LABEL, "%s", A_ptr->colLabels[templateCol]);
               	result->graph[row][col] = A_ptr->graph[templateRow][templateCol++];
			}
			row++;
		}
        for(templateRow = 0; templateRow < B_ptr->rows; templateRow++){
            if(B_ptr->labels)
                safef(result->rowLabels[row], MAX_LABEL, "%s", B_ptr->rowLabels[templateRow]);
            for(col = 0, templateCol = 0; col < result->cols; col++)
                result->graph[row][col] = B_ptr->graph[templateRow][templateCol++];
			row++;
        }
		if(B_ptr->labels && A_ptr->labels)
			result->labels=1;
	}else
		errAbort("Unsupported direction for appending matrices. Exiting.\n");
	return result;
}

matrix * r_fill_matrix(int rows, int cols, double min, double max){
	matrix * A_ptr = init_matrix(rows, cols);	
	int i, j;
    	time_t seconds;
    	time(&seconds);
    	srand((unsigned int) seconds);
    	for(i = 0; i < rows; i++){
        	for(j = 0; j < cols; j++){
            		A_ptr->graph[i][j] = (rand()/(((double)RAND_MAX + 1) / max));
        	}
    	}
	return A_ptr;
}

matrix * sr_fill_matrix(unsigned int seed, int rows, int cols, double min, double max){
	matrix * A_ptr = init_matrix(rows, cols);
    int i, j;
        srand(seed);
        for(i = 0; i < A_ptr->rows; i++){
            for(j = 0; j < A_ptr->cols; j++){
                    A_ptr->graph[i][j] = (rand()/(((double)RAND_MAX + 1) / max));
            }
        }
	return A_ptr;
}

matrix * unif_fill_matrix(int rows, int cols, double val){
	matrix * A_ptr = init_matrix(rows, cols);
    	int i,j;
    	for(i = 0; i < A_ptr->rows; i++){
        	for(j = 0; j < A_ptr->cols; j++){
            		A_ptr->graph[i][j] = val;
        	}
    	}
	return A_ptr;
}

/* Allows you to set a table of random values to select uniformly from */
/* so you can have any shaped distribution you want, so long as table */
/* holds values from it*/
matrix * r_fill_matrix_from_table(int rows, int cols, double *table, int n){
	matrix * A_ptr = init_matrix(rows, cols);
	int i, j;
    	double val;
    	time_t seconds;
    	time(&seconds);
    	srand((unsigned int) seconds);
    	for(i = 0; i < A_ptr->rows; i++){
        	for(j = 0; j < A_ptr->cols; j++){
            		val = table[(rand()%(n + 1))];
            		if(val == 0)
                		val+= MIN_RANDOM_VAL; //never allow the value to be zero - can't learn on that
           		A_ptr->graph[i][j] = val;
        	}
    	}
	return A_ptr;
}

/*Scales the matrix A to add to val. Direction = 1 means scale by row, direction 2 means scale by column*/
void scale_matrix(matrix *A_ptr, int direction, double val){
	int i, j;
    	double scale_factor = 0, sum=0,min;
    	if(direction == 1){
        	for(i = 0; i < A_ptr->rows; i++){
            		sum=0;
			min = A_ptr->graph[i][0];
            		for(j = 0; j < A_ptr->cols; j++){
				if(min > A_ptr->graph[i][j] && A_ptr->graph[i][j] != NULL_FLAG)
					min = A_ptr->graph[i][j];
			}
			for(j = 0; j < A_ptr->cols; j++){
                		if(min < 0 && A_ptr->graph[i][j] != NULL_FLAG)
                    			A_ptr->graph[i][j] -= min+MIN_RANDOM_VAL;
				if(A_ptr->graph[i][j] != NULL_FLAG)	
                			sum+= fabs(A_ptr->graph[i][j]);
			}
            		scale_factor = val/sum;
           		for(j = 0; j < A_ptr->cols; j++){
				if(A_ptr->graph[i][j] != NULL_FLAG)
                			A_ptr->graph[i][j] *= scale_factor;
			}
        	}
    	}else if(direction == 2){
        	for(j = 0; j < A_ptr->cols; j++){
            		sum=0;
			min = A_ptr->graph[0][j];
            		for(i = 0; i < A_ptr->rows; i++){
				if(min > A_ptr->graph[i][j] && A_ptr->graph[i][j] != NULL_FLAG)
					min = A_ptr->graph[i][j];
			}
			for(i = 0; i < A_ptr->rows; i++){
				if(min < 0 && A_ptr->graph[i][j] != NULL_FLAG)
					A_ptr->graph[i][j] -= min+MIN_RANDOM_VAL;
				if(A_ptr->graph[i][j] != NULL_FLAG)
                			sum+= fabs(A_ptr->graph[i][j]);
			}
            		scale_factor = val/sum;
            		for(i = 0; i < A_ptr->rows; i++)
				if(A_ptr->graph[i][j] != NULL_FLAG)
                			A_ptr->graph[i][j] *= scale_factor;
        	}
    	}else if(direction == 3){
		sum = 0;
		min = A_ptr->graph[0][0];
		for(i = 0; i < A_ptr->rows; i++){
			for(j = 0; j < A_ptr->cols; j++){
				if(min > A_ptr->graph[i][j] && A_ptr->graph[i][j] != NULL_FLAG)
                    			min = A_ptr->graph[i][j];
			}
		}
		for(i= 0; i < A_ptr->rows; i++){
			for(j = 0; i < A_ptr->cols; j++){
				if(min < 0 && A_ptr->graph[i][j] != NULL_FLAG)
                    			A_ptr->graph[i][j] -= min+MIN_RANDOM_VAL;
				if(A_ptr->graph[i][j] != NULL_FLAG)
                			sum+= fabs(A_ptr->graph[i][j]);
			}
		}
        	scale_factor = val/sum;
        	for(i = 0; i < A_ptr->rows; i++)
			for(j = 0; j < A_ptr->cols; j++)
				if(A_ptr->graph[i][j] != NULL_FLAG)
					A_ptr->graph[i][j] *= scale_factor;
	}else{
        	fprintf(stderr, "Warning: Incorrect direction setting on a call to scale a matrix\n");
    	}
}

/*Moves the matrix up to contain no negative or zero values. Returns the amount the matrix was shifted by*/
double shift_matrix(matrix *A_ptr){
        double min = A_ptr->graph[0][0];
	int i, j;
        for(i = 0; i < A_ptr->rows; i++){
                for(j = 0; j < A_ptr->cols; j++){
                        if(min > A_ptr->graph[i][j] && A_ptr->graph[i][j] != NULL_FLAG)
                                min = A_ptr->graph[i][j];
                }
        }
	if(min < 0){
		min+=MIN_RANDOM_VAL; //makes whole matrix non-zero
        	for(i= 0; i < A_ptr->rows; i++){
                	for(j = 0; j < A_ptr->cols; j++){
                        	if(A_ptr->graph[i][j] != NULL_FLAG)
                                	A_ptr->graph[i][j] -= min;
                	}
        	}
		return -min;
	}
	return 0;
}



/*Read fp into matrix A. labels is a flag for if there is naming  (must be BOTH row and col) in the file*/
matrix * f_fill_matrix(FILE *fp, int labels){
	int rows = lineCount(fp);
	int cols = colCount(fp);
	if(labels){
		rows--;
		cols--;
	}
	
	matrix * A_ptr = init_matrix(rows, cols);
	char *tok, *line=malloc((MAX_LABEL+A_ptr->cols*MAX_TOKEN) * sizeof(char));
	int i=0,j=0,colCounter=0, lineCounter=0;

	while(fgets(line, (MAX_LABEL+A_ptr->cols*MAX_TOKEN), fp)){
		j=0;
		if(labels && lineCounter==0){
        	tok = strtok(line, "\t\n");

        	//if there's a top-left label, skip it
        	if(line[0] !='\t'){
            	tok = strtok(NULL,"\t\n");
        	}
			j=0;
        	while(tok != NULL){
            	if(j >= A_ptr->cols){
                	fprintf(stderr, "ERROR: More labels found than matrix has columns. Exiting...\n");
                	exit(1);
           		}
            	safef(A_ptr->colLabels[j++],MAX_LABEL, "%s", tok);
            	tok = strtok(NULL, "\t\n");
        	}
    	}else{
			if(i >= A_ptr->rows){
				fprintf(stderr, "ERROR: matrix file has too many lines. Exiting...\n");	
				exit(1);
			}
			tok = strtok(line, "\t\n");
			if(labels && colCounter == 0){
				safef(A_ptr->rowLabels[i],MAX_LABEL, "%s", tok);
			}else{
				if(isnum(tok))
					A_ptr->graph[i][j++] = atof(tok);
				else
					A_ptr->graph[i][j++] = NULL_FLAG;
			}
			colCounter++;

			while( (tok = strtok(NULL, "\t\n")) != NULL){
				if(j >= A_ptr->cols){
					fprintf(stderr, "ERROR: line %d of matrix file has too many tokens. Exiting...\n", i);
					fprintf(stderr, "ERROR ON THIS LINE:\n%s\n", line);
					exit(1);
				}
				if(isnum(tok))
					A_ptr->graph[i][j++] = atof(tok);
				else
					A_ptr->graph[i][j++] = NULL_FLAG;
				colCounter++;
			}
			if(j != A_ptr->cols){
				fprintf(stderr, "ERROR: line %d didn't have enough tokens. Exiting...\n", i);
				exit(1);
			}
			i++;
		}
		colCounter=0;
		lineCounter++;
	}
	if(i != A_ptr->rows){
		fprintf(stderr, "ERROR: matrix file didn't have enough lines (Expecting %d, got %d). Exiting...\n", A_ptr->rows, i);
		exit(1);
	}
	free(line);
	if(labels)
		A_ptr->labels=1;	
	return A_ptr;
}

/*make n randomly selected columns from data matrix*/
matrix * permute_fill_matrix(matrix * data, int n)
{
int i, j;
n = n >= data->cols ? data->cols : n;
matrix * result = init_matrix(data->rows, n);
copy_matrix_labels(result, data, 1, 1);

struct slInt * colList = list_indices(n);
struct slInt * currCol, *shuffledColList = shuffle_indices(colList);
currCol = shuffledColList;
struct slInt * rowList = list_indices(data->rows);
struct slInt *currRow, *shuffledRowList = shuffle_indices(rowList);
for(j = 0; j < result->cols; j++)
	{
    i = 0;
    currRow = shuffledRowList;
    while(currRow){
        result->graph[i++][j] = data->graph[currRow->val][currCol->val];
        currRow = currRow->next;
    }
    safef(result->colLabels[j], MAX_LABEL, "PermutedSample%d", (j+1));
	currCol = currCol->next;
	}
slFreeList(&rowList);
slFreeList(&colList);
slFreeList(&shuffledColList);
slFreeList(&shuffledRowList);
result->labels=1;
return result;
}

matrix * copyMetadataByColLabels(matrix * data, matrix * raw_metadata)
/*Read the labels from data, match the raw_metadata to those labels. If a label isn't found, convert to NULLs.*/
{
matrix * result = init_matrix(raw_metadata->rows, data->cols);
copy_matrix_labels(result, raw_metadata, 1,1);
copy_matrix_labels(result, data, 2,2);
result->labels=1;

int targetCol, col, row;
for(targetCol = 0; targetCol < data->cols; targetCol++)
	{
	for(col = 0; col <= raw_metadata->cols; col++)
		{
		if(col == raw_metadata->cols)
			break;
		else
			{
			if(sameString(data->colLabels[targetCol], raw_metadata->colLabels[col]))
				break;
			}
		}	
	if(col == raw_metadata->cols)
		{
		for(row = 0; row < result->rows; row++)	
			result->graph[row][targetCol] = NULL_FLAG;
		}
	else
		{
		for(row = 0; row < result->rows; row++)
			result->graph[row][targetCol] = raw_metadata->graph[row][col];
		}
	}
return result;
}


matrix * bioInt_fill_matrix(struct sqlConnection * conn, char *tableName)
/*Fills a data matrix from the bioInt database structure in tableName*/
{
//make some templates to save interim results to
int rows = bioInt_lineCount(conn, tableName);
int cols = bioInt_colCount(conn, tableName);
//make a place to save results
matrix * result = init_matrix(rows, cols);
//save the gene labels out of db
matrix * featureTemplate = init_matrix(rows, 1);
char query[256];
safef(query, sizeof(query), "SELECT feature_name,id FROM (SELECT DISTINCT(feature_id) "
							"FROM %s) AS a JOIN analysisFeatures ON "
							"(analysisFeatures.id=a.feature_id) ORDER BY a.feature_id", 
							tableName);
struct sqlResult * sr = sqlGetResult(conn, query);
char **resultRow;
int row = 0;
while((resultRow = sqlNextRow(sr)) != NULL)
    {
    safef(featureTemplate->rowLabels[row], MAX_LABEL, "%s", resultRow[0]);
	featureTemplate->graph[row++][0] = atoi(resultRow[1]);
    }
sqlFreeResult(&sr);
//copy the feature names from the template to the result matrix
copy_matrix_labels(result,featureTemplate,1,1);

//iterate samples
safef(query, sizeof(query), "SELECT DISTINCT sample_id FROM %s", tableName);
struct slInt *sampleId, *sampleIdList = sqlQuickNumList(conn, query);
int col = 0;
for(sampleId=sampleIdList; sampleId; sampleId=sampleId->next)
    {
    //save sample label
    safef(query, sizeof(query), "SELECT name FROM samples WHERE id=%d", sampleId->val);
    char * sampleLabel = sqlQuickNonemptyString(conn, query);
    safef(result->colLabels[col], MAX_LABEL, "%s", sampleLabel);
    freeMem(sampleLabel);
	
	//iterate resujlts, assigning them to the correct rows in result matrix
    safef(query, sizeof(query), "SELECT feature_id,val FROM %s "
								"WHERE sample_id=%d ORDER BY feature_id", 
								tableName, sampleId->val);
	sr = sqlGetResult(conn, query);
	int templateRow = 0;
	while((resultRow = sqlNextRow(sr)) != NULL)
		{
		int feature_id = atoi(resultRow[0]);
		while(featureTemplate->graph[templateRow][0] != feature_id)
			{
			templateRow++;
			if(templateRow >= featureTemplate->rows)
				{
				errAbort("Feature %d found in sample %d doesn't exist in table %s", feature_id, sampleId->val, tableName);
				}
			}
		if(sameString(resultRow[1], "NULL"))
			result->graph[templateRow++][col] = NULL_FLAG;
		else
			result->graph[templateRow++][col] = atof(resultRow[1]);
		}
	sqlFreeResult(&sr);
	col++;
    }
slFreeList(&sampleIdList);
free_matrix(featureTemplate);
result->labels=1;
return result;
}


matrix * bioInt_fill_KH(struct sqlConnection * conn, matrix * V, char *clinField)
/*Fill a clinical row matrix in the same order as the given template matrix*/
{
matrix * A_ptr = init_matrix(1, V->cols);
safef(A_ptr->rowLabels[0],MAX_LABEL, "knownVal");

char query[256];
safef(query, sizeof(query), "SELECT id FROM features WHERE name=\'%s\'", clinField);
int featureId = sqlNeedQuickNum(conn, query);
int i;

//save the values to a matrix
for(i = 0; i < V->cols; i++)
    {
    safef(A_ptr->colLabels[i], MAX_LABEL, "%s", V->colLabels[i]);

    safef(query, sizeof(query),"SELECT id FROM samples WHERE name=\'%s\' limit 1", V->colLabels[i]);
	if(sqlExists(conn, query))
		{
	    int sampleId = sqlNeedQuickNum(conn, query);
	    safef(query, sizeof(query), "SELECT val FROM clinicalData WHERE sample_id=%d AND feature_id=%d", sampleId, featureId);
	    char * qresult = sqlQuickString(conn, query);//have to use string despite expecting doubel to handle nulls
	    if(qresult == NULL)
	        A_ptr->graph[0][i] = NULL_FLAG;
	    else
	        A_ptr->graph[0][i] = atof(qresult);
		freeMem(qresult);
	    }
	else
		{
		A_ptr->graph[0][i] = NULL_FLAG;
		}
	}
A_ptr->labels=1;
return A_ptr;
}

matrix * five3db_fill_matrix(struct sqlite3 * conn, char *tableName)
/*Fills a data matrix from the bioInt database structure in tableName*/
{
int rows = five3db_lineCount(conn, tableName);
int cols = five3db_colCount(conn, tableName);

matrix * A_ptr = init_matrix(rows, cols);

char query[256];
struct sqlite3_stmt *stmt = NULL;
safef(query, sizeof(query), "SELECT id FROM data_files WHERE name LIKE \"%s\"", tableName);
sqlite3_prepare(conn, query, -1, &stmt, 0);
sqlite3_step(stmt);
int data_file_id = sqlite3_column_int(stmt, 0);
safef(query, sizeof(query), "SELECT DISTINCT sample_id COLLATE sql_latin1_general_cp1_cs_as FROM data WHERE data_file_id=%d", data_file_id);
sqlite3_prepare(conn, query, sizeof(query), &stmt, NULL);

int row = 0, col = 0;
while(sqlite3_step(stmt) == SQLITE_ROW)
    {
	int sample_id = sqlite3_column_int(stmt, 0);
    //save row labels if this is the first sample being saved
    if(col == 0)
        {
        #ifdef VERBOSE
        printf("Saving feature labels .. \n");
        #endif
        safef(query, sizeof(query), "SELECT DISTINCT feature_id COLLATE sql_latin1_general_cp1_cs_as FROM data WHERE data_file_id=%d", data_file_id);
		struct sqlite3_stmt *featureStmt = NULL;
		sqlite3_prepare(conn, query, sizeof(query), &featureStmt, NULL);
        row = 0;
		while(sqlite3_step(featureStmt) == SQLITE_ROW)
            {
			int feature_id = sqlite3_column_int(featureStmt, 0);
            safef(query, sizeof(query), "SELECT name,type FROM features WHERE id=%d", feature_id);
			struct sqlite3_stmt *featureNameStmt = NULL;
			sqlite3_prepare(conn, query, sizeof(query), &featureNameStmt, NULL);
			sqlite3_step(featureNameStmt);
            char *featureName = (char *)sqlite3_column_text(featureNameStmt, 0);
			stripChar(featureName, '\"');
			stripChar(featureName, '\'');
			char *featureType = (char *)sqlite3_column_text(featureNameStmt, 1);
			if(sameString(featureType, "gene"))
            	safef(A_ptr->rowLabels[row++], MAX_LABEL, "\'%s\'", featureName);
			else
           		safef(A_ptr->rowLabels[row++], MAX_LABEL, "\'%s %s\'", featureName, featureType);
            }
        #ifdef VERBOSE
        printf("Done. Continuing to save matrix..\n");
        #endif
        }

    //save sample label
    safef(query, sizeof(query), "SELECT name FROM samples WHERE id=%d", sample_id);
	struct sqlite3_stmt *sampleNameStmt = NULL;
	sqlite3_prepare(conn, query, sizeof(query), &sampleNameStmt, NULL);
	sqlite3_step(sampleNameStmt);
    char * sampleLabel = (char *)sqlite3_column_text(sampleNameStmt, 0); 
    safef(A_ptr->colLabels[col], MAX_LABEL, "%s", sampleLabel);
	A_ptr->labels=1;

    //save values
    safef(query, sizeof(query), "SELECT val FROM data WHERE sample_id=%d AND data_file_id=%d",
		sample_id, data_file_id);
	sqlite3_stmt *valStmt = NULL;
	sqlite3_prepare(conn, query, sizeof(query), &valStmt, NULL);
	row = 0;
	while(sqlite3_step(valStmt) == SQLITE_ROW)
        A_ptr->graph[row++][col] = sqlite3_column_double(valStmt, 0);
    col++;
    }
A_ptr->labels=1;
return A_ptr;
}


/* Return a matrix of clin values in the order found in V */
matrix * five3db_fill_KH(struct sqlite3 * conn, matrix * V, char *clinField)
{
matrix * A_ptr = init_matrix(1, V->cols);
safef(A_ptr->rowLabels[0],MAX_LABEL, "knownVal");

char query[256];
safef(query, sizeof(query), "SELECT id FROM features WHERE name=\'%s\'", clinField);
struct sqlite3_stmt * stmt = NULL;
sqlite3_prepare(conn, query, sizeof(query), &stmt, NULL);
sqlite3_step(stmt);
int featureId = sqlite3_column_int(stmt, 0);

int i;
//save the values to a matrix
for(i = 0; i < V->cols; i++)
    {
    safef(A_ptr->colLabels[i], MAX_LABEL, "%s", V->colLabels[i]);

    safef(query, sizeof(query),"SELECT id FROM samples WHERE name=\'%s\' limit 1", V->colLabels[i]);
	sqlite3_prepare(conn, query, sizeof(query), &stmt, NULL);
	sqlite3_step(stmt);	
    int sampleId = sqlite3_column_int(stmt, 0);
    safef(query, sizeof(query), "SELECT val FROM clinicalData WHERE sample_id=%d AND feature_id=%d", sampleId, featureId);
	sqlite3_prepare(conn, query, sizeof(query), &stmt, NULL);
	sqlite3_step(stmt);
    double qresult = sqlite3_column_double(stmt, 0);
    if(qresult == SQLITE_NULL)
        A_ptr->graph[0][i] = NULL_FLAG;
    else
        A_ptr->graph[0][i] = qresult;
    }
A_ptr->labels=1;
return A_ptr;
}

/*This is a complementary function to the f_fill fucntion, that will assign more memory when needed*/
/*This version of this grow_matrix fucntion fills the extra space with 1's in the first row, and zeroes in every other row. This keeps the columns adding to one, for a 'vote table'*/
void grow_matrix(matrix *A_ptr, int new_rows, int new_cols){
	int i = 0, j = 0;

 	if(new_cols){
        	for(i = 0; i < new_cols; i++){
            		A_ptr->graph[i] = realloc(A_ptr->graph[i], new_cols*sizeof(double));
        	}
        	for(i = 0; i < A_ptr->rows; i++){
           		A_ptr->graph[i][new_cols] = NULL_FLAG;
        	}
        	A_ptr->cols = new_cols;

    	}

    	if(new_rows){
        	A_ptr->graph = realloc(A_ptr->graph, new_rows*sizeof(double));
        	for(i = A_ptr->rows; i < A_ptr->rows; i++){
           		A_ptr->graph[i] = malloc(A_ptr->cols*sizeof(double));
        	}
        	for(i = A_ptr->rows; i < new_rows; i++){
            		for(j = 0; j < A_ptr->cols; j++){
                		A_ptr->graph[i][j] = NULL_FLAG;
            		}
          	}
        	A_ptr->rows = new_rows;
    	}
}

/*Return an array of the values in the specified row or col index*/
double * rowToArray(matrix *A_ptr, int row){
	double * result = malloc(A_ptr->cols * sizeof(double));
	int i;
	for(i = 0; i < A_ptr->cols; i++)
		result[i] = A_ptr->graph[row][i];
	return result;
}

double * rowToArray_NArm(matrix *  A_ptr, int row, double null_val){
	int i, count = 0;
	for(i = 0; i < A_ptr->cols; i++){
		if(A_ptr->graph[row][i] != null_val)
			count++;
	}
   	double *rowVals = malloc(count * sizeof(double));
	int curr = 0;
    for(i = 0; i < A_ptr->cols; i++){
		if(A_ptr->graph[row][i] != null_val)
        	rowVals[curr++] = A_ptr->graph[row][i];
	}
    return rowVals;
}

double * colToArray(matrix *A_ptr, int col){
	double * result = malloc(A_ptr->rows * sizeof(double));
	int i;
	for(i = 0; i < A_ptr->rows; i++)
		result[i] = A_ptr->graph[i][col];
	return result;
}

double * colToArray_NArm(matrix * A_ptr, int col, double null_val){
    int i, count = 0;
    for(i = 0; i < A_ptr->rows; i++){
        if(A_ptr->graph[i][col] != null_val)
            count++;
    }
    double * colVals = malloc(count * sizeof(double));
	int curr=0;
    for(i = 0; i < A_ptr->rows; i++){
        if(A_ptr->graph[i][col] != null_val)
            colVals[curr++] = A_ptr->graph[i][col];
    }
    return colVals;
}

int rowCountNonNA(matrix * A_ptr, int row, double null_val){
	int count = 0, i;
	for(i = 0 ; i < A_ptr->cols; i++)
		{
		if(A_ptr->graph[row][i] != null_val)
			count++;
		}
	return count;
}

int colCountNonNA(matrix * A_ptr, int col, double null_val){
    int count = 0, i;
    for(i = 0 ; i < A_ptr->rows; i++)
        {
        if(A_ptr->graph[i][col] != null_val)
            count++;
        }
    return count;
}


void fprint_matrix(FILE *fp, matrix *A_ptr){
	int i, a;
		if(A_ptr->labels){
			fprintf(fp, "labels");	
			for(i = 0; i < A_ptr->cols; i++){
				fprintf(fp, "\t%s", A_ptr->colLabels[i]);
			}
			fprintf(fp,"\n");
		}
    	for(i = 0; i < A_ptr->rows; i++){
			if(A_ptr->labels){
				fprintf(fp, "%s\t", A_ptr->rowLabels[i]);
			}
        	for(a = 0; a < A_ptr->cols; a++){
				if(A_ptr->graph[i][a] != NULL_FLAG)
            		fprintf(fp, "%.10f", A_ptr->graph[i][a]);
				else
					fprintf(fp, "NULL");
				if(a != (A_ptr->cols -1))
					fprintf(fp, "\t");
        	}
        	fprintf(fp,"\n");
    	}
}

void fprint_discreteMatrix(FILE *fp, matrix *A_ptr){
    int i, a;
        if(A_ptr->labels){
            fprintf(fp, "labels");
            for(i = 0; i < A_ptr->cols; i++){
                fprintf(fp, "\t%s", A_ptr->colLabels[i]);
            }
            fprintf(fp,"\n");
        }
        for(i = 0; i < A_ptr->rows; i++){
            if(A_ptr->labels){
                fprintf(fp, "%s\t", A_ptr->rowLabels[i]);
            }
            for(a = 0; a < A_ptr->cols; a++){
                if(A_ptr->graph[i][a] != NULL_FLAG)
                    fprintf(fp, "%d", (int)A_ptr->graph[i][a]);
                else
                    fprintf(fp, "NULL");
                if(a != (A_ptr->cols -1))
                    fprintf(fp, "\t");
            }
            fprintf(fp,"\n");
        }
}


matrix * transpose(matrix *A_ptr){
    	matrix * result = init_matrix(A_ptr->cols, A_ptr->rows);
    	int i = 0, j = 0;
    	for(i = 0; i < result->rows; i++){
			if(A_ptr->labels)
            	safef(result->rowLabels[i], MAX_LABEL, "%s", A_ptr->colLabels[i]);
        	for(j = 0; j < result->cols; j++){
           		result->graph[i][j] = A_ptr->graph[j][i];
            	if(i == 0 && A_ptr->labels)
                	safef(result->colLabels[j], MAX_LABEL, "%s", A_ptr->rowLabels[j]);
        	}
    	}
		if(A_ptr->labels)
			result->labels=1;
    	return result;
}

void free_matrix(matrix *A_ptr){
	int i =0;
    for(i = 0; i < A_ptr->rows; i++){
        freeMem(A_ptr->rowLabels[i]);
    }
    freeMem(A_ptr->rowLabels);
    for(i = 0; i < A_ptr->cols; i++){
        freeMem(A_ptr->colLabels[i]);
    }
    freeMem(A_ptr->colLabels);
	for(i = 0; i < A_ptr->rows; i++){
		freeMem(A_ptr->graph[i]);
	}
	freeMem(A_ptr->graph);
	freeMem(A_ptr);
}

int lineCount(FILE * fp){
    char c;
    int wc = 0;
    while ((c = getc(fp)) != EOF) {
        if (c == '\n') wc++;
    }
    rewind(fp);
    return wc;
}

int colCount(FILE * fp){
    char c;
    int wc = 0;
    while ((c = getc(fp)) != EOF) {
        if(c == '\t' || c == '\n') wc++;
        if (c == '\n') break;
    }
    //wc--;
    rewind(fp);
    return wc;
}

int bioInt_lineCount(struct sqlConnection * conn, char *tableName){
	char query[256];
	safef(query, sizeof(query), "SELECT COUNT( DISTINCT feature_id) FROM %s", tableName);
	int rows = sqlQuickNum(conn, query);
	return rows;
}

int bioInt_colCount(struct sqlConnection * conn, char *tableName){
    char query[256];
	safef(query, sizeof(query), "SELECT COUNT( DISTINCT sample_id) FROM %s", tableName);
	int cols = sqlQuickNum(conn, query);
	return cols;
}

int five3db_lineCount(struct sqlite3 * conn, char *tableName){
	char query[256];
	safef(query, sizeof(query), "SELECT id FROM data_files WHERE name LIKE \'%s\'", tableName);	
	struct sqlite3_stmt *stmt= NULL;
	sqlite3_prepare(conn, query, -1, &stmt, 0);
	sqlite3_step(stmt);
	int data_file_id = sqlite3_column_int(stmt, 0);
    safef(query, sizeof(query), "SELECT COUNT( DISTINCT feature_id) FROM data WHERE data_file_id=%d", 
		data_file_id);
	sqlite3_prepare(conn, query, -1, &stmt, 0);
	sqlite3_step(stmt);
    int rows = sqlite3_column_int(stmt, 0);
    return rows;
}

int five3db_colCount(struct sqlite3 * conn, char *tableName){
    char query[256];
    safef(query, sizeof(query), "SELECT id FROM data_files WHERE name LIKE \'%s\'", tableName);
    struct sqlite3_stmt *stmt= NULL;
    sqlite3_prepare(conn, query, -1, &stmt, 0);
	sqlite3_step(stmt);
    int data_file_id = sqlite3_column_int(stmt, 0);
    safef(query, sizeof(query), "SELECT COUNT( DISTINCT sample_id ) FROM data WHERE data_file_id=%d",
        data_file_id);
    sqlite3_prepare(conn, query, -1, &stmt, 0);
    sqlite3_step(stmt);
    int cols = sqlite3_column_int(stmt, 0);
    return cols;
}


matrix * matricesToSVM(matrix * data, matrix * metadata)
/*Consolidates a pair of data matrices into SVM format*/
{
matrix * dataCpy = copy_matrix(data);
tagLabelsWithMetadata(dataCpy, metadata);
matrix * result = transpose(dataCpy);
free_matrix(dataCpy);
return result;
}

matrix * SVMtoMetadataMatrix(char * filename)
/*Reads the metadata out of an SVMlight format and returns as a matrix*/
{
FILE * fp = fopen(filename, "r");
if(fp == NULL)
	errAbort("ERROR: Couldn't open file %s for reading into metadata matrix\n", filename);

//get row and col count
int rowCount = lineCount(fp) - 1; //take one off for the commented out line of feature names
char c;
int wc = 0;
while ((c = getc(fp)) != EOF) {
	if(c == ' ' || c == '\n') wc++;
 	if (c == '\n') break;
}
rewind(fp);
int colCount = wc - 1; //remove one for the label column

//set up somewher to save resutls
matrix * result = init_matrix(1,rowCount);  
safef(result->rowLabels[0],MAX_LABEL, "knownVal");

//iterate over file, saving the class to the matrix and the label to the matrix label space
int currCol = 0;
char * line=malloc((MAX_LABEL+colCount*MAX_TOKEN) * sizeof(char));
while(fgets(line, (MAX_LABEL+colCount*MAX_TOKEN), fp))
	{
	if(!startsWith("#", line))
		{
		char * classString = cloneFirstWordByDelimiter(line, ' ');
		int class = atoi(classString);
		if(class < 0)
			result->graph[0][currCol] = 0;
		else if(class > 0)
			result->graph[0][currCol] = 1;
		else
			result->graph[0][currCol] = NULL_FLAG;
		
		char *c = strpbrk(line, "#");
		if(c == NULL)
			errAbort("Error: No label on line %d of file %s.", currCol, filename);
		c++;
	    result->colLabels[currCol] = cloneFirstWordByDelimiter(c, '\n');
		currCol++;
		}
	}
fclose(fp);
result->labels=1;
return result;
}

matrix * WEKAtoMetadataMatrix(char * filename)
{
FILE * fp = fopen(filename, "r");
if(fp == NULL)
    errAbort("ERROR: Couldn't open file %s for reading into metadata matrix\n", filename);

//read the file to figure out how many data rows there are
char *line = malloc(MAX_LABEL* sizeof(char));
char c;
long int dataStart = SEEK_SET;
while((c = fgetc(fp)) && c != EOF)
	{
	if(c == '@')
		{
		fgets(line, MAX_LABEL, fp);
		if(sameString(line, "DATA\n"))
			{
			dataStart = ftell(fp);
			break;
			}
		}
	}
if(c == EOF)
	errAbort("ERROR: Trying to read metadata from %s but couldn't read format.\n", filename);
int rowCount = 0;
while ((c = fgetc(fp)) != EOF)
	{
    if(c == '\n')
		rowCount++;
	}

fseek(fp,dataStart, SEEK_SET);//set the pointer back to the start of the data

int wc = 0;
while ((c = getc(fp)) != EOF) {
    if(c == ',' || c == '\n') wc++;
    if (c == '\n') break;
}
int colCount = wc - 1; //remove one for the label column

//set up somewhere to save resutls
matrix * result = init_matrix(1,rowCount);
safef(result->rowLabels[0],MAX_LABEL, "knownVal");

//iterate over file, saving the class to the matrix and the label to the matrix label space
int currCol = 0;
line = realloc(line,(MAX_LABEL+colCount*MAX_TOKEN) * sizeof(char));
if(line == NULL)
	errAbort("Couldn't assign more memory to save line of data");
fseek(fp,dataStart, SEEK_SET);//set the pointer back to the start of the data
fgets(line, sizeof(line), fp);
char * ptr;
while(fgets(line, (MAX_LABEL+colCount*MAX_TOKEN), fp))
    {
	//convert the label into a class in the results matrix
	ptr = strrchr(line, ',');
	ptr++;
	char * classString  = cloneFirstWordByDelimiter(ptr, '%');
	
    if(sameString(classString, "\'subgroup1\'"))
        result->graph[0][currCol] = 0;
    else if(sameString(classString, "\'subgroup2\'"))
        result->graph[0][currCol] = 1;
    else if(sameString(classString,"?"))
        result->graph[0][currCol] = NULL_FLAG;
	else
		errAbort("Unsupported class type found: %s",classString); 

	//copy the label at the end of the line
   	ptr = strchr(line, '%');
    if(ptr == NULL)
        errAbort("Error: No label on line %d of file %s.", currCol, filename);
    ptr++;
    result->colLabels[currCol] = cloneFirstWordByDelimiter(ptr, '\n');
    currCol++;
    }
fclose(fp);
result->labels=1;
return result;
}
