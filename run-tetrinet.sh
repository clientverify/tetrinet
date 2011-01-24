#!/bin/bash

GAME_ROOT="/playpen2/rac/games/tetrinet"

SERVER_BIN="tetrinet-server"
SERVER_OPT=" "
SERVER_COMMAND="$GAME_ROOT/$SERVER_BIN $SERVER_OPT "

CLIENT_BIN="tetrinet-ktest"
CLIENT_OPT=" "
CLIENT_COMMAND="$GAME_ROOT/$CLIENT_BIN $CLIENT_OPT "

###### CONFIG ######
SERVER_ADDRESS="localhost"
PLAYER_NAME="p1"

MAX_ROUND=1000
MAX_ROUND=100
MAX_PARTIAL_FIELD_TYPE=4
COUNT=16

minR=2
maxR=16
stepR=2
####################

ROOT_DIR="$GAME_ROOT/data_tetrinet"

RUN_PREFIX=$(date +%F.%T)

mkdir -p $ROOT_DIR/$RUN_PREFIX 

rm $ROOT_DIR/"last-run"
ln -sf $ROOT_DIR/$RUN_PREFIX $ROOT_DIR/"last-run"

if  test -z "$1" 
then 
  COUNT=5
else
  COUNT=$2
fi

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
  LOG_DIR=$ROOT_DIR/$RUN_PREFIX/log
  KTEST_DIR=$ROOT_DIR/$RUN_PREFIX/ktest

  mkdir -p $LOG_DIR $KTEST_DIR 

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
          OPTS+=" -autostart -random -seed $i -maxround $MAX_ROUND -partialtype $ptype -partialrate $rate"
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
