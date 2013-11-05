#!/bin/bash

###############################################################################
BEGIN_TIME=`date +%s`

###########################USER DEFINE BEGIN###################################
#METHOD: 0=rise 1=sma 2=mma 3=oma
METHOD_ID=2
PARAM_BEGIN=( 5 -20 -4 1 )
PARAM_END=( 60 -5 15 8 )
PARAM_SETP=( 1 1 1 1 )
PARAM_DIVIDER=( 1 100 100 1 )
###########################USER DEFINE END#####################################

PROGRAM="/cygdrive/f/StockAnalyser/build/StockAnalyser/Debug/Simulate.exe"
LOG_FILE="stat_oma.csv"
PARAM_STR=

if [ -e "${LOG_FILE}" ] 
then 
	rm -rf ${LOG_FILE} 
fi

PARAM_LOOP=()
PARAM_REAL=()
for ((PARAM_LOOP[0]=PARAM_BEGIN[0]; PARAM_LOOP[0] <= PARAM_END[0]; PARAM_LOOP[0]+=PARAM_SETP[0])); do
	PARAM_REAL[0]=$(echo "scale=2; ${PARAM_LOOP[0]}/${PARAM_DIVIDER[0]}" | bc)
for ((PARAM_LOOP[1]=PARAM_BEGIN[1]; PARAM_LOOP[1] <= PARAM_END[1]; PARAM_LOOP[1]+=PARAM_SETP[1])); do
	PARAM_REAL[1]=$(echo "scale=2; ${PARAM_LOOP[1]}/${PARAM_DIVIDER[1]}" | bc)
for ((PARAM_LOOP[2]=PARAM_BEGIN[2]; PARAM_LOOP[2] <= PARAM_END[2]; PARAM_LOOP[2]+=PARAM_SETP[2])); do
	PARAM_REAL[2]=$(echo "scale=2; ${PARAM_LOOP[2]}/${PARAM_DIVIDER[2]}" | bc)
for ((PARAM_LOOP[3]=PARAM_BEGIN[3]; PARAM_LOOP[3] <= PARAM_END[3]; PARAM_LOOP[3]+=PARAM_SETP[3])); do
	PARAM_REAL[3]=$(echo "scale=2; ${PARAM_LOOP[3]}/${PARAM_DIVIDER[3]}" | bc)
	PARAM_STR="(${PARAM_REAL[0]},${PARAM_REAL[1]},${PARAM_REAL[2]},${PARAM_REAL[3]})"
	echo "PARAM=$PARAM_STR"
	$PROGRAM -m oma -c 1000000 -p $PARAM_STR >> $LOG_FILE
done
done
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

