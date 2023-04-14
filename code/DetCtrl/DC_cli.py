# -*- coding: utf-8 -*-

"""
Created on Mar 4, 2022

Modified on Mar 23, 2023

@author: hilee
"""

import sys, os
sys.path.append(os.path.dirname(os.path.abspath(os.path.dirname(__file__))))

import Libs.SetConfig as sc
from Libs.MsgMiddleware import *

from DC_def import *

import threading

class DC_cli(threading.Thread):
    def __init__(self):
                
        #-------------------------------------------------------
        # load ini file
        cfg = sc.LoadConfig(WORKING_DIR + "DCS/DCS.ini")
        
        self.myid = cfg.get(IAM, 'myid')
        self.pwd = cfg.get(IAM, 'pwd')
        
        self.gui_ex = cfg.get("DC", 'gui_exchange')
        self.gui_q = cfg.get("DC", 'gui_routing_key')
        self.core_ex = cfg.get("DC", 'core_exchange')
        self.core_q = cfg.get("DC", 'core_routing_key')
        #-------------------------------------------------------
        
        self.samplingMode = UTR_MODE
        self.producer = None
        self.consumer = None
        self.roi_mode = False
    
    
    def __del__(self):
        for th in threading.enumerate():
            print(th.name + " exit.")
            
        #self.producer.channel.close()
        #self.consumer.channel.close()
    
    
    def connect_to_server_ex(self):
        # RabbitMQ connect
        self.producer = MsgMiddleware("cli.ex", "localhost", self.myid, self.pwd, self.gui_ex)
        self.producer.connect_to_server()
        self.producer.define_producer()


    def connect_to_server_q(self):
        # RabbitMQ connect
        self.consumer = MsgMiddleware("cli.q", "localhost", self.myid, self.pwd, self.core_ex)
        self.consumer.connect_to_server()
        self.consumer.define_consumer(self.core_q, self.callback)

        th = threading.Thread(target=self.consumer.start_consumer)
        th.start() 

        self.producer.send_message(self.gui_q, CMD_VERSION)
        
        
    def callback(self, ch, method, properties, body):
        cmd = body.decode()
        msg = "receive: %s" % cmd
        print(msg)
        
    
    def show_func(self, show):
        if show:
            print("------------------------------------------\n"
                "Usage: Command [Options] [Args]...\n\n"
                "Options:\n"
                "  -h, --help  Show this message and exit.\n\n"
                "Command:\n"
                "  showcommand show\n"  #help
                "  initialize timeout\n"    #help
                "  initialize2 muxtype outputs\n"   #help
                "  setfsmode mode\n"    #help
                "  setwinparam xstart xstop ystart ystop\n" #help
                "  setrampparam p1 p2 p3 p4 p5\n"   #help
                "  setfsparam p1 p2 p3 p4 p5\n" #help
                "  acquireramp\n"
                "  stopacquisition\n"
                "  gettelemetry\n"
                "  exit\n"
                "------------------------------------------\n")
        print("(If you want to show commands, type 'show'!!!)\n")
        print(">>", end=" ")
        args = list(input().split())
        return args


    def show_subfunc(self, cmd, *args):
        msg = "Usage: %s [Options] %s\n\n  %s\n\n" % (cmd, args[0], args[1])
        print(msg+"Options:\n" 
                "  -h, --help  Show this message and exit")

    def show_errmsg(self, args):
        print("Please input '%s' or '-h'/'--help'." % args)


    def show_checkmsg(self, pkg):
        print("Please check the interface status!!!")
        

    def show_noargs(self, cmd):
        msg = "'%s' has no arguments. Please use just command." % cmd
        print(msg)


    def start(self):
        print( '================================================\n'+
            '                Ctrl + C to exit or type: exit  \n'+
            '================================================\n')

        args = self.show_func(True)

        show = True
        while(show):
            while len(args) < 1:
                args = self.show_func(False)
            
            #print(args)
            if args[0] == "show":
                args = self.show_func(True)

            elif args[0] == "initialize":
                _args = "timeout"
                try:
                    if args[1] == "-h" or args[1] == "--help":
                        self.show_subfunc(args[0], _args, "timeout: milisecond (default is 200ms)")
                    elif int(args[1]) < 1:
                        self.show_errmsg(_args)
                    else:
                        msg = "%s %s" % (CMD_INITIALIZE1, args[1])
                        self.producer.send_message(self.gui_q, msg)
                except:
                    self.show_errmsg(_args)

            elif args[0] == "initialize2":
                if len(args) > 2:
                    self.show_noargs(args[0])
                else:
                    msg = "%s %d %s" % (CMD_INITIALIZE2, args[1], args[2])
                    self.producer.send_message(self.gui_q, msg)

            elif args[0] == "resetASIC":
                if len(args) > 1:
                    self.show_noargs(args[0])
                else:
                    self.producer.send_message(self.gui_q, CMD_RESET)

            elif args[0] == "setfsmode":
                _args = "mode"
                try:
                    if args[1] == "-h" or args[1] == "--help":
                        self.show_subfunc(args[0], _args, "mode: 0(UTR)/1(CDS)/2(CDS Noise)/3(Fowler Sampling)")
                    elif int(args[1]) < 0 or 3 < int(args[1]) :
                        print("Please input '0~3' for mode")
                    else:
                        self.samplingMode = int(args[1])
                        msg = "%s %s" % (CMD_SETFSMODE, args[1])
                        self.producer.send_message(self.gui_q, msg)
                except:
                    self.show_errmsg(_args)
                    
            elif args[0] == "setwinparam":
                self.roi_mode = True
                if args[1] == 1 and args[2] == 2048 and args[3] == 1 and args[4] == 2048:
                    self.roi_mode = False
                    
                _args = "xstart xstop ystart ystop"
                try:
                    if args[1] == "-h" or args[1] == "--help":
                        self.show_subfunc(args[0], _args, "xstart: 1~2048, xstop: 1~2048, ystart: 1~2048, ystop: 1~2048")
                    elif int(args[1]) < 1 or int(args[1]) < 1 or int(args[3]) < 1 or int(args[4]) < 1:
                        print("Please input '1~2048' for each argument.")
                    else:
                        msg = "%s %s %s %s %s" % (CMD_SETWINPARAM, args[1], args[2], args[3], args[4])
                        self.producer.send_message(self.gui_q, msg)
                except:
                    self.show_errmsg(_args)

            elif args[0] == "setrampparam":
                _args = "p1 p2 p3 p4 p5"
                try:
                    if args[1] == "-h" or args[1] == "--help":
                        self.show_subfunc(args[0], _args, "p1: resets, p2: reads, p3: groups, p4: drops, p5: ramps")
                    elif int(args[1]) < 1 or int(args[2]) < 1 or int(args[3]) < 1 or int(args[4]) < 1 or int(args[5]) < 1:
                        print("Please input '>0' for each argument")
                    else:
                        expTime = (T_frame * int(args[2]) * int(args[3])) + (T_frame * int(args[4]) * (int(args[3]) -1 ))
                        
                        msg = "%s %.3f %s %s %s %s %s" % (CMD_SETRAMPPARAM, expTime, args[1], args[2], args[3], args[4], args[5])
                        self.producer.send_message(self.gui_q, msg)
                except:
                    self.show_errmsg(_args)

            elif args[0] == "setfsparam":
                _args = "p1 p2 p3 p4 p5"
                try:
                    if args[1] == "-h" or args[1] == "--help":
                        self.show_subfunc(args[0], _args, "p1: resets, p2: reads, p3: groups, p4: fowler time (float, sec), p5: ramps")
                    elif int(args[1]) < 1 or int(args[2]) < 1 or int(args[3]) < 1 or float(args[4]) < 0 or int(args[5]) < 1:
                        print("Please input '>1' for p1, p2, p3, p5, '>=0' for p4.")
                    else:
                        expTime = float(args[4]) + T_frame * int(args[2])                     
                        msg = "%s %.3f %s %s %s %.3f %s" % (CMD_SETFSPARAM, expTime, args[1], args[2], args[3], float(args[4]), args[5])
                        self.producer.send_message(self.gui_q, msg)
                    
                except:
                    self.show_errmsg(_args)

            elif args[0] == "acquireramp":
                if len(args) > 1:
                    self.show_noargs(args[0])
                else:
                    msg = "%s %d" % (CMD_ACQUIRERAMP, self.roi_mode)
                    self.producer.send_message(self.gui_q, msg)

            elif args[0] == "stopacquisition":
                if len(args) > 1:
                    self.show_noargs(args[0])
                else:
                    self.producer.send_message(self.gui_q, CMD_STOPACQUISITION)

            elif args[0] == "gettelemetry":
                if len(args) > 1:
                    self.show_noargs(args[0])
                else:
                    self.producer.send_message(self.gui_q, CMD_GETTELEMETRY)

            elif args[0] == "disconnect":
                print("be getting ready")

            elif args[0] == "exit":
                if len(args) > 1:
                    self.show_noargs(args[0])
                else:
                    show = False
                    break
            else:
                print("Please confirm command.")
            
            args = ""           
            ti.sleep(1)
         

if __name__ == "__main__":
    
    dc = DC_cli()
    dc.connect_to_server_ex()
    dc.connect_to_server_q()
        
    dc.start()
    
    dc.consumer.channel.close()
    dc.producer.channel.close()