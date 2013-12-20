#!/bin/bash -e

#################
## cszeto 2011 ##
#############################################################################
# Small script to handle SVMlight command line args properly with 
# parasol-style input
#############################################################################

if [  -d "/inside/grotto" ]; then
    CLUSTERROOT=/inside/grotto/users
else
    CLUSTERROOT=/data/home/common/topmodel_tools
fi

if [ -z "$1" ]
then
	echo "Incorrect usage!"
	echo "Usage:"
	echo "SVMlightWrapper [SVMlight command] [SVMlight arguments]"
fi

if [ "$1" == "svm_learn" ]
then
	if [[ -z "$2" || -z "$3" || -z "$4" ]]
	then
		echo "Incorrect usage!"
    	echo "Usage:"
    	echo "SVMlightWrapper svm_learn [SVMlight arguments] [data] [model]"
	else
		parsedArgs=`echo $2 | perl -pe 's/([^,]+)=/ -$1 /g' | perl -pe 's/,/ /g'`
		${CLUSTERROOT}/SVM-light_V6.02/svm_learn $parsedArgs $3 $4
		head -n 1 $3 >> $4
	fi
elif [ "$1" == "svm_classify" ]
then
	if [[ -z "$2" || -z "$3" || -z "$4" || -z "$5" ]]
	then
		echo "Incorrect usage!"
		echo "Usage:"
		echo "SVMlightWrapper svm_classify [data] [model] [results] [log]"
	else
		if [[ -f "$2" && -f $3 ]]
		then
			${CLUSTERROOT}/SVM-light_V6.02/svm_classify $2 $3 $4 > $5
		else
			echo "Missing data file or model file"
		fi
	fi
else
	echo "Incorrect usage!"
	echo "SVMlight commands are svm_learn and svm_classify. You've provided $1"
fi
