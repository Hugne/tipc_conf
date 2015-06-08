#!/bin/bash
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$DIR/lib/json.sh"
set +e

SERV_NAME='TipcConfigService'
SERV_TYPE='_http._tcp'
CFG_REQ='/?request_config'

resolve () {
	#lolers way of resolving an mdns service
	res=`avahi-browse -vrptk $SERV_TYPE`
	if [ $? -ne 0 ]; then
		echo "avahi-browse fail" >&2
		exit 1
	fi
	srv=`echo "$res" | awk '$1 ~ /=/ && match($0, /'$SERV_NAME'/) {gsub(";"," "); print $8":"$9;}'`
	URL="http://$srv"
}

get_config () {
	res=`curl -s $URL$CFG_REQ`
	if [ $? -ne 0 ]; then
		echo "curl $URL$CFG_REQ failed" >&2
		exit 1
	fi
	IFS=$(echo -en "\n\b")
		CONFIG=`json <<< $res`
}

get_bearer_param () {
	idx=$1
	param=$2
	for row in $CONFIG
	do
		IFS=' ' read -a param <<< "$row"
		if [ ${param[0]} == "/bearers/"$1"/$2" ]; then
			echo ${param[2]}
			return
		fi
	done
}

configure_address () {
	#Get the TIPC address and assign it, there can be only one
	for row in $CONFIG
	do
		IFS=' ' read -a param <<< "$row"
		if [ ${param[0]} == "/address/tipcaddress" ]; then
			addr=${param[2]}
		fi
	done
	echo "tipc node set addr $addr"
}

enable_eth () {
	echo "Configure Ethernet media $1"
	idx=$1
	interface=$(get_bearer_param $idx "interface")
	`tipc bearer enable media eth device $interface`
}
enable_udp () {
	echo "Configure UDP media $1"
	idx=$1
	ip=$(get_bearer_param $idx "localip")
	name=$(get_bearer_param $idx "name")
	`tipc bearer enable media udp name $name localip $ip`
}


configure_bearers () {
	i=0
	for row in $CONFIG
	do
		IFS=' ' read -a param <<< "$row"
		if [ ${param[0]} == "/bearers/"$i"/media" ]; then
			if [ ${param[2]} == "eth" ];then
				enable_eth $i
				((i+=1))

			elif [ ${param[2]} == "udp" ];then
				enable_udp $i
				((i+=1))
			fi
		fi
	done

}

resolve
get_config
configure_address
configure_bearers

