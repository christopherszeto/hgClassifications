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

char * profile = "localDb";
char * db = "bioInt";

void usage()
{
errAbort(	"Usage: ./MLgetCoefficients [config]\n");
}

int main(int argc, char *argv[])
/* Process command line. */
{
if(argc != 2) 
	usage();
char * configFile=argv[1];
struct hash * config = raReadSingle(configFile);
matrix * coefficients = extractCoefficients(config);
if(coefficients)
	{
	fprint_matrix(stdout, coefficients);
	free_matrix(coefficients);
	}
else
	fprintf(stderr, "Model type doesn't support linear coefficients.\n");
return 0;
}
