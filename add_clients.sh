#!/bin/bash

operations=("add" "subtract" "multiply" "divide")

client_rate=$1
client_time=$(bc <<< "scale=2; 1/$client_rate")
ip=$(hostname -I | awk '{print $1}')


if [ ! -n "$client_rate" ]
then
    echo "Usage: ./add_clients <client_rate>"
else
   while true;
   do
      operation=${operations[$(( $RANDOM % 4))]}
      first_number=$(( $RANDOM % 10 + 1))
      second_number=$(( $RANDOM % 10 + 1))
      ./client $ip $operation $first_number $second_number &
      sleep $client_time
   done
fi