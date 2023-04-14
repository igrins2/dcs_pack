# -*- coding: utf-8 -*-

"""
Created on Nov 22, 2022

Modified on Dec 15, 2022

@author: hilee, Francisco
"""

import pika
import threading
import time as ti

# RabbitMQ communication
class MsgMiddleware():
    
    def __init__(self, iam, ip_addr, id, pwd, exchange):
        
        self.iam = iam       
        self.ip_addr = ip_addr
        self.id = id
        self.pwd = pwd
        
        self.exchange = exchange
        self.queue = None
                
        self.channel = None
        self.connection = None
                
        
    def __del__(self):
        
        for th in threading.enumerate():
            print(th.name + f"{th.name} exit.")
            
        print(f"'{self.iam}' closed rabbitmq queue and connections")
      
        try:
            if self.connection.is_open:
                self.connection.close()
        except Exception as e:
            raise
                                   

    def connect_to_server(self):
        try:       
            id_pwd = pika.PlainCredentials(self.id, self.pwd)
            self.connection = pika.BlockingConnection(pika.ConnectionParameters(host=self.ip_addr, port=5672, credentials=id_pwd, heartbeat=0, tcp_options={"TCP_KEEPIDLE":60}))
            self.channel = self.connection.channel()
                        
        except Exception as e:
            print(f"'{self.iam}' cannot connect to RabbitMQ server.\r\nPlease check the server and try again!")
            raise 

        
    # as producer
    def define_producer(self):
        try:
            self.channel.exchange_declare(exchange=self.exchange, exchange_type="direct")
                    
        except Exception as e:
            print(f"'{self.iam}' cannot define producer.\r\nPlease check the server and try again!")
            raise
        
    
    def send_message(self, _routing_key, _message): 
        try:
            self.channel.basic_publish(exchange=self.exchange, routing_key=_routing_key, body=_message.encode())
            
        except Exception as e:
            print(f"Cannot send the '{_message}' for the '{_routing_key}' provider.")
            raise
    
    
    # as consumer
    def define_consumer(self, _routing_key, _callback):        
        try:
            if self.queue is None:
                self.channel.exchange_declare(exchange=self.exchange, exchange_type="direct")
                result = self.channel.queue_declare(queue='', durable=True, exclusive=True)
                self.queue = result.method.queue
                print(f"queueName= '{self.queue}'")
            
            #for _key in _routing_key:
            self.channel.queue_bind(exchange=self.exchange, queue=self.queue, routing_key=_routing_key)
            self.channel.basic_consume(queue=self.queue, on_message_callback=_callback, auto_ack=True)

        except Exception as e:
            print(f"'{self.iam}' cannot define consumer for the '{_routing_key}' provider.")      
            raise 
                
        
    def start_consumer(self):
        try:
            if self.queue:
                self.channel.start_consuming()
                
        except Exception as e:            
            print(f"'{self.iam}' error starting consuming msg")         
            raise
        
        

    
        

            