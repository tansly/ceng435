#!/bin/bash

set -e

if [[ "$#" -eq 1 ]]; then
    CORRUPT="${1}"
else
    echo "${0} PERCENT"
    exit
fi

ssh r1_geni\
    "sudo tc qdisc replace dev eth1 root netem delay 3ms corrupt "${CORRUPT}"% &&\
    sudo tc qdisc replace dev eth2 root netem delay 3ms corrupt "${CORRUPT}"%"

ssh r2_geni\
    "sudo tc qdisc replace dev eth1 root netem delay 3ms corrupt "${CORRUPT}"% &&\
    sudo tc qdisc replace dev eth2 root netem delay 3ms corrupt "${CORRUPT}"%"

ssh d_geni\
    "sudo tc qdisc replace dev eth1 root netem delay 3ms corrupt "${CORRUPT}"% &&\
    sudo tc qdisc replace dev eth2 root netem delay 3ms corrupt "${CORRUPT}"%"

ssh b_geni\
    "sudo tc qdisc replace dev eth2 root netem delay 3ms corrupt "${CORRUPT}"% &&\
    sudo tc qdisc replace dev eth3 root netem delay 3ms corrupt "${CORRUPT}"%"
