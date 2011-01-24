#!/bin/bash

if [[ $HOSTNAME == "kudzoo" ]] 
then
  BASEDIR="/home/rac/research/games/tetris/tetrinet"
  KLEE_DIR="/home/rac/research/gsec/src/klee/"
elif [[ $HOSTNAME == "brawn.cs.unc.edu" ]]
then
  BASEDIR="/playpen2/rac/games/tetrinet"
  KLEE_DIR="/playpen2/rac/gsec/src/klee/"
else
  echo "Set correct configuration dirs in $0"
  exit
fi

KLEEOUT="$BASEDIR/nuklear-results-tetrinet"
BCFILE="$BASEDIR/tetrinet.bc "
BCOPTIONS=" -autostart p1 localhost "

RUNPREFIX=$(date +%F.%T)

if  test -z "$1" 
then 
  DEBUG="release"
else
  DEBUG=$1 
fi

if  test -z "$2" 
then 
  KTESTFILE="$BASEDIR/tetrinet.ktest"
else
  KTESTFILE=$2 
fi

if  test -z "$3" 
then 
  SELECTIONNAME="none"
else
  SELECTIONNAME=$3
fi

if  test -z "$4"
then 
	KLEEOUT="$BASEDIR/nuklear-results-tetrinet"
else
  KLEEOUT=$4 
fi

if  test -z "$5" 
then
	BCOPTIONS=" -autostart p1 localhost "
else
	BCOPTIONS=$5
fi
#if  test -z "$5" 
#then 
#  #BCFILE="/playpen/gsec/svn/bin/ktest_client.bc"
#  BCFILE="/playpen/gsec/svn/games/xpilot/bin/xpilot-ng-x11.bc "
#else
#  BCFILE=$5 
#fi

#---------------------------------------------------------------------

#echo "BCOPTIONS: $BCOPTIONS"

mkdir -p $KLEEOUT/{log,output,run-dir,selected-logs} ;
/bin/ln -sf $KLEEOUT/output/$RUNPREFIX.output $KLEEOUT/last-output ; 
/bin/ln -sf $KLEEOUT/log/$RUNPREFIX.log $KLEEOUT/last-log; 

if [ "$SELECTIONNAME" != "none" ]
then
/bin/ln -sf $KLEEOUT/output/$RUNPREFIX.output $KLEEOUT/selected-logs/$SELECTIONNAME.output ;
/bin/ln -sf $KLEEOUT/log/$RUNPREFIX.log $KLEEOUT/selected-logs/$SELECTIONNAME.log ;
fi

if [ "$DEBUG" == "release" ]
then
  BINDIR=$KLEE_DIR"/Release/bin"
  KLEECOMMAND="time $BINDIR/nuklear --run-in=$KLEEOUT/run-dir/ --output-dir=$KLEEOUT/output/$RUNPREFIX.output "
else
  BINDIR=$KLEE_DIR"/Debug/bin"
  KLEECOMMAND="run --run-in=$KLEEOUT/run-dir/ --output-dir=$KLEEOUT/output/$RUNPREFIX.output "
  KLEECOMMANDLOG="set logging on $KLEEOUT/log/$RUNPREFIX.log"
fi


#KLEECOMMAND+=" -allow-external-sym-calls -check-div-zero=0 -libc=uclibc -print-function-calls -posix-runtime -all-external-warnings -emit-all-errors "
KLEECOMMAND+=" -only-error-output "
#KLEECOMMAND+=" -all-external-warnings "
KLEECOMMAND+=" -no-output "
KLEECOMMAND+=" -check-div-zero=0 -posix-runtime -emit-all-errors "
#KLEECOMMAND+=" -use-cex-cache=0 -use-fast-cex-solver=0 "
#KLEECOMMAND+=" -libc=uclibc "

KLEECOMMAND+=" -no-xwindows "

######### BEGIN DEBUG SWITCHES ########
##KLEECOMMAND+=" -print-function-calls "
#KLEECOMMAND+=" -debug-print-instructions"
#KLEECOMMAND+=" -debug-log-merge "
#KLEECOMMAND+=" -debug-log-state-merge "

#KLEECOMMAND+=" -debug-nuklear-merge "
#KLEECOMMAND+=" -debug-nuklear-remaining-merge "
#KLEECOMMAND+=" -nuklear-dbg=Details" 
#KLEECOMMAND+=" -nuklear-dbg-socket-failure " 
#KLEECOMMAND+=" -nuklear-dbg-socket-success " 

#KLEECOMMAND+=" -nuklear-dbg=BasicInfo " 
#KLEECOMMAND+=" -pc-width-as-arg=false "
#KLEECOMMAND+=" -pc-prefix-width=false "
#KLEECOMMAND+=" -nuklear-print-state-branch=true "
######### END DEBUG SWITCHES ########

#KLEECOMMAND+=" -nuklear-print-messages "

KLEECOMMAND+=" -nuklear-no-symbolic-print=true "
KLEECOMMAND+=" -nuklear-prune-hack=true "
#KLEECOMMAND+=" -nuklear-prune-hack-2=true "

#KLEECOMMAND+=" -nuklear-merge-checkpoint-digest "

KLEECOMMAND+=" -nuklear-merge-digest "

KLEECOMMAND+=" -max-rounds 2000 "
#KLEECOMMAND+=" -queue-size 4 "

#KLEECOMMAND+=" -nuklear-xpilot-mode "
#KLEECOMMAND+=" -xevent-optimization "

KLEECOMMAND+=" -nuklear-no-recv-return-zero "

KLEECOMMAND+=" -nuklear "
KLEECOMMAND+=" -use-nuklear-merge  " 
KLEECOMMAND+=" -nuklear-merge-function=nuklear_merge "
#KLEECOMMAND+=" -nuklear-merge-function=nuklear_checkpoint "
KLEECOMMAND+=" -socket-replay $KTESTFILE "

#KLEECOMMAND+=" -load=/usr/lib64/libSM.so   -load=/usr/lib64/libICE.so  "
#KLEECOMMAND+=" -load=/usr/lib64/libXxf86misc.so  -load=/usr/lib64/libXext.so  -load=/usr/lib64/libX11.so "

KLEECOMMAND+=" $BCFILE $BCOPTIONS "

if [ "$DEBUG" == "release" ]
then
  #KLEECOMMAND+=" 2>&1 | tee $KLEEOUT/log/$RUNPREFIX.log " 
  KLEECOMMAND+=" &> $KLEEOUT/log/$RUNPREFIX.log " 
  echo "$KLEECOMMAND" > $KLEEOUT/log/$RUNPREFIX.command ; bash $KLEEOUT/log/$RUNPREFIX.command
else
  echo "$KLEECOMMANDLOG" > $KLEEOUT/log/$RUNPREFIX.command ; echo "$KLEECOMMAND" >> $KLEEOUT/log/$RUNPREFIX.command ; gdb $BINDIR/nuklear -x $KLEEOUT/log/$RUNPREFIX.command
fi

