#!/bin/bash
for i in {1..2}
do
    ./worker 192.168.1.19 &
done
