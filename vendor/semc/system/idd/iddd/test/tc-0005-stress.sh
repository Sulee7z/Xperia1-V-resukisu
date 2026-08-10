. ./test-lib.sh
set_module stress tc-0005-data

############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 1 "Massive Counters" "Test setting massive amount of counters (so that we have to grow the internal list)"
echo "Generating massive amount of counters.."
START=$(date +%s)
pump_counters 5000/10/200   # count/size/rate
clean_flush_and_get_output_slow # This will take about 5 seconds
END=$(date +%s)
ACTUAL=$(($END - $START))
echo "It took $ACTUAL seconds"
compare_ref2

############################################################
cp $TESTROOT/tc-0009-data/empty_exclude_config $IDD_HOME/input/config
start_test 2 "Massive Static Data" "Test setting massive amount of static data (so that we have to grow the internal list)"
echo "Generating massive amount of static data."
START=$(date +%s)
pump_sdata 5000/10/200   # count/size/rate
clean_flush_and_get_output_slow # This will take about 5 seconds
END=$(date +%s)
ACTUAL=$(($END - $START))
echo "It took $ACTUAL seconds"
compare_ref2
