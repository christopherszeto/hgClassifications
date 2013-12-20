options("scipen"=100, "digits"=16)
args=(commandArgs(TRUE))
library(glmnet)
args
rawDat<-read.table(args[1], as.is=T, quote="", sep="\t", header=T, row.names=1, na.string="NULL")
rawLabels<-read.table(args[2], as.is=T, quote="", sep="\t", header=T, row.names=1, na.string="NULL")
nonNullLabels<-rawLabels[,which(!is.na(rawLabels))]
commonSamples<-intersect(colnames(rawDat), colnames(nonNullLabels))
dat<-t(rawDat[,commonSamples])
labels<-t(nonNullLabels[,commonSamples])
errorCodes<-tryCatch(
	if( (length(which(labels == 0)) > 1) && (length(which(labels == 1)) > 1) ){
	lassoModel<-cv.glmnet(dat, labels, family="binomial", alpha=1, nlambda=100)
	save(lassoModel, file=args[3])
    }, error = function(e) print("Error detected"))
q("no")
