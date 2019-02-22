#!/bin/sh

declare -i conIndex
declare -i flag
declare -i init

init=0
flag=0
conIndex=0
indexstr=
phone=
content=

for line in `grep "contnets::PdSmssMsgBtItem index" ./ -r`
do

if [ $flag -eq 1 ]; then
    indexstr=$line
    flag=0
    if [ $indexstr == "0" ]; then
	if [ $init -eq 0 ]; then
	    init=1
	else
	    conIndex=conIndex+1
	fi
	
	declare -i cFlag
	declare -i cIndex
	cIndex=0
	cFlag=0
	for cLine in `grep "contnets::MessageBtPack" ./ -r`
	do
	    
	    if [ $cFlag -eq 1 ]; then
		if [ $cIndex -eq $conIndex ]; then
			content=$cLine
			break
		fi
		cIndex=cIndex+1
		cFlag=0
	    fi

	    if [ $cLine == "content" ]; then
    		cFlag=1
	    fi
	done
	#echo $content
    fi
fi

if [ $flag -eq 2 ]; then
    phone=$line
    flag=0
    echo "$phone,$content" >> export.txt
fi

if [ $line == "index" ]; then
    flag=1
fi

if [ $line == "phone" ]; then
    flag=2
fi


done

