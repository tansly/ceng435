#!/bin/bash

set -e

ssh r1_geni\
    "sudo tc qdisc del dev eth1 root\
    && sudo tc qdisc del dev eth2 root"

ssh r2_geni\
    "sudo tc qdisc del dev eth1 root\
    && sudo tc qdisc del dev eth2 root"

ssh d_geni\
    "sudo tc qdisc del dev eth1 root\
    && sudo tc qdisc del dev eth2 root"

ssh b_geni\
    "sudo tc qdisc del dev eth2 root\
    && sudo tc qdisc del dev eth3 root"
