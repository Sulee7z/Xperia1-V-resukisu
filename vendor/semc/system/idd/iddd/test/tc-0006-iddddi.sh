. ./test-lib.sh
set_module iddddi

############################## IDDDDI ##############################
# This test unfortunately does not work on target yet.
if [ "$1" = "PC" -o "$2" = "PC" ]; then
    start_test_stopped 1 "iddddi" "Debug Interface"
    start_iddd_iddddi
    sleep 1
    $NC 127.0.0.1 2305 > $IDD_HOME/iddddi.out &
    sleep 1
    set_counter foo bar 123
    set_sdata gazonk fie "fum fum fum"
    echo "EV foo bar gazonk" | $IDDC --raw
    send_flush
    sleep 1
    killall nc
    sleep 1
    cd $IDD_HOME/
    sha1sum iddddi.out > $IDD_HOME/test/actual
    cat > $IDD_HOME/test/reference <<EOF
da39a3ee5e6b4b0d3255bfef95601890afd80709  iddddi.out
EOF
    compare_ref
    stop_iddd
    clean_iddd
fi
