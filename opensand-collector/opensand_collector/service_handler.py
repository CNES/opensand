"""
OpenSAND collector Avahi service handler.
"""

from dbus.mainloop.glib import DBusGMainLoop
import avahi
import dbus
import logging

LOGGER = logging.getLogger('service_handler')

class ServiceHandler(object):
    """
    Avahi service handler to publish the collector listening port (so the
    daemons can find it) and find the OpenSAND daemons and their IPs.
    """

    def __init__(self, collector, listen_port, transfer_port, service_type):
        self.collector = collector
        self.listen_port = listen_port
        self.transfer_port = transfer_port
        self.service_type = service_type
        self._pub_group = None
        self._disco_server = None
        self._known_hosts = set()

    def __enter__(self):
        """
        Publish the service and set up service discovery
        """

        bus = dbus.SystemBus(mainloop=DBusGMainLoop())

        # Publishing
        pub_server = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
            avahi.DBUS_PATH_SERVER), avahi.DBUS_INTERFACE_SERVER)

        self._pub_group = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
            pub_server.EntryGroupNew()), avahi.DBUS_INTERFACE_ENTRY_GROUP)

        additional_data = ["transfer_port=%d" % self.transfer_port]

        self._pub_group.AddService(avahi.IF_UNSPEC, avahi.PROTO_UNSPEC,
            dbus.UInt32(0), "collector", self.service_type, "", "",
            dbus.UInt16(self.listen_port), additional_data)

        self._pub_group.Commit()

        # Discovery
        self._disco_server = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
            '/'), 'org.freedesktop.Avahi.Server')

        disco_browser = dbus.Interface(bus.get_object(avahi.DBUS_NAME,
            self._disco_server.ServiceBrowserNew(avahi.IF_UNSPEC,
                avahi.PROTO_UNSPEC, self.service_type, 'local',
                dbus.UInt32(0))), avahi.DBUS_INTERFACE_SERVICE_BROWSER)

        disco_browser.connect_to_signal("ItemNew", self._handle_new)
        disco_browser.connect_to_signal("ItemRemove", self._handle_remove)

        LOGGER.info("Avahi service handler started.")

        return self

    def _handle_new(self, interface, protocol, name, stype, domain, flags):
        """
        Handles a newly detected service
        """
    
        if name == "collector":
            return

        def error_handler(*args):
            LOGGER.error("Unable to get resolve service %s, ignoring.", name)
            return

        self._disco_server.ResolveService(interface, protocol, name, stype,
            domain, avahi.PROTO_UNSPEC, dbus.UInt32(0),
            reply_handler=self._handle_resolve, error_handler=error_handler)

    def _handle_resolve(self, *args):
        """
        Called when a detected service is resolved
        """
    
        name = args[2]
        addr = args[7]
        txt = args[9]

        try:
            items = dict("".join(chr(i) for i in arg).split("=", 1)
                for arg in txt)
            port = int(items.get('ext_port', ""))
        except ValueError:
            LOGGER.error("Failed to get UDP port from '%s' daemon.", name)
            return

        if name in self._known_hosts:
            self.collector.host_manager.add_host_addr(name, (addr, port))
            return

        LOGGER.debug("Daemon on host '%s' has address %s:%d.", name, addr, port)
        self._known_hosts.add(name)
        self.collector.host_manager.add_host(name, (addr, port))

    def _handle_remove(self, interface, protocol, name, stype, domain, flags):
        """
        Called when a service is removed
        """
    
        if name == "collector":
            return

        LOGGER.debug("Daemon on host '%s' has exited.", name)
        self.collector.host_manager.remove_host(name)
        self._known_hosts.discard(name)

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Unpublish the service and exit the event loop
        """

        self._pub_group.Reset()

        LOGGER.info("Avahi service handler stopped.")

        return False
