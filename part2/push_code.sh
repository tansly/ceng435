#!/bin/sh

scp ./source.py ./config.py s_geni:
scp ./dest.py ./config.py d_geni:
scp -r ./broker/ b_geni:
