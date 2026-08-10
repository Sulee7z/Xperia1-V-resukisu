. ./test-lib.sh
set_module basic tc-0001-data

############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 1 "Header" "Check if header is correct"
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
flush_and_get_output
compare_ref2

############################################################
start_test 2 "Flushing" "Multi-flushing - so that we ignore SIGCHLD"
send_flush 0
send_flush 0
send_flush 0
send_flush
flush_and_get_output
compare_ref2

############################################################
start_test 3 "Help Information" "Tests printing of help information, and correct exit code."
$IDDD_BIN --help > /dev/null
if [ "$?" -ne "2" ]; then
    fail_test "Long option"
fi
pass_test

# Test printing of help information, short option
$IDDD_BIN -h > /dev/null
if [ "$?" -ne "2" ]; then
    fail_test "Short option"
fi
pass_test


############################################################
start_test_stopped 4 "Build Info" "Test loading of build information."
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
secure_mode: 1
EOF
start_iddd
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
flush_and_get_output
compare_ref2

############################################################
start_test_stopped 5 "New software revision" "Test that new software revision generates flush."
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
secure_mode: 1
EOF

start_iddd
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
# Create a iddd.state but no report
stop_iddd

# New revision
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.15
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
secure_mode: 1
EOF

# New revision should force a report with old revision when iddd starts.
start_iddd
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
# This flush will generate a report with the new revision.
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 6 "New software variant" "Test that new software variant generates flush."
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
secure_mode: 1
EOF

# Create a iddd.state but no report
start_iddd
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
stop_iddd

# New variant should force a flush with the not yet reported events on the old variant.
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: T-MOBILE
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
secure_mode: 1
EOF

# New variant should force a report with old variant when iddd starts.
start_iddd
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
# This flush will generate a report with the new variant.
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 8 "Timezone changed" "Test that timezone changed."
# Only timezone change will not force flush.
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
secure_mode: 1
EOF

start_iddd
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
stop_iddd

# Change timezone to 3600.
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 3600
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
secure_mode: 1
EOF

# New timezone should not force a report.
start_iddd
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
stop_iddd

# Change software version.
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.15
variant: GENERIC
type: user-debug
timezone: 3600
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
secure_mode: 1
EOF

# New software version will force force a report. This will have the 'old' timezone.
start_iddd
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"

# This flush will generate a report with the 'new' timezone.
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 10 "New group id -- add one" "Test that add a new group id generates flush."
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
group id: test1
secure_mode: 1
EOF

# Create a iddd.state but no report
start_iddd
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
stop_iddd

#Add a new group id.
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
group id: test1
group id: test2
secure_mode: 1
EOF

# New group should force a report with old group.
start_iddd
check_output
compare_ref2


############################################################
start_test_stopped 11 "New group id -- add reach the max, remove oldest" "Test if the 7th group id comes, the oldest should be removed."
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
group id: test1
group id: test2
group id: test3
group id: test4
group id: test5
group id: test6
secure_mode: 1
EOF

start_iddd
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
send_flush
set_group_id test7
# BUG found! There will be no new report on group change. The restart of iddd should not be needed.
stop_iddd
start_iddd
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat ShutDown"
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 12 "Delete group id"
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
group id: test1
group id: test2
group id: test3
group id: test4
group id: test5
group id: test6
secure_mode: 1
EOF

start_iddd
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
send_flush
delete_group_id test1
# BUG found! There will be no new report on group change. The restart of iddd should not be needed.
stop_iddd
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
secure_mode: 1
EOF
start_iddd
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat ShutDown"
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 13 "Replace group id"
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
group id: test1
group id: test2
group id: test3
group id: test4
group id: test5
group id: test6
secure_mode: 1
EOF

start_iddd
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
send_flush
replace_group_id testreplace1 test1
# BUG found! There will be no new report on group change. The restart of iddd should not be needed.
stop_iddd
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
secure_mode: 1
EOF
start_iddd
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat ShutDown"
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 14 "Set IMEI" "Test that IMEI can be set."
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 15 "Hostile IMEI attack" "Test that IMEI can be changed but with a DaemonError event."
# This happens if the iddd.state file is copied between devices, for example on software clone.
# "iddd-util: IMEI not supposed to change: old  <004401020304050>, new  <004401020304051>"
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
set_imei 004401020304051
# Now a report with two BootComplete will be created with imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
flush_and_get_output
# Now a report with one 'DaemonError IMEI changed' and one BootComplete will be created with
# imei 004401020304051
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 16 "Set serial number" "Test that serial number can't be changed."
set_serial_number 0123456789
flush_and_get_output
# The set_serial_number failed, a "idd::DaemonError" event is generated with payload
# "iddd-util: Serial number not supposed to change: old  <ABCDEFGHIJ>, new  <0123456789>"
compare_ref2


############################################################
start_test_stopped 18 "IDD Active?" "Should perhaps have been the first test ever"
verify_idd_active 0
start_iddd
verify_idd_active 1


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 19 "Disable data collection" "Test disable data collection"

pump_events 100/100/10 # around 10kb of data.
set_counter foo bar 1
inc_counter foo bar 10
set_sdata foo bar "gazonk"

flush_and_get_output

set_data_collection 0
pump_events 200/100/10 # around 20kb of data.
set_counter foo bar 11
inc_counter foo bar 20
set_sdata foo bar "qwerty"

flush_and_get_output

compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 20 "Remove report" "Test remove all reports or remove specified report."
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
send_flush 0
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
send_flush 0
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
send_flush 0
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
send_flush
# idd_1, idd_2, idd_3, idd_4 should be generated.
if [ ! -e $IDD_HOME/output/idd_1 -o ! -e $IDD_HOME/output/idd_2 -o ! -e $IDD_HOME/output/idd_3 -o ! -e $IDD_HOME/output/idd_4 ]; then
fail_test "Test data:reports haven't been generated"
fi
remove_report 3
sleep 1
# idd_3 should be removed.
if [ -e $IDD_HOME/output/idd_3 ]; then
fail_test "idd_3 hasn't been deleted"
fi
remove_report 0
sleep 1
# idd_1, idd_2, idd_4 should be removed.
if [ -e $IDD_HOME/output/idd_1 -o -e $IDD_HOME/output/idd_2 -o -e $IDD_HOME/output/idd_4 ]; then
fail_test "idd_3 hasn't been deleted"
fi
pass_test $TEST_NAME

############################################################
start_test 21 "Change session id" "Test session id is changed"
stop_iddd
clean_iddd
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
device imei: 004401020304050
secure_mode: 1
EOF
start_iddd_nontest
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
clean_flush_and_get_output
stop_iddd
OLD_SHA1=$(cat $IDD_TEST/actual)

# restart iddd and then new session id is generated.
clean_iddd
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: user-debug
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
device imei: 004401020304050
secure_mode: 1
EOF
start_iddd_nontest
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
clean_flush_and_get_output
stop_iddd
NEW_SHA1=$(cat $IDD_TEST/actual)
compare_sha1 ne
