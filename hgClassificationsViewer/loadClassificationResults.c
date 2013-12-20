#include "common.h"
#include "linefile.h"
#include "options.h"
#include "jksql.h"
#include "ra.h"
#include "../inc/classificationTables.h"
#include "../inc/populateClassificationTables.h"
#include "../inc/sqlite_populateClassificationTables.h"
#include "../inc/MLmatrix.h" 
#include "../inc/MLcrossValidation.h" 
#include "../inc/MLreadConfig.h" 
#include "../inc/MLio.h"
#include "../inc/MLextractCoefficients.h"
#include "../inc/MLfilters.h"
#include "../inc/sqlite3.h"

void usage()
/* Explain usage and exit. */
{
errAbort(
  "loadClassificationResults\n"
  "   loadClassificationResults [OPTIONS] db raFile\n"
  "options:\n"
  "   -dropAll   Drop/recreate any table\n"
  "   -profile=localDb   handles alternate db profiles\n"
  "   -five3db   Flag to load to five3's sqlite database format instead of bioInt mysql format\n"
  "\n"
  );
}

//default options
boolean dropTables = FALSE;   // If true, any table that should be dropped/recreated will be
char *profile = "localDb";
boolean five3db = FALSE;
static struct optionSpec options[] = {
    {"dropAll", OPTION_BOOLEAN},
    {"profile", OPTION_STRING},
	{"five3db", OPTION_BOOLEAN},
    {NULL, 0},
};

int main(int argc, char *argv[])
/* Process command line. */
{
optionInit(&argc, argv, options);
if (argc != 3)
    usage();

dropTables = FALSE;
if (optionExists("dropAll"))
    dropTables = TRUE;
if(optionExists("profile"))
    profile = optionVal("profile", profile);
if(optionExists("five3db"))
	five3db = TRUE;

if(five3db)
	sqlite_populateClassificationTables(argv[1], argv[2], dropTables);
else
	populateClassificationTables(profile, argv[1], argv[2], dropTables);
return 0;
}
