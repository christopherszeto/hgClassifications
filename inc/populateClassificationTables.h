#include "common.h"
#include "linefile.h"
#include "hash.h"
#include "jksql.h"
#include "hPrint.h"
#include "hdb.h"
#include "ra.h"
#include "classificationTables.h"

void createJobsTable(struct sqlConnection * conn);
void createTasksTable(struct sqlConnection * conn);
void createClassifiersTable(struct sqlConnection * conn);
void createClassifierResultsTable(struct sqlConnection * conn);
void createFeatureSelectionsTable(struct sqlConnection * conn);
void createTransformationsTable(struct sqlConnection * conn);
void createSubgroupsTable(struct sqlConnection * conn);
void createSamplesInSubgroupsTable(struct sqlConnection * conn);
void setupTable(struct sqlConnection *conn, char *tableName, int dropTables);
void saveTaskRa(struct sqlConnection * conn, struct hash * raHash);
void saveSubgroupingRa(struct sqlConnection * conn, struct hash * raHash);
void saveClassifierRa(struct sqlConnection * conn, struct hash * raHash);
void saveFeatureSelectionRa(struct sqlConnection * conn, struct hash * raHash);
void saveTransformationRa(struct sqlConnection * conn, struct hash * raHash);
void saveJobRa(struct sqlConnection * conn, struct hash * raHash);
void populateClassificationTables(char * profile, char *db, char *raFile, int dropTables);
