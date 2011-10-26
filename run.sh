#!/bin/bash
#=============================================================================
# run.sh: batch script to generate ktest files from game client and to verify
# ktest files with nuklear
#=============================================================================

#=============================================================================
# host specific path
#=============================================================================
if [[ $HOSTNAME == "kudzoo" ]] 
then
  BASE_DIR="/home/rac/research/gsec/games/tetrinet"
elif [[ $HOSTNAME == "brawn.cs.unc.edu" ]]
then
  BASE_DIR="/playpen1/rac/gsec/games/tetrinet"
elif [[ $HOSTNAME == "albus" ]]
then
  BASE_DIR="/home/rac/research/gsec/games/tetris/tetrinet"
else
  echo "Set correct configuration dirs in $0"
  exit
fi

if  test -z "$1" 
then 
  echo "usage: $0 [game|test|all]"
  exit
else
  MODE=$1 
fi

#=============================================================================
# configuration parameters
#=============================================================================
SERVER_ADDRESS="localhost"
PLAYER_NAME="p1"
KTEST_SUFFIX="ktest"
RECENT_LINK="last-run"

COUNT=5

maxRound=100
#ptypeValues=`seq 1 6`
ptypeValues=`seq 1`
#rateValues=`echo 1; seq 2 2 10`
rateValues=`echo 1`

#=============================================================================
# game client and server paths
#=============================================================================
SERVER_BIN="tetrinet-server"
SERVER_OPT=" "
SERVER_COMMAND="$BASE_DIR/$SERVER_BIN $SERVER_OPT "

CLIENT_BIN="tetrinet-ktest"
CLIENT_OPT=" "
CLIENT_COMMAND="$BASE_DIR/$CLIENT_BIN $CLIENT_OPT "

#=============================================================================
# output paths
#=============================================================================
DATA_DIR=$BASE_DIR/"data_tetrinet"
RESULTS_DIR=$BASE_DIR/"results_tetrinet"

RUN_PREFIX=$(date +%F.%T)
LOG_DIR=$DATA_DIR/$RUN_PREFIX/log
KTEST_DIR=$DATA_DIR/$RUN_PREFIX/ktest
OUT_DIR=$RESULTS_DIR/$RUN_PREFIX


#=============================================================================
# script path
#=============================================================================
SCRIPTS_ROOT=$BASE_DIR
SCRIPT=$SCRIPTS_ROOT"/run-nuklear.sh "
TMP_SCRIPT="/tmp/$RANDOM.sh"
cp $SCRIPT $TMP_SCRIPT
chmod +x $TMP_SCRIPT
SCRIPT=$TMP_SCRIPT

#=============================================================================
# generate ktest files from game client
#=============================================================================
if [[ $MODE == "game" || $MODE == "all" ]]
then
  echo "running game client"

  mkdir -p $LOG_DIR $KTEST_DIR 

  rm $DATA_DIR/$RECENT_LINK
  ln -sf $DATA_DIR/$RUN_PREFIX $DATA_DIR/$RECENT_LINK

  for ptype in $ptypeValues
  do
    for rate in $rateValues
    do 
      for i in `seq 1 $COUNT`
      do
        zpad_ptype=`printf "%02d" $ptype`
        zpad_rate=`printf "%02d" $rate`
        zpad_i=`printf "%02d" $i`
        DESC="tetrinet_"$zpad_i"_type-"$ptype"_rate-"$zpad_rate
        KTEST_FILE=$KTEST_DIR/$DESC"."$KTEST_SUFFIX

        while ! [ -e $KTEST_FILE ] 
        do
          while ! [[ `pgrep $SERVER_BIN` ]] 
          do
            echo "starting server in background..."
            echo "$SERVER_COMMAND &> /dev/null &"
            exec $SERVER_COMMAND &> /dev/null &
            sleep 1
          done

          echo "creating $KTEST_FILE"
          OPTS=" -log $LOG_DIR/$DESC.log -ktest $KTEST_FILE "
          OPTS+=" -random -seed $i -maxround $maxRound"
          OPTS+=" -autostart -partialtype $ptype -partialrate $rate"
          OPTS+=" $PLAYER_NAME $SERVER_ADDRESS "

          echo "executing $CLIENT_COMMAND $OPTS"
          $CLIENT_COMMAND $OPTS

          echo "exiting. now killing server process. "
          pkill $SERVER_BIN
          sleep 1
        done
      done
    done
  done
else
  echo "not running game client"
fi

#=============================================================================
# verify ktest files
#=============================================================================
if [[ $MODE == "test" || $MODE == "all" ]]
then
  echo "running test"

  if [[ $MODE != "all" ]] ; then
    KTEST_DIR=$DATA_DIR/$RECENT_LINK/ktest
  fi

  mkdir -p $OUT_DIR

  rm $RESULTS_DIR/$RECENT_LINK
  ln -sf $OUT_DIR $RESULTS_DIR/$RECENT_LINK

  for ptype in $ptypeValues
  do
    for rate in $rateValues
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

        echo "verifying $KTEST_FILE"
        bash $SCRIPT release $KTEST_FILE $DESC $OUT_DIR "$OPTS" &
        sleep 1
      done

      echo "waiting for $COUNT jobs to finish"
      wait
    done
  done
else
  echo "not running test"
fi
#=============================================================================
#=============================================================================


