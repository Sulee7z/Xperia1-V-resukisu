. ./test-lib.sh
set_module "conf-verify"


############################################################
start_test_stopped 1 "Raw-Verify Success" ""
cp $TESTROOT/tc-0010-data/idd_config.pem $IDD_HOME/private/idd_config.pem
$IDDD -C --verify-config $TESTROOT/tc-0010-data/good_sig.data -c $IDD_HOME/private/ > /dev/null
if [ "$?" -ne "42" ]; then
    fail_test "result: $?"
fi
pass_test


############################################################
start_test_stopped 2 "Raw-Verify Fail" ""
cp $TESTROOT/tc-0010-data/idd_config.pem $IDD_HOME/private/idd_config.pem
$IDDD -C --verify-config $TESTROOT/tc-0010-data/bad_sig.data -c $IDD_HOME/private/ > /dev/null
if [ "$?" -ne "33" ]; then
    fail_test "result: $?"
fi
pass_test


############################################################
start_test_stopped 3 "Correct Sig" "Simple pre-built verification"
cp $TESTROOT/tc-0010-data/idd_config.pem $IDD_HOME/private/idd_config.pem
cp $TESTROOT/tc-0010-data/signed_conf $IDD_HOME/input/config
start_iddd_encrypted $IDD_HOME/private/
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
start_test_stopped 4 "Correct Sig Replace" "Verifies that signed configurations are replaced accordingly"
cp $TESTROOT/tc-0010-data/idd_config.pem $IDD_HOME/private/idd_config.pem
cp $TESTROOT/tc-0010-data/signed_conf $IDD_HOME/input/config
start_iddd_encrypted $IDD_HOME/private/
send_load_config
cp $TESTROOT/tc-0010-data/signed_conf2 $IDD_HOME/input/config
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

############################################################
start_test_stopped 5 "Incorrect Sig" "Simple pre-built verification"
cp $TESTROOT/tc-0010-data/idd_config.pem $IDD_HOME/private/idd_config.pem
cp $TESTROOT/tc-0010-data/badsigned_conf $IDD_HOME/input/config
start_iddd_encrypted $IDD_HOME/private/
cat > $IDD_HOME/test/reference <<EOF
EOF
check_config
send_load_config
check_input
compare_ref
check_config
cat > $IDD_HOME/test/reference <<EOF
EOF
compare_ref

############################################################
start_test_stopped 6 "Incorrect Sig Keep-Old" "Simple pre-built verification"
cp $TESTROOT/tc-0010-data/idd_config.pem $IDD_HOME/private/idd_config.pem
cp $TESTROOT/tc-0010-data/signed_conf $IDD_HOME/input/config
start_iddd_encrypted $IDD_HOME/private/
send_load_config
cp $TESTROOT/tc-0010-data/badsigned_conf2 $IDD_HOME/input/config
send_load_config
sleep 1
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
start_test_stopped 7 "Unsigned" "Simple pre-built verification"
cp $TESTROOT/tc-0010-data/idd_config.pem $IDD_HOME/private/idd_config.pem
cp $TESTROOT/tc-0010-data/unsigned_conf $IDD_HOME/input/config
start_iddd_encrypted $IDD_HOME/private/
cat > $IDD_HOME/test/reference <<EOF
EOF
check_config
send_load_config
check_input
compare_ref
check_config
cat > $IDD_HOME/test/reference <<EOF
EOF
compare_ref

############################################################
start_test_stopped 8 "Unsigned Keep-Old" "Simple pre-built verification"
cp $TESTROOT/tc-0010-data/idd_config.pem $IDD_HOME/private/idd_config.pem
cp $TESTROOT/tc-0010-data/signed_conf $IDD_HOME/input/config
start_iddd_encrypted $IDD_HOME/private/
send_load_config
cp $TESTROOT/tc-0010-data/unsigned_conf2 $IDD_HOME/input/config
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


end_tests
