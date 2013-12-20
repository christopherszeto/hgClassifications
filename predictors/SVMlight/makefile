include ../../../common.mk

ifeq ($(HOSTNAME),tcga1)
	CLUSTERROOT=/inside/grotto/users
else
	CLUSTERROOT=/data/home/common/topmodel_tools
endif

all::
	rsync SVMlightWrapper.sh ${CLUSTERROOT}/SVMlightWrapper.sh

clean:: 
	rm -rf ${CLUSTERROOT}/SVMlightWrapper.sh
