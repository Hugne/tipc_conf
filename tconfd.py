#!/usr/bin/env python
import time
import sys
import BaseHTTPServer
import json
import re
from avahiservice import AvahiService
from log import err, warn, info

CONFIG_FILE = './config.json'
LEASE_FILE = './lease.json' #Whenever we give a "lease", store it here
HOST_NAME = '0.0.0.0'
PORT_NUMBER = 1337 # No

##
# HTTP request handler
#
class TconfHttpRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    def do_HEAD(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
    def do_GET(self):
        """Respond to a GET request."""
        if (self.path == "/?request_config"):
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            reply = self.server.get_config(self.client_address)
            self.wfile.write(reply)
            return
        if (self.path == "/?show_leases"):
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(self.server.lease_json, indent=4))
            return
        else:
            self.send_error(400, "Bad request")

##
# HTTP server implementation
#
class TipcConfigServer(BaseHTTPServer.HTTPServer, AvahiService):

    lease_fp = None
    lease_json = None
    config_fp = None
    config_json = None
    last_lease = None

    def initialize(self):
        if not (self.publish(name="TipcConfigService", host="", TXT={"?show_leases", "?request_config"})):
            warn('Is the AVAHI daemon running?')
            sys.exit(1)
        try:
            self.config_fp = open(CONFIG_FILE, 'r')
            self.config_json = json.load(self.config_fp)
        except:
            err('No configuration found')
            sys.exit(1)
        try:
            self.lease_fp = open(LEASE_FILE, 'r+')
            self.lease_json =json.load(self.lease_fp)
            return
        except:
            info('No leasefile found')
            self.create_leasefile()

    def create_leasefile(self):
        try:
            self.lease_fp = open(LEASE_FILE, 'w+')
            self.lease_fp.write('{"leases":[]}')
            self.lease_fp.flush()
            self.lease_fp.seek(0,0)
            self.lease_json = json.load(self.lease_fp)
            info('New leasefile created')
            return
        except IOError as e:
            err('Failed to create lease file: %s' %e.msg)
            sys.exit(1)


    def checklease(self, client_ip):
        for n in self.lease_json['leases']:
            if (n['clientip'] == client_ip):
                return n
        return None

    def writelease(self):
        self.lease_fp.seek(0)
        self.lease_fp.write(json.dumps(self.lease_json, indent=4))
        self.lease_fp.flush()

    def getaddress(self, client_address):
        client_ip = client_address[0]
        lease = self.checklease(client_ip)
        if lease:
            return lease
        if (self.last_lease):
            sa = self.last_lease
        else:
            sa = self.config_json['start']
        ea = self.config_json['end']
        if (sa == ea):
            err('Address pool exhausted!')
            return None
        re_addr = re.compile('(\d*).(\d*).(\d*)')
        tipc_addr = re_addr.search(sa).groups()
        z = int(tipc_addr[0])
        c = int(tipc_addr[1])
        n = int(tipc_addr[2])
        if self.last_lease:
            n += 1
        sa = "%d.%d.%d" % (z,c,n)
        self.last_lease = sa
        lease = '{"clientip" : "%s", "tipcaddress" : "%s"}' % (client_ip, sa)
        self.lease_json['leases'].append(json.loads(lease))
        self.writelease()
        return sa

    def getbearers(self, client_address):
        return self.config_json['bearers']

##
# Build a reply containing TIPC config in json format
#
    def get_config(self, client_address):
        reply = {}
        addr = self.getaddress(client_address)
        bearers = self.getbearers(client_address)
        reply['bearers'] = bearers
        reply['address'] = addr
        return json.dumps(reply, ensure_ascii=False, indent=4)

if __name__ == '__main__':
    httpd = TipcConfigServer((HOST_NAME, PORT_NUMBER), TconfHttpRequestHandler)
    httpd.initialize()
    info(time.asctime(), "Server started - %s:%s" % (HOST_NAME, PORT_NUMBER))
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.unpublish()
    httpd.server_close()
    info(time.asctime(), "Server stopped - %s:%s" % (HOST_NAME, PORT_NUMBER))

