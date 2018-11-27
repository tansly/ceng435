import config
from socket import *
import time
import string
import sys

#IP and port of Broker.
serverName = '10.10.1.2'
serverPort = 26298 + 300

#Create client socket and open TCP connection to Broker.
clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((serverName,serverPort))

#If source.py is run with parameter 'pl_test' then you can observe the result of the Packet Loss Test.
#Packet Loss Test sends 1000, 8 byte packages wihtout waiting for any packet to return.
if (len(sys.argv) > 1 and sys.argv[1] == 'pl_test'):
    for i in range(0, min(1000, 10**config.msg_size)):
        msg = ('%' + str(config.msg_size) + 's') % i
        clientSocket.send(msg.encode())
        print(msg)

#Otherwise you can see the packet delay results. Each packet is sent right after the previous ones feedback comes.
else:
    i = 0
    while True:
        msg = str(1000*time.time())[:16]
        clientSocket.send((('%' + str(config.msg_size) + 's') % msg).encode())
        time_sent = time.perf_counter()
    
        (dataFBroker, addrBroker) = clientSocket.recvfrom(config.msg_size)
        time_recved = time.perf_counter()
    
        print(dataFBroker.decode() + '\t' + str(1000*(time_recved - time_sent)))
    
        if i >= 10**config.msg_size:
            i = 0
        else:
            i += 1
