#!/bin/bash

BASE_DIR="/playpen2/rac/games/tetrinet"
DATA_DIR=$BASE_DIR/"data_tetrinet"
RESULTS_DIR=$BASE_DIR/"results_tetrinet"

if  test -z "$1" 
then 
	KTEST_DIR="$DATA_DIR/last-run/ktest"
else
  KTEST_DIR=$2
fi

SCRIPTS_ROOT=$BASE_DIR
SCRIPT=$SCRIPTS_ROOT"/run-nuklear.sh "

TMP_SCRIPT="/tmp/$RANDOM.sh"
cp $SCRIPT $TMP_SCRIPT
echo "$SCRIPT copied to $TMP_SCRIPT "
chmod +x $TMP_SCRIPT
SCRIPT=$TMP_SCRIPT


###### CONFIG ######
KTEST_SUFFIX="ktest"
MAX_PARTIAL_FIELD_TYPE=4
#COUNT=30
#minR=1
#maxR=15
#stepR=1

COUNT=5
minR=2
maxR=10
stepR=2

SERVER_ADDRESS="localhost"
PLAYER_NAME="p1"
####################
RUN_PREFIX=$(date +%F.%T)

OUT_DIR=$RESULTS_DIR/$RUN_PREFIX

mkdir -p $OUT_DIR

for rate in `echo 1; seq $minR $stepR $maxR`
do 
	for ptype in `seq 1 $MAX_PARTIAL_FIELD_TYPE`
	do
		for i in `seq 1 $COUNT`
		do
			zpad_ptype=`printf "%02d" $ptype`
			zpad_rate=`printf "%02d" $rate`
			zpad_i=`printf "%02d" $i`

			DESC="tetrinet_"$zpad_i"_type-"$ptype"_rate-"$zpad_rate
			KTEST_FILE=$KTEST_DIR/$DESC"."$KTEST_SUFFIX

			OPTS=" -autostart -partialtype $ptype -partialrate $rate"
			OPTS+=" $PLAYER_NAME $SERVER_ADDRESS "
			echo "Starting verification of $KTEST_FILE"
			#echo "bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR \"$OPTS\" &"
			bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR "$OPTS" &
			#bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR "$OPTS"
			sleep 1
		done

		echo "Waiting for $COUNT jobs to finish (rate=$rate, ptype=$ptype) "
		wait
  done
done


