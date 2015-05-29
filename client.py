#!/usr/bin/env python
import sys
import json
import urllib2
from subprocess import call, check_call,STDOUT

#This should ideally use pymnl instead of relying on
#an external program..

def set_address(addr):
    cmd = "node set address %s" %addr
    rc = check_call(["tipc", cmd], shell=False, stderr=STDOUT)


def enable_bearer(b):
    #TODO: support all config options and bearer types
    cmd = "bearer enable media %s interface %s" %(b['media'],b['interface'])
    rc = check_call(["tipc", cmd], shell=False, stderr=STDOUT)


if __name__ == '__main__':
    try:
        response = urllib2.urlopen('http://localhost:1337/?request_config')
        html = response.read()
    except (urllib2.URLError, urllib2.HTTPError) as e:
        print "Failed to fetch config: %s" % e.msg
        sys.exit(1)

    print 'Response: %s \n\n' %html
    config = json.loads(html)
    for b in config['bearers']:
        print b
        enable_bearer(b)

