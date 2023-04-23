#!/bin/bash

myArray=("add" "subtract" "multiply" "divide")

clients_per_second=$1
client_rate=$(bc <<< "scale=2; 1/$clients_per_second")
one=1
ip=$(hostname -I)

while true
do
   operation=${myArray[$(( $RANDOM % 4))]}
   first_number=$(( $RANDOM % 10 + 1))
   second_number=$(( $RANDOM % 10 + 1))
   ./client $ip "$operation $first_number $second_number" &
   sleep $client_rate
done