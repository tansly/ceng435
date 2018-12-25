import config

from socket import*
import threading
import struct

#Port of this server.
localServerPort = 26299 + 300

#IP of the first interface of Destination (eth1)
eth1InterfaceIP = '10.10.5.2'

#IP of the second interface of Destination (eth2)
eth2InterfaceIP = '10.10.3.2'

#Name of the file where the packets received in correct order are written to.
filename = 'payload'

seq = 0
seq_lock = threading.Lock()

class ClientThread(threading.Thread):
    def __init__(self, localIP, localPort):
        threading.Thread.__init__(self)
        self.csocket = socket(AF_INET, SOCK_DGRAM)
        self.csocket.bind((localIP, localPort))
        print ("New socket thread binded to: ", localIP, ' ', localPort)
    def run(self):
        message = ''
        while True:
            (dataReceived, senderAddr) = self.csocket.recvfrom(1000)
            (seq_received, checksum, payload) = struct.unpack('!I16s' + str(len(dataReceived) - 20) + 's', dataReceived)
            
            seq_lock.acquire()
            global seq
            if (seq_received == seq + 1):
                seq = seq_received
                with open(filename, 'ab') as file:
                    file.write(payload);
            self.csocket.sendto(struct.pack('!I16s', seq, checksum), senderAddr)
            seq_lock.release()

eth1SocketThread = ClientThread(eth1InterfaceIP, localServerPort);
eth2SocketThread = ClientThread(eth2InterfaceIP, localServerPort);

eth1SocketThread.start();
eth2SocketThread.start();

print("Server started")
print("Waiting for messages")

eth1SocketThread.join();
eth2SocketThread.join();
