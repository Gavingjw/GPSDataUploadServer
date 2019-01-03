# coding=utf-8

import time
import datetime
import pymongo
from pymongo import MongoClient
from bson.objectid import ObjectId

 
import threading
import socket
 
client = MongoClient('localhost', xxxport)
db = client.xxdb
posts = db.xxtb

encoding = 'utf-8'
BUFSIZE = 1024
 
 
# a read thread, read data from remote
class Reader(threading.Thread):
    def __init__(self, client):
        threading.Thread.__init__(self)
        self.client = client
        
    def run(self):
        while True:
            data = self.client.recv(BUFSIZE)
            print("data",data)
            if(data and  data[0:2]=="##"):
                string = bytes.decode(data, encoding)
                print(string)  #, end='')
                basedata=string[2:-4]
                datalist=basedata.split(',')
                datatime=datetime.datetime.now()
                print("datalist",datalist)
                new_posts = [{"ID":int(datalist[0]),"date": datatime,"lat":float(datalist[2])/100000,"lon":float(datalist[1])/100000,"speed":float(datalist[3])/1000,"reserve1":float(0),"reseve2":float(0),"reserve3":float(0)}]
                print("posts",new_posts)
                result = posts.insert(new_posts)
            else:
                break
        print("close:", self.client.getpeername())
        
    def readline(self):
        rec = self.inputs.readline()
        if rec:
            string = bytes.decode(rec, encoding)
            if len(string)>2:
                string = string[0:-4]
            else:
                string = ' '
        else:
            string = False
        return string

 
# a listen thread, listen remote connect
# when a remote machine request to connect, it will create a read thread to handle
class Listener(threading.Thread):
    def __init__(self, port):
        threading.Thread.__init__(self)
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((socket.gethostname(), port))
        self.sock.listen(0)
    def run(self):
        print("listener started")
        while True:
            client, cltadd = self.sock.accept()
            Reader(client).start()
            cltadd = cltadd
            print("accept a connect")
 
 
lst  = Listener(xxxx)   # create a listen thread
lst.start() # then start
 
 
# Now, you can use telnet to test it, the command is "telnet 127.0.0.1 9011"
# You also can use web broswer to test, input the address of "http://127.0.0.1:9011" and press Enter button
# Enjoy it....

