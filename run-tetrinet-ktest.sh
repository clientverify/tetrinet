#!/bin/bash

SCRIPTS_ROOT="$GSECROOT/../games/tetris/tetrinet"
SCRIPT=$SCRIPTS_ROOT"/run-nuklear.sh "

TMP_SCRIPT="/tmp/$RANDOM.sh"
cp $SCRIPT $TMP_SCRIPT

echo "$SCRIPT copied to $TMP_SCRIPT "

chmod +x $TMP_SCRIPT
SCRIPT=$TMP_SCRIPT


# Loss rate config: $M.$L
minM=0
maxM=5
stepM=1

minL=0
maxL=99
stepL=25

# XXX
minM=3
maxM=4
stepM=2

minL=50
maxL=55
stepL=25
# XXX


# Burst size config
minBurst=2
maxBurst=14
stepBurst=2

minBurst=6
maxBurst=14
stepBurst=4

#minBurst=14
#maxBurst=16
#stepBurst=4


KTEST_SUFFIX="ktest"
REC_SUFFIX="rec"

if  test -z "$1" 
then 
  MODE="uniformloss"
else
  MODE=$1
fi

echo $MODE

HASHTYPE="16bit"
#if  test -z "$2" 
#then 
#  HASHTYPE="16bit"
#else
#  HASHTYPE=$2
#fi


if [ "$MODE" == "uniformloss" ]
then

lossrateconfig=""
for M in `seq $minM $stepM $maxM`
do 
	for L in `seq $minL $stepL $maxL`
	do
		lossrateconfig+=" $M.$L"
	done
done

echo "Lossrates to be processed: $lossrateconfig"

#echo "sleeping for 10 minutes..."
#sleep 600

#PREFIX=$2
#
#if  test -z "$3" 
#then 
#  BASE_DIR="/playpen/gsec/svn/games/loss_data"
#else
#  BASE_DIR=$3
#fi

#for PREFIX in "xpilot-1 xpilot-2 xpilot-3 xpilot-4 xpilot-5"


BASE_DIR="/playpen/gsec/svn/games/loss_data"

KTEST_DIR="$BASE_DIR/ktest"

OUT_DIR="/playpen/gsec/results-$MODE/$PREFIX"

mkdir -p $OUT_DIR

for M in `seq $minM $stepM $maxM`
do 
	for L in `seq $minL $stepL $maxL`
	do
		LOSSRATE=$M.$L
		#for id in `seq 1 1 5`
		# XXX
		for id in `seq 5 1 5`
		do
		# XXX
			PREFIX=xpilot-$id
			BASENAME=$PREFIX"_"$HASHTYPE"_lossrate-"$LOSSRATE
			KTEST_FILE="$KTEST_DIR/$BASENAME.ktest"

			echo "Starting verification of $KTEST_FILE"
			#echo "bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR &"
			bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR &
			sleep 1

		done

		echo "Waiting for jobs to finish (lossrate: $LOSSRATE) "
		wait

  done
done


elif [ "$MODE" == "lazy-xpilot-nohash" ]
then

minM=0
maxM=0
stepM=""

minL=0
maxL=0
stepL=""

SCRIPT=$SCRIPTS_ROOT"/run-lazy-verifier-xpilot-nohash.sh "

BASE_DIR="/playpen/gsec/svn/games/data_xpilot_nohash/"

KTEST_DIR="$BASE_DIR/ktest"

OUT_DIR="/playpen/gsec/results-$MODE/$PREFIX"

mkdir -p $OUT_DIR

for M in `seq $minM $stepM $maxM`
do 
	for L in `seq $minL $stepL $maxL`
	do
		LOSSRATE=$M.$L
		for id in `seq 1 1 5`
		do
			PREFIX=xpilot-$id
			BASENAME=$PREFIX"_"$HASHTYPE"_lossrate-"$LOSSRATE
			KTEST_FILE="$KTEST_DIR/$BASENAME.ktest"

			echo "Starting verification of $KTEST_FILE"
			echo "bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR &"
			bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR 
			sleep 1

		done

		#echo "Waiting for jobs to finish (lossrate: $LOSSRATE) "
		#wait

  done
done

elif [ "$MODE" == "lazy-xpilot-nohash-vanilla" ]
then

minM=0
maxM=0
stepM=""

minL=0
maxL=0
stepL=""

SCRIPT=$SCRIPTS_ROOT"/run-lazy-verifier-xpilot-nohash-vanilla.sh "

BASE_DIR="/playpen/gsec/svn/games/data_xpilot_nohash/"

KTEST_DIR="$BASE_DIR/ktest"

OUT_DIR="/playpen/gsec/results-$MODE/$PREFIX"


mkdir -p $OUT_DIR

for M in `seq $minM $stepM $maxM`
do 
	for L in `seq $minL $stepL $maxL`
	do
		LOSSRATE=$M.$L
		for id in `seq 1 1 5`
		do
			PREFIX=xpilot-$id
			BASENAME=$PREFIX"_"$HASHTYPE"_lossrate-"$LOSSRATE
			KTEST_FILE="$KTEST_DIR/$BASENAME.ktest"

			echo "Starting verification of $KTEST_FILE"
			#echo "bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR &"
			bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR 
			sleep 1

		done

		#echo "Waiting for jobs to finish (lossrate: $LOSSRATE) "
		#wait

  done
done



elif [ "$MODE" == "burstlossone" ]
then

if  test -z "$2" 
then 
  #BASE_DIR="/playpen/gsec/svn/games/data_xpilot/$MODE"
  BASE_DIR="/playpen/gsec/svn/games/burst_data"
else
  BASE_DIR=$2
fi

KTEST_DIR="$BASE_DIR/ktest"
REC_DIR="$BASE_DIR/rec"

#OUT_DIR="/playpen/gsec/results-$MODE.2/$PREFIX"
OUT_DIR="/playpen/gsec/results-burstloss/$PREFIX"
mkdir -p $OUT_DIR

BURSTCNT="1"
BURSTSTART="100"
QUITROUND="250"
LOSSRATE="0.0"
burst=6
id=5

recfile=xpilot-$id.$REC_SUFFIX
REC_BASENAME=`basename $recfile .$REC_SUFFIX`
BASENAME=$REC_BASENAME"_"$HASHTYPE"_burstsz-"$burst"_burstcnt-"$BURSTCNT"_burststart-"$BURSTSTART
KTEST_FILE="$KTEST_DIR/$BASENAME.ktest"

echo "starting verification of $KTEST_FILE"
echo "bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR 250 "
bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR 250 
sleep 3



elif [ "$MODE" == "burstloss" ]
then

burstsizeconfig=""
for burst in `seq $minBurst $stepBurst $maxBurst`
do
		burstsizeconfig+=" $burst"
done

echo "Burst sizes to be processed: $burstsizeconfig"

if  test -z "$2" 
then 
  #BASE_DIR="/playpen/gsec/svn/games/data_xpilot/$MODE"
  #BASE_DIR="/playpen/gsec/svn/games/burst_data"
  #BASE_DIR="/playpen/gsec/svn/games/data_xpilot_2/$MODE"
  BASE_DIR="/playpen/gsec/svn/games/data_xpilot_4/$MODE"
else
  BASE_DIR=$2
fi

KTEST_DIR="$BASE_DIR/ktest"
#REC_DIR="$BASE_DIR/rec"
REC_DIR="/playpen/gsec/svn/games/data_xpilot_4/recordings"

#OUT_DIR="/playpen/gsec/results-$MODE.2/$PREFIX"
OUT_DIR="/playpen/gsec/results-$MODE.4/$PREFIX"
mkdir -p $OUT_DIR

BURSTCNT="1"
BURSTSTART="100"
QUITROUND="250"
LOSSRATE="0.0"

REC_FILES=""
echo "looking for rec files in $REC_DIR/*.$REC_SUFFIX ..."
for i in $REC_DIR/*.$REC_SUFFIX
do
  REC_FILES+=" $i"
done

echo "Found recfiles: $REC_FILES"

for burst in `seq $minBurst $stepBurst $maxBurst`
  do
  for recfile in $REC_FILES
	#for id in `seq 1 1 5`
	#for id in `seq 1 1 2`
	do
		#recfile=xpilot-$id.$REC_SUFFIX
    REC_BASENAME=`basename $recfile .$REC_SUFFIX`
    BASENAME=$REC_BASENAME"_"$HASHTYPE"_burstsz-"$burst"_burstcnt-"$BURSTCNT"_burststart-"$BURSTSTART
    #BASENAME_2="xpilot-2_"$HASHTYPE"_burstsz-"$burst"_burstcnt-"$BURSTCNT"_burststart-"$BURSTSTART
    KTEST_FILE="$KTEST_DIR/$BASENAME.ktest"

    echo "starting verification of $KTEST_FILE"
    #ls $KTEST_FILE
    #bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR 250 &
    bash $SCRIPT release $KTEST_FILE `basename $KTEST_FILE .$KTEST_SUFFIX` $OUT_DIR 250 &
    #bash $SCRIPT release $KTEST_FILE $BASENAME_2 $OUT_DIR 250 &
    sleep 3
  done

	#echo "Waiting for jobs to finish (burst size: $burst) "
	#wait

done

	echo "Waiting for jobs to finish "
	wait
fi

rm $TMP_SCRIPT

#for ktestfile in $KTESTDIR/$PREFIX*$KTEST_SUFFIX
#do
#  echo "startin verification of $ktestfile"
#  #bash $SCRIPT release $ktestfile `basename $ktestfile $KTEST_SUFFIX` $OUTDIR
#  echo $ktestfile 
#  #sleep 2
#done

