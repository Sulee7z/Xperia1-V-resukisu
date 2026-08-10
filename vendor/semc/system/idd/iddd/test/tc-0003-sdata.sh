. ./test-lib.sh
set_module "sdata" tc-0003-data

############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 1 "Simple Static Data" "Very simple static data test"
set_sdata foo bar "gazonk" # (ts 07:00:01)
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 2 "Advanced Static Data" "More advanced static data test"
set_sdata sdatapack sdata1 foo             # (ts 07:00:01)
set_sdata sdatapack sdata2 "longer value"  # (ts 07:00:02)
set_sdata sdatapack sdata3 foo             # (ts 07:00:03)
set_sdata sdatapack sdata3 bar             # (ts 07:00:04)
set_sdata sdatapack sdata3 "foo bar fie"   # (ts 07:00:05)
set_sdata sdatapack sdata1 "gazonk"        # (ts 07:00:06)

flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 3 "Verifying Static Data input" "Mainly probe ID test. Also tests serialization/deserialization"
send_raw "SD"
send_raw "SD PackageOnly"
set_sdata ThisProbeIdHasMaxLength_256________________________________________________________________________100______110________120__5678 ThisProbeIdHasMaxLength_256________________________________________________________________________100______110________120__5678 1
set_sdata ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__56789 ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__5678 1
set_sdata ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__5678 ThisProbeIdIsTooLarge_257__________________________________________________________________________100______110________120__56789 1
set_sdata PackageNoValue NameNoValue ""
set_sdata PackageNoValue SpaceAsValueIsOK " "
# Will cause a serialization to file
restart_iddd
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 4 "Static Data w/ newline, reload" "Serialization/Deserialization test with newline"
set_sdata SpecialChars NewLine "This is before newline
and this is after"
restart_iddd # Will cause a serialization to file, and re-read
flush_and_get_output
compare_ref2


############################################################
start_test_stopped 5 "Expiry" "Verifies that static data expire"
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
$IDDD -t -C -e 10 -c NO_ENCRYPTION
sleep 1
set_sdata foo bar01 hej  # ts 07:00:01
set_sdata foo bar02 hej  # ts 07:00:02
set_sdata foo bar03 hej  # ts 07:00:03
set_sdata foo bar04 hej  # ts 07:00:04
set_sdata foo bar05 hej  # ts 07:00:05
set_sdata foo bar06 hej  # ts 07:00:06
set_sdata foo bar07 hej  # ts 07:00:07
set_sdata foo bar08 hej  # ts 07:00:08
set_sdata foo bar09 hej  # ts 07:00:09
set_sdata foo bar10 hej  # ts 07:00:10
set_sdata foo bar11 hej  # ts 07:00:11
restart_iddd # save config + save sdata => increase timestamp => 07:00:13
flush_and_get_output
# we should have entries from 07:00:03 and forward.
compare_ref2


############################################################
cp $TESTROOT/tc-0003-data/sdata_expire_conf $IDD_HOME/input/config
start_test 6 "Expiry configuration" "Verifies that static data expire age can be configured"

# expire_counters_age = 5

set_sdata foo bar01 hej  # ts 07:00:01
set_sdata foo bar02 hej  # ts 07:00:02
set_sdata foo bar03 hej  # ts 07:00:03
set_sdata foo bar04 hej  # ts 07:00:04
set_sdata foo bar05 hej  # ts 07:00:05
set_sdata foo bar06 hej  # ts 07:00:06
set_sdata foo bar07 hej  # ts 07:00:07
set_sdata foo bar08 hej  # ts 07:00:08
set_sdata foo bar09 hej  # ts 07:00:09
set_sdata foo bar10 hej  # ts 07:00:10
set_sdata foo bar11 hej  # ts 07:00:11
restart_iddd # save config + save sdata => increase timestamp => 07:00:13
flush_and_get_output
# we should have entries from 07:00:08 and forward.
compare_ref2


############################################################
start_test_stopped 7 "Reset expiry to default" "Verifies that static data expire age is reset to default if not included in the latest configuration"
clean_iddd
cp $TESTROOT/tc-0003-data/sdata_expire_conf $IDD_HOME/input/config
$IDDD -t -C -e 10 -c NO_ENCRYPTION # we set a low default value so it can be verified
sleep 1

# expire_counters_age = 5

set_sdata foo bar01 hej  # ts 07:00:01
set_sdata foo bar02 hej  # ts 07:00:02
set_sdata foo bar03 hej  # ts 07:00:03
set_sdata foo bar04 hej  # ts 07:00:04
set_sdata foo bar05 hej  # ts 07:00:05
set_sdata foo bar06 hej  # ts 07:00:06
set_sdata foo bar07 hej  # ts 07:00:07
set_sdata foo bar08 hej  # ts 07:00:08
set_sdata foo bar09 hej  # ts 07:00:09
set_sdata foo bar10 hej  # ts 07:00:10
set_sdata foo bar11 hej  # ts 07:00:11
stop_iddd # save config + save counter => increase timestamp => 07:00:13
$IDDD -t -C -e 10 -c NO_ENCRYPTION # we set a low default value so it can be verified
sleep 1
flush_and_get_output
# we should have entries from 07:00:08 and forward.

#Configuration without settings config section
cp $TESTROOT/tc-0003-data/unsigned_conf $IDD_HOME/input/config
send_load_config

set_sdata foo bar12 hej  # ts 07:00:14
set_sdata foo bar13 hej  # ts 07:00:15
set_sdata foo bar14 hej  # ts 07:00:16
set_sdata foo bar15 hej  # ts 07:00:17
set_sdata foo bar16 hej  # ts 07:00:18
set_sdata foo bar17 hej  # ts 07:00:19
set_sdata foo bar18 hej  # ts 07:00:20
set_sdata foo bar19 hej  # ts 07:00:21
set_sdata foo bar20 hej  # ts 07:00:22
set_sdata foo bar21 hej  # ts 07:00:23
set_sdata foo bar22 hej  # ts 07:00:24
restart_iddd # save config + save sdata => increase timestamp => 07:00:26
flush_and_get_output
# we should have entries from 07:00:16 and forward
compare_ref2


############################################################
