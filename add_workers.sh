#!/bin/bash

iparray=$(hostname -I)
ip=${iparray[0]}

workers_amount=$1

if [ ! -n "$workers_amount" ]
then
    echo "Usage: ./add_workers <workers_amount>"
else
    for ((i = 1; i <= $workers_amount; i++));
    do
        ./worker $ip &
    done
fi