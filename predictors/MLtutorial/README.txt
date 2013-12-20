This directory contains some examples of how to use the machine learning
pipeline written by Chris Szeto to run several different machine learning
packages on the same data. Data provided here for example predictions 
is a subset of the NCI60 cancer cell line expression data, and drug response
data for one compound tested on those cell lines.

### Example run of machine learning using SVMlight ###
cd ~/cancer/instinct/predictors
make clean
make
cd ~/cancer/instinct/predictors/MLtutorial
make test1
MLwriteResultsToRa example1.cfg tasks.ra classifiers.ra subgroups.ra jobs.ra

## Example run using WEKA ##
cd ~/cancer/instinct/predictors 
make clean 
make 
cd ~/cancer/instinct/predictors/MLtutorial 
make test2 #NOTE: This will only work on machines with java installed
MLwriteResultsToRa example2.cfg tasks.ra classifiers.ra subgroups.ra jobs.ra 

### How to load results into hgClassifications ###
cd /data/home/cszeto/cancer/instinct/hgClassifications
make clean
make
make populateClassificationTables
./populateClassificationTables bioInt tasks.ra
./populateClassificationTables bioInt classifiers.ra
./populateClassificationTables bioInt subgroups.ra
./populateClassificationTables bioInt jobs.ra


### Description of options in the config file ###
name	GEM_IC50_1 #Required .ra field. Ignored for now. May use this in the future to track which config files generated which results
task	GEM_IC50 response #Name of the prediction task. This will group results from the same task together in hgClassifications
inputType	bioInt #flatfiles
profile	localDb #profile and db to use to find bioInt tables
db	bioInt
outputType	NMFpredictor #SVMlight/WEKA
tableName	collisson2010paradigm_cl #Name of the table in bioInt to grab data from. Only applied if inputType is 'bioInt'
clinField	GEM_IC50 #exact string of the clinical variable being predicted on.
trainingDir	trainingDir #path to a space to save training files.
validationDir	validationDir #path to a space to save validation files
modelDir	modelDir #path to a space to save interim model files, etc.
crossValidation	k-fold #loo
folds	10 #any integer. Ignored if crossValidation is 'loo'
foldMultiplier	5 #number of times you should repeat over the same samples
classifier	NMFpredictor #SVMlight/WEKA //tells the writeResults program  which way to calculate accuracy
parameters	NA #parameters to run classifier with, here you'd specify the classifier from weka classes, or the kernel to use with SVMlight
dataDiscretizer	sign #median/percent/NA
clinDiscretizer	threshold #median/quartile/NA
clinLowThreshold	365 #Everything below this val is classed as 'low' class. Only applied when clinDiscretizer is 'threshold'
clinHiThreshold	365 #Everything above this val is classed as 'high' class. Only applied when clinDiscretizer is 'threshold'
dataPercentThreshold	33 #Splitting percentile. Only applied when dataDiscretizer is 'percent'
clinPercentThreshold	50 #Splitting percentile. Only applied when clinDiscretizer is 'percent'
dataFilepath	/hive/users/cszeto/scratch/data.txt #path to data file. Only applied when inputType is 'flatfiles'
metadataFilepath	/hive/users/cszeto/scratch/metadata.txt #path to metadata file. Only applied when inputType is 'flatfiles'
excludeList	/hive/users/cszeto/scratch/excludeList.txt #path to a csv file stating which samples to leave out. Leave out if unwanted
foldsReport	/hive/users/cszeto/scratch/foldsReport.txt #path where to write a list of which fold each sample lands in. Leave out if unwanted
clusterJobPrefix	job1 #prefix to write onto the cluster jobs files, so you can make them atomically on the cluster
