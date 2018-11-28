> 2171429 Doruk Coşkun
> Yağmur Oymak

# CENG435 Term Project Part 1

## How to run

Copy each script and the config.py to the relevant node and run the scripts except the Broker using Python3.

```
>python3 _name_.py 
```
Broker is written in C and thus has to be compiled first. 

```
>cd ./Broker
>make
>./raspiped.out 
```

**Note:** While at Source, ```>python3 source.py pl_test``` runs the packet loss test, where the source sends 1000, 8 byte packets consecutively. You can observe the result in Destination by looking at the index of the packets that arrived.


