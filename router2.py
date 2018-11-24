import config

from socket import *

#Port of this server.
localServerPort = 26301 + 300

#IP of Broker.
brokerServerName = '10.10.1.2'

#Port of Broker.
brokerServerPort = 26298 + 300

#Initialize localSocket and listen to localServerPort.
localSocket = socket(AF_INET, SOCK_DGRAM)
localSocket.bind(('10.10.4.2', localServerPort))

#Initialize brokerSocket and connect to Broker.
brokerSocket = socket(AF_INET, SOCK_DGRAM)
brokerSocket.connect((brokerServerName, brokerServerPort))

while True:
    (dataFDest, addrDest) = localSocket.recvfrom(config.msg_size)
    #print('\nReceived message:\n', dataFDest)
    #Forward the sentence to Destination
    brokerSocket.send(dataFDest)

localSocket.close()
brokerSocket.close()
