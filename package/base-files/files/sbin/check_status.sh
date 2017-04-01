#! /bin/sh
#Check config var streamboost_enable every hour
while :; do
	sleep 60
	streamboost_enable=`config get streamboost_enable`
	if [ "x$streamboost_enable" = "x1" ];then
		streamboost status > /tmp/streamboost_status
		status=`cat /tmp/streamboost_status |grep DOWN`
		if [ "x$status" != "x" ]; then
			/etc/init.d/streamboost restart
			time=`date '+%Y-%m-%dT%H:%M:%SZ'`
			echo "Restart streamboost:$time" >> /tmp/restart_process_list
		fi
	fi
done

