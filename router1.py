from socket import *

#Port of this server.
localServerPort = 26300 + 300

#IP of Destination
destServerName = '10.10.3.2'

#Port of Destination
destServerPort = 26299 + 300

#Listen to localServerPort and record the sentence.
localSocket = socket(AF_INET, SOCK_DGRAM)
localSocket.bind(('10.10.2.2', localServerPort))

#Initialize brokerSocket and connect to Broker.
destSocket = socket(AF_INET, SOCK_DGRAM)
destSocket.connect((destServerName, destServerPort))

while True:
    (dataFBroker, addrBroker) = brokerSocket.recvfrom(128)
    print('\nReceived message:\n', dataFBroker)
    #Forward the sentence to Destination
    destSocket.send(dataFBroker.encode())

localSocket.close()
destSocket.close()
