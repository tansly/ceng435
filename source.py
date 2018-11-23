from socket import *
from random import randint
serverName = '10.10.1.2'
serverPort = 26298 + 300
clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((serverName,serverPort))
index = 1
sentence = str(index) + 127 * 'a'
clientSocket.send(sentence.encode())
clientSocket.close()

