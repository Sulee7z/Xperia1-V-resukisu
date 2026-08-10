. ./test-lib.sh
set_module rca-partition-space TC-0017-data

# This test manipulates iddd rca home folder.
# Therefor the device shall be reflashed with erase user data after this test.
# Thats why this test is not part of the regular iddd unit test suite, files matching tc-*.sh.
#
# IDD partition only take effect in phone, so they are not meant to run on a PC
if [ "$1" != "PC" -a "$2" != "PC" ]; then

# three sample idd report:
# idd_small < 1  kb
# idd_medium  10 kb
# idd_large > 50 kb

# Set limit of output folder size in MB.
# See OUTPUT_DIR_MAX_SIZE_DEFAULT in config.h for value and documentation.
OUTPUT_MAX_SIZE=7.5

RCA_PLUGIN_DIR=/rca/plugins
RCA_NONCRASH_DIR=$RCA_PLUGIN_DIR/rca-noncrash
RCA_NONCRASH_CURRENT_REPORT=rcanonc
RCA_NONCRASH_REPORT=rcanoncp

copy_sample_report()
{
    if [ $1 = "medium" ]; then
        cp $TESTROOT/TC-0014-data/idd_medium $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1
    elif [ $1 = "large" ]; then
        cp $TESTROOT/TC-0014-data/idd_large $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1
    elif [ $1 = "all" ]; then
        cp $TESTROOT/TC-0014-data/idd_small $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1
        cp $TESTROOT/TC-0014-data/idd_medium $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_2
        cp $TESTROOT/TC-0014-data/idd_large $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_3
    fi
}

ls_file_size()
{
    if [ $1 = "medium" ]; then
        FILE_SIZE=$(ls -l $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 | awk '{print $5}')
    elif [ $1 = "large" ]; then
        FILE_SIZE=$(ls -l $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 | awk '{print $5}')
    elif [ $1 = "all" ]; then
        SMALL_FILE_SIZE=$(ls -l $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 | awk '{print $5}')
        MEDIUM_FILE_SIZE=$(ls -l $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_2 | awk '{print $5}')
        LARGE_FILE_SIZE=$(ls -l $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_3 | awk '{print $5}')
    fi
}

# For each case, we copy data close to the limit of output folder, then iddd will remove oldest
# reports for flush of a new report.

############################################################
cp $TESTROOT/TC-0017-data/fill_rca_partition.config $IDD_HOME/input/config
rm -rf $RCA_PLUGIN_DIR
start_test 1 "Remove oldest plugin report (10kB reports)" "fill only with idd_medium"

copy_sample_report "medium"
ls_file_size "medium"

LOOP=`awk "BEGIN \
{ \
  print(int(($OUTPUT_MAX_SIZE * 1024 * 1024) / $FILE_SIZE)) \
}"`

COUNT=2
while [ $COUNT -le $LOOP ]
do
  cp $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_$COUNT
  let "COUNT += 1"
done

# Force flush of a new plugin report.
pump_events 4/$((128*1024))/0 com.sonyericsson.idd.probe.android.devicemonitor.fill_rca_partition
# Wait for the compactor
sleep 1
#  The first plugin report should be removed because of almost full partition (~37 is deleted).
if [ -e $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 ]; then
  fail_test "$RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 has not been removed"
fi

# The new report, rcanoncp_673, shall be there.
NEW_REPORT=${RCA_NONCRASH_REPORT}_$COUNT
if [ -e $RCA_NONCRASH_DIR/$NEW_REPORT ]; then
    pass_test
else
    fail_test "$RCA_NONCRASH_DIR/$NEW_REPORT plugin report hasn't been generated"
fi

############################################################
cp $TESTROOT/TC-0017-data/fill_rca_partition.config $IDD_HOME/input/config
rm -rf $RCA_PLUGIN_DIR
start_test 2 "Remove oldest plugin report (filled with various size reports)" "equal amount of with idd_small, idd_medium and idd_large"

copy_sample_report "all"
ls_file_size "all"

LOOP=`awk "BEGIN \
{ \
  print(int(($OUTPUT_MAX_SIZE * 1024 * 1024) / \
            ($SMALL_FILE_SIZE + $MEDIUM_FILE_SIZE + $LARGE_FILE_SIZE))) \
}"`

COUNT=2
while [ $COUNT -le $LOOP ]
do
    cp $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_$(($COUNT * 3 - 2))
    cp $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_2 $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_$(($COUNT * 3 - 1))
    cp $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_3 $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_$(($COUNT * 3))
    let "COUNT += 1"
done

# Force flush of a new plugin report
pump_events 3/$((128*1024))/0 com.sonyericsson.idd.probe.android.devicemonitor.fill_rca_partition
# Wait for the compactor
sleep 1
# The first plugin report should be removed because of almost full partition (~6 is deleted).
#
if [ -e $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 ]; then
    fail_test "$RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 has not been removed"
fi

# The new compressed report, rcanoncp_229), shall be there.
NEW_REPORT=${RCA_NONCRASH_REPORT}_$(((($COUNT - 1) * 3) + 1))
if [ -e $RCA_NONCRASH_DIR/$NEW_REPORT ]; then
    pass_test
else
    fail_test "$RCA_NONCRASH_DIR/$NEW_REPORT plugin report hasn't been generated"
fi

############################################################
# Check whether iddd could handle a large event when rca partition already fully utilized
# idd_small:idd_medium:idd_large
# 3:1:1
rm -rf $RCA_PLUGIN_DIR
cp $TESTROOT/TC-0017-data/fill_rca_partition.config $IDD_HOME/input/config
start_test 3 "Incoming 2MB event" "2MB event on fully utilized rca partition"

copy_sample_report "all"

ls_file_size "all"

LOOP=`awk "BEGIN \
{ \
  print(int(($OUTPUT_MAX_SIZE * 1024 * 1024) / (($SMALL_FILE_SIZE * 3) + $MEDIUM_FILE_SIZE + $LARGE_FILE_SIZE))) \
}"`

COUNT=2
while [ $COUNT -le $LOOP ]
do
    cp $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_$(($COUNT * 5 - 4))
    cp $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_$(($COUNT * 5 - 3))
    cp $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_$(($COUNT * 5 - 2))
    cp $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_2 $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_$(($COUNT * 5 - 1))
    cp $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_3 $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_$(($COUNT * 5))
    let "COUNT += 1"
done

add_event $TESTROOT/TC-0014-data/2MB_text_file
# Wait for the compactor
sleep 1
# The first plugin report should be removed because of almost full partition. Nr deleted ~115
if [ -e $RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 ]; then
    fail_test "$RCA_NONCRASH_DIR/${RCA_NONCRASH_REPORT}_1 has not been removed"
fi

# The new report, rcanonc_381, shall be there and contain the large event.
NEW_REPORT=${RCA_NONCRASH_CURRENT_REPORT}_$(((($COUNT - 1) * 5) + 1))
compare_ref_file $RCA_NONCRASH_DIR $NEW_REPORT

############################################################

fi
end_tests
