import config

from socket import *
import time
import string

#IP
serverName = '10.10.1.2'
serverPort = 26298 + 300

clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((serverName,serverPort))

i = 0
while True:
    clientSocket.send((('%' + str(config.msg_size) + 's') % i).encode())
    time_sent = time.perf_counter()

    (dataFBroker, addrBroker) = clientSocket.recvfrom(config.msg_size)
    time_recved = time.perf_counter()

    print(dataFBroker.decode())
    print(time_recved - time_sent)

    if i >= 10**config.msg_size:
        i = 0
    else:
        i += 1
