. ./test-lib.sh
set_module "logreader"

# These tests extensively use Android's logging, so they are
# mot meant to run on a PC
if [ "$1" != "PC" -a "$2" != "PC" ]; then

############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 1 "Simple rules" "Start with a set of rules and verify that they are enforced"
logreader $TESTROOT/tc-0012-data/simple.rules $TESTROOT/tc-0012-data/loginput
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 2 "Keyword filtering" "Only entries matching a keyword"
logreader $TESTROOT/tc-0012-data/keyword.rules $TESTROOT/tc-0012-data/loginput
flush_and_get_output
compare_ref2


############################################################
cp $TESTROOT/tc-0012-data/not_active $IDD_HOME/input/config
start_test 3 "Start from IDDD" "Start of log reader from IDDD"
# First verify that it won't turn off when it shouldn't
check_input
verify_logreader_not_running

# Then verify that it should be active when we tell it to
cp $TESTROOT/tc-0012-data/simple_rules $IDD_HOME/input/config
send_load_config
check_input
verify_logreader_running

# Also when it should reload rules
cp $TESTROOT/tc-0012-data/simple_rules $IDD_HOME/input/config
send_load_config
check_input
verify_logreader_running


# And finally it should terminate when asked to.
cp $TESTROOT/tc-0012-data/not_active $IDD_HOME/input/config
send_load_config
check_input
verify_logreader_not_running


############################################################
start_test 4 "RunActions at startup" ""

cp $TESTROOT/tc-0012-data/simple_rules $IDD_HOME/input/config
send_load_config
check_input
verify_logreader_running

stop_iddd
stop_logreader

verify_logreader_not_running

start_iddd
verify_logreader_running

fi


############################################################


end_tests
