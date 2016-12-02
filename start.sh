#!/bin/bash

chmod 777 ./daemon_server ./node_server

./daemon_server &
#pid=(`ps aux | grep 'daemon_server' | grep -v 'grep' | awk '{print $2}'`)
#echo daemon_server started, pid[$pid]
echo daemon_server started.
