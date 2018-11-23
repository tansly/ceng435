from socket import *
from random import randint
serverName = '10.10.1.2'
serverPort = 26298 + 300
clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((serverName,serverPort))
index = 1
sentence = chr(index) + 124 * 'a'
clientSocket.send(sentence.encode())
clientSocket.close()
