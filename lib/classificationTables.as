table jobs
"All sample predictions and their accuracies"
(
uint id;                    	"Prediction Id (auto_increment)"
uint datasets_id;				"Datasets Id"
uint tasks_id;    				"Task id"
uint classifiers_id;            "Classifiers Id"
uint featureSelections_id;      "FeatureSelections Id"
uint transformations_id;		"Trasnformations Id"
uint subgroups_id;          	"Subgroups Id"
float avgTestingAccuracy;		"Average accuracy in tests"
float avgTestingAccuracyGain;	"Gain of average testing accuracy over majority classifier"
string accuracyType;			"Accuracy Type (e.g. k-fold)"
string features;   				"csv of features used"
string weights;     			"csv of weights of features used"
string modelPath;				"Path on disk to the model generated from this job"
)

table tasks
"All prediction tasks"
(
uint id;              "Prediction task Id (auto_increment)"
string name;          "Prediction task name"
)

table classifiers
"All classifiers and their parameters"
(
uint id;            "Classifier Id (auto_increment)"
string name;        "Classifier method"
string parameters;  "Classifier parameters"
)

table classifierResults
"All classifier results"
(
uint id;                		"Classifier Results Id"
uint tasks_id;					"Task Id"
uint jobs_id;           		"Job Id"
uint sample_id;         		"Sample"
float trainingAccuracy; 		"Training accuracy"
float testingAccuracy;			"Testing accuracy"
float predictionScore;			"Raw prediction score, intended for binary classes use"
)

table subgroups
"Names and parameters for subgroups"
(
uint id;            "Subgroups Description Id (auto_increment)"
string name;        "Subgrouping method"
string parameters;  "Subgrouping parameters"
uint datasets_id;	"Dataset this subgrouping came from"
)

table samplesInSubgroups
"Sample subgroups used in classifications"
(
uint id;                            "SamplesInSubgroups id (auto_increment)"
uint subgroups_id;      			"Subgroups Id (corrosponds to subgroups Ids)"
uint subgroup;                      "Subgroup assignment"
uint sample_id;                     "Sample Id"
)

table featureSelections
"Names and parameters for feature select methods in machine learning"
(
uint id;			"featureSelections id (auto_increment)"
string name;		"Name of feature selection algorithm"
string parameters;	"Parameters used to run feature selection"
uint datasets_id;	"Dataset this feature selection was applied to"
uint featureCount;	"Number of features after feature selection"
)

table transformations
"Names and parameters for data transformation methods in machine learning"
(
uint id;            "featureSelections id (auto_increment)"
string name;        "Name of feature selection algorithm"
string parameters;  "Parameters used to run feature selection"
)

table requestQueue
"Queue of jobs to run in MLpipeline submitted via the online requester"
(
uint id;					"requestQueue id (auto_increment)"
uint usrConfig_id;			"id of the config associated with this queue item"
string status;				"status of the request (1=unprocessed, 2=processing, 3=processed, 4=error)"
double estimatedRunTime;	"Estimated time in seconds for this run to complete after submission"
string submissionTime;		"Time submission was saved to db"
string runBeginTime;		"Time the job started running"
string runEndTime;			"Time the job finished running"
)

table usrConfig
"Table of fields in user submitted machine-learning configurations"
(
uint id;							"usrConfig id (auto_increment)"
string profile;						"Connection profile to use to get and store data"
string db;							"Database to use to get and store data"
string classifiersRootDir;			"Location on disk to find the classifiers installed"
string clusterJobPrefix;			"Path and name space to store tmp files for classification"
string task;						"Name of the task being predicted"
string inputType;					"Type of input (default bioInt)"
string dataTableName;				"Name of table to use (if inputType=bioInt)"
string clinField;					"Clinical variable to predict (if inputType=bioInt)"
string dataFilepath;				"Path to genomic data file (if inputType=flatfiles)"
string metadataFilepath;			"Path to clinical data file (if inputType=flatfiles)"
string trainingDir;					"Where to store training files"
string validationDir;				"Where to store validation files"
string modelDir;					"Where to store model files"
string crossValidation;				"crossValidation type"
uint folds;							"Number of folds to use (if crossValidation=k-fold or loo)"
uint foldMultiplier;				"Number of times to repeat crossValidation"
string classifier;					"Classifier to use"
string outputType;					"File format to feed classifier"
string parameters;					"Classifier parameters"
string dataDiscretizer;				"Data transformation to use"
string dataDiscretizerParameters;	"Parameters for data transformation"
string clinDiscretizer;				"Subgrouping style to use"
string clinDiscretizerParameters;	"Subgrouping parameters"
string featureSelection;			"Feature selection to use"
string featureSelectionParameters;	"feature select parameters"
string modelPath; 					"Path to the fully trained model"
)

table backgroundModelScores
"Table of values found when applying top models to background dataset"
(
uint id;							"backgroundModelScores id (auto_increment)"
uint jobs_id;						"id of job this score set comes from"
string data_table;					"Background dataset used"
float max;							"Max score from background dataset"
float mean;							"Average score from background dataset"
float median;						"Median score from background dataset"
float sd;							"Standard deviation from scores applied to background dataset"
float min;							"Minimum score when applied to background dataset"
uint estimatedRunTime;				"Time taken to run background data through model"
)
