import config

from socket import *

#Port of this server.
localServerPort = 26300 + 300

#IP of Destination
destServerName = '10.10.3.2'

#Port of Destination
destServerPort = 26299 + 300

#Listen to localServerPort.
localSocket = socket(AF_INET, SOCK_DGRAM)
localSocket.bind(('10.10.2.2', localServerPort))

#Initialize destSocket and connect to Destination.
destSocket = socket(AF_INET, SOCK_DGRAM)
destSocket.connect((destServerName, destServerPort))

while True:
    (dataFBroker, addrBroker) = localSocket.recvfrom(config.msg_size)
    #Forward the sentence to Destination
    destSocket.send(dataFBroker)

localSocket.close()
destSocket.close()
