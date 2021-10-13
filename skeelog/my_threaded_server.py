import socket
import threading
import random
import string

class ThreadedServer(object):
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.host, self.port))

    def createFileName(self):
        rfn = ''.join(random.SystemRandom().choice(string.ascii_uppercase + string.digits) for _ in range(20))
        ext = '.bin'
        rfn += ext
        return rfn

    def listen(self):
        self.sock.listen(5)
        while True:
            client, address = self.sock.accept()
            client.settimeout(60)
            threading.Thread(target = self.listenToClient,args = (client,address)).start()

    def listenToClient(self, client, address):
        size = 15000000
        fName = self.createFileName()
        with open(fName, "wb") as out:
            while True:
                try:
                    data = client.recv(size)
                    if data:
                        out.write(bytes(data))
                    else:
                        break
                except:
                    client.close()
                    return False

if __name__ == "__main__":
    ThreadedServer('', 8080).listen()