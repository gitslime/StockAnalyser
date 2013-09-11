#!/bin/bash

###############################################################################
BEGIN_TIME=`date +%s`

###########################USER DEFINE BEGIN###################################
#METHOD: 0=rise 1=sma 2=mma
METHOD_ID=2
PARAM_BEGIN=( 5 30 )
PARAM_END=( 20 70 )
PARAM_SETP=( 1 2 )
PARAM_DIVIDER=( 1 1 )
###########################USER DEFINE END#####################################

PROGRAM="/cygdrive/f/StockAnalyser/build/StockAnalyser/Debug/Simulate.exe"
LOG_FILE="mma.csv"
PARAM_STR=

if [ -e "${LOG_FILE}" ] 
then 
	rm -rf ${LOG_FILE} 
fi

PARAM_LOOP=()
for ((PARAM_LOOP[0]=PARAM_BEGIN[0]; PARAM_LOOP[0] <= PARAM_END[0]; PARAM_LOOP[0]+=PARAM_SETP[0])); do
for ((PARAM_LOOP[1]=PARAM_BEGIN[1]; PARAM_LOOP[1] <= PARAM_END[1]; PARAM_LOOP[1]+=PARAM_SETP[1])); do
	PARAM_STR="(${PARAM_LOOP[0]},${PARAM_LOOP[1]})"
	echo "PARAM=$PARAM_STR"
	$PROGRAM -m mma -b 20000101 -p $PARAM_STR >> $LOG_FILE
done
done

###############################################################################
#USED TIME
END_TIME=`date +%s`
USED_SECONDS=$((END_TIME-BEGIN_TIME))
USED_DAYS=$(($USED_SECONDS/86400))
USED_SECONDS=$(($USED_SECONDS-USED_DAYS*86400))
USED_HOURS=$(($USED_SECONDS/3600))
USED_SECONDS=$(($USED_SECONDS-USED_HOURS*3600))
USED_MINUTES=$(($USED_SECONDS/60))
USED_SECONDS=$(($USED_SECONDS-USED_MINUTES*60))
echo
echo "Used time: $USED_DAYS day $USED_HOURS hour $USED_MINUTES minute $USED_SECONDS second"

