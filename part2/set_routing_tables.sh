#!/bin/bash

set -e

ssh b_geni\
    "sudo ip link set eth1 down && sudo ip link set eth1 up &&\
    sudo ip link set eth2 down && sudo ip link set eth2 up &&\
    sudo ip route add 10.10.3.0/24 via 10.10.2.2 src 10.10.2.1 &&\
    sudo ip route add 10.10.5.0/24 via 10.10.4.2 src 10.10.4.1"

ssh r1_geni\
    "sudo ip link set eth1 down && sudo ip link set eth1 up &&\
    sudo ip link set eth2 down && sudo ip link set eth2 up"

ssh r2_geni\
    "sudo ip link set eth1 down && sudo ip link set eth1 up &&\
    sudo ip link set eth2 down && sudo ip link set eth2 up"

ssh d_geni\
    "sudo ip link set eth1 down && sudo ip link set eth1 up &&\
    sudo ip link set eth2 down && sudo ip link set eth2 up &&\
    sudo ip route add 10.10.2.0/24 via 10.10.3.1 src 10.10.3.2 &&\
    sudo ip route add 10.10.4.0/24 via 10.10.5.1 src 10.10.5.2"
