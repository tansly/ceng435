from socket import *
import time
import string

#IP
serverName = '10.10.1.2'
serverPort = 26298 + 300

clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((serverName,serverPort))

while True:
    sentence = 128 * 'a'
    clientSocket.send(sentence.encode())
    time_sent = time.perf_counter()

    (dataFBroker, addrBroker) = clientSocket.recvfrom(128)
    time_recved = time.perf_counter()
    print(time_recved - time_sent)
