. ./test-lib.sh
set_module "encryption" tc-0011-data

############################################################
start_test_stopped 1 "Basic Functionality" ""
cp $TESTROOT/tc-0011-data/idd_report.pem $IDD_HOME/private/idd_report.pem
echo "foo bar gazonk" > $IDD_HOME/testfile
$IDDD -C --encrypt $IDD_HOME/testfile -c $IDD_HOME/private/
$IDDC --decrypt $TESTROOT/tc-0011-data/report-priv.pem $IDD_HOME/testfile.enc $IDD_HOME/testfile.key $IDD_HOME/testfile.iv > $IDD_HOME/testfile.dec
cat $IDD_HOME/testfile.dec | sha1sum > $IDD_HOME/test/actual
cat $IDD_HOME/testfile | sha1sum > $IDD_HOME/test/reference
check_reference
rm -f $IDD_HOME/testfile $IDD_HOME/testfile.enc $IDD_HOME/testfile.key $IDD_HOME/testfile.iv


############################################################
start_test_stopped 2 "Encrypted Flush" ""
cp $TESTROOT/tc-0011-data/idd_report.pem $IDD_HOME/private/idd_report.pem
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd_encrypted $IDD_HOME/private/
send_raw "EV com.sonyericsson.idd.probe.android.devicemonitor.powerstat BootComplete"
send_flush

# Split the encrypted file into parts that we can decrypt, and
# then compare that to a reference (look at tc-0001-basic.sh, test 1)
decrypt $IDD_HOME/output/idd_1 $TESTROOT/tc-0011-data/report-priv.pem
check_output
compare_ref2


############################################################
start_test_stopped 3 "Encrypted Several" ""
cp $TESTROOT/tc-0011-data/idd_report.pem $IDD_HOME/private/idd_report.pem
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_iddd_non_compressed_encrypted $IDD_HOME/private/
set_imei 004401020304050
echo "Generating over 12MB of data"
pump_events 97/$((128 * 1024))/200 # 12Mb of payload
send_flush
sleep 2
for i in `seq 2 25`
do
    decrypt $IDD_HOME/output/idd_$i $TESTROOT/tc-0011-data/report-priv.pem
done

sleep_if_valgrind 1
flush_and_get_output
compare_ref2


############################################################

end_tests
