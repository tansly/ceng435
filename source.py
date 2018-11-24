from socket import *
import time
import string

#IP
serverName = '10.10.1.2'
serverPort = 26298 + 300
msgSize = 8

clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((serverName,serverPort))

i = 0
while True:
    sentence = msgSize * string.ascii_lowercase[i]
    i = (i + 1) % len(string.ascii_lowercase)
    clientSocket.send(sentence.encode())
    time_sent = time.perf_counter()

    (dataFBroker, addrBroker) = clientSocket.recvfrom(msgSize)
    time_recved = time.perf_counter()
    print(dataFBroker.decode())
    print(time_recved - time_sent)
