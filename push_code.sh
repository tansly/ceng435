#!/bin/sh

scp -P26302 -r ./source.py ./config.py  yagmuroy@pc2.instageni.ku.gpeni.net:
scp -P26301 -r ./router2.py ./config.py  yagmuroy@pc2.instageni.ku.gpeni.net:
scp -P26300 -r ./router1.py ./config.py  yagmuroy@pc2.instageni.ku.gpeni.net:
scp -P26299 -r ./dest.py ./config.py  yagmuroy@pc2.instageni.ku.gpeni.net:
scp -P26298 -r ./raspipe/raspiped/  yagmuroy@pc2.instageni.ku.gpeni.net:
