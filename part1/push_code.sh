#!/bin/sh

scp ./source.py ./config.py s_geni:
scp ./router1.py ./config.py  r1_geni:
scp ./router2.py ./config.py r2_geni:
scp ./dest.py ./config.py d_geni:
scp -r ./broker/ b_geni:
