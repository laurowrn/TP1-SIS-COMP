#!/bin/bash

myArray=("add" "subtract" "multiply" "divide")

clients_per_second=$1
client_rate=$(bc <<< "scale=2; 1/$clients_per_second")
iparray=$(hostname -I)
ip=${iparray[0]}

while true;
do
   operation=${myArray[$(( $RANDOM % 4))]}
   first_number=$(( $RANDOM % 10 + 1))
   second_number=$(( $RANDOM % 10 + 1))
   ./client $ip $operation $first_number $second_number &
   sleep $client_rate
done