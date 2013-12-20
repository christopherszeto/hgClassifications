#include "common.h"
#include "linefile.h"
#include "hash.h"
#include "hPrint.h"
#include "hdb.h"
#include "ra.h"
#include "classificationTables.h"

void sqlite_createJobsTable(struct sqlConnection * conn);
void sqlite_createTasksTable(struct sqlConnection * conn);
void sqlite_createClassifiersTable(struct sqlConnection * conn);
void sqlite_createClassifierResultsTable(struct sqlConnection * conn);
void sqlite_createFeatureSelectionsTable(struct sqlConnection * conn);
void sqlite_createTransformationsTable(struct sqlConnection * conn);
void sqlite_createSubgroupsTable(struct sqlConnection * conn);
void sqlite_createSamplesInSubgroupsTable(struct sqlConnection * conn);
void sqlite_setupTable(struct sqlConnection *conn, char *tableName);
void sqlite_saveTaskRa(struct sqlConnection * conn, struct hash * raHash);
void sqlite_saveSubgroupingRa(struct sqlConnection * conn, struct hash * raHash);
void sqlite_saveClassifierRa(struct sqlConnection * conn, struct hash * raHash);
void sqlite_saveFeatureSelectionRa(struct sqlConnection * conn, struct hash * raHash);
void sqlite_saveTransformationRa(struct sqlConnection * conn, struct hash * raHash);
void sqlite_saveJobRa(struct sqlConnection * conn, struct hash * raHash);
void sqlite_populateClassificationTables(char *db, char *raFile, int dropTables);
