from socket import *

#IP of Broker.
brokerServerName = '10.10.1.2'

#Port of Broker.
brokerServerPort = 26298 + 300

#Initialize brokerSocket and connect to Broker.
brokerSocket = socket(AF_INET, SOCK_STREAM)
brokerSocket.connect((brokerServerName, brokerServerPort))

#Loop
index = 1
sentence = chr(index) + 124 * 'a'
clientSocket.send(sentence.encode())
clientSocket.close()
