#!/usr/bin/python

import socket
import struct
import consulate


MCAST_GRP = '239.1.10.10'
MCAST_PORT = 9000


def notify_device(device_id,version_buf,local_ip,netmask,gateway_ip):
  print "Device(id=%s,v=%s,ip=%s/%s,gateway=%s)" % (device_id,version_buf,local_ip,netmask,gateway_ip)
	
  # register
  consul = consulate.Consul()
  consul.agent.service.register(device_id,address=local_ip);


def main():
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

  sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

  sock.bind((MCAST_GRP, MCAST_PORT))
  mreq = struct.pack("4sl", socket.inet_aton(MCAST_GRP), socket.INADDR_ANY)

  sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
  
  print "Listening on %s:%s, hit CTRL-C to stop..." % (MCAST_GRP, MCAST_PORT)

  while True:
    data = sock.recv(64)
    local_ip = 	"%d.%d.%d.%d" % (ord(data[0]),ord(data[1]),ord(data[2]),ord(data[3]))
    netmask = 	"%d.%d.%d.%d" % (ord(data[4]),ord(data[5]),ord(data[6]),ord(data[7]))
    gateway_ip = 	"%d.%d.%d.%d" % (ord(data[8]),ord(data[9]),ord(data[10]),ord(data[11]))

    version_buf = (ord(data[12]) << 24 | ord(data[13]) << 16 | ord(data[14]) << 8 | ord(data[15]))

    device_id = data[16:16+24]

    # notify consul about this new device
    notify_device(device_id,version_buf,local_ip,netmask,gateway_ip)


if __name__ == "__main__":
	main()

