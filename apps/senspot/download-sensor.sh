#!/bin/bash

HERE=/root/bin

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

if ( test -z "$1" ) ; then
	echo "download-sensor.sh must specify target woof name"
	echo "download-sensor woof_name IP-addre-of-iperf3-server"
	exit 1
fi

WNAME=$1

if ( test -z "$2" ) ; then
	echo "download-sensor.sh must specify target server"
	echo "download-sensor woof_name IP-addre-of-iperf3-server"
	exit 1
fi

ADDR=$2


AVG=`/usr/bin/iperf3 -c $ADDR -R -p 8008 | grep receiver | awk '{print $7}'`

## third argument can be "-d" for debugging

if ( ! test -z "$3" ) ; then
	if ( test "$3" = "-d" ) ; then
		echo $AVG | $HERE/senspot-put -W $WNAME -H senspot_log -T d
		$HERE/senspot-get -W $WNAME
	else
		echo $AVG | $HERE/senspot-put -W $WNAME -T d
	fi
else
	echo $AVG | $HERE/senspot-put -W $WNAME -T d
fi
		
		
