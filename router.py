from socket import *

#IP of Destination
#serverNameDest = '10.10.3.2'
serverNameDest = '10.10.1.2'

#Port of Destination
#serverPortDest = 26299 + 300

serverPort = 26300 + 300
#Listen to serverPort and record the sentence.
serverSocket = socket(AF_INET, SOCK_DGRAM)
serverSocket.bind(('10.10.2.2', serverPort))

while True:
    (data, addr) = serverSocket.recvfrom(128)
    print('\nReceived message:\n', data)
    serverSocket.sendto(data, addr)	
#Forward the sentence to Destination
#clientSocket = socket(AF_INET, SOCK_DGRAM)
#clientSocket.connect((serverNameDest,serverPortDest))
#clientSocket.send(data.encode())

clientSocket.close()

