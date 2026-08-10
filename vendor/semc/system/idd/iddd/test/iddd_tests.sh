export PATH=/data/busybox:$PATH

TESTROOT=`readlink -f .`

if [ "$2" = "gcov" ]; then
    lcov --directory ../ --zerocounters
    rm -f ../iddd
    cd ../
    make GCOV='-fprofile-arcs -ftest-coverage -DRUNNING_WITH_GCOV'
    cd $TESTROOT
fi

echo "EXECUTING TESTS FROM $TESTROOT"

RUNNING_ON_PC=1
if [ "$1" != "PC" -a "$2" != "PC" ]; then
    RUNNING_ON_PC=0
fi

# Cleanup iddd test files.
cleanup_iddd()
{
    rm $IDD_HOME/idd_co 2> /dev/null
    rm $IDD_HOME/idd_cl 2> /dev/null
    rm $IDD_HOME/iddd.pid 2> /dev/null
    rm -r $IDD_HOME/test 2> /dev/null
    rm -r $IDD_HOME/input 2> /dev/null
    rm -r $IDD_HOME/output 2> /dev/null
    rm -r $IDD_HOME/private 2> /dev/null
}


#Remove iddd as a service while testing
if [ "$1" != "PC" -a "$2" != "PC" ]; then
    stop iddd
fi

cleanup_iddd

#Restart iddd as a service when test finished
#TODO: If test is aborted by user (CTRL-C), iddd will not be restarted as service
restart_service()
{
    if [ "$RUNNING_ON_PC" -ne "1" ]; then
        PID=`/data/busybox/busybox pidof iddd`
        kill -9 $PID
        cleanup_iddd
        start iddd
    else
        cleanup_iddd
    fi
}

echo $1 | egrep "tc|TC"
TARGET_SUITE=$?
if [ "$TARGET_SUITE" -ne "0" ]; then
     TARGET_SUITE=tc-*.sh
else
     TARGET_SUITE=$1
fi

# If you only want to execute a specific test, modify the expression below.
for fname in $TARGET_SUITE
do
    chmod 755 $fname
    sh ./$fname "$@"
    if [ "$?" -ne "0" ]; then
        echo "ABORTING TESTS"
        restart_service
        exit 1
    fi
    echo
done

if [ "$2" = "gcov" ]; then
    echo "Generating code coverage report"
    cd $TESTROOT
    lcov --directory ../ --capture --output-file app.info
    lcov --remove app.info "*.pb-c.*" -o app2.info
    rm -rf gcov
    mkdir gcov
    genhtml -o gcov app2.info
    echo "Opening firefox to show the results"
    firefox ./gcov/index.html &
fi

echo "ALL TESTS PASSED"
restart_service "$@"
