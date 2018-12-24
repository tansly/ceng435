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
