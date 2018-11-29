#!/bin/bash

set -e

if [[ "$#" -eq 1 ]]; then
    DELAY="${1}"
    JITTER=5ms
elif [[ "$#" -eq 2 ]]; then
    DELAY="${1}"
    JITTER="${2}"
else
    echo "${0} DELAY [JITTER]"
    exit
fi

ssh r1_geni\
    "sudo tc qdisc replace dev eth1 root netem delay ${DELAY} ${JITTER} distribution normal\
    && sudo tc qdisc replace dev eth2 root netem delay ${DELAY} ${JITTER} distribution normal"

ssh r2_geni\
    "sudo tc qdisc replace dev eth1 root netem delay ${DELAY} ${JITTER} distribution normal\
    && sudo tc qdisc replace dev eth2 root netem delay ${DELAY} ${JITTER} distribution normal"

ssh d_geni\
    "sudo tc qdisc replace dev eth1 root netem delay ${DELAY} ${JITTER} distribution normal\
    && sudo tc qdisc replace dev eth2 root netem delay ${DELAY} ${JITTER} distribution normal"

ssh b_geni\
    "sudo tc qdisc replace dev eth2 root netem delay ${DELAY} ${JITTER} distribution normal\
    && sudo tc qdisc replace dev eth3 root netem delay ${DELAY} ${JITTER} distribution normal"
