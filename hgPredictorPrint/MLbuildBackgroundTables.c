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
#include "../inc/MLsignificance.h"

void usage()
{
errAbort(	"Usage: ./MLbuildBackgrounds [profile] [db]\n"
			"\tMakes tables of 100 samples of 40,000 'genes' of randomly selected values from other tables\n");
}

int main(int argc, char *argv[])
/* Process command line. */
{
if(argc != 3) 
	usage();

char * profile = argv[1];
char * db = argv[2];

if(!buildBackgroundTables(profile, db))
	errAbort("ERROR: Couldn't build table. Exiting..\n");

return 0;
}
