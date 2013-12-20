#!/bin/bash -e

###############
# cszeto 2012 #
########################################################
# bash script that can be run by parasol. The main problem
# was parasol's handling of command-line arguments (one-per-space).
# This allows kent-src style [arg1=val1,arg2=val2,etc] to be parsed
# and run as R expects.
#########################################################

rscript=$1
args=$2
outfile=$3

parsedArgs=`echo $args | perl -pe 's/([^,]+)=//g' | perl -pe 's/,/ /g'`
#echo "R CMD BATCH \"--args $parsedArgs\" $rscript $outfile"
"/data/home/cszeto/lib/R-2.15.1/bin/R" CMD BATCH --no-save --no-restore "--args $parsedArgs" $rscript "$outfile"
