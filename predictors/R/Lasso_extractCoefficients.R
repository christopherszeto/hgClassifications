options("scipen"=100, "digits"=16)
args=(commandArgs(TRUE))
library(glmnet)
args
if(file.exists(args[1])){
	load(args[1])
	coeffs<-as.matrix(coef(lassoModel, type="coefficients", s="lambda.min"))
	#remove Intercept row
	coeffs<-coeffs[-1,]
	options(scipen=999)
	write.table(as.matrix(coeffs),file=args[2], sep="\t", quote=F, row.names=T, col.names="labels\tcoefficients")
}
q("no")
