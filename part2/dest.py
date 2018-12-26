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

#SEQ number couter. It is used to ensure the packets are received in the right order. Unexpected packets are discarded.
seq = -1

#SEQ Lock is used to prevent different socket threads from accesing the SEQ number at the same time.
seq_lock = threading.Lock()

# Socket thread class.
class ClientThread(threading.Thread):
    # Initialize the socket thread by opening a UDP socket and binding it to an Interface.
    def __init__(self, localIP, localPort):
        threading.Thread.__init__(self)
        self.csocket = socket(AF_INET, SOCK_DGRAM)
        self.csocket.bind((localIP, localPort))
        print ("New socket thread binded to: ", localIP, ' ', localPort)
    def run(self):
        message = ''
        while True:
            # UDP socket awaits packets.
            (dataReceived, senderAddr) = self.csocket.recvfrom(config.msg_size)
            print(len(dataReceived))
            # UDP socket unpacks the received packet.
            (seq_received, checksum, payload) = struct.unpack('!I16s' + str(len(dataReceived) - 20) + 's', dataReceived)
            
            # SEQ Lock acquired to prevent other treads accesing at the same time.
            seq_lock.acquire()
            global seq
            
            # If the packet has the expected SEQ number..
            if (seq_received == seq + 1):
                seq = seq_received
                
                # If the packets payload is NULL then the file is fully received. Broker handles this.
                if (len(dataReceived) == 0):
                    print('File received')
                
                # Else write the payload to the file.
                else:
                    with open(filename, 'ab') as file:
                        file.write(payload);
        
            # Destination sends an ACK message indicating the SEQ number of the last received expected packet.
            self.csocket.sendto(struct.pack('!I16s', seq, checksum), senderAddr)
            
            # SEQ Lock is released after use.
            seq_lock.release()

# Create threads.
eth1SocketThread = ClientThread(eth1InterfaceIP, localServerPort);
eth2SocketThread = ClientThread(eth2InterfaceIP, localServerPort);

# Start threads.
eth1SocketThread.start();
eth2SocketThread.start();

print("Server started")
print("Waiting for messages")

# Join the threads so that they would keep running.
eth1SocketThread.join();
eth2SocketThread.join();
