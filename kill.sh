#!/bin/bash
pid=(`ps aux | grep 'daemon_server' | grep -v 'grep' | awk '{print $2}'`)
if [ ! -n "$pid"  ]
then
	echo daemon_server process not found.
else
	kill $pid
	echo daemon_server process[$pid] be killed.
fi

killall node_server

de_servers=(`ps aux | grep 'node_server' | grep -v 'grep' | awk -F" " '{print \$2;}'`)
count=${#node_servers[*]}

if [ $count -gt 0 ]
then
	for((i=0;i<$count;i++))
	do
		kill ${node_servers[$i]}
	done
else
	echo all node_server process be killed
fi

ps aux | grep node_server
