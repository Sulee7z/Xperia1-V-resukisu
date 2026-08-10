. ./test-lib.sh
set_module config tc-0007-data


check_config_file()
{
    get_config_sha1 $1 $2
    cat > $IDD_HOME/test/reference <<EOF
$3  -
EOF
    compare_ref
}

############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 1 "Config Request" "Testing start of iddd, and verify contents of config request"
set_imei 004401020304050
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
write_config_request
flush_and_get_output
# Test default values expected
# Both config_request and idd_1 have secure_mode UNKNOWN (0)
compare_ref2

send_raw "SS D"
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
write_config_request
flush_and_get_output
# Now the secure_mode has changed to DEBUG (1) in config_request
# idd_2 also have secure_mode DEBUG
# Actually here is a bug. The header will not be recognized as changed and no pack_header() will be called!
# icompare_ref2 "1.1"

send_raw "SS L"
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
write_config_request
flush_and_get_output
# Now the secure_mode has changed to LIVE (2) in config_request
# idd_3 also have secure_mode LIVE
# Actually here is a bug. The header will not be recognized as changed and no pack_header() will be called!
# compare_ref2 "1.2"


############################################################
start_test 2 "No configuration" "Handle 'notify new configuration' when no new configuration is available."
cat > $IDD_HOME/test/reference <<EOF
EOF
send_load_config
check_input
compare_ref
check_config
compare_ref


############################################################
start_test 3 "Delete all input" "Verifies that all input files are removed"
cat > $IDD_HOME/test/reference <<EOF
EOF
echo "foo" > $IDD_HOME/input/foo
echo "bar" > $IDD_HOME/input/bar
echo "gazonk" > $IDD_HOME/input/gazonk
send_load_config
check_input
compare_ref
check_config
compare_ref


############################################################
start_test 4 "Corrupt config" "Simple test of configuration that is all wrong"
cat > $IDD_HOME/test/reference <<EOF
EOF
echo "foobargazonk" > $IDD_HOME/input/config
send_load_config
check_input
compare_ref
check_config
compare_ref


############################################################
start_test 5 "Unsigned Configuration" "Simple unsigned configuration"
cp $TESTROOT/tc-0007-data/unsigned_conf $IDD_HOME/input/config
send_load_config
check_input
cat > $IDD_HOME/test/reference <<EOF
EOF
compare_ref
check_config
cat > $IDD_HOME/test/reference <<EOF
a13f2157f6c42fde4db23ceb345c250732ae6168  conf_1.bin
da39a3ee5e6b4b0d3255bfef95601890afd80709  conf_2.bin
79fb86e161e6bee2f14740099c4bee8247926999  conf_3.bin
328ef9e07dd31d535532032efd542a23e995491d  conf_4.bin
dd5ec1b907f39eb5111b611cab2180c9d30b0a8f  conf_5.bin
EOF
compare_ref


############################################################
start_nontest 6 "Checksum" "Configuration Checksum"
cp $TESTROOT/tc-0007-data/unsigned_conf $IDD_HOME/input/config
send_load_config
check_input

# When re-generating tests: Extract the config_request and parse it
# and verify that the checksum in it is the same as sha1sum(unsigned_conf)
compare_ref_conf unsigned_conf

# Also verify that it's saved and re-generated on startup
stop_iddd
start_iddd_nontest
compare_ref_conf unsigned_conf


############################################################
start_nontest 7 "Replace Configurations" "Verifies that configurations are replaced accordingly"
cp $TESTROOT/tc-0007-data/unsigned_conf $IDD_HOME/input/config
send_load_config
sleep 1
cp $TESTROOT/tc-0007-data/unsigned_conf2 $IDD_HOME/input/config
send_load_config

check_config
cat > $IDD_HOME/test/reference <<EOF
da39a3ee5e6b4b0d3255bfef95601890afd80709  conf_2.bin
79fb86e161e6bee2f14740099c4bee8247926999  conf_3.bin
010f62eb87e9d59aa49a45c105e28f554dc2324b  conf_4.bin
dd5ec1b907f39eb5111b611cab2180c9d30b0a8f  conf_5.bin
f877828281631cd58c50f0426bd941bc9cf7a59f  conf_6.bin
EOF
compare_ref

# And verify the new checksum
# i.e, parse the config_request and verify that the checksum
# is the same as sha1sum(unsigned conf2)
compare_ref_conf unsigned_conf2


############################################################
#start_test 8 "Purge keeps config request" "Verifies that a purge keeps the config request"
# TODO
#fail_test "Not implemented"


#############################################################
start_test 9 "Get Config None" "Verifies that no configurations can be fetched"

NO_CONF_FOUND=7cd048c8e44984907803ae328fb433d93ae413a3
check_config_file foo bar $NO_CONF_FOUND
check_config_file idd logreader $NO_CONF_FOUND
check_config_file should.never exist $NO_CONF_FOUND


#############################################################
start_test 10 "Get Config Real" "Verifies that clients can retrieve the configurations correctly"
cp $TESTROOT/tc-0007-data/unsigned_conf $IDD_HOME/input/config
send_load_config

NO_CONF_FOUND=7cd048c8e44984907803ae328fb433d93ae413a3
check_config_file foo bar a13f2157f6c42fde4db23ceb345c250732ae6168        #conf_1.bin
check_config_file idd logreader da39a3ee5e6b4b0d3255bfef95601890afd80709  #conf_2.bin
check_config_file iddd testfile 79fb86e161e6bee2f14740099c4bee8247926999  #conf_3.bin
check_config_file other big.file 328ef9e07dd31d535532032efd542a23e995491d #conf_4.bin

check_config_file foooo baaaar $NO_CONF_FOUND
check_config_file a b $NO_CONF_FOUND


#############################################################
cp $TESTROOT/tc-0007-data/maxsizes_conf $IDD_HOME/input/config
start_test 11 "Set ReportMaxSize and OutputDirMaxSize" "Verifies that max sizes can be set in configurations file"
set_imei 004401020304050
# max_report_size = 1000 and max_dir_size = 2000

pump_events 80/100/1 # around 8kb of data
flush_and_get_output
compare_ref2


#############################################################
cp $TESTROOT/tc-0007-data/maxsizes_conf $IDD_HOME/input/config
start_test 12 "Reset ReportMaxSize and OutputDirMaxSize to default" "Verifies that max sizes are reset to default values if not included in the latest configuration"
set_imei 004401020304050
# max_report_size = 1000 and max_dir_size = 2000

pump_events 80/100/1 # around 8kb of data
flush_and_get_output
compare_ref2 "12.1"

# Configuration without settings config section
cp $TESTROOT/tc-0007-data/unsigned_conf $IDD_HOME/input/config
send_load_config

pump_events 80/100/1 # around 8kb of data
flush_and_get_output
compare_ref2 "12.2"


############################################################
start_test 13 "Kill switch" "Verifies that the kill switch works, at all times"

verify_idd_active 1

cp $TESTROOT/tc-0007-data/kill_switch_conf $IDD_HOME/input/config
send_load_config
check_input
cat > $IDD_HOME/test/reference <<EOF
EOF
compare_ref

verify_idd_active 0

echo "You will get 6 \"idd_flush() failed...\" which is expected."
send_flush
send_flush
cat > $IDD_HOME/test/reference <<EOF
da39a3ee5e6b4b0d3255bfef95601890afd80709  idd_disabled
EOF
flush_and_get_output
compare_ref

# Verify that this is true also after a restart.
restart_iddd

verify_idd_active 0

send_flush
send_flush
cat > $IDD_HOME/test/reference <<EOF
da39a3ee5e6b4b0d3255bfef95601890afd80709  idd_disabled
EOF
flush_and_get_output
compare_ref
stop_iddd

#############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 15 "RunActionsOnce" "RunActionsOnce - remove reports"
set_imei 004401020304050
pump_events 80/100/1 # around 8kb of data
flush_and_get_output
pump_events 80/100/1 # around 8kb of data
flush_and_get_output
pump_events 80/100/1 # around 8kb of data
flush_and_get_output


if [ ! -e $IDD_HOME/output/idd_3 ]; then
    fail_test "Test data:idd_3 hasn't been generated"
fi
cp $TESTROOT/tc-0007-data/no_remove_reports_conf $IDD_HOME/input/config
send_load_config
if [ ! -e $IDD_HOME/output/idd_3 ]; then
    fail_test "Idd_3 shouldn't be removed with no_remove_reports_conf"
fi
cp $TESTROOT/tc-0007-data/remove_reports_conf $IDD_HOME/input/config
send_load_config
if [ ! -e $IDD_HOME/output/idd_3 ] ; then
    pass_test $TEST_NAME
else
    fail_test "remove reports not successfully"
fi


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 16 "Studies" "Testing enter and leave of a study"
# This test verifies that the report header group changes when entering/leaving a study.
# The report device id is changed when entering/leaving a study.
# The config request device id is the same all the time.

flush_and_get_output

# Entering study_one
cp $TESTROOT/tc-0007-data/study_one_conf $IDD_HOME/input/config
send_load_config
flush_and_get_output

# Leaving study_one, entering study two
cp $TESTROOT/tc-0007-data/study_two_conf $IDD_HOME/input/config
send_load_config
flush_and_get_output

# Staying in study two
cp $TESTROOT/tc-0007-data/study_two_nr2_conf $IDD_HOME/input/config
send_load_config
flush_and_get_output

# Leaving study_two
cp $TESTROOT/tc-0007-data/no_study_conf $IDD_HOME/input/config
send_load_config
write_config_request
flush_and_get_output

# idd_1 with no group
# idd_2 group study:study_one and a new device_id
# idd_3 with group study:study_two and a new device_id
# idd_4 with group study:study_two and a same device_id as in idd_3
# idd_5 with no and a new device_id
# config_request with same device_id reports as idd_5 and device_id request as in idd_1

# BUG found! There will be no header change recognized when groups changes.
# compare_ref2


############################################################


end_tests