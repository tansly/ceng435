from socket import *

#IP of Destination
destServerName = '10.10.3.2'

#Port of Destination
destServerPort = 26299 + 300

#Port of this server
localServerPort = 26300 + 300

#Listen to localServerPort and record the sentence.
brokerSocket = socket(AF_INET, SOCK_DGRAM)
brokerSocket.bind(('10.10.2.2', localServerPort))

destSocket = socket(AF_INET, SOCK_DGRAM)
destSocket.connect((destServerName, destServerPort))

while True:
    (dataFBroker, addrBroker) = brokerSocket.recvfrom(128)
    print('\nReceived message:\n', dataFBroker)
    #Forward the sentence to Destination
    destSocket.send(dataFBroker.encode())
    (dataFDest, addrDest) = destSocket.recvfrom(128)
    brokerSocket.sendTo(dataFDest, addrBroker)

brokerSocket.close()
destSocket.close()
