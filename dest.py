import config

from socket import*
import time

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
routerSocket.connect((routerServerName, routerServerPort))

while True:
    (dataFRouter, addrRouter) = localSocket.recvfrom(config.msg_size)
    sent_ts = float(dataFRouter.decode())
    curr_ts = time.time()*1000
    print(dataFRouter.decode() + '\t' + str(curr_ts - sent_ts))
    # Create return sentence.
    routerSocket.send(dataFRouter)

localSocket.close()
routerSocket.close()
