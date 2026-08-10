. ./test-lib.sh
set_module performance

# We don't care the performance in PC, so they are
# not meant to run on a PC
if [ "$1" != "PC" -a "$2" != "PC" ]; then

#generate ramdom number
performance_random()
{
    MIN=$1
    MAX=$(($2 - $1))
    SECOND=$(date +%s)
    NUM=$(($SECOND % $MAX + $MIN))
    echo $NUM
}

performance_command_get()
{
    if [ "$1" = "EV" ]; then
        PREFIX="EV-start: iddtest: "
        POSTFIX="EV-end: iddtest: "
    elif [ "$1" = "CS" ]; then
        PREFIX="CS-start: iddtest: "
        POSTFIX="CS-end: iddtest: "
    elif [ "$1" = "SD" ]; then
        PREFIX="SD-start: iddtest: "
        POSTFIX="SD-end: iddtest: "
    elif [ "$1" = "CI" ]; then
        PREFIX="CI-start: iddtest: "
        POSTFIX="CI-end: iddtest: "
    elif [ "$1" = "FL" ]; then
        PREFIX="FL-start: test: "
        POSTFIX="FL-end: test: "
    else
        echo "unexpected parameter input!"
    fi
}

#performanceTest.log is similar like this:
#CS-start: iddtest: 138175451111
#CS-end: iddtest: 138175451219
#CS-start: iddtest: ......
#......
#we will scan each line from file and calculate the difference of time one by one and accumulate

# in log file, the unit of time is us
performance_daemon_calc()
{
    stop_iddd
    performance_command_get $1
    cp /mnt/sdcard/performanceTest.log performanceTest.log

    # this marks we are processing $PREFIX or $POSTFIX, 1 means $PREFIX, 0 means $POSTFIX
    FLAG=1
    TOTALTIME=0
    TOTALCOUNT=0
    while read LINE
    do
        PRELENGTH=${#LINE}
        if [ $FLAG -eq 1 ]; then
            PRETIME=${LINE#$PREFIX}
            POSLENGTH=${#PRETIME}
            if [ $POSLENGTH -eq $PRELENGTH ]; then
                continue
            fi
            FLAG=0
        elif [ $FLAG -eq 0 ]; then
            POSTIME=${LINE#$POSTFIX}
            POSLENGTH=${#POSTIME}
            if [ $POSLENGTH -lt $PRELENGTH ]; then
                let "TOTALTIME += POSTIME - PRETIME"
                let "TOTALCOUNT += 1"
                #echo $TOTALCOUNT
            else
                fail_test $1 "post-time lost in log"
            fi
            FLAG=1
        fi
    done < performanceTest.log

    if [ $TOTALCOUNT -ne $2 ]; then
        fail_test $1 "incorrect count in log"
    fi

    let "AVERAGE = TOTALTIME / $2"
    echo "$1:$TOTALTIME microseconds in $2 times, the average time is $AVERAGE microseconds"
}

#we define the unit time for each command as follows:
#EV: 500  us
#CS: 150  us
#SD: 100  us
#CI: 100  us
#FL: 100  ms
compare_target()
{
    if [ "$1" = "EV" ]; then
        let "TARGET = $3 * 500"
        if [ $TOTALTIME -le $TARGET ]; then
            pass_test $2
        else
            fail_test $2
        fi
    elif [ "$1" = "CS" ]; then
        let "TARGET = $3 * 150"
        if [ $TOTALTIME -le $TARGET ]; then
            pass_test $2
        else
            fail_test $2
        fi
    elif [ "$1" = "SD" ]; then
        let "TARGET = $3 * 100"
        if [ $TOTALTIME -le $TARGET ]; then
            pass_test $2
        else
            fail_test $2
        fi
    elif [ "$1" = "CI" ]; then
        let "TARGET = $3 * 100"
        if [ $TOTALTIME -le $TARGET ]; then
            pass_test $2
        else
            fail_test $2
        fi
    elif [ "$1" = "FL" ]; then
        let "TARGET = $3 * 100 * 1000"
        if [ $TOTALTIME -le $TARGET ]; then
            pass_test $2
        else
            fail_test $2
        fi
    else
        echo "unexpected parameter input!"
    fi
}

############################################################
CI_TIME=100
start_test_stopped 1 "incCounter" "incCounter for $CI_TIME times"
rm /mnt/sdcard/performanceTest.log > /dev/null
start_iddd

int=1
while [ $int -le $CI_TIME ]
do
    inc_counter iddtest performance 1
    int=`expr $int + 1`
done
performance_daemon_calc CI $CI_TIME
compare_target CI "incCounter" $CI_TIME

############################################################
EV_TIME=500
start_test_stopped 2 "addEvent" "addEvent for $EV_TIME times with 2k data"
rm /mnt/sdcard/performanceTest.log > /dev/null
start_iddd

pump_events $EV_TIME/2000/0
performance_daemon_calc EV $EV_TIME
compare_target EV "addEvent" $EV_TIME

############################################################
SD_TIME=500
start_test_stopped 3  "setStaticData" "setStaticData for $SD_TIME times"
rm /mnt/sdcard/performanceTest.log > /dev/null
start_iddd

pump_sdata $SD_TIME/2000/0
performance_daemon_calc SD $SD_TIME
compare_target SD "setStaticData" $SD_TIME

############################################################
CS_TIME=500
start_test_stopped 4 "setCounter" "setCounter for $CS_TIME times"
rm /mnt/sdcard/performanceTest.log > /dev/null
start_iddd

pump_counters $CS_TIME/2000/0
performance_daemon_calc CS $CS_TIME
compare_target CS "setCounter" $CS_TIME

############################################################
FL_TIME=100
start_test_stopped 5 "flush" "flush for $FL_TIME times"
rm /mnt/sdcard/performanceTest.log > /dev/null
start_iddd

send_flush 0
int=1
while [ $int -lt $FL_TIME ]
do
    #generate random payload between 10 and 90
    PAYLOAD=$(performance_random 1000 9000)
    pump_events 1/$PAYLOAD/0
    send_flush 0
    int=`expr $int + 1`
done
performance_daemon_calc FL $FL_TIME
compare_target FL "flush" $FL_TIME

fi
end_tests
