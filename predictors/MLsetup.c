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

void usage()
{
errAbort(	"Usage: ./MLsetup [configFile]\n");
}

int main(int argc, char *argv[])
/* Process command line. */
{
if(argc != 2) 
	usage();

char * configFile=argv[1];

struct hash * config = raReadSingle(configFile);

if(!dataExists(config))
	splitData(config);
createModelDirs(config);
writeClusterJobs(config);

freeHash(&config);
return 0;
}
