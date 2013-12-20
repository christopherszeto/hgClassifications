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
#include "../inc/sqlite3.h"

void usage()
{
errAbort(	"Usage: ./MLpermute [path to tab file] [number of shuffle examples]\n");
}

int main(int argc, char *argv[])
/* Process command line. */
{
if(argc != 3) 
	usage();

FILE * fp = fopen(argv[1], "r");
if(!fp)
{
	errAbort("Couldn't open file %s", argv[1]);
}
matrix * template = f_fill_matrix(fp,1);
fclose(fp);
matrix * result = init_matrix(template->rows, atoi(argv[2]));
copy_matrix_labels(result,template, 1,1);
result->labels=1;
int j = 0; 
srand(time(NULL));
for(j = 0; j < result->cols; j++){
	int randSample = rand()%template->cols;
	struct slInt * list = list_indices(template->rows);
	struct slInt * curr, *shuffledList = shuffle_indices(list);
	curr = shuffledList;
	int i = 0; 
	while(curr){
		result->graph[i++][j] = template->graph[curr->val][randSample];
		curr = curr->next;
	}
	safef(result->colLabels[j], MAX_LABEL, "PermutedSample%d", (j+1));
	slFreeList(&list);
	slFreeList(&shuffledList);
}
fprint_matrix(stdout,result);

free_matrix(template);
free_matrix(result);
return 0;
}
