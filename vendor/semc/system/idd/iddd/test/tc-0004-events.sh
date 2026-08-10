. ./test-lib.sh
set_module events tc-0004-data

############################################################
start_test_stopped 1 "Auto-flush" "Test auto-flush of file after 400 kb"
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd_non_compressed
set_imei 004401020304050
echo "Generating over 512kb of data"
pump_events 5/$((128*1024))/200 # 512kb of payload
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 2 "Auto-purge" "Test auto-purge after 12MB of data"
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd_non_compressed
set_imei 004401020304050
echo "Generating over 12MB of data"
pump_events 97/$((128 * 1024))/200 # 12Mb of payload
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 3 "Rescue corrupt report header" "Remove event file because report header is corrupt"
cp $TESTROOT/tc-0004-data/corrupt-header $IDD_HOME/private/events
cp $TESTROOT/tc-0004-data/empty_include.config $IDD_HOME/input/config

start_iddd_non_compressed
if [ -e "$IDD_HOME/private/events" ]; then
    fail_test "Rescue corrupt report header event file failed"
fi
pass_test


############################################################
start_test_stopped 4 "Rescue corrupt event without header" "Remove corrupt event when no header is present"
cp $TESTROOT/tc-0004-data/corrupt-event-without-header $IDD_HOME/private/events
cp $TESTROOT/tc-0004-data/empty_include.config $IDD_HOME/input/config
start_iddd_non_compressed
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 5 "Rescue corrupt event" "Remove corrupt event"
cp $TESTROOT/tc-0004-data/corrupt-event $IDD_HOME/private/events
cp $TESTROOT/tc-0004-data/empty_include.config $IDD_HOME/input/config
start_iddd_non_compressed
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 6 "Rescue corrupt event (null bytes)" "Remove null bytes. Also add DeamonError's buffered during startup"
cp $TESTROOT/tc-0004-data/null-event $IDD_HOME/private/events
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd_non_compressed
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 7 "Testing max payload size" "maxsize accepted maxsize+1 not accepted"
# Maximum payload size is found in idd_config.h
send_raw_len $((260864)) "EV large event"
send_raw_len $((260864+1)) "EV to_large event"
# Expected: One event with maxsize and one IDD error event and one IDD error counter
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 8 "Handle daemon error flooding" 
# Generate 101 daemon error probe, only 100 is accepted due to flooding control.
send_raw_loop 101 "DAEMON_ERROR_TEST"
flush_and_get_output

# Preserve daemon error flooding during restarts. Generate one daemon error probe, stopped by flooding control.
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
restart_iddd
send_raw "DAEMON_ERROR_TEST"
# No report will be generated!
flush_and_get_output

# Daemon error flooding period ended, accept error probes again.
# This has to be started after the error period is ended from the first daemon error test.
# Synchronize this with the constant DAEMON_ERROR_TEST_PERIOD in iddd_error_probe.c (6 seconds)

sleep 4
restart_iddd
send_raw "DAEMON_ERROR_TEST"
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 10 "Verify Events input" "Mainly checking probe ID for events"
send_raw "EV"
send_raw "EV PackageOnly"
send_raw "EV ThisProbeIdHasMaxLength_256________________________________________________________________________100______110________120__5678 ThisProbeIdHasMaxLength_256________________________________________________________________________100______110________120__5678 1"
send_raw "EV ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__56789 ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__5678 1"
send_raw "EV ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__5678 ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__56789 1"
send_raw "EV PackageNoValueIsOK NameNoValueIsOK"
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 11 "Compress large events" "text compressed successfully, binary fail to compress"
add_event $TESTROOT/tc-0004-data/large_text_file
add_event $TESTROOT/tc-0004-data/large_binary_file
flush_and_get_output
compare_ref2


############################################################
