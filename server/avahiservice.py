import avahi
import dbus
from dbus.exceptions import DBusException
from log import err, warn, info

class AvahiService:

    def publish(self, name, servicetype="_http._tcp", TXT="", host="", port=1337,
                domain="local", af=avahi.PROTO_INET):
        self.name = name
        self.servicetype = servicetype
        self.TXT = TXT
        self.host = host
        self.port = port
        self.domain = domain

        try:
            bus = dbus.SystemBus()
            server = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
                                    avahi.DBUS_PATH_SERVER),
                                    avahi.DBUS_INTERFACE_SERVER)
            group = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
                                   server.EntryGroupNew()),
                                   avahi.DBUS_INTERFACE_ENTRY_GROUP)
            group.AddService(avahi.IF_UNSPEC, af, dbus.UInt32(0),
                             self.name, self.servicetype, self.domain, self.host,
                             dbus.UInt16(self.port), self.TXT)
            group.Commit()
            self.group = group
        except DBusException as e:
            err('DBUS error %s' % e.get_dbus_message())
            return False
        info('Service %s published in %s domain)' % (self.name, self.domain))
        return True

    def unpublish(self):
        self.group.Reset()
        info('Service %s withdrawn from %s)' % (self.name, self.domain))
