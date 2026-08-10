. ./test-lib.sh
set_module "filter-config" tc-0009-data

is_probe_active()
{
    $IDDC --active $1 $2 | sha1sum > $IDD_HOME/test/actual
}

expect_is_probe_active()
{
    ACTIVE=455897959f244d22478e56e2a1f758aadd335f7d
    NOT_ACTIVE=01e25d36f71d08dfecfdd980d01589ab85693a51
    if [ "$3" -eq "1" ]; then
        echo "$ACTIVE  -" > $IDD_HOME/test/reference
    else
        echo "$NOT_ACTIVE  -" > $IDD_HOME/test/reference
    fi
    is_probe_active $1 $2
    compare_ref
}

send_test_events()
{
    send_raw "EV foo bar gazonk"
    send_raw "EV foo boo ooboof"
    send_raw "EV fie fum1 1flopp"
    send_raw "EV fie fum2 2flopp"
    send_raw "EV fie fum3 3flopp"
    set_counter cnt.pkg random_name 1
    set_counter cnt.pkg include_name 1
    set_counter cnt.pkg exclude_name 1
    set_counter cnt.wildcard random_name 1
    set_sdata staticdata.pkg random_name random_value
    set_sdata staticdata.pkg include_name include_value
    set_sdata staticdata.pkg exclude_name exclude_value
    set_sdata staticdata.wildcard wildcard_name wildcard_value
}

############################################################
cp $TESTROOT/tc-0009-data/no_config $IDD_HOME/input/config
start_test 1 "No Filter" "No filter at all -> All probes disabled"

send_test_events
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 2 "Permissive exclusion filter" ""

send_test_events
flush_and_get_output

# Check after restart
restart_iddd
send_test_events
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/wildcard_include_config $IDD_HOME/input/config
start_test 4 "Wildcard include" ""

send_test_events
flush_and_get_output

# Check after restart
restart_iddd
send_test_events
flush_and_get_output
compare_ref2

# Is Probe Active? Verifies the Is Probe Active call.

# All except foo.bar anf fie:* should be disabled
expect_is_probe_active fum fie 0
expect_is_probe_active foo boo 0

# All else should be active
expect_is_probe_active foo bar 1
expect_is_probe_active fie fie 1
expect_is_probe_active fie boo 1


############################################################
cp $TESTROOT/tc-0009-data/simple_exclude_config $IDD_HOME/input/config
start_test 5 "Reset filter" ""

send_test_events
flush_and_get_output

#Configuration with empty exclude probe filter
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
send_load_config
send_test_events

# Only new staticdata (staticdata.pkg::exclude_name)
flush_and_get_output

# Check after restart - note: timestamps diffed.
restart_iddd
send_test_events
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/wildcard_include_config $IDD_HOME/input/config
start_test 6 "Is probe active?" "Verify the api with both IDD enabled and disabled"
# No filename expansion, needed for the *.
set -o noglob
compare "ACTIVE" "$IDDC --active foo *"
compare "ACTIVE" "$IDDC --active foo bar"
compare "ACTIVE" "$IDDC --active fie *"
compare "ACTIVE" "$IDDC --active fie bar"
compare "not active" "$IDDC --active fum *"
set_data_collection 0
compare "not active" "$IDDC --active foo *"
compare "not active" "$IDDC --active foo bar"
compare "not active" "$IDDC --active fie *"
compare "not active" "$IDDC --active fie bar"
set +o noglob


############################################################

end_tests
