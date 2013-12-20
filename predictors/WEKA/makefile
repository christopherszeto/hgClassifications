include ../../../common.mk

ifeq ($(HOSTNAME),tcga1)
	CLUSTERROOT=/inside/grotto/users
else
	CLUSTERROOT=/data/home/common/topmodel_tools
endif

all::
	rsync WEKAwrapper.sh ${CLUSTERROOT}/WEKAwrapper.sh

clean:: 
	rm -rf ${CLUSTERROOT}/WEKAwrapper.sh
