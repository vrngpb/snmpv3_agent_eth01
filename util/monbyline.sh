#!/bin/bash

HOST="10.149.130.75"
COMMUNITY="-v3 -l authPriv -u snmpv3admin -a SHA -A authpassphrase01 -x AES -X privpassphrase01"

get() { snmpget -Oqv $COMMUNITY "$HOST" "$1" 2>/dev/null; }

while true; do
    TEMP=$(get 1.3.6.1.4.1.9999.1.1.0)
    HEAP_FREE=$(get 1.3.6.1.4.1.9999.1.2.0)
    UPTIME=$(get 1.3.6.1.4.1.9999.1.3.0)
    HEAP_PCT=$(get 1.3.6.1.4.1.9999.1.4.0)
    MAC=$(get 1.3.6.1.4.1.9999.1.5.0)
    LEAK=$(get 1.3.6.1.4.1.9999.1.6.0)
    DOOR1=$(get 1.3.6.1.4.1.9999.1.7.0)
    DOOR2=$(get 1.3.6.1.4.1.9999.1.8.0)
    IP=$(get 1.3.6.1.4.1.9999.1.10.0)
    GW=$(get 1.3.6.1.4.1.9999.1.11.0)
    MASK=$(get 1.3.6.1.4.1.9999.1.12.0)

    TEMP_C=$(awk "BEGIN {printf \"%.1f\", $TEMP/10}")
    [ "$LEAK"  = "1" ] && LEAK_S="ТРЕВОГА"  LEAK_S="ОК"
    [ "$DOOR1" = "1" ] && D1_S="ОТКРЫТА"   D1_S="закрыта"
    [ "$DOOR2" = "1" ] && D2_S="ОТКРЫТА"  || D2_S="закрыта"

    echo "[$(date '+%H:%M:%S')]  Температура: ${TEMP_C}°C  Heap: ${HEAP_PCT}%  Протечка: ${LEAK_S}  Дверь1: ${D1_S}  Дверь2: ${D2_S}"

    sleep 2
done