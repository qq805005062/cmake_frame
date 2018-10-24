#!/bin/sh

mode=$1

start_imbizsvr()
{
	./imbizsvr
}

stop_imbizsvr()
{
	pid=`cat imbizsvr.pid`
	kill $pid
}

restart_imbizsvr()
{
	pid=`cat imbizsvr.pid`
	kill $pid

	while true
	do
	    status=`ps -ef | grep imbizsvr | grep $pid`
	    if [ "$status" = "" ]
	    then
		#echo "start imbizsvr"
		./imbizsvr
		break
	    else
		usleep 10000
	    fi
	done
}

case "$mode" in
	start)
		start_imbizsvr
		;;
	stop)
		stop_imbizsvr
		;;
	restart)
		restart_imbizsvr
		;;
	*)
		echo "parameter error; Usage: $0 (start|stop|restart)"
		;;
esac
exit 0