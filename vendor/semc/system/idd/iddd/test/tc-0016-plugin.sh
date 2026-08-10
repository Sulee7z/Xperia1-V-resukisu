. ./test-lib.sh
set_module "plugin" tc-0016-data

############################################################
cp $TESTROOT/tc-0016-data/plugin1.config $IDD_HOME/input/config
start_test 1 "IDD Plugin Is probe enabled?" "Verify IDD api idd_is_probe_active"
compare "ACTIVE" "$IDDC --active p1 probe1"
compare "ACTIVE" "$IDDC --active p1 probe2"
set_data_collection 0
compare "ACTIVE" "$IDDC --active p1 probe1"
compare "not active" "$IDDC --active p1 probe2"


############################################################
cp $TESTROOT/tc-0016-data/plugin1.config $IDD_HOME/input/config
start_test 2 "IDD Plugin new config" "Test that plugin folders is created and removed"
compare_ref_ls $PLUGIN_DIR 2/1
cp $TESTROOT/tc-0016-data/plugin2.config $IDD_HOME/input/config
send_load_config
compare_ref_ls $PLUGIN_DIR 2/2


############################################################
cp $TESTROOT/tc-0016-data/plugin2.config $IDD_HOME/input/config
start_test 3 "IDD Plugin reports" "Verify that reports are compressed and deleted"
send_raw_loop 50 "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat ShutDown"
send_raw_loop 10 "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
sleep 1 # Wait for compactor
compare_ref3 $PLUGIN_DIR/plugin3 3/plugin3
compare_ref3 $PLUGIN_DIR/xd 3/xd


############################################################
print_test_name 4 "IDD Plugin rescue report" "Verify that corrupt reports are corrected"
stop_iddd
rm $IDD_HOME/$PLUGIN_DIR/plugin3/*
cp $TESTROOT/tc-0016-data/report.corrupt $IDD_HOME/$PLUGIN_DIR/plugin3/p3_$((INT_MAX-1))
start_iddd
compare_ref3 $PLUGIN_DIR/plugin3


############################################################
print_test_name 5 "IDD Plugin max report nr" "Verify that the report number sequence is restarted"
send_raw_loop 40 "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
sleep 1 # Wait for compactor
compare_ref3 $PLUGIN_DIR/plugin3


############################################################
print_test_name 6 "IDD Plugin stop data collection" "Verify that correct reports are deleted"
send_raw "EV p1 probe1"
set_data_collection 0
compare_ref_ls $PLUGIN_DIR
compare_ref3 $PLUGIN_DIR/plugin1 6/plugin1
compare_ref3 $PLUGIN_DIR/plugin3 6/plugin3
compare_ref3 $PLUGIN_DIR/xd 6/xd
# ignore_idd_enabled shall keep the report iie_1
compare_ref3 $PLUGIN_DIR/ignore_idd_enabled 6/ignore_idd_enabled


############################################################
print_test_name 7 "IDD Plugin stopped data collection" "Verify that events is stored only to 'ignore IDD enabled' plugin"
send_raw_loop 10 "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat ShutDown"
send_raw "EV p1 probe1"
compare_ref3 $PLUGIN_DIR/xd 7/xd
compare_ref3 $PLUGIN_DIR/ignore_idd_enabled 7/ignore_idd_enabled


############################################################
print_test_name 8 "IDD Plugin start data collection" "Verify that events is stored"
set_data_collection 1
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
compare_ref3 $PLUGIN_DIR/xd


############################################################
cp $TESTROOT/tc-0016-data/plugin3.config $IDD_HOME/input/config
start_test 9 "Different Plugin and IDD filter" "Verify that correct data goes to correct reports"
send_raw_loop 10 "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
send_raw_loop 10 "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat ShutDown"
flush_and_get_output
compare_ref2 "9/output"
compare_ref3 $PLUGIN_DIR/xd 9/xd


############################################################
start_test_stopped 10 "New software type" "Test that new software type generates flush."
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

clean_plugin_output_dir
cp $TESTROOT/tc-0016-data/plugin3.config $IDD_HOME/input/config
start_iddd
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
stop_iddd

# New type should force a flush on plugins with report header.
cat > $IDD_HOME/private/header_info.conf <<EOF
id: 9876-5432
revision: 2.1.A.4.14
variant: GENERIC
type: eng
timezone: 0
device id: 584acec7-bd43-4e3d-926a-3a48dfaed7a9
cdf_id: 1234-5678_R6D
serial num: 0123456789
device imei: 004401020304050
secure_mode: 1
EOF

start_iddd
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
compare_ref_ls $PLUGIN_DIR/xd/ 10/xd


############################################################
start_test_stopped 11 "Encrypted reports" "Test that plugins generates encrypted reports"

cp $TESTROOT/tc-0016-data/plugin2.config $IDD_HOME/input/config
cp $TESTROOT/tc-0016-data/idd_report.pem $IDD_HOME/private/idd_report.pem
start_iddd_encrypted $IDD_HOME/private/

send_raw_loop 20 "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"

sleep 1 # Wait for compactor
# The reports are encrypted in dut. In the reference they are not encrypted.
decrypt $IDD_HOME/$PLUGIN_DIR/plugin3/p3p_1 $TESTROOT/tc-0016-data/report-priv.pem
decrypt $IDD_HOME/$PLUGIN_DIR/xd/xdp_1 $TESTROOT/tc-0016-data/report-priv.pem
compare_ref3 $PLUGIN_DIR/plugin3/ 11/plugin3/
compare_ref3 $PLUGIN_DIR/xd/ 11/xd/


############################################################


end_tests
