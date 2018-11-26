#!/bin/sh

set -e

ssh -p26301 yagmuroy@pc2.instageni.ku.gpeni.net\
    "sudo tc qdisc del dev eth1 root\
    && sudo tc qdisc del dev eth2 root"

ssh -p26300 yagmuroy@pc2.instageni.ku.gpeni.net\
    "sudo tc qdisc replace dev eth1 root\
    && sudo tc qdisc del dev eth2 root"

ssh -p26299 yagmuroy@pc2.instageni.ku.gpeni.net\
    "sudo tc qdisc replace dev eth1 root\
    && sudo tc qdisc del dev eth2 root"

ssh -p26298 yagmuroy@pc2.instageni.ku.gpeni.net\
    "sudo tc qdisc del dev eth2 root\
    && sudo tc qdisc del dev eth3 root"
