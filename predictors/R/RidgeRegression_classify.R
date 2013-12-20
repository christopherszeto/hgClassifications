options("scipen"=100, "digits"=16)
args=(commandArgs(TRUE))
library(glmnet)
args
rawDat<-read.table(args[1], as.is=T, quote="",sep="\t", header=T, row.names=1, na.string="NULL", check.names=F)
dat<-t(rawDat)
#convert nulls to zeroes
dat[is.na(dat)]<-0
if(file.exists(args[2])){
	load(args[2])
	predictions<-predict(lassoModel, newx=dat, type="link", s="lambda.min")
	write.table(predictions,file=args[3], sep="\t", quote=F, row.names=T, col.names="labels\tPrediction")
}
q("no")
