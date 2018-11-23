from socket import*

#Port of this server.
localServerPort = 26299 + 300

#IP of router2
routerServerName = '10.10.4.2'

#Port of router2.
routerServerPort = 26301 + 300

#Initialize localSocket and listen to localServerPort.
localSocket = socket(AF_INET, SOCK_DGRAM)
localSocket.bind(('10.10.3.2', localServerPort))

#Initialize routerSocket and connect to router2.
routerSocket = socket(AF_INET, SOCK_DGRAM)
routerSocket.connect(routerServerName, routerServerPort)

while True:
    (dataFRouter, addrRouter) = localSocket.recvfrom(128)
    print('\nReceived message:\n', dataFRouter)
    #Create return sentence.
    routerSocket.send(dataFRouter.upper().encode())

localSocket.close()
routerSocket.close()
