decrypt()
{
    $IDDC --crypto-split $1 > /dev/null
    $IDDC --decrypt $2 crypto_split.data crypto_split.key crypto_split.iv > crypto_split.decoded
    mv crypto_split.decoded $1
    rm -f crypto_split.data crypto_split.key crypto_split.iv
}

say_color ()
{
  echo
}

do_sleep()
{
    if [ "$TESTING_ON_PC" -ne "0" ]; then
#	echo "sleep PC $2"
	sleep $2
    else
#	echo "sleep target $1"
	sleep $1
    fi
    if [ "$VALGRIND" -ne "0" ]; then
        sleep 1
    fi
}

stop_iddd()
{
    cd $TESTROOT

    sleep_if_valgrind 1

    if [ "$VALGRIND" -ne "0" ]; then
        PS=$(ps -A | grep memcheck | head -n 1 | awk '{print $4}')
        killall -SIGTERM -w $PS 2> /dev/null
        if [ "$VALGRIND" -eq "1" -a -e "$VALGRIND_XMLFILE" ]; then
            python check_valgrind.py valgrind.xml
            if [ "$?" -ne "0" ]; then
                exit 1
            fi
            rm -f $VALGRIND_XMLFILE
            rm -f $VALGRIND_LOGFILE
        fi
    else
        killall -SIGTERM -w iddd 2> /dev/null
        do_sleep 1 0.2
    fi
}

stop_logreader()
{
    killall idd-logreader 2> /dev/null
}

start_iddd()
{
    $IDDD -t -C -c NO_ENCRYPTION
    do_sleep 1 0.2
}

start_iddd_encrypted()
{
    $IDDD -t -C -c $1
    do_sleep 2 0.2
}

start_iddd_nontest()
{
    $IDDD -C -c NO_ENCRYPTION
    do_sleep 1 0.2
}

start_iddd_iddddi()
{
    $IDDD -t -C -c NO_ENCRYPTION --enable-di
    do_sleep 1 1
}

restart_iddd()
{
    stop_iddd
    start_iddd
}

start_iddd_non_compressed()
{
    $IDDD -t -Z -C -c NO_ENCRYPTION
    do_sleep 1 0.2
}

start_iddd_non_compressed_encrypted()
{
    $IDDD -t -Z -C -c $1
    do_sleep 1 0.2
}

start_iddd_data_collection_default()
{
    $IDDD -t -c NO_ENCRYPTION
    do_sleep 1 0.2
}

clean_output_dir()
{
    rm $IDD_HOME/output/* 2> /dev/null
}

clean_iddd()
{
    rm -rf $IDD_HOME/private/* 2> /dev/null
}

clean_plugin_output_dir()
{
    rm -rf $IDD_HOME/$PLUGIN_DIR/* 2> /dev/null
}


clean_restart_iddd()
{
    stop_iddd
    clean_iddd
    clean_output_dir
    start_iddd
}

clean_flush_and_get_output()
{
    rm $IDD_HOME/output/* 2> /dev/null
    $IDDC --flush
    do_sleep 1 0.2
    check_output
}


clean_flush_and_get_output_slow()
{
    rm $IDD_HOME/output/* 2> /dev/null
    $IDDC --flush
    do_sleep 5 1
    check_output
}


flush_and_get_output()
{
    $IDDC --flush
    do_sleep 1 0.2
    check_output
}

send_flush()
{
    SECONDS=${1:-1}
    $IDDC --flush
    do_sleep $SECONDS 0.2
}

logreader()
{
    $LOGREADER -r $1 -t $2 main
}

check_output()
{
    if [ $# -eq 0 ]
    then
        cd $IDD_HOME/output/
    else
        cd $IDD_HOME/$1
    fi

    # The zlib implementation on the PC will set the XFL field
    # to indicate the compression level, but the zlib implementation
    # in android doesn't for some reason. It's not mandatory, and the file
    # can be decompressed anyway. Anyway. For this reason, the checksums
    # will differ between the platforms, and a simple solution is to patch
    # the gzip-files and clear the XFL field.
    if [ $# -eq 0 ]; then
        $IDDC --patch-output-files * > /dev/null
    fi
    sleep_if_valgrind 1
    sha1sum * > $IDD_TEST/actual 2> /dev/null
}

check_dir()
{
    cd $IDD_HOME/$1
    ls > $IDD_TEST/actual
}

check_config()
{
    cd $IDD_HOME/private/
    sha1sum conf_* > $IDD_TEST/actual 2> /dev/null
}

check_input()
{
    cd $IDD_HOME/input/
    sha1sum * > $IDD_TEST/actual 2> /dev/null
}

check_events()
{
    cd $IDD_HOME/private/
    sha1sum events > $IDD_TEST/actual 2> /dev/null
}

check_file()
{
    cd $1
    sha1sum $2 > $IDD_TEST/actual 2> /dev/null
}

send_load_config()
{
    $IDDC --load-config
    # Since IDDD only works in one process, a synchronous command following
    # asynchronous command IDDC --load-config can be used to block
    # the test execution until the handling of IDDC --load-config is finished.
    # To avoid that the synchronous command is handled before
    # IDDC --load-config we also insert a sleep.
    do_sleep 1 0.2
    $IDDC --active foo bar > /dev/null
}

add_event()
{
    $IDDC --add-event $1
}

set_counter()
{
    $IDDC --set-counter "$@"
}

inc_counter()
{
    $IDDC --inc-counter "$@"
}

set_sdata()
{
    $IDDC --set-sdata "$@"
}

# For pump_* functions, parameter $1 = cnt/size/rate
# cnt  : Number of events/counters/sdata to send.
# size : Size of each event (not used for the rest)
# rate : Micro seconds to sleep between runs
# e.g. 500/1000/30 will produce around 500kb of events and they will be sent during 15 seconds.
# parameter $2 (optional) = package
# parameter $3 (optional) = name

pump_events()
{
    $IDDC --pump-events $* > /dev/null
    sleep_if_valgrind 1
}

pump_counters()
{
    $IDDC --pump-counters $1 > /dev/null
    sleep_if_valgrind 1
}

pump_sdata()
{
    $IDDC --pump-sdata $1 > /dev/null
    sleep_if_valgrind 1
}

set_imei()
{
    $IDDC --set-imei $1 > /dev/null
}

set_serial_number()
{
    $IDDC --set-serial-number $1 > /dev/null
}

set_group_id()
{
    $IDDC --set-group-id $1 > /dev/null
}

delete_group_id()
{
    $IDDC --delete-group-id $1 > /dev/null
}

replace_group_id()
{
    $IDDC --replace-group-id "$@" > /dev/null
}

set_data_collection()
{
    $IDDC --set-data-collection $1 > /dev/null
}

remove_report()
{
    $IDDC --remove-report $1 > /dev/null
    sleep_if_valgrind 1
}

set_module()
{
    MODULE=$1
    TESTDATA=$TESTROOT/$2
}

send_raw()
{
    echo "$1" | $IDDC --raw
}

send_raw_len()
{
    payload=`seq 1 $1 | sed 's/.*/#/' | tr -d '\n'`
    send_raw "$2 $payload"
}

send_raw_loop()
{
    i=0
    while [ $i -lt $1 ]
    do
        send_raw "$2"
        i=$((i+1))
    done
}

print_test_name()
{
    TEST_NR=$1
    TEST_NAME=$2
    TEST_DESCRIPTION=$3
    echo -e "Starting \033[1mTC-$MODULE-$TEST_NR\033[0m: $TEST_NAME"
}

start_test_stopped()
{
    print_test_name "$@"
    stop_iddd
    stop_logreader
    clean_iddd
    clean_output_dir
}

start_test()
{
    start_test_stopped "$@"
    start_iddd
}

start_nontest()
{
    start_test_stopped "$@"
    start_iddd_nontest
}

fail_test()
{
    echo -e "\033[1m  [FAIL]:\033[0m $@"
    exit 1
}

pass_test()
{
    echo -e "\033[1m  [OK]\033[0m"
}

verify_empty_dir()
{
    ls $DIR_TO_INVESTIGATE/* 2> /dev/null
    if [ "$?" -ne "2" ]; then
        fail_test "Directory $DIR_TO_INVESTIGATE was not empty"
    fi
    pass_test
}

check_reference()
{
    TEST_NR=$1
    diff -w $IDD_TEST/actual $IDD_TEST/reference > /dev/null

    if [ "$?" -ne "0" ]; then
        echo "== REFERENCE =="
        cat $IDD_TEST/reference
        echo "==  ACTUAL   =="
        cat $IDD_TEST/actual
        echo "==============="
        fail_test $2
    fi
    pass_test $2
}

compare()
{
    local result="$($2)"
    if [ "$1" != "$result" ]; then
        echo "cmd <$2>"
        echo "== REFERENCE =="
        echo $1
        echo "==  ACTUAL   =="
        echo $result
        fail_test $TEST_NAME
    fi
    pass_test $TEST_NAME
}

compare_ref()
{
    diff -w $IDD_TEST/actual $IDD_TEST/reference > /dev/null

    if [ "$?" -ne "0" ]; then
        echo "== REFERENCE =="
        cat $IDD_TEST/reference
        echo "==  ACTUAL   =="
        cat $IDD_TEST/actual
        echo "==============="
        fail_test $TEST_NAME
    fi
    pass_test $TEST_NAME
}

compare_ref2()
{
    if [ $# -eq 0 ]
    then
        cd $TESTDATA/ref/$TEST_NR
    else
        cd $TESTDATA/ref/$1
    fi
    sha1sum * > $IDD_TEST/reference 2> /dev/null
    compare_ref
}

compare_ref3()
{
    check_output $1
    if [ $# -eq 1 ]
    then
        cd $TESTDATA/ref/$TEST_NR
    else
        cd $TESTDATA/ref/$2
    fi
    sha1sum * > $IDD_TEST/reference 2> /dev/null
    compare_ref
}

compare_ref_ls()
{
    check_dir $1

    if [ $# -eq 1 ]
    then
        cd $TESTDATA/ref/$TEST_NR
    else
        cd $TESTDATA/ref/$2
    fi
    ls > $IDD_TEST/reference 2> /dev/null
    compare_ref
}

compare_ref_conf()
{
    config_request_checksum_get > $IDD_TEST/actual
    cd $TESTDATA
    echo "config_checksum: `sha1sum $1 | cut -d " " -f 1`" > $IDD_TEST/reference 2> /dev/null
    compare_ref
}

compare_ref_file()
{
    check_file $1 $2

    if [ $# -eq 2 ]
    then
        cd $TESTDATA/ref/$TEST_NR
    else
        cd $TESTDATA/ref/$3
    fi
    sha1sum * > $IDD_TEST/reference 2> /dev/null
    compare_ref
}

compare_ref_events()
{
    if [ $# -eq 0 ]
    then
        cd $TESTDATA/ref/$TEST_NR
    else
        cd $TESTDATA/ref/$1
    fi
    sha1sum * > $IDD_HOME/test/reference 2> /dev/null
    compare_ref
}

ne_sha1()
{
    if [ "$OLD_SHA1" = "$NEW_SHA1" ]; then
        echo "==  SHA1 is same =="
        echo $OLD_SHA1
        echo "==================="
        fail_test $TEST_NAME
    fi
    pass_test $TEST_NAME
}

eq_sha1()
{
    if [ "$OLD_SHA1" != "$NEW_SHA1" ]; then
        echo "==  OLD SHA1  =="
        echo $OLD_SHA1
        echo "==  NEW SHA1  =="
        echo $NEW_SHA1
        echo "==================="
        fail_test $TEST_NAME
    fi
    pass_test $TEST_NAME
}

compare_sha1()
{
    if [ "$1" = "eq" ]; then
        eq_sha1
    else
        ne_sha1
    fi
}

end_tests()
{
    stop_iddd
}

get_config_sha1()
{
    $IDDC --get-config $1 $2 | sha1sum > $IDD_TEST/actual
}

verify_idd_active()
{
    EXPECTED=0
    if [ "$1" -eq "0" ]; then
	EXPECTED=9
    fi

    $IDDC --is-active > /dev/null
    if [ "$?" -ne "$EXPECTED" ]; then
	fail_test $TEST_NAME
    else
	pass_test $TEST_NAME
    fi
}

write_config_request()
{
    $IDDC -q $IDD_HOME/output/config_request > /dev/null
}

config_request_checksum_get()
{
    $IDDC -q | grep config_checksum:
}

#
# sleep the amount of seconds given as parameter if valgrind analysis is used.
# This is needed because execution with valgrind is much slower than normal execution.
# A test case can fail because iddd don't manage to handle the data in proper time.
# usage: sleep_if_valgrind 2
#
sleep_if_valgrind()
{
    if [ "$VALGRIND" -ne "0" ]; then
        do_sleep $1 $1
    fi
}

verify_logreader_running()
{
    ps > $TESTROOT/pstmp.idd
    grep idd-logreader $TESTROOT/pstmp.idd > /dev/null
    if [ "$?" -ne "0" ]; then
        fail_test "log reader is not running"
    fi
    pass_test
}

verify_logreader_not_running()
{
    ps > $TESTROOT/pstmp.idd
    grep idd-logreader $TESTROOT/pstmp.idd > /dev/null
    if [ "$?" -eq "0" ]; then
        fail_test "log reader is running"
    fi
    pass_test
}

### INITIALIZATION GOES HERE ###
TESTS_INITIALIZED=${TESTS_INITIALIZED:-0}

if [ "$TESTS_INITIALIZED" -ne "1" ]; then
VALGRIND=0
TESTROOT=`readlink -f .`

if [ "$1" = "PC" -o "$2" = "PC" ]; then
    TESTING_ON_PC=1
    IDD_HOME=~/idd_test/
    IDD_TEST=$IDD_HOME/test
    IDDD_BIN=`readlink -f ../iddd`
    IDDD="$IDDD_BIN -b $IDD_HOME"
    IDDC_BIN=`readlink -f ../../iddc/iddc`
    IDDC="$IDDC_BIN -s $IDD_HOME/"
    LOGREADER_BIN=`readlink -f ../../logreader/idd-logreader`
    LOGREADER="$LOGREADER_BIN"
    if [ "$2" = "valgrind" -o "$3" = "valgrind" ]; then
        rm -f $VALGRIND_LOGFILE
        if [ "$3" = "gen-supp" -o "$4" = "gen-supp" ]; then
            VALGRIND=2
            VALGRIND_LOGFILE=valgrind.log
            IDDD="valgrind --tool=memcheck --leak-check=full --show-reachable=yes --suppressions=valgrind_idd_supressions.supp --gen-suppressions=all --verbose --num-callers=50 --log-file=$VALGRIND_LOGFILE $IDDD"
        else
            VALGRIND=1
            VALGRIND_XMLFILE=valgrind.xml
            VALGRIND_LOGFILE=valgrind.log
            rm -f $VALGRIND_XMLFILE
            IDDD="valgrind --tool=memcheck --leak-check=full --show-reachable=yes --suppressions=valgrind_idd_supressions.supp --xml=yes --xml-file=$VALGRIND_XMLFILE --num-callers=50 --verbose --log-file=$VALGRIND_LOGFILE $IDDD"
        fi
    fi
    NC=nc
else
    cd /data/busybox
    chmod 4775 busybox
    ./busybox --install
    IDD_HOME=/data/idd_run/
    IDD_TEST=$IDD_HOME/test
    IDDD_BIN=/system/bin/iddd
    IDDD="$IDDD_BIN -b $IDD_HOME"
    IDDC_BIN=/system/bin/iddc
    IDDC="$IDDC_BIN -s $IDD_HOME/"
    LOGREADER_BIN=/system/bin/idd-logreader
    LOGREADER="$LOGREADER_BIN -s $IDD_HOME/"
    PATH=/data/busybox:$PATH
    NC=/data/busybox/nc
fi

echo -e "\033[1mRunning tests from : $0\033[0m $@"
#echo "  IDD_HOME = $IDD_HOME"
#echo "  iddd = $IDDD"
#echo "  iddc = $IDDC"
mkdir $IDD_HOME 2> /dev/null
mkdir $IDD_TEST 2> /dev/null
rm $IDD_HOME/output/* 2> /dev/null
rm $IDD_HOME/private/* 2> /dev/null
rm $IDD_HOME/test/* 2> /dev/null
rm $IDD_HOME/* 2> /dev/null

chmod 755 $IDDD_BIN
chmod 755 $IDDC_BIN

TESTS_INITIALIZED=1
INT_MAX=2147483647 # platform dependent
PLUGIN_DIR=private/plugins
fi
