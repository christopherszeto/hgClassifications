include ../../../common.mk

ifeq ($(HOSTNAME),tcga1)
	CLUSTERROOT=/inside/grotto/users
else
	CLUSTERROOT=/data/home/common/topmodel_tools
endif

all::
	rsync Rwrapper.sh ${CLUSTERROOT}/Rwrapper.sh
	rsync Lasso_train.R ${CLUSTERROOT}/Lasso_train.R
	rsync Lasso_classify.R ${CLUSTERROOT}/Lasso_classify.R
	rsync Lasso_extractCoefficients.R ${CLUSTERROOT}/Lasso_extractCoefficients.R
	rsync ElasticNet_train.R ${CLUSTERROOT}/ElasticNet_train.R
	rsync ElasticNet_classify.R ${CLUSTERROOT}/ElasticNet_classify.R
	rsync ElasticNet_extractCoefficients.R ${CLUSTERROOT}/ElasticNet_extractCoefficients.R
	rsync RidgeRegression_train.R ${CLUSTERROOT}/RidgeRegression_train.R
	rsync RidgeRegression_classify.R ${CLUSTERROOT}/RidgeRegression_classify.R
	rsync RidgeRegression_extractCoefficients.R ${CLUSTERROOT}/RidgeRegression_extractCoefficients.R

clean:: 
	rm -rf ${CLUSTERROOT}/Rwrapper.sh \
		${CLUSTERROOT}/Lasso_train.R \
		${CLUSTERROOT}/Lasso_classify.R \
		${CLUSTERROOT}/Lasso_extractCoefficients.R \
		${CLUSTERROOT}/ElasticNet_train.R \
		${CLUSTERROOT}/ElasticNet_classify.R \
		${CLUSTERROOT}/ElasticNet_extractCoefficients.R \
		${CLUSTERROOT}/RidgeRegression_train.R \
		${CLUSTERROOT}/RidgeRegression_classify.R \
		${CLUSTERROOT}/RidgeRegression_extractCoefficients.R
