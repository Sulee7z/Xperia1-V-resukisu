. ./test-lib.sh
set_module permissions

# Validate permission check on the IDD_PRIVATE api

############################################################
start_test_stopped 1 "Checking IDD_PRIVATE (flush)" "Test that connection less client got the right flush permissions"
start_iddd

if [ "$TESTING_ON_PC" -ne "0" ]; then

echo "TO BE DONE: Host tests -> Permission error ... E/idd-api: idd_add_event: send failed, socket 3, errcode -13 (Permission denied)"
#sudo su nobody <<EOF
#../../iddc/iddc -s../../../../../../../../idd_test/ --pump-events 1/1/0
#../../iddc/iddc -s../../../../../../../../idd_test/ --flush
#exit
#EOF

else

#Starting a new shell as a user that dont have IDD_PRIVATE permission
#The flush should fail, giving an empty output directory even though there is one event in events file
/system/xbin/su shell <<EOF
iddc -s/data/idd_run/ --pump-events 1/1/0 > /dev/null
iddc -s/data/idd_run/ --flush
exit
EOF

fi

rm -f $IDD_HOME/output/config_request
do_sleep 1 0.2
check_output
cat > $IDD_HOME/test/reference <<EOF
EOF
compare_ref
