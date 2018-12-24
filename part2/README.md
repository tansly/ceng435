> 2171429 Doruk Coşkun

> 2171783 Yağmur Oymak

# CENG435 Term Project Part 2

## Routing Tables
Initially the broker, routers and destination nodes had some routing entries present
in the routing tables, such as:

```
yagmuroy@r1:~$ ip route
default via 172.16.0.1 dev eth0
10.0.0.0/8 via 10.10.3.2 dev eth2
10.10.0.0/22 via 10.10.2.1 dev eth1
10.10.2.0/24 dev eth1  proto kernel  scope link  src 10.10.2.2
10.10.3.0/24 dev eth2  proto kernel  scope link  src 10.10.3.1
10.10.4.0/31 via 10.10.2.1 dev eth1
172.16.0.0/12 dev eth0  proto kernel  scope link  src 172.17.1.7
```

```
yagmuroy@b:~$ ip route
default via 172.16.0.1 dev eth0
10.0.0.0/8 via 10.10.2.2 dev eth2
10.10.1.0/24 dev eth1  proto kernel  scope link  src 10.10.1.2
10.10.2.0/24 dev eth2  proto kernel  scope link  src 10.10.2.1
10.10.4.0/24 dev eth3  proto kernel  scope link  src 10.10.4.1
10.10.4.0/22 via 10.10.4.2 dev eth3
10.10.5.2/31 via 10.10.2.2 dev eth2
172.16.0.0/12 dev eth0  proto kernel  scope link  src 172.17.1.5
```

We did not desire the behaviour such as
```
10.0.0.0/8 via 10.10.2.2
10.10.4.0/22 via 10.10.4.2
10.10.5.2/31 via 10.10.2.2
```
etc. so we decided to remove such entries to implement our own routing logic.
We could delete all such entries using `ip route del ...`, however since those
entries were not default entries (who adds them, some configuration scripts?)
bringing the interfaces down and up one by one with commands like
`ip link set eth1 down && ip link set eth1 up` (for all hosts and interfaces)
did the trick. Then we had a clean slate to create our routing tables from scratch.
Following script was used to achieve all that.
```
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
```

After that, we tested the routing tables by pinging the destination from the broker:
```
yagmuroy@b:~$ ping -c 3 10.10.3.2
PING 10.10.3.2 (10.10.3.2) 56(84) bytes of data.
64 bytes from 10.10.3.2: icmp_seq=1 ttl=63 time=1.12 ms
64 bytes from 10.10.3.2: icmp_seq=2 ttl=63 time=1.03 ms
64 bytes from 10.10.3.2: icmp_seq=3 ttl=63 time=1.11 ms

--- 10.10.3.2 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2002ms
rtt min/avg/max/mdev = 1.030/1.089/1.124/0.056 ms
yagmuroy@b:~$ ping -c 3 10.10.5.2
PING 10.10.5.2 (10.10.5.2) 56(84) bytes of data.
64 bytes from 10.10.5.2: icmp_seq=1 ttl=63 time=1.05 ms
64 bytes from 10.10.5.2: icmp_seq=2 ttl=63 time=0.889 ms
64 bytes from 10.10.5.2: icmp_seq=3 ttl=63 time=0.996 ms

--- 10.10.5.2 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2002ms
rtt min/avg/max/mdev = 0.889/0.981/1.058/0.069 ms
```
