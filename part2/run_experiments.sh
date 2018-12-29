#!/usr/bin/env bash

# Loss tests
#./del_tc.sh
#./set_loss.sh 0.5
#for i in {1..1}; do
#    ssh b_geni -- systemctl --user restart broker-script.service
#    ssh d_geni -- systemctl --user restart dest-script.service
#    ssh s_geni -- /usr/bin/time -f"%e" ./source.sh file 2>>./data/loss_1.txt
#done

for run in {1..11}; do
    ./del_tc.sh
    ./set_loss.sh 10
    for i in {1..1}; do
        ssh b_geni -- systemctl --user restart broker-script.service
        ssh d_geni -- systemctl --user restart dest-script.service
        ssh s_geni -- /usr/bin/time -f"%e" ./source.sh file 2>>./data/loss_2.txt
    done

    ./del_tc.sh
    ./set_loss.sh 20
    for i in {1..1}; do
        ssh b_geni -- systemctl --user restart broker-script.service
        ssh d_geni -- systemctl --user restart dest-script.service
        ssh s_geni -- /usr/bin/time -f"%e" ./source.sh file 2>>./data/loss_3.txt
    done

    # Corruption tests
    ./del_tc.sh
    ./set_corrupt.sh 0.2
    for i in {1..1}; do
        ssh b_geni -- systemctl --user restart broker-script.service
        ssh d_geni -- systemctl --user restart dest-script.service
        ssh s_geni -- /usr/bin/time -f"%e" ./source.sh file 2>>./data/corrupt_1.txt
    done

    ./del_tc.sh
    ./set_corrupt.sh 10
    for i in {1..1}; do
        ssh b_geni -- systemctl --user restart broker-script.service
        ssh d_geni -- systemctl --user restart dest-script.service
        ssh s_geni -- /usr/bin/time -f"%e" ./source.sh file 2>>./data/corrupt_2.txt
    done

    ./del_tc.sh
    ./set_corrupt.sh 20
    for i in {1..1}; do
        ssh b_geni -- systemctl --user restart broker-script.service
        ssh d_geni -- systemctl --user restart dest-script.service
        ssh s_geni -- /usr/bin/time -f"%e" ./source.sh file 2>>./data/corrupt_3.txt
    done

    # Reorder tests
    ./del_tc.sh
    ./set_reorder.sh 1
    for i in {1..1}; do
        ssh b_geni -- systemctl --user restart broker-script.service
        ssh d_geni -- systemctl --user restart dest-script.service
        ssh s_geni -- /usr/bin/time -f"%e" ./source.sh file 2>>./data/reorder_1.txt
    done

    ./del_tc.sh
    ./set_reorder.sh 10
    for i in {1..1}; do
        ssh b_geni -- systemctl --user restart broker-script.service
        ssh d_geni -- systemctl --user restart dest-script.service
        ssh s_geni -- /usr/bin/time -f"%e" ./source.sh file 2>>./data/reorder_2.txt
    done

    ./del_tc.sh
    ./set_reorder.sh 35
    for i in {1..1}; do
        ssh b_geni -- systemctl --user restart broker-script.service
        ssh d_geni -- systemctl --user restart dest-script.service
        ssh s_geni -- /usr/bin/time -f"%e" ./source.sh file 2>>./data/reorder_3.txt
    done
done

notify-admin "Test run completed"
