> 2171429 Doruk Coşkun
> 2171783 Yağmur Oymak

# CENG435 Term Project Part 1

## How to run

-Copy each script to the relevant node. Copy config.py to every node, it will be imported in python scripts.

-Nodes are synchronised using NTP. You can find more detailed information in the report.

```
> sudo ntpdate −s time.nist.gov
```

-To set the netem/tc delays we use the following script. To use it you have set your ssh config accordingly. Apart from that, you can see the shell commands inside the script.

```
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
```

-After all configuration explained above are done, we start the broker first, the routers and the destination next, and finally the source.

Broker is written in C and thus has to be compiled first. 

```
>cd ./Broker
>make
>./broker
```

Then run the router and destination scripts, and finally run the source script to start the measurement.

```
>python3 _name_.py 
```

**Note:** While at Source, ```>python3 source.py pl_test``` runs the packet loss test, where the source sends 1000, 8 byte packets consecutively. You can observe the result in Destination by looking at the index of the packets that arrived. More detailed analysis of packet loss test is written in the report.
