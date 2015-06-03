#!/bin/bash
source ./lib/json.sh
set +e

SERV_NAME='TipcConfigService'
SERV_TYPE='_http._tcp'
CFG_REQ='/?request_config'

function resolve {
	#lolers way of resolving an mdns service
	res=`avahi-browse -vrptk $SERV_TYPE`
	if [ $? -ne 0 ]; then
		echo "avahi-browse fail" >&2
		exit 1
	fi
	srv=`echo "$res" | awk '$1 ~ /=/ && match($0, /'$SERV_NAME'/) {gsub(";"," "); print $8":"$9;}'`
	URL="http://$srv"
}

function get_config {
	res=`curl -s $URL$CFG_REQ`
	if [ $? -ne 0 ]; then
		echo "curl $URL$CFG_REQ failed" >&2
		exit 1
	fi
	IFS=$(echo -en "\n\b")
		CONFIG=`json <<< $res`
}

function configure_address {
	#Get the TIPC address and assign it, there can be only one
	for row in $CONFIG
	do
		IFS=' ' read -a param <<< "$row"
		if [ ${param[0]} == "/address/tipcaddress" ]; then
			addr=${param[2]}
		fi
	done
	echo "tipc node address set $addr"
}

function configure_bearers {
	echo "TODO"

}

resolve
get_config
configure_address
configure_bearers

