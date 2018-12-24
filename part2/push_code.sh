#!/bin/sh

scp ./source.sh s_geni:
scp ./dest.py ./config.py d_geni:
scp -r ./broker/ b_geni:
