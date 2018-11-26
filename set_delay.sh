#!/bin/sh

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

ssh -p26301 yagmuroy@pc2.instageni.ku.gpeni.net\
    "sudo tc qdisc replace dev eth1 root netem delay ${DELAY} ${JITTER} distribution normal\
    && sudo tc qdisc replace dev eth2 root netem delay ${DELAY} ${JITTER} distribution normal"

ssh -p26300 yagmuroy@pc2.instageni.ku.gpeni.net\
    "sudo tc qdisc replace dev eth1 root netem delay ${DELAY} ${JITTER} distribution normal\
    && sudo tc qdisc replace dev eth2 root netem delay ${DELAY} ${JITTER} distribution normal"

ssh -p26299 yagmuroy@pc2.instageni.ku.gpeni.net\
    "sudo tc qdisc replace dev eth1 root netem delay ${DELAY} ${JITTER} distribution normal\
    && sudo tc qdisc replace dev eth2 root netem delay ${DELAY} ${JITTER} distribution normal"

ssh -p26298 yagmuroy@pc2.instageni.ku.gpeni.net\
    "sudo tc qdisc replace dev eth2 root netem delay ${DELAY} ${JITTER} distribution normal\
    && sudo tc qdisc replace dev eth3 root netem delay ${DELAY} ${JITTER} distribution normal"
