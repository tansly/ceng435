> 2171429 Doruk Coşkun
> 2171783 Yağmur Oymak

# CENG435 Term Project Part 1

## How to run

-Copy each script to the relevant node. Copy config.py to every node, it will be imported in python scripts. A shell script can be used to automate this task:
```
#!/bin/sh

scp ./source.py ./config.py s_geni:
scp ./router1.py ./config.py  r1_geni:
scp ./router2.py ./config.py r2_geni:
scp ./dest.py ./config.py d_geni:
scp -r ./broker/ b_geni:
```
Note that one has to define the hosts (s\_geni, r1\_geni...) in their ```~/.ssh/config``` file in order for this script to work.

-Nodes are synchronised using NTP. You can find more detailed information in the report.

```
> sudo ntpdate −s time.nist.gov
```

-To set the netem/tc delays we use the following script. To use it you have set your ssh config accordingly. Apart from that, you can see the shell commands inside the script.

```
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
```

Also, following script was used to remove all netem delays from all links:
```
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
