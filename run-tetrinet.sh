#!/bin/bash

if [[ $HOSTNAME == "kudzoo" ]] 
then
  BASE_DIR="/home/rac/research/games/tetris/tetrinet"
elif [[ $HOSTNAME == "brawn.cs.unc.edu" ]]
then
  BASE_DIR="/playpen2/rac/games/tetrinet"
else
  echo "Set correct configuration dirs in $0"
  exit
fi

SERVER_BIN="tetrinet-server"
SERVER_OPT=" "
SERVER_COMMAND="$BASE_DIR/$SERVER_BIN $SERVER_OPT "

CLIENT_BIN="tetrinet-ktest"
CLIENT_OPT=" "
CLIENT_COMMAND="$BASE_DIR/$CLIENT_BIN $CLIENT_OPT "

###### CONFIG ######
SERVER_ADDRESS="localhost"
PLAYER_NAME="p1"
KTEST_SUFFIX="ktest"
maxRound=100

COUNT=5
ptypeValues=`seq 2 4`
rateValues=`echo 1; seq 2 2 16`

COUNT=2
ptypeValues=`echo 5`
rateValues=`seq 1 16`
rateValues=`echo 1`

COUNT=2
ptypeValues=`seq 1 4`
ptypeValues=`echo 3`
rateValues=`seq 1 16`

####################

DATA_DIR="$BASE_DIR/data_tetrinet"

RUN_PREFIX=$(date +%F.%T)

mkdir -p $DATA_DIR/$RUN_PREFIX 

#rm $DATA_DIR/"last-run"
#ln -sf $DATA_DIR/$RUN_PREFIX $DATA_DIR/"last-run"
rm $DATA_DIR/"last-run-new"
ln -sf $DATA_DIR/$RUN_PREFIX $DATA_DIR/"last-run-new"

#if  test -z "$1" 
#then 
#  COUNT=5
#else
#  COUNT=$2
#fi
#
if  test -z "$2" 
then 
  MODE="ktest"
else
  MODE=$1
fi

# For each configuration, create N runs (random seed 1-N)
# Configurations:
#   1. Full fields
#   2. Partial fields with height only
#   3. Partial fields with column only
#   3. Partial fields with height and column only
#   4. Partial fields with height and rotation only
# Each configuration with full field sent every other round up to every 10th round

if [ "$MODE" == "ktest" ]
then
  LOG_DIR=$DATA_DIR/$RUN_PREFIX/log
  KTEST_DIR=$DATA_DIR/$RUN_PREFIX/ktest

  mkdir -p $LOG_DIR $KTEST_DIR 

	for rate in $rateValues
  do 
		for ptype in $ptypeValues
    do
      for i in `seq 1 $COUNT`
      do
        zpad_ptype=`printf "%02d" $ptype`
        zpad_rate=`printf "%02d" $rate`
        zpad_i=`printf "%02d" $i`
        DESC="tetrinet_"$zpad_i"_type-"$ptype"_rate-"$zpad_rate

        while ! [ -e $KTEST_DIR/$DESC.ktest ] 
        do
          while ! [[ `pgrep $SERVER_BIN` ]] 
          do
            echo "starting server in background..."
            echo "$SERVER_COMMAND &> /dev/null &"
            exec $SERVER_COMMAND &> /dev/null &
            sleep 1
          done

          echo "creating $DESC.ktest"
          OPTS=" -log $LOG_DIR/$DESC.log -ktest $KTEST_DIR/$DESC.ktest "
          OPTS+=" -autostart -random -seed $i -maxround $maxRound -partialtype $ptype -partialrate $rate"
          OPTS+=" $PLAYER_NAME $SERVER_ADDRESS "
          echo "$CLIENT_COMMAND $OPTS"
          $CLIENT_COMMAND $OPTS

          echo "killing server process ..."
          pkill $SERVER_BIN
          sleep 1
        done
      done
    done
  done
fi
