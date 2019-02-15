#!/bin/bash
echo "running silvercheetah..."
ls ./csv/* | xargs -n1 silvercheetah
echo "sorting tss.log"
sort -nu ./tss.log > ./tss.temp
cat ./tss.temp > ./tss.log
rm ./tss.temp
echo "running freshcheetah..."
./freshcheetah ./tss.log
echo "done"
