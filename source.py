from socket import *
import datetime

#IP
serverName = '10.10.1.2'
serverPort = 26298 + 300

clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((serverName,serverPort))

sentTimeList = []
returnTimeList = []

for index in range(0, 9):
    sentence = str(index) + 127 * 'a'
    clientSocket.send(sentence.encode())
    sentTimeList.append(datetime.datetime.now().time())

flag = True;


while flag:
    (dataFBroker, addrBroker) = clientSocket.recvfrom(128)
    returnTimeList.append(datetime.datetime.now().time() - sentTimeList[dataFBroker.decode()[0]])
    if (size(returnTimeList) == 10):
        flag = false;

print(returnTimeList)
clientSocket.close()

