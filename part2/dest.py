import config

from socket import*
import threading

#Port of this server.
localServerPort = 26299 + 300

#IP of first interface of Destination (eth1)
eth1InterfaceIP = '10.10.5.2'

#IP of first interface of Destination (eth2)
eth2InterfaceIP = '10.10.3.2'

seq = 0
seq_lock = threading.Lock()

class ClientThread(threading.Thread):
    def __init__(self, localIP, localPort):
        threading.Thread.__init__(self)
        self.csocket = socket(AF_INET, SOCK_DGRAM)
        self.csocket.bind((localIP, localPort))
        print ("New socket thread binded to: ", localIP, ' ', localPort)
    def run(self):
        #self.csocket.send(bytes("Hi, This is from Server..",'utf-8'))
        message = ''
        while True:
            (dataReceived, senderAddr) = self.csocket.recvfrom(1000)
            message = dataReceived.decode()
            seq_lock.acquire()
            #Some fuckery
            if (message[:4])
                
            print ("Message received: ", message)
            ACKMessage = "Selam"
            self.csocket.sendto(bytes(ACKMessage), senderAddr)

eth1SocketThread = ClientThread(eth1InterfaceIP, localServerPort);
eth2SocketThread = ClientThread(eth2InterfaceIP, localServerPort);

eth1SocketThread.start();
eth2SocketThread.start();

eth1SocketThread.join();
eth2SocketThread.join();

print("Server started")
print("Waiting for messages")






