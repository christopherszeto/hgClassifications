#!/bin/bash -e

###############
# cszeto 2011 #
########################################################
# This is a quick script to wrap the java-based took WEKA in a 
# bash script that can be run by parasol. The main problem
# was parasol's handling of command-line arguments (one-per-space).
# This allows kent-src style [arg1=val1,arg2=val2,etc] to be parsed
# and run as WEKA expects.
#########################################################

args=$1
out=$2

if [  -d "/inside/grotto" ]; then
    CLUSTERROOT=/inside/grotto/users
else
    CLUSTERROOT=/data/home/common/topmodel_tools
fi

if [ -z "$1" ]
then
	echo "Incorrect usage!"
	echo "Usage:"
	echo "WEKAwrapper [weka arguments] [output file]"
	echo "(output file is optional)"
fi

if [ -z "$2" ]
then
	parsedArgs=`echo $args | perl -pe 's/([^,]+)=/ -$1 /g' | perl -pe 's/,/ /g'`
	java -Xmx1024m -cp ${CLUSTERROOT}/weka-3-6-4/weka.jar $parsedArgs -no-cv
else
	parsedArgs=`echo $args | perl -pe 's/([^,]+)=/ -$1 /g' | perl -pe 's/,/ /g'`
	outtmp=$out.$(hostname).$$.tmp
	java -Xmx1024m -cp ${CLUSTERROOT}/weka-3-6-4/weka.jar $parsedArgs -no-cv > $outtmp
	mv $outtmp $out
fi
