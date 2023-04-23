#!/bin/bash

myArray=("add" "subtract" "multiply" "divide")

#for i in {1..1000}
#do
#    operation=${myArray[$(( $RANDOM % 4))]}
#    first_number=$(( $RANDOM % 10))
#    second_number=$(( $RANDOM % 10))
#    ./client 192.168.1.19 "$operation $first_number $second_number" &
#done

clients_per_second = $1
one = 1

while true
do
    operation=${myArray[$(( $RANDOM % 4))]}
    first_number=$(( $RANDOM % 10 + 1))
    second_number=$(( $RANDOM % 10 + 1))
    ./client 192.168.1.19 "$operation $first_number $second_number" &
    sleep `expr $one/$clients_per_second `
done