. ./test-lib.sh
set_module "counters" tc-0002-data


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 1 "Counter 1" "Simple counter"
set_counter foo bar 1  # start timestamp 07:00:01
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 2 "Counter 2" "Advanced counters"
set_counter foo bar 1    # start timestamp 07:00:01
inc_counter foo bar 1    # 2 (ts 07:00:02)
inc_counter foo bar 1    # 3 (ts 07:00:03)
inc_counter foo bar 10   # 13 (ts 07:00:04)
inc_counter foo bar 10   # 23 (ts 07:00:05)
inc_counter foo gazonk 1 # 1  (start ts 07:00:06)
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 3 "Preserve counters" "Verify that counters are preserved during restarts."
inc_counter foo gazonk 1 # 1 (ts 07:00:01)
inc_counter foo gazonk 1 # 2 (ts 07:00:02)
restart_iddd
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 4 "Automatic Saving" "Saving of counters and static data at regular intervals"
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
$IDDD -t -C -s 10 -c NO_ENCRYPTION
sleep 1
set_counter foo bar 123  # ts 07:00:01
set_sdata fum fie gazonk # ts 07:00:02
echo "Waiting until counters are automatically saved - 12 seconds"
sleep 12
# Simulate a crash in a more controlled way (we can't kill -9
# since that will prevent our gcov data from being saved)
cp $IDD_HOME/private/counters.txt $IDD_HOME/counters_backup
cp $IDD_HOME/private/sdata.txt $IDD_HOME/sdata_backup
stop_iddd
clean_iddd
mv $IDD_HOME/counters_backup $IDD_HOME/private/counters.txt
mv $IDD_HOME/sdata_backup $IDD_HOME/private/sdata.txt
start_iddd
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 5 "Expiry" "Verifies that counters expire"
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
$IDDD -t -C -e 10 -c NO_ENCRYPTION
sleep 1
set_counter foo bar01 2  # ts 07:00:01
set_counter foo bar02 2  # ts 07:00:02
set_counter foo bar03 2  # ts 07:00:03
set_counter foo bar04 2  # ts 07:00:04
set_counter foo bar05 2  # ts 07:00:05
set_counter foo bar06 2  # ts 07:00:06
set_counter foo bar07 2  # ts 07:00:07
set_counter foo bar08 2  # ts 07:00:08
set_counter foo bar09 2  # ts 07:00:09
set_counter foo bar10 2  # ts 07:00:10
set_counter foo bar11 2  # ts 07:00:11
restart_iddd # save config + save counter => increase timestamp => 07:00:13
flush_and_get_output
# we should have entries from 07:00:03 and forward.
compare_ref2


############################################################
cp $TESTROOT/tc-0002-data/counters_expire_conf $IDD_HOME/input/config
start_test 6 "Expiry configuration" "Verifies that counters expire age can be configured"

# expire_counters_age = 5

set_counter foo bar01 2  # ts 07:00:01
set_counter foo bar02 2  # ts 07:00:02
set_counter foo bar03 2  # ts 07:00:03
set_counter foo bar04 2  # ts 07:00:04
set_counter foo bar05 2  # ts 07:00:05
set_counter foo bar06 2  # ts 07:00:06
set_counter foo bar07 2  # ts 07:00:07
set_counter foo bar08 2  # ts 07:00:08
set_counter foo bar09 2  # ts 07:00:09
set_counter foo bar10 2  # ts 07:00:10
set_counter foo bar11 2  # ts 07:00:11
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
restart_iddd # save config + save counter => increase timestamp => 07:00:13
flush_and_get_output
# we should have entries from 07:00:08 and forward.
compare_ref2


############################################################
start_test_stopped 7 "Reset expiry to default" "Verifies that counters expire age is reset to default if not included in the latest configuration"
cp $TESTROOT/tc-0002-data/counters_expire_conf $IDD_HOME/input/config
$IDDD -t -C -e 10 -c NO_ENCRYPTION # we set a low default value so it can be verified
sleep 1

# expire_counters_age = 5

set_counter foo bar01 2  # ts 07:00:01
set_counter foo bar02 2  # ts 07:00:02
set_counter foo bar03 2  # ts 07:00:03
set_counter foo bar04 2  # ts 07:00:04
set_counter foo bar05 2  # ts 07:00:05
set_counter foo bar06 2  # ts 07:00:06
set_counter foo bar07 2  # ts 07:00:07
set_counter foo bar08 2  # ts 07:00:08
set_counter foo bar09 2  # ts 07:00:09
set_counter foo bar10 2  # ts 07:00:10
set_counter foo bar11 2  # ts 07:00:11
stop_iddd # save config + save counter => increase timestamp => 07:00:13
$IDDD -t -C -e 10 -c NO_ENCRYPTION # we set a low default value so it can be verified
sleep 1
flush_and_get_output
# we should have entries from 07:00:08 and forward.

# Configuration without settings config section
cp $TESTROOT/tc-0002-data/unsigned_conf $IDD_HOME/input/config
send_load_config

set_counter foo bar12 2  # ts 07:00:14
set_counter foo bar13 2  # ts 07:00:15
set_counter foo bar14 2  # ts 07:00:16
set_counter foo bar15 2  # ts 07:00:17
set_counter foo bar16 2  # ts 07:00:18
set_counter foo bar17 2  # ts 07:00:19
set_counter foo bar18 2  # ts 07:00:20
set_counter foo bar19 2  # ts 07:00:21
set_counter foo bar20 2  # ts 07:00:22
set_counter foo bar21 2  # ts 07:00:23
set_counter foo bar22 2  # ts 07:00:24
restart_iddd # save config + save counter => increase timestamp => 07:00:26
flush_and_get_output
# we should have entries from 07:00:16 and forward
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 8 "Verifying Counters input" "Mainly probe ID test. Also tests serialization/deserialization"
send_raw "CI"
send_raw "CS PackageOnly"
set_counter ThisProbeIdHasMaxLength_256________________________________________________________________________100______110________120__5678 ThisProbeIdHasMaxLength_256________________________________________________________________________100______110________120__5678 1
set_counter ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__56789 ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__5678 1
set_counter ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__5678 ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__56789 1
send_raw "CS PackageNoValue NameNoValue"
set_counter "PackageDefaultTo0" "NameDefaultTo0" "A"
restart_iddd # Will cause a serialization to file
flush_and_get_output
compare_ref2


############################################################
