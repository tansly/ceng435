> 2171429 Doruk Coşkun
> 2171783 Yağmur Oymak

# CENG435 Term Project Part 1

## How to run

Copy each script and the config.py to the relevant node.

```
Nodes are synchronised using NTP.
> sudo ntpdate −s time.nist.gov
```

After all configuration explained above are done, we start the broker first,
the routers and the destination next, and finally the source.

Compile and run the broker, then run the router and destination scripts, and finally
run the source script to start the measurement.
```
>python3 _name_.py 
```
Broker is written in C and thus has to be compiled first. 

```
>cd ./Broker
>make
>./broker
```

**Note:** While at Source, ```>python3 source.py pl_test``` runs the packet loss test, where the source sends 1000, 8 byte packets consecutively. You can observe the result in Destination by looking at the index of the packets that arrived.

In order to set delays on each node, we've used the following shell script:

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
