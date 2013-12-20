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
#include "../inc/MLsignificance.h"

void usage()
{
errAbort(	"Usage: ./MLrunBackgroundAnalysis [profile] [db]\n");
}

int main(int argc, char *argv[])
/* Process command line. */
{
if(argc != 3)
    usage();

char * profile = argv[1];
char * db = argv[2];

struct sqlConnection *conn = hAllocConnProfile(profile, db);
printf("Grabbing top ids\n");
struct slInt * topModelIds = listTopModelIds(conn);
printf("Done. Creating background dirs...\n");
createBackgroundDirs(topModelIds);
printf("Done. Writing minimal config files..\n");
writeBackgroundConfigFiles(conn, topModelIds);
printf("Done. Running models...\n");
printBackgroundModelResults(conn, topModelIds);
printf("Done!\n");
hFreeConn(&conn);

return 0;
}
