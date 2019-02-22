#!/bin/sh

for i in {0..1500}
#for i in {0..1}
do
#echo $i

declare -i flag
flag=0

phone=
content=

for line in `grep "contnets::PdSmssMsgItem index ${i}" ./ -r`
do
#echo $x
#let flag=x%38
#if [ $flag -eq 10 ]; then
#echo "$flag -eq 0"
#echo $line
#fi

#if [ $flag -eq 34 ]; then
#echo "$flag -eq 1"
#echo $line
#fi

#if [ $flag -eq 37 ]; then
#echo "$flag -eq 2"
#echo $line
#exit 0
#fi

if [ $flag -eq 1 ]; then
    phone=$line
    flag=0
fi

if [ $flag -eq 2 ]; then
    content=$line
    flag=0
    echo "$phone,$content" >> export.txt
fi

if [ $line == "phone" ]; then
    flag=1
fi

if [ $line == "content" ]; then
    flag=2
fi

#x=x+1

#echo $line
done


done
