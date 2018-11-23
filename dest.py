from socket import*

#Port of this server
localServerPort = 26299 + 300

#Listen to localServerPort and record the sentence.
destSocket = socket(AF_INET, SOCK_DGRAM)
destSocket.bind(('10.10.3.2', localServerPort))

while True:
    (dataFRouter, addrRouter) = destSocket.recvfrom(128)
    print('\nReceived message:\n', dataFRouter)
    #Create return sentence.
    dataFRouter.upper()
    destSocket.sendTo(dataFRouter, addrRouter)

destSocket.close()
