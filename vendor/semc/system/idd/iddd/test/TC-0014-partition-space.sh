. ./test-lib.sh
set_module partition-space TC-0014-data

# This test manipulates iddd normal home folder.
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
OUTPUT_MAX_SIZE=12

# We change the home to "real" idd path, because this case is related to the idd partition.
IDD_HOME=/idd
IDDD="$IDDD_BIN -b $IDD_HOME"
IDDC="$IDDC_BIN -s $IDD_HOME/"

copy_sample_report()
{
    if [ $1 = "medium" ]; then
        cp $TESTROOT/TC-0014-data/idd_medium $IDD_HOME/output/idd_1
    elif [ $1 = "large" ]; then
        cp $TESTROOT/TC-0014-data/idd_large $IDD_HOME/output/idd_1
    elif [ $1 = "all" ]; then
        cp $TESTROOT/TC-0014-data/idd_small $IDD_HOME/output/idd_1
        cp $TESTROOT/TC-0014-data/idd_medium $IDD_HOME/output/idd_2
        cp $TESTROOT/TC-0014-data/idd_large $IDD_HOME/output/idd_3
    fi
}

ls_file_size()
{
    if [ $1 = "medium" ]; then
        FILE_SIZE=$(ls -l $IDD_HOME/output/idd_1 | awk '{print $5}')
    elif [ $1 = "large" ]; then
        FILE_SIZE=$(ls -l $IDD_HOME/output/idd_1 | awk '{print $5}')
    elif [ $1 = "all" ]; then
        SMALL_FILE_SIZE=$(ls -l $IDD_HOME/output/idd_1 | awk '{print $5}')
        MEDIUM_FILE_SIZE=$(ls -l $IDD_HOME/output/idd_2 | awk '{print $5}')
        LARGE_FILE_SIZE=$(ls -l $IDD_HOME/output/idd_3 | awk '{print $5}')
    fi
}

# For each case, we copy data enough in order to exceed the limit of output folder, then iddd will
# remove oldest reports for flush of a new report

############################################################
start_test_stopped 1 "medium report" "fill in output only with idd_medium"

copy_sample_report "medium"

ls_file_size "medium"
let "LOOP = OUTPUT_MAX_SIZE * 1024 * 1024 / FILE_SIZE"
COUNT=2
while [ $COUNT -le $LOOP ]
do
    cp $IDD_HOME/output/idd_1 $IDD_HOME/output/idd_$COUNT
    let "COUNT += 1"
done

# Check whether iddd could flush a new report with 10kB events
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd
pump_events 10/1000/0
send_flush

# The idd_1 should be removed because of full space
# In total ~37 reports are deleted
if [ -e $IDD_HOME/output/idd_1 ]; then
    fail_test "idd_1 has not been removed yet"
fi

# New report should be generated.
if [ -e $IDD_HOME/output/idd_$COUNT ]; then
    pass_test
else
    fail_test "new report hasn't been generated"
fi

############################################################
start_test_stopped 2 "large report" "fill in output only with idd_large"

copy_sample_report "large"

ls_file_size "large"
let "LOOP = OUTPUT_MAX_SIZE * 1024 * 1024 / FILE_SIZE"
COUNT=2
while [ $COUNT -le $LOOP ]
do
    cp $IDD_HOME/output/idd_1 $IDD_HOME/output/idd_$COUNT
    let "COUNT += 1"
done

# Check whether iddd could flush a new report with 10kB events
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd
pump_events 10/1000/0
send_flush

# The idd_1 should be removed because of full space
# In total ~4 reports are deleted
if [ -e $IDD_HOME/output/idd_1 ]; then
    fail_test "idd_1 has not been removed yet"
fi

# A new report should be generated.
if [ -e $IDD_HOME/output/idd_$COUNT ]; then
    pass_test
else
    fail_test "new report hasn't been generated"
fi

############################################################
start_test_stopped 3 "multi report 1" "fill in output with same mount of idd_small, idd_medium and idd_large"

copy_sample_report "all"

ls_file_size "all"
let "LOOP = OUTPUT_MAX_SIZE * 1024 * 1024 / (SMALL_FILE_SIZE + MEDIUM_FILE_SIZE + LARGE_FILE_SIZE)"
COUNT=2
while [ $COUNT -le $LOOP ]
do
    cp $IDD_HOME/output/idd_1 $IDD_HOME/output/idd_$(($COUNT * 3 - 2))
    cp $IDD_HOME/output/idd_2 $IDD_HOME/output/idd_$(($COUNT * 3 - 1))
    cp $IDD_HOME/output/idd_3 $IDD_HOME/output/idd_$(($COUNT * 3))
    let "COUNT += 1"
done

# Check whether iddd could flush a new report with 10kB events
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd
pump_events 10/1000/0
send_flush

# The idd_1 should be removed because of full space
# In total ~12 reports are deleted
if [ -e $IDD_HOME/output/idd_1 ]; then
    fail_test "idd_1 has not been removed yet"
fi

# A new report should be generated.
if [ -e $IDD_HOME/output/idd_$(($COUNT * 3 - 2)) ]; then
    pass_test
else
    fail_test "new report hasn't been generated"
fi

############################################################
# idd_small:idd_medium:idd_large
# 3:1:1
start_test_stopped 4 "multi report 2" "fill in output with many idd_small, few idd_medium and idd_large"

copy_sample_report "all"

ls_file_size "all"
let "LOOP = OUTPUT_MAX_SIZE * 1024 * 1024 / (SMALL_FILE_SIZE * 3 + MEDIUM_FILE_SIZE + LARGE_FILE_SIZE)"
COUNT=2
while [ $COUNT -le $LOOP ]
do
    cp $IDD_HOME/output/idd_1 $IDD_HOME/output/idd_$(($COUNT * 5 - 4))
    cp $IDD_HOME/output/idd_1 $IDD_HOME/output/idd_$(($COUNT * 5 - 3))
    cp $IDD_HOME/output/idd_1 $IDD_HOME/output/idd_$(($COUNT * 5 - 2))
    cp $IDD_HOME/output/idd_2 $IDD_HOME/output/idd_$(($COUNT * 5 - 1))
    cp $IDD_HOME/output/idd_3 $IDD_HOME/output/idd_$(($COUNT * 5))
    let "COUNT += 1"
done

# Check whether iddd could flush a new report with 10kB events
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd
pump_events 10/1000/0
send_flush

# The idd_1 should be removed because of full space
# In total ~20 reports are deleted
if [ -e $IDD_HOME/output/idd_1 ]; then
    fail_test "idd_1 has not been removed yet"
fi

# A new report should be generated.
if [ -e $IDD_HOME/output/idd_$(($COUNT * 5 - 4)) ]; then
    pass_test
else
    fail_test "new report hasn't been generated"
fi

############################################################
# idd_medium:idd_small:idd_large
# 3:1:1
start_test_stopped 5 "multi report 3" "fill in output with many idd_medium, few idd_small and idd_large"

copy_sample_report "all"

ls_file_size "all"
let "LOOP = OUTPUT_MAX_SIZE * 1024 * 1024 / (SMALL_FILE_SIZE + MEDIUM_FILE_SIZE * 3 + LARGE_FILE_SIZE)"
COUNT=2
while [ $COUNT -le $LOOP ]
do
    cp $IDD_HOME/output/idd_1 $IDD_HOME/output/idd_$(($COUNT * 5 - 4))
    cp $IDD_HOME/output/idd_2 $IDD_HOME/output/idd_$(($COUNT * 5 - 3))
    cp $IDD_HOME/output/idd_2 $IDD_HOME/output/idd_$(($COUNT * 5 - 2))
    cp $IDD_HOME/output/idd_2 $IDD_HOME/output/idd_$(($COUNT * 5 - 1))
    cp $IDD_HOME/output/idd_3 $IDD_HOME/output/idd_$(($COUNT * 5))
    let "COUNT += 1"
done

# Check whether iddd could flush a new report with 10kB events
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd
pump_events 10/1000/0
send_flush

# The idd_1 should be removed because of full space
# In total ~15 reports are deleted
if [ -e $IDD_HOME/output/idd_1 ]; then
    fail_test "idd_1 has not been removed yet"
fi

# A new report should be generated.
if [ -e $IDD_HOME/output/idd_$(($COUNT * 5 - 4)) ]; then
    pass_test
else
    fail_test "new report hasn't been generated"
fi

############################################################
# idd_large:idd_medium:idd_small
# 3:1:1
start_test_stopped 6 "multi report 4" "fill in output with many idd_large, few idd_small and idd_medium"

copy_sample_report "all"

ls_file_size "all"
let "LOOP = OUTPUT_MAX_SIZE * 1024 * 1024 / (SMALL_FILE_SIZE + MEDIUM_FILE_SIZE + LARGE_FILE_SIZE * 3)"
COUNT=2
while [ $COUNT -le $LOOP ]
do
    cp $IDD_HOME/output/idd_1 $IDD_HOME/output/idd_$(($COUNT * 5 - 4))
    cp $IDD_HOME/output/idd_2 $IDD_HOME/output/idd_$(($COUNT * 5 - 3))
    cp $IDD_HOME/output/idd_3 $IDD_HOME/output/idd_$(($COUNT * 5 - 2))
    cp $IDD_HOME/output/idd_3 $IDD_HOME/output/idd_$(($COUNT * 5 - 1))
    cp $IDD_HOME/output/idd_3 $IDD_HOME/output/idd_$(($COUNT * 5))
    let "COUNT += 1"
done

# Check whether iddd could flush a new report with 10kB events
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd
pump_events 10/1000/0
send_flush

# The idd_1 should be removed because of full space
# In total ~8 reports are deleted
if [ -e $IDD_HOME/output/idd_1 ]; then
    fail_test "idd_1 has not been removed yet"
fi

# A new report should be generated.
if [ -e $IDD_HOME/output/idd_$(($COUNT * 5 - 4)) ]; then
    pass_test
else
    fail_test "new report hasn't been generated"
fi

############################################################
# idd_small:idd_medium:idd_large
# 3:1:1
start_test_stopped 7 "Incoming 2MB event" "2MB event on fully utilized IDD partition"

copy_sample_report "all"

ls_file_size "all"
let "LOOP = OUTPUT_MAX_SIZE * 1024 * 1024 / (SMALL_FILE_SIZE * 3 + MEDIUM_FILE_SIZE + LARGE_FILE_SIZE)"
COUNT=2
while [ $COUNT -le $LOOP ]
do
    cp $IDD_HOME/output/idd_1 $IDD_HOME/output/idd_$(($COUNT * 5 - 4))
    cp $IDD_HOME/output/idd_1 $IDD_HOME/output/idd_$(($COUNT * 5 - 3))
    cp $IDD_HOME/output/idd_1 $IDD_HOME/output/idd_$(($COUNT * 5 - 2))
    cp $IDD_HOME/output/idd_2 $IDD_HOME/output/idd_$(($COUNT * 5 - 1))
    cp $IDD_HOME/output/idd_3 $IDD_HOME/output/idd_$(($COUNT * 5))
    let "COUNT += 1"
done

# Check whether iddd could handle a large event when partition already fully utilized
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd
add_event $TESTROOT/TC-0014-data/2MB_text_file
sleep 1

# 114 reports are deleted
if [ -e $IDD_HOME/output/idd_114 ]; then
    fail_test "idd_114 has not been removed"
fi
if [ ! -e $IDD_HOME/output/idd_115 ]; then
    fail_test "idd_115 has been removed"
fi

# A new report (idd_611) shall contain the large event.
NEW_REPORT=idd_$(($COUNT * 5 - 4))
compare_ref_file $IDD_HOME/output $NEW_REPORT

############################################################
start_test_stopped 8 "Missing IMEI, skip generating IDD report" "Test that generating IDD report is skipped in the case of missing IMEI even if the event file reaches the report max size."
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd
add_event $TESTROOT/TC-0014-data/400KB_text_file
sleep 1
if [ -e $IDD_HOME/output/idd_* ]; then
    fail_test "IDD report was generated although IMEI is missing and the generating IDD report should be skipped."
fi
pass_test

############################################################
start_test_stopped 9 "Missing IMEI, but the event file reaches the limit to skip generating IDD report" "Test that IDD report is generated if the event file reaches the limit to skip generating IDD report even though IMEI is still missing."
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd
add_event $TESTROOT/TC-0014-data/2MB_text_file
sleep 1
if [ ! -e $IDD_HOME/output/idd_* ]; then
    fail_test "IDD report was not generated although the event file reaches to the limit to skip generating IDD report."
fi
pass_test

############################################################
# We restore the home
IDD_HOME=/data/idd_run
IDDD="$IDDD_BIN -b $IDD_HOME"
IDDC="$IDDC_BIN -s $IDD_HOME/"

fi
end_tests
