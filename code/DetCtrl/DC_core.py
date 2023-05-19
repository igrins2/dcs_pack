# -*- coding: utf-8 -*-

"""
Created on Mar 4, 2022

Modified on May 19, 2023

@author: hilee
"""

#import subprocess
import numpy as np
import astropy.io.fits as fits
from astropy.time import Time

#from ctypes import *
from math import *
import time as ti
import datetime
import os, sys

sys.path.append(os.path.dirname(os.path.abspath(os.path.dirname(__file__))))

import Libs.SetConfig as sc
from Libs.MsgMiddleware import *
from Libs.logger import *

from MACIE import *
from DC_def import *

import tunning as tn

import threading

# for c++
lib2 = cdll.LoadLibrary(WORKING_DIR + "dcs_pack/code/FowlerCalculation/libsampling_cal.so")
fowler_calculation = lib2.fowler_calculation
fowler_calculation.argtypes = (c_int, c_int, c_int, POINTER(c_ushort))
fowler_calculation.restype = POINTER(c_float)

class MACIE_IpAddr(Structure):
    _fields_ = [("ipAddr", c_ubyte*4)]

lib.MACIE_CheckInterfaces.argtypes = [c_ushort, POINTER(MACIE_IpAddr), c_ushort, POINTER(c_ushort), POINTER(POINTER(MACIE_CardInfo))]
lib.MACIE_CheckInterfaces.restype = c_int

class MACIE_FitsHdr(Structure):
        _fields_ = [("key", c_char*9),
                    ("valType", c_int),
                    ("iVal", c_int),
                    ("fVal", c_float),
                    ("sVal", c_char*72),
                    ("comment", c_char*72)]


lib.MACIE_WriteFitsFile.argtypes = [c_char_p, c_ushort, c_ushort, POINTER(c_ushort), c_ushort, POINTER(MACIE_FitsHdr)]
lib.MACIE_WriteFitsFile.restype = c_int

FieldNames = [('pressure', float),
              ('bench', float), ('bench_sp', float),
              ('grating', float), ('grating_sp', float),
              ('detS', float), ('detS_sp', float),
              ('detK', float), ('detK_sp', float),
              ('camH', float),
              ('detH', float), ('detH_sp', float),
              ('benchcenter', float), ('coldhead01', float), 
              ('coldhead02', float), ('coldstop', float), 
              ('charcoalBox', float), ('camK', float), 
              ('shieldtop', float), ('air', float)]

class DC(threading.Thread):
    def __init__(self):
        
        self._iam = "CORE"
        self.log = LOG(WORKING_DIR + "DCS", self._iam)        
        self._target = "GUI"

        self.log.send(self._iam, INFO, "start DCS core!!!")

        #-------------------------------------------------------
        # load ini file
        cfg = sc.LoadConfig(WORKING_DIR + "DCS/DCS.ini")

        # ===========================================
        # ICS
        self.ics_ip_addr = cfg.get('ICS', 'ip_addr')
        self.ics_id = cfg.get('ICS', 'id')
        self.ics_pwd = cfg.get('ICS', 'pwd')
    
        # exchange - queue for InstSeq
        self.InstSeq_ex = cfg.get('ICS', 'InsetSeq_exchange')
        self.InstSeq_q = cfg.get('ICS', 'InsetSeq_routing_key')
        
        # for OpsApp
        self.ObsApp_ex = cfg.get('ICS', "ObsApp_exchange")  
        self.ObsApp_q = cfg.get('ICS', "ObsApp_routing_key")   
        
        # for dt
        self.dt_ex = cfg.get('ICS', "dt_exchange")     
        self.dt_q = cfg.get('ICS', "dt_routing_key")
        
        self.dcs_ip_addr = cfg.get(IAM, 'ip_addr')
        self.dcs_q = IAM+'.q'

        # ===========================================
        # local
        self.myid = cfg.get(IAM, 'myid')
        self.pwd = cfg.get(IAM, 'pwd')
        self.macieSN = int(cfg.get(IAM, 'sn'))

        # exchange - queue
        self.gui_ex = cfg.get("DC", 'gui_exchange')
        self.gui_q = cfg.get("DC", 'gui_routing_key')
        self.core_ex = cfg.get("DC", 'core_exchange')
        self.core_q = cfg.get("DC", 'core_routing_key')

        # ===========================================
        # DCS
        self.loadfile_path = cfg.get('DC', 'config-dir')
        self.loadfile_path = WORKING_DIR + self.loadfile_path

        self.macie_file = cfg.get('DC', 'MACIE-Register')
        self.asic_file = cfg.get('DC', 'ASIC-Firmware')

        self.exe_path = cfg.get('DC', 'Img-dir')
        self.exe_path = WORKING_DIR + self.exe_path

        self.gige_timeout = cfg.get('DC', 'timeout')
        #-------------------------------------------------------

        self.handle = 0
        self.slctCard = 0
        self.slctMACIEs = 0
        self.slot1 = False  # slot 1-True, slot 2-False
        Cards = MACIE_CardInfo * 2
        self.pCard = Cards()

        self.slctASICs = 0
        self.option = True

        self._24Bit = False  # 24bit-True, 16bit-False

        self.expTime = 0.0
        self.fowlerTime = 0.0

        self.preampInputScheme = 1    # 2
        self.preampInputVal = 0x4502    # 0xaaaa
        self.preampGain = 8  # 1

        self.V_refmain = 0

        self.samplingMode = UTR_MODE

        self.ROIMode = False
        self.x_start, self.x_stop, self.y_start, self.y_stop = 0, FRAME_X-1, 0, FRAME_Y
        self.resets, self.reads, self.ramps, self.groups, self.drops = 1, 1, 1, 1, 1
        
        self.loadimg = []

        self.measured_startT = 0

        #self.showfits = False

        self.init1 = False  #for ics
        self.init2 = False  #for ics
        self.acquiring = False  #prevent conflict

        self.producer = [None, None]    #local, ics
        self.consumer = [None for _ in range(4)]    #local, ics, uploader, ObsApp

        self.stop = False
        self.dewar_info = False

        self.param = ""

        ti.sleep(10)

        self.connect_to_server_dcs_ex() # InstSeq, DTP, ObsApp
        
        self.connect_to_server_InstSeq_q() 
        # ObsApp
        if IAM == "DCSS":
            self.connect_to_server_ObsApp_q() 
        self.connect_to_server_dt_q()
        
        # uploader
        self.connect_to_server_uploader_q()
        
        # gui, cli for local
        self.connect_to_server_ex()
        self.connect_to_server_q()

        threading.Thread(target=self.control_MACIE).start()


    def __del__(self):

        self.MemoryFree()

        self.log.send(self._iam, INFO, "DCS core closing...")

        for th in threading.enumerate():
            self.log.send(self._iam, INFO, th.name + " exit.")

        for i in range(2):
            self.producer[i].channel.close()
        for i in range(4):
            self.consumer[i].channel.close()

        self.log.send(self._iam, INFO, "DCS core closed!")
             
       
    #---------------------------------------------------------------------------------------------
    def connect_to_server_dcs_ex(self):
        # RabbitMQ connect        
        self.producer[ICS] = MsgMiddleware(IAM, self.ics_ip_addr, self.ics_id, self.ics_pwd, IAM+'.ex')
        self.producer[ICS].connect_to_server()
        self.producer[ICS].define_producer()
        
        
    def publish_to_ics_queue(self, msg):
        if self.producer[ICS] == None:
            return
        
        self.producer[ICS].send_message(self.dcs_q, msg)
        
        msg = "%s -> [ICS]" % msg
        self.log.send(IAM, INFO, msg)


    #---------------------------------------------------------------------------------------------
    # from InstSeq
    def connect_to_server_InstSeq_q(self):
        # RabbitMQ connect
        self.consumer[ICS] = MsgMiddleware(IAM, self.ics_ip_addr, self.ics_id, self.ics_pwd, self.InstSeq_ex)
        self.consumer[ICS].connect_to_server()
        self.consumer[ICS].define_consumer(self.InstSeq_q, self.callback_InstSeq)

        th = threading.Thread(target=self.consumer[ICS].start_consumer)
        th.start() 


    def callback_InstSeq(self, ch, method, properties, body):
        cmd = body.decode()        
        self.process_from_ICS(cmd, "InstSeq")
        
        
    #-------------------------------     
    # In case of DCSS   
    def connect_to_server_ObsApp_q(self):
        # RabbitMQ connect
        self.consumer[OBSAPP] = MsgMiddleware(IAM, self.ics_ip_addr, self.ics_id, self.ics_pwd, self.ObsApp_ex)      
        self.consumer[OBSAPP].connect_to_server()
        self.consumer[OBSAPP].define_consumer(self.ObsApp_q, self.callback_ObsApp)
        
        th = threading.Thread(target=self.consumer[OBSAPP].start_consumer)
        th.start() 
        
        
    def callback_ObsApp(self, ch, method, properties, body):
        cmd = body.decode()                
        self.process_from_ICS(cmd, "ObsApp")
        
        
    #---------------------------------------------------------------------------------------------
    # from InstSeq
    def connect_to_server_dt_q(self):
        # RabbitMQ connect
        self.consumer[ICS] = MsgMiddleware(IAM, self.ics_ip_addr, self.ics_id, self.ics_pwd, self.dt_ex)
        self.consumer[ICS].connect_to_server()
        self.consumer[ICS].define_consumer(self.dt_q, self.callback_dt)

        th = threading.Thread(target=self.consumer[ICS].start_consumer)
        th.start() 


    def callback_dt(self, ch, method, properties, body):
        cmd = body.decode()        
        self.process_from_ICS(cmd, "DTP")
    
        
    def process_from_ICS(self, cmd, where):

        param = cmd.split()

        if len(param) < 3:
            return

        if param[0] == CMD_STOPACQUISITION:
            self.stop = True
            #print("received 'stop'!!!!!!")
        elif param[0] == OBSAPP_BUSY:
            self.stop = True

        if self.acquiring:
            return

        if IAM == "DCSS" and param[1] == "H_K":
            return    
        if not (param[1] == IAM or param[1] == "all"):
            return

        msg = "<- [%s] %s" % (cmd, where)
        self.log.send(IAM, INFO, msg)

        # simulation mode
        if bool(int(param[2])):
            if param[0] == CMD_INIT2_DONE or param[0] == CMD_INITIALIZE2_ICS or param[0]  == CMD_SETFSPARAM_ICS:
                self.publish_to_ics_queue(param[0])
                #print("Answer!!!")
            elif param[0] == CMD_ACQUIRERAMP_ICS:                
                ti.sleep(2)

                _t = datetime.datetime.utcnow()
                cur_datetime = [_t.year, _t.month, _t.day, _t.hour, _t.minute, _t.second, _t.microsecond]
                folder_name = "%04d%02d%02d_%02d%02d%02d" % (cur_datetime[0], cur_datetime[1], cur_datetime[2], cur_datetime[3], cur_datetime[4], cur_datetime[5])

                msg = "%s 5 %s" % (param[0], folder_name)
                self.publish_to_ics_queue(msg)
            
                msg = "%s ->" % msg
                self.log.send(IAM, INFO, msg)

            elif param[0] == CMD_STOPACQUISITION:
                self.publish_to_ics_queue(param[0])
            
        else:
            if self.init1:
                self.param = cmd
       


    #-------------------------------
    # from UPLOADER
    def connect_to_server_uploader_q(self):
        # RabbitMQ connect
        self.consumer[UPLOADER] = MsgMiddleware(IAM, self.ics_ip_addr, self.ics_id, self.ics_pwd, "uploader.ex")      
        self.consumer[UPLOADER].connect_to_server()
        self.consumer[UPLOADER].define_consumer("uploader.q", self.callback_uploader)
        
        th = threading.Thread(target=self.consumer[UPLOADER].start_consumer)
        th.start()        

    
    def callback_uploader(self, ch, method, properties, body):
        cmd = body.decode()
        param = cmd.split()
        #print("uploader:", param)
        if len(param) < 2:
            return
        
        msg = "<- [DB uploader] %s" % cmd
        self.log.send(self._iam, INFO, msg)

        if param[0] == UPLOAD_Q:
            dewar_list = param[1:]
            if len(dewar_list) != len(FieldNames):
                return None

            self.dewar_dict = dict((k, t(v)) for (k, t), v in zip(FieldNames, dewar_list))
            self.dewar_info = True
        

    #+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    def connect_to_server_ex(self):
        # RabbitMQ connect
        self.producer[LOCAL] = MsgMiddleware(self._iam, "localhost", self.myid, self.pwd, self.core_ex)
        self.producer[LOCAL].connect_to_server()
        self.producer[LOCAL].define_producer()
        
        
    def publish_to_local_queue(self, msg):
        if self.producer[LOCAL] == None:
            return
        
        self.producer[LOCAL].send_message(self.core_q, msg)
        
        msg = "%s -> [GUI]" % msg
        self.log.send(self._iam, INFO, msg)


    def connect_to_server_q(self):
        # RabbitMQ connect
        self.consumer[LOCAL] = MsgMiddleware(self._iam, "localhost", self.myid, self.pwd, self.gui_ex)
        self.consumer[LOCAL].connect_to_server()
        self.consumer[LOCAL].define_consumer(self.gui_q, self.callback)

        th = threading.Thread(target=self.consumer[LOCAL].start_consumer)
        th.start() 

    def callback(self, ch, method, properties, body):
        cmd = body.decode()
        msg = "<- [GUI] %s" % cmd
        self.log.send(self._iam, INFO, msg)

        self.param = cmd

        param = self.param.split()
        if param[0] == CMD_VERSION:
            if self.init1:
                msg = "%s %s %s 1" % (CMD_VERSION, self.LibVersion(), self.pCard[self.slctCard].contents.macieSerialNumber)
            else:
                msg = "%s %s --- 0" % (CMD_VERSION, self.LibVersion())

            self.publish_to_local_queue(msg)
            

        #elif param[0] == CMD_SHOWFITS:
        #    self.showfits = bool(int(param[1]))
            
        #elif param[0] == CMD_EXIT:
        #    self.__del__()

        elif param[0] == CMD_SETFSMODE:
            self.samplingMode = int(param[1]) 
            #self.log.send(IAM, INFO, param[1])

        elif param[0] == CMD_SETWINPARAM:
            self.x_start = int(param[1])
            self.x_stop = int(param[2])
            self.y_start = int(param[3])
            self.y_stop = int(param[4])
        
        elif param[0] == CMD_STOPACQUISITION:
            self.stop = True
            #self.publish_to_local_queue(CMD_STOPACQUISITION)


    def control_MACIE(self):
        
        self.Initialize(int(self.gige_timeout))

        while True:
            if self.param == "":
                continue     
            
            self.acquiring = True

            param = self.param.split()
            
            if param[0] == CMD_INITIALIZE1:  
                if self.Initialize(int(param[1])):
                    msg = "%s %s %s" % (CMD_INITIALIZE1, self.LibVersion(), self.pCard[self.slctCard].contents.macieSerialNumber)
                    self.publish_to_local_queue(msg)

            elif param[0] == CMD_INITIALIZE2:
                if self.Initialize2() == False:
                    continue
                if self.ResetASIC() == False:
                    continue
                if self.DownloadMCD() == False:
                    continue
                if self.SetDetector(int(param[1]), int(param[2])):
                    self.publish_to_local_queue(CMD_INITIALIZE2)

            elif param[0] == CMD_RESET:
                if self.ResetASIC():
                    self.publish_to_local_queue(CMD_RESET)

            elif param[0] == CMD_SETRAMPPARAM:
                self.expTime = float(param[1])
                self.SetRampParam(int(param[2]), int(param[3]), int(param[4]), int(param[5]), int(param[6]))

            elif param[0] == CMD_SETFSPARAM:
                self.expTime = float(param[1])
                self.SetFSParam(int(param[2]), int(param[3]), int(param[4]), float(param[5]), int(param[6]))

            elif param[0] == CMD_ACQUIRERAMP:
                #print("acquire!!!!")
                if param[1] == "0":
                    if self.AcquireRamp() == False:
                        continue
                    if self.ImageAcquisition():
                        msg = "%s %.3f %s" % (CMD_ACQUIRERAMP, self.measured_durationT, self.full_path)
                        self.publish_to_local_queue(msg)
                else:
                    if self.AcquireRamp_window() == False:
                        continue
                    if self.ImageAcquisition_window():
                        self.publish_to_local_queue(CMD_ACQUIRERAMP)

            elif param[0] == CMD_ASICLOAD:
                _read = [0 for _ in range(4)]
                idx = 1
                for i in range(4):
                    _addr = int("0x" + param[idx], 16)
                    _value = int("0x" + param[idx+1], 16)
                    res = self.write_ASIC_reg(_addr, _value)
                    if res == MACIE_OK:
                        val, sts = self.read_ASIC_reg(_addr)
                        if sts == MACIE_OK:
                            _read[i] = val[0]                          
                    idx += 2

                msg = "%s %s %s %s %s" % (CMD_ASICLOAD, _read[0], _read[1], _read[2], _read[3])
                self.publish_to_local_queue(msg)

            elif param[0] == CMD_WRITEASICREG:
                res = self.write_ASIC_reg(int(param[1]), int(param[2]))
                if res == MACIE_OK:
                    result = RET_OK
                    msg = "%s %s" % (CMD_WRITEASICREG, param[1])
                    self.publish_to_local_queue(msg)
                else:
                    result = RET_FAIL
                msg = "WriteASICReg %s - h%04x = %04x" % (result, int(param[1]), int(param[2]))
                self.log.send(self._iam, INFO, msg)
            
            elif param[0] == CMD_READASICREG:
                val, sts = self.read_ASIC_reg(int(param[1]))
                if sts == MACIE_OK:
                    result = RET_OK
                    _value = val[0]
                    msg = "%s %s %d" % (CMD_READASICREG, param[1], _value)
                    self.publish_to_local_queue(msg)
                else:
                    result = RET_FAIL
                    _value = 0

                msg = "ReadASICReg %s - h%04x = %04x" % (result, int(param[1]), _value)     #need to check
                self.log.send(self._iam, INFO, msg)

            elif param[0] == CMD_GETTELEMETRY:
                self.GetTelemetry()

            #--------------------------------------------------------         

            elif param[0] == CMD_INIT2_DONE or param[0] == CMD_INITIALIZE2_ICS:
                if self.init2:
                    self.publish_to_ics_queue(param[0])
                else:
                    if self.Initialize2() == False:
                        continue
                    if self.ResetASIC() == False:
                        continue
                    if self.DownloadMCD() == False:
                        continue
                    if self.SetDetector(MUX_TYPE, 32):
                        self.publish_to_ics_queue(param[0])

            elif param[0] == CMD_SETFSPARAM_ICS:
                self.samplingMode = FOWLER_MODE
                self.expTime = float(param[3])
                if self.SetFSParam(int(param[4]), int(param[5]), int(param[6]), float(param[7]), int(param[8])) == False:
                    continue
                #if self.AcquireRamp() == False:
                #    continue
                #if self.ImageAcquisition(False):
                #    msg = "%s %.3f %s" % (param[0], self.measured_durationT, self.folder_name)
                #else:
                #    msg = CMD_STOPACQUISITION
                self.publish_to_ics_queue(param[0])

            elif param[0] == CMD_ACQUIRERAMP_ICS:
                if self.AcquireRamp() == False:
                    continue
                if self.ImageAcquisition(False):
                    msg = "%s %.3f %s" % (param[0], self.measured_durationT, self.folder_name)
                    self.publish_to_ics_queue(msg)
                else:
                    msg = CMD_STOPACQUISITION
                    self.publish_to_ics_queue(msg)
            
            self.param = ""
            self.acquiring = False


    #+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    # Version
    def LibVersion(self):
        ver = "%.2f" % lib.MACIE_LibVersion()
        msg = "Version: "
        self.log.send(self._iam, INFO, msg+ver)
        return ver

    # Error Msg
    def GetErrMsg(self):
        errMsg = lib.MACIE_Error()
        return errMsg.decode()

    # not yet!!!!
    # Memory Fee 
    def MemoryFree(self):
        sts = lib.MACIE_Free()
        res = ""
        if sts == MACIE_OK:
            res =  RET_OK
        else:
            res = RET_FAIL
        self.log.send(self._iam, INFO, "MACIE_Free: " + res)
        return sts
        
    
    def read_ASIC_reg(self, asic_addr):
        if self.handle == 0:
            return 0, MACIE_FAIL

        arr_list = []
        arr = np.array(arr_list)
        val = arr.ctypes.data_as(POINTER(c_uint))
        sts = lib.MACIE_ReadASICReg(self.handle, self.slctASICs, asic_addr, val, self._24Bit, self.option)
        
        return val, sts

    
    def write_ASIC_reg(self, asic_addr, input):
        res = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, asic_addr, input, self.option)

        return res


    # Init
    def Init(self):
        sts = lib.MACIE_Init()
        res = ""
        if sts == MACIE_OK:
            res = RET_OK
        else:
            res = RET_FAIL
        self.log.send(self._iam, INFO, "MACIE_Init: " + res)
        return sts


    # SetGigeTimeout
    def SetGigeTimeout(self, timeout):
        sts = lib.MACIE_SetGigeTimeout(timeout)
        if sts == MACIE_OK:
            self.log.send(self._iam, INFO, "MACIE_SetGigeTimeout: " + RET_OK)
            return True
        else:
            self.log.send(self._iam, INFO, self.GetErrMsg())
            return False
        

    # CheckInterfaces
    def CheckInterfaces(self):
        #slctCard = 0
        connection = MACIE_NONE
        try:

            #ipaddr_array = MACIE_IpAddr
            #pIPs = ipaddr_array()
            #IPs = MACIE_IpAddr(ipAddr = (192, 168, 1, 100))
            ip = self.dcs_ip_addr.split(".")
            IPs = MACIE_IpAddr(ipAddr = (int(ip[0]), int(ip[1]), int(ip[2]), int(ip[3])))

            arr_list = [0,]
            arr = np.array(arr_list)
            card = arr.ctypes.data_as(POINTER(c_ushort))
            self.pCard = pointer(pointer(MACIE_CardInfo()))

            sts = lib.MACIE_CheckInterfaces(0, IPs, 0, card, self.pCard)
            res = ""
            if sts == MACIE_OK:
                res = RET_OK
            else:
                res = RET_FAIL
            self.log.send(self._iam, INFO, "MACIE_CheckInterfaces: " + res)

            if self.pCard == None or res == RET_FAIL:
                self.log.send(self._iam, ERROR, RET_FAIL)
                return None, None
                
            macieSN = self.pCard[self.slctCard].contents.macieSerialNumber
            self.log.send(self._iam, INFO, str(macieSN))
            self.log.send(self._iam, INFO, "True" if self.pCard[self.slctCard].contents.bUART == True else "False")
            self.log.send(self._iam, INFO, "True" if self.pCard[self.slctCard].contents.bGigE == True else "False")
            self.log.send(self._iam, INFO, "True" if self.pCard[self.slctCard].contents.bUSB == True else "False")
            ipaddr = "%d.%d.%d.%d" % (self.pCard[self.slctCard].contents.ipAddr[0], self.pCard[self.slctCard].contents.ipAddr[1], self.pCard[self.slctCard].contents.ipAddr[2], self.pCard[self.slctCard].contents.ipAddr[3])
            self.log.send(self._iam, INFO, ipaddr)
            self.log.send(self._iam, INFO, str(self.pCard[self.slctCard].contents.gigeSpeed))
            self.log.send(self._iam, INFO, str(self.pCard[self.slctCard].contents.serialPortName.decode()))
            self.log.send(self._iam, INFO, str(self.pCard[self.slctCard].contents.firmwareSlot1.decode()))

            #if self.ip_addr == ipaddr:

            return ipaddr, macieSN

        except:
            return None, None


    # GetHandle
    def GetHandle(self):
        #slctCard = 0
        connection = MACIE_GigE  # input by user

        self.handle = lib.MACIE_GetHandle(
            self.pCard[self.slctCard].contents.macieSerialNumber, connection)
        msg = "(macie serial number: %d) Handle = %d" % (self.pCard[self.slctCard].contents.macieSerialNumber, self.handle)
        self.log.send(self._iam, INFO, msg)


    def Initialize(self, timeout):

        # self.InitBuffer()

        # 1. init
        if self.Init() == MACIE_FAIL:
            return -1

        # 2. SetGigeTimeout
        if self.SetGigeTimeout(timeout) == False:
            return -2
        
        # 3. CheckInterfaces
        _ip, _sn = self.CheckInterfaces()
        while _ip != self.dcs_ip_addr and _sn != self.macieSN:
            ti.sleep(0.5)
            _ip, _sn = self.CheckInterfaces()

        # 4. GetHandle
        self.GetHandle()

        self.log.send(self._iam, INFO, "Initialize " + RET_OK)

        self.init1 = True
        
        return True


    def Initialize2(self):
        self.init2 = False

        if self.handle == 0:
            return False

        msg = "Initialize with handle: %ld" % self.handle
        self.log.send(self._iam, INFO, msg)

        # step 1. GetAvailableMACIEs
        if self.GetAvailableMACIEs() == False:
            return False

        if self.slctMACIEs == 0:
            self.log.send(self._iam, ERROR, "MACIE0 is not available")
            return False

        arr_list = []
        arr = np.array(arr_list)
        val = arr.ctypes.data_as(POINTER(c_uint))

        # step 2. load MACIE firmware from slot1 or slot2
        if lib.MACIE_loadMACIEFirmware(self.handle, self.slctMACIEs, self.slot1, val) != MACIE_OK:
            #msg = "LOAD MACIE firmware " + RET_FAIL + ": " + self.GetErrMsg()
            self.log.send(self._iam, ERROR, self.GetErrMsg())
            return False
        if val[0] != 0xac1e:
            msg = "Verification of MACIE firmware load failed: readback of hFFFB = %d" % val[0]
            self.log.send(self._iam, ERROR, msg)
            return False
        self.log.send(self._iam, INFO, "Load MACIE firmware " + RET_OK)

        # step 3. download MACIE registers
        file =  self.loadfile_path + "/" + self.macie_file
        if lib.MACIE_DownloadMACIEFile(self.handle, self.slctMACIEs, file.encode()) != MACIE_OK:
            #msg = RET_FAIL + ": " + self.GetErrMsg()
            self.log.send(self._iam, ERROR, self.GetErrMsg())
            return False
        self.log.send(self._iam, INFO, "Download MACIE register file " + RET_OK)

        # check again!!!
        arr_list = []
        arr = np.array(arr_list)
        data = arr.ctypes.data_as(POINTER(c_uint))
        res = -1
        sts = lib.MACIE_ReadMACIEBlock(self.handle, self.slctMACIEs, MACIE_Block_Reg, data, 5)
        if sts != MACIE_OK:
            res = sts
        else:
            for i in range(5):
                msg = "val h%04x = %04x" % (MACIE_Block_Reg, data[i])
                self.log.send(self._iam, INFO, msg)
            res = data[1]

        self.log.send(self._iam, INFO, "Initialize2 " + RET_OK)

        return True


    def ResetASIC(self):
        if self.handle == 0:
            return False

        # step 1. reset science data error counters
        if lib.MACIE_ResetErrorCounters(self.handle, self.slctMACIEs) != MACIE_OK:
            self.log.send(self._iam, ERROR, "Reset MACIE error counters " + RET_FAIL)
            return False
        self.log.send(self._iam, INFO, "Reset error counters " + RET_OK)

        # step 2. download ASIC file
        if lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_State, 0x8002, self.option) != MACIE_OK:
            self.log.send(self._iam, ERROR, "Reconfiguration sequence " + RET_FAIL)
            return False
        self.log.send(self._iam, INFO, "Reconfiguration sequence " + RET_OK)

        self.log.send(self._iam, INFO, "ResetASIC " + RET_OK)

        return True
        

    def DownloadMCD(self):
        if self.handle == 0:
            return False

        # step 1. downlaod asic file
        file = self.loadfile_path + "/" + self.asic_file 
        sts = lib.MACIE_DownloadASICFile(self.handle, self.slctMACIEs, file.encode(), True)
        if sts != MACIE_OK:
            self.log.send(self._iam, WARNING, self.GetErrMsg())
            return False
        self.log.send(self._iam, INFO, "Download ASIC firmware " + RET_OK)

        # step 2. GetAvailableASICs
        if self.GetAvailableASICs() == False:
            return False

        self.log.send(self._iam, INFO, "DownloadMCD " + RET_OK)         

        return True


    def GetAvailableMACIEs(self):
        if self.handle == 0:
            return False

        avaiMACIEs = lib.MACIE_GetAvailableMACIEs(self.handle)
        msg = "MACIE_GetAvailableMACIEs = %d" % avaiMACIEs
        self.log.send(self._iam, INFO, msg)

        self.slctMACIEs = avaiMACIEs & 1
        
        arr_list = []
        arr = np.array(arr_list)
        val = arr.ctypes.data_as(POINTER(c_uint))
        msg = ""

        if self.slctMACIEs == 0:
            msg = "Select MACIE = %d invalid" % avaiMACIEs
            self.log.send(self._iam, ERROR, msg)
            return False
        elif lib.MACIE_ReadMACIEReg(self.handle, avaiMACIEs, MACIE_Reg, val) != MACIE_OK:
                #msg = "MACIE read %d failed: %s" % (MACIE_Reg, self.GetErrMsg())
            self.log.send(self._iam, ERROR, self.GetErrMsg())
            return False
        else:
            msg = "MACIE h%04x = %04x" % (MACIE_Reg, val[0])
            self.log.send(self._iam, INFO, msg)
        
        return True


    def GetAvailableASICs(self):
        if self.handle == 0:
            return False
        
        self.slctASICs = lib.MACIE_GetAvailableASICs(self.handle, False)
        if self.slctASICs == 0:
            self.log.send(self._iam, ERROR, "MACIE_GetAvailableASICs " + RET_FAIL)
            return False
        else:           
            val, sts = self.read_ASIC_reg(ASICAddr)
            msg = "ASIC h%04x = %04x" % (ASICAddr, val[0])
            self.log.send(self._iam, INFO, msg)

            arr_list = []
            arr = np.array(arr_list)
            data = arr.ctypes.data_as(POINTER(c_uint))
            lib.MACIE_ReadASICBlock(
                self.handle, self.slctASICs, ASICAddr_NResets, data, 5, self._24Bit, self.option)
            for i in range(5):
                msg = "val h%04x = %04x" % (ASICAddr_NResets + i, data[i])
                self.log.send(self._iam, INFO, msg)
        msg = "Available ASICs = %d" % self.slctASICs
        self.log.send(self._iam, INFO, msg)

        return True


    def SetDetector(self, muxType, outputs):  # muxType = 2 (H2RG)
        if self.handle == 0:
            return False

        res = [MACIE_OK, MACIE_OK]

        res[0] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_HxRGVal, muxType, self.option)
        res[1] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_HxRGNumOutVal, outputs, self.option)

        for i in range(2):
            if res[i] == MACIE_FAIL:
                self.log.send(self._iam, ERROR, "Set Detector " + RET_FAIL)
                return False

        msg = "Set Detector (%d, %d) succeeded" % (muxType, outputs)
        self.log.send(self._iam, INFO, msg)         

        self.init2 = True   

        return True


    def GetErrorCounters(self):
        if self.handle == 0:
            return

        arr_list = [0 for _ in range(MACIE_ERROR_COUNTERS)]
        arr = np.array(arr_list)
        errArr = arr.ctypes.data_as(POINTER(c_ushort))
        if lib.MACIE_GetErrorCounters(self.handle, self.slctMACIEs, errArr) != MACIE_OK:
            self.log.send(self._iam, ERROR, "Read MACIE error counter failed")
            return

        else:
            self.log.send(self._iam, ERROR, "Error counters....")
            for i in range(MACIE_ERROR_COUNTERS):
                msg = "%d" % errArr[i]
                self.log.send(self._iam, INFO, msg)


    def SetRampParam(self, p1, p2, p3, p4, p5):  # p1~p5 : int
        if self.handle == 0:
            return False

        self.resets, self.reads, self.groups, self.drops, self.ramps = p1, p2, p3, p4, p5

        res = [0 for _ in range(8)]

        res[0] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NResets, p1, self.option)
        res[1] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NReads, p2, self.option)
        res[2] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_Config, 12, self.option)
        res[3] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NRamps, p5, self.option)
        res[4] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NGroups, p3, self.option)
        res[5] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_nNDrops, p4, self.option)

        lowerCounter, upperCounter = 0, 0
        if self.expTime * pow(10, 6) >= BITSIZE_65536:
            upperCounter = int(
                (self.expTime * pow(10, 6) / 20) / BITSIZE_65536)
            lowerCounter = int(
                (self.expTime * pow(10, 6) / 20) % BITSIZE_65536)
        else:
            lowerCounter = int(self.expTime * pow(10, 6) / 20)
        res[6] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_TexpLower, lowerCounter, self.option)
        res[7] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_TexpUpper, upperCounter, self.option)

        for i in range(8):
            if res[i] != MACIE_OK:
                self.log.send(self._iam, ERROR, "SetRampParam failed - write ASIC registers")
                return False

        msg = "SetRampParam(%d, %d, %d, %d, %d)" % (p1, p2, p3, p4, p5)
        self.log.send(self._iam, INFO, msg) 

        return True


    def SetFSParam(self, p1, p2, p3, p4, p5):  # p1~5:int, p4:double

        self.resets, self.reads, self.groups, self.ramps = p1, p2, p3, p5
        self.fowlerTime = p4

        if self.handle == 0:
            return False

        res = [0 for _ in range(9)]

        res[0] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NResets, p1, self.option)
        res[1] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NReads, p2, self.option)
        res[2] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_Config, 12, self.option)
        res[3] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NRamps, p5, self.option)
        res[4] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NGroups, p3, self.option)

        lowerCounter, upperCounter = 0, 0
        if p4 * pow(10, 6) >= BITSIZE_65536:
            upperCounter = int((p4 * pow(10, 6) / 20) / BITSIZE_65536)
            lowerCounter = int((p4 * pow(10, 6) / 20) % BITSIZE_65536)
        else:
            lowerCounter = int((p4 * pow(10, 6)) / 20)
        res[5] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_TfowlerLower, lowerCounter, self.option)
        res[6] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_TfowlerUpper, upperCounter, self.option)

        t_FowlerPair = 0
        # self.expTime = 2 * t_FowlerPair + p4
        t_FowlerPair = (self.expTime - p4) / 2
        if t_FowlerPair * pow(10, 6) >= BITSIZE_65536:
            upperCounter = int(
                ((t_FowlerPair * pow(10, 6)) / 20) / BITSIZE_65536)
            lowerCounter = int(
                ((t_FowlerPair * pow(10, 6)) / 20) % BITSIZE_65536)
        else:
            lowerCounter = int((t_FowlerPair * pow(10, 6)) / 20)
        res[7] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_TexpLower, lowerCounter, self.option)
        res[8] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_TexpUpper, upperCounter, self.option)

        for i in range(9):
            if res[i] != MACIE_OK:
                self.log.send(self._iam, ERROR, "SetFSParam failed - write ASIC registers")
                return False

        msg = "SetFSParam(%d, %d, %d, %.3f, %d)" % (p1, p2, p3, p4, p5)
        self.log.send(self._iam, INFO, msg)           

        return True


    def AcquireRamp(self):

        if self.handle == 0:
            return False

        self.stop = False

        self.measured_startT = ti.time()

        #self.loadimg = []

        self.log.send(self._iam, INFO, "Acquire Science Data....")

        # step 1. ASIC configuration
        res = [0 for _ in range(7)]

        res[0] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_ASICInputRefVal, self.preampInputScheme, self.option)
        res[1] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_PreAmpReg1Ch1ENAddr, self.preampInputVal, self.option)
        res[2] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_ASICPreAmpGainVal, self.preampGain, self.option)
        res[3] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NReads, self.reads, self.option)
        res[4] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NRamps, self.ramps, self.option)

        if self.samplingMode == UTR_MODE:
            res[5] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_HxRGExpModeVal, 0, self.option)  # UTR, Full frame
        else:
            res[5] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_HxRGExpModeVal, 1, self.option)  # FS, Full frame
        res[6] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_State, 0x8002, self.option)

        for i in range(7):
            if res[i] != MACIE_OK:
                self.log.send(self._iam, ERROR, "ASIC configuration failed - write ASIC registers")
                return False

        ti.sleep(1.5)

        val, sts = self.read_ASIC_reg(ASICAddr_State)
        if (val[0] & 1) != 0 or sts != MACIE_OK:
            self.log.send(self._iam, ERROR, "ASIC configuration for shorted preamp inputs failed")
            return False
        self.log.send(self._iam, INFO, "Configuration succeeded")

        # step 2.science interface
        frameSize = 0
        if self.samplingMode == UTR_MODE:
            frameSize = FRAME_X * FRAME_Y * self.reads * self.groups * self.ramps
        else:
            frameSize = FRAME_X * FRAME_Y * 2 * self.reads * self.ramps

        arr_list = []
        arr = np.array(arr_list)
        buf = arr.ctypes.data_as(POINTER(c_int))

        sts = lib.MACIE_ConfigureGigeScienceInterface(self.handle, self.slctMACIEs, 0, frameSize, 42037, buf)  # 0-16bit
        if sts != MACIE_OK:
            msg = "Science interface configuration failed. buf = %d" % buf[0]
            self.log.send(self._iam, ERROR, msg)
            return False
        msg = "Science interface configuration succeeded. buf (KB) = %d" % buf[0]
        self.log.send(self._iam, INFO, msg)

        # step 3.trigger ASIC to read science data
        self.log.send(self._iam, INFO, "Trigger image acquisition....")

        # make sure h6900 bit<0> is 0 before triggering.

        val, sts = self.read_ASIC_reg(ASICAddr_State)
        if sts != MACIE_OK:
            msg = "Read ASIC h%04x failed" % ASICAddr_State
            self.log.send(self._iam, ERROR, msg)
            return False
        if (val[0] & 1) != 0:
            msg = "Configure idle mode by writing ASIC h%04x failed" % ASICAddr_State
            self.log.send(self._iam, ERROR, msg)
            return False

        if lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_State, 0x8001, self.option) != MACIE_OK:
            self.log.send(self._iam, ERROR, "Triggering " + RET_FAIL)
            return False

        self.log.send(self._iam, INFO, "Triggering succeeded")
        #print(self.stop)

        return True


    def AcquireRamp_window(self):
        if self.handle == 0:
            return False

        self.log.send(self._iam, INFO, "Acquire Science Data....")

        # step 1. ASIC configuration
        res = [0 for _ in range(6)]

        res[0] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_ASICInputRefVal, self.preampInputScheme, self.option)
        res[1] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_PreAmpReg1Ch1ENAddr, self.preampInputVal, self.option)
        res[2] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_ASICPreAmpGainVal, self.preampGain, self.option)
        res[3] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NReads, 1, self.option)

        
        res[4] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_HxRGExpModeVal, 2, self.option)  # UTR, window
        arr_list = [self.x_start, self.x_stop, self.y_start, self.y_stop] # x1, x2, y1, y2
        arr = np.array(arr_list)
        winarr = arr.ctypes.data_as(POINTER(c_uint))
        wr = lib.MACIE_WriteASICBlock(self.handle, self.slctASICs, ASICAddr_WinArr, winarr, 4, self.option)

        res[5] = lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_State, 0x8002, self.option)

        for i in range(6):
            if res[i] != MACIE_OK:
                self.log.send(self._iam, ERROR, "ASIC configuration failed - write ASIC registers")
                return False

        ti.sleep(1.5)

        val, sts = self.read_ASIC_reg(ASICAddr_State)
        if (val[0] & 1) != 0 or sts != MACIE_OK:
            self.log.send(self._iam, ERROR, "ASIC configuration for shorted preamp inputs failed")
            return False
        self.log.send(self._iam, INFO, "Configuration succeeded")

        # step 2.science interface
        frameSize = (self.x_stop - self.x_start + 1) * (self.y_stop - self.y_start + 1)

        arr_list = []
        arr = np.array(arr_list)
        buf = arr.ctypes.data_as(POINTER(c_int))

        sts = lib.MACIE_ConfigureGigeScienceInterface(self.handle, self.slctMACIEs, 0, frameSize, 42037, buf)  # 0-16bit
        if sts != MACIE_OK:
            msg = "Science interface configuration failed. buf = %d" % buf[0]
            self.log.send(self._iam, ERROR, msg)
            return False
        msg = "Science interface configuration succeeded. buf (KB) = %d" % buf[0]
        self.log.send(self._iam, INFO, msg)

        # step 3.trigger ASIC to read science data
        self.log.send(self._iam, INFO, "Trigger image acquisition....")

        # make sure h6900 bit<0> is 0 before triggering.

        val, sts = self.read_ASIC_reg(ASICAddr_State)
        if sts != MACIE_OK:
            msg = "Read ASIC h%04x failed" % ASICAddr_State
            self.log.send(self._iam, ERROR, msg)
            return False
        if (val[0] & 1) != 0:
            msg = "Configure idle mode by writing ASIC h%04x failed" % ASICAddr_State
            self.log.send(self._iam, INFO, msg)
            return False

        if lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_NReads, 15, self.option) != MACIE_OK:
            self.log.send(self._iam, ERROR, "write ASIC h4001 " + RET_FAIL)
            return False

        if lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_State, 0x8001, self.option) != MACIE_OK:
            self.log.send(self._iam, ERROR, "Triggering " + RET_FAIL)
            return False

        self.log.send(self._iam, INFO, "Triggering succeeded")
        
        return True


    
    def StopAcquisition(self):
        if self.handle == 0:
            return False

        #if lib.MACIE_WriteASICReg(self.handle, self.slctASICs, ASICAddr_State, 0x8002, self.option) != MACIE_OK:
        #    self.log.send(self._iam, WARNING, "Acquire Stop " + RET_FAIL)
        #    return False

        self.log.send(self._iam, INFO, "Acquire Stop " + RET_OK)            

        return True


    def ImageAcquisition(self, local=True):
        if self.handle == 0:
            return False

        # Wait for available science data bytes
        idleReset, moreDelay = 1, 2000
        triggerTimeout = (T_frame * 1000) * (self.resets + idleReset) + moreDelay  # delay time for one frame
        msg = "triggerTimeout 1: %.3f" % triggerTimeout
        self.log.send(self._iam, DEBUG, msg)

        ti.sleep(1.5)
        
        getByte = 0
        if self.samplingMode == UTR_MODE:
            getByte = FRAME_X * FRAME_Y * 2 * self.reads * self.groups * self.ramps
            # 1000 -> 10000: increse wating time for long exposure

            drops = self.drops
            if drops == 0:
                drops = 1                
            
            triggerTimeout = triggerTimeout + ((T_frame * self.resets) + T_frame * drops * self.groups) * self.ramps * 1000 #100000
            msg = "triggerTimeout 2: %.3f" % triggerTimeout
            self.log.send(self._iam, DEBUG, msg)

        else:
            getByte = FRAME_X * FRAME_Y * 2 * 2 * self.reads * self.ramps
            triggerTimeout = triggerTimeout + ((T_frame * self.resets) + self.fowlerTime + (2 * T_frame * self.reads)) * self.ramps * 1000 #100000
            msg = "triggerTimeout 2: %.3f %.3f" % (self.fowlerTime, triggerTimeout)
            self.log.send(self._iam, DEBUG, msg)

        byte = 0
        for i in range(int(triggerTimeout / 1000 * 2)):
            if self.stop:
                break

            byte = lib.MACIE_AvailableScienceData(self.handle)
            if byte >= getByte:
                msg = "Available science data = %d bytes, Loop = %d" % (
                    byte, i)
                self.log.send(self._iam, INFO, msg)
                break
            log = "Wait....(%d), stop(%d)" % (i, self.stop)
            self.log.send(self._iam, INFO, log)
            #ti.sleep(triggerTimeout / 100 / 1000)
            ti.sleep(1)

        if self.stop:
            self.log.send(self._iam, INFO, "Stop: Image Acquiring")
            return False

        if byte <= 0:
            self.log.send(self._iam, WARNING, "Trigger timeout: no available science data")
            return False

        #data = None
        arr_list = []
        arr = np.array(arr_list)
        data = arr.ctypes.data_as(POINTER(c_ushort))
        
        data = lib.MACIE_ReadGigeScienceFrame(self.handle, int(1500 + 5000))
        if data == None:
            self.log.send(self._iam, WARNING, "Null frame")
            return False

        frmcnt = 0
        if self.samplingMode == UTR_MODE:
            frmcnt = self.reads * self.groups * self.ramps
        else:
            frmcnt = 2 * self.reads * self.ramps

        #self.loadimg = data[0:FRAME_X * FRAME_Y*frmcnt]

        self.loadimg = []
        for i in range(frmcnt):
            start = FRAME_X * FRAME_Y
            self.loadimg.append(data[start*i:start*(i+1)])

        lib.MACIE_CloseGigeScienceInterface(self.handle, self.slctMACIEs)

        self.folder_name, self.full_path = self.WriteFitsFile(local)

        return True


    def ImageAcquisition_window(self):
        if self.handle == 0:
            return False

        # Wait for available science data bytes
        idleReset, moreDelay = 1, 2000
        triggerTimeout = (T_frame * 1000) * (self.resets + idleReset) + moreDelay  # delay time for one frame
        msg = "triggerTimeout 1: %.3f" % triggerTimeout
        self.log.send(self._iam, DEBUG, msg)

        ti.sleep(1.5)
        
        getByte, frameSize = 0, 0
        frameSize = (self.x_stop - self.x_start + 1) * (self.y_stop - self.y_start + 1)
        getByte = frameSize * 2
        #print(getByte)
        
        byte = 0        
        for i in range(20):
            byte = lib.MACIE_AvailableScienceData(self.handle)
            if byte >= getByte:
            #if byte > 0:
                msg = "Available science data = %d bytes, Loop = %d" % (
                    byte, i)
                self.log.send(self._iam, INFO, msg)
                break
            self.log.send(self._iam, INFO, "Wait (ROI)....")
            ti.sleep(triggerTimeout / 20 / 1000)

        if byte <= 0:
            self.log.send(self._iam, WARNING, "Trigger timeout: no available science data")
            return False

        #data = None
        arr_list = []
        arr = np.array(arr_list)
        data = arr.ctypes.data_as(POINTER(c_ushort))
        
        data = lib.MACIE_ReadGigeScienceFrame(self.handle, int(1500 + 5000))

        if data == None:
            self.log.send(self._iam, WARNING, "Null frame (ROI)")
            return False

        self.loadimg = data

        lib.MACIE_CloseGigeScienceInterface(self.handle, self.slctMACIEs)

        self.folder_name = self.WriteFitsFile_window()

        return True


    def createFolder(self, dir):
        try:
            if not os.path.exists(dir):
                os.makedirs(dir)
        except OSError:
            self.log.send(self._iam, WARNING, "Error: Creating directory. " + dir)


        
    def WriteFitsFile(self, local = True):
        self.log.send(self._iam, INFO, "Write Fits file now....")

        _t = datetime.datetime.utcnow()

        cur_datetime = [_t.year, _t.month, _t.day, _t.hour, _t.minute, _t.second, _t.microsecond]

        path = "%s/Data/" % self.exe_path
        self.createFolder(path)

        if self.samplingMode == UTR_MODE:  # single mode
            path += "UpTheRamp/"
        elif self.samplingMode == CDS_MODE:  # ramp=1, group=1, read=1
            path += "CDS/"
        elif self.samplingMode == CDSNOISE_MODE:  # ramp=2, group=1, read=1
            path += "CDSNoise/"
        elif self.samplingMode == FOWLER_MODE:  # ramp=1, group=1, read=1,2,4,8,16
            path += "Fowler/"
        self.createFolder(path)

        #folder_name = "%04d%02d%02d_%02d%02d%02d" % (cur_datetime[0], cur_datetime[1], cur_datetime[2], cur_datetime[3], cur_datetime[4], cur_datetime[5])
        folder_name = "%04d%02d%02d/" % (cur_datetime[0], cur_datetime[1], cur_datetime[2])
        path += folder_name + "/"
        self.createFolder(path)

        numbers = list(map(int, os.listdir(path)))
        next_idx = max(numbers) + 1
        path2 += next_idx + "/"
        self.createFolder(path2)
        
        idx = 0
        #------------------------------------------------------------------------
        if self.samplingMode == UTR_MODE:  # single mode
            for ramp in range(self.ramps):
                for group in range(self.groups):
                    for read in range(self.reads):

                        filename = "%sH2RG_R%02d_M%02d_N%02d.fits" % (path2, ramp + 1, group+1, read + 1)
                        sts = self.save_fitsfile_sub(idx, filename, cur_datetime, ramp+1, group+1, read+1)

                        if sts != MACIE_OK:
                            self.log.send(self._iam, ERROR, self.GetErrMsg())
                            return -1
                        else:
                            self.log.send(self._iam, INFO, filename)

                        idx += 1

            self.measured_durationT = ti.time() - self.measured_startT
            
            '''
            if local and self.showfits and self.ramps == 1 and self.groups == 1 and self.reads == 1:
                ds9 = WORKING_DIR + 'DCS/ds9'
                #subprocess.run([ds9, '-b', filename, '-o', 'newfile'], shell = True)
                subprocess.Popen(['sudo', ds9, filename])
                self.log.send(self._iam, INFO, ds9)
            '''

            return '', filename

        elif self.samplingMode == CDS_MODE:  # ramp=1, group=1, read=1
            for read in range(self.reads*2):
                filename = "%sH2RG_R01_M01_N%02d.fits" % (path2, read + 1)
                sts = self.save_fitsfile_sub(idx, filename, cur_datetime, 1, 1, read+1)

                if sts != MACIE_OK:
                    self.log.send(self._iam, ERROR, self.GetErrMsg())
                    return -1
                else:
                    self.log.send(self._iam, INFO, filename)

                idx += 1

        elif self.samplingMode == CDSNOISE_MODE:  # ramp=2, group=1, read=1
            for ramp in range(self.ramps):
                for read in range(self.reads*2):
                    filename = "%sH2RG_R%02d_M01_N%02d.fits" % (path2, ramp + 1, read + 1)
                    sts = self.save_fitsfile_sub(idx, filename, cur_datetime, ramp + 1, 1, read+1)

                    if sts != MACIE_OK:
                        self.log.send(self._iam, ERROR, self.GetErrMsg())
                        return -1
                    else:
                        self.log.send(self._iam, INFO, filename)

                    idx += 1

        elif self.samplingMode == FOWLER_MODE:  # ramp=1, group=1, read=1,2,4,8,16
            for group in range(2):
                for read in range(self.reads):
                    filename = "%sH2RG_R01_M%02d_N%02d.fits" % (path2, group+1, read + 1)
                    sts = self.save_fitsfile_sub(idx, filename, cur_datetime, 1, group+1, read+1)

                    if sts != MACIE_OK:
                        self.log.send(self._iam, ERROR, self.GetErrMsg())
                        return -1
                    else:
                        self.log.send(self._iam, INFO, filename)

                    idx += 1
    
        startime = ti.time()

        #-----------------------------------------------------------------------
        arr = np.array(self.loadimg, dtype=np.int16)
        data = arr.ctypes.data_as(POINTER(c_ushort))

        arr_list = []
        arr = np.array(arr_list, dtype=np.float32)
        res = arr.ctypes.data_as(POINTER(c_float))

        res = fowler_calculation(self.samplingMode, self.reads, idx, data)

        filename = ""
        if self.samplingMode == CDS_MODE:
            lastfilename = "%sH2RG_R01_M01_N02.fits" % path2
            #filename = "CDSResult.fits"
            filename = "SDC%s_CDSResult_%s_%d.fits" % (IAM[-1], folder_name, next_idx)
            self.save_fitsfile_final(lastfilename, path, filename, self.reads, res)
        
        elif self.samplingMode == CDSNOISE_MODE:
            reslist = []
            for i in range(3):
                start = FRAME_X * FRAME_Y
                reslist.append(res[start*i:start*(i+1)])

                lastfilename = "%sH2RG_R02_M01_N02.fits" % path2
                if i < 2:
                    #filename = "CDSResult%d.fits" % (i+1)
                    filename = "SDC%s_CDSResult%d_%s_%d.fits" % (IAM[-1], (i+1), folder_name, next_idx)
                else:
                    #filename = "CDSNoise.fits"
                    filename = "SDC%s_CDSNoise_%s_%d.fits" % (IAM[-1], folder_name, next_idx)

                self.save_fitsfile_final(lastfilename, path, filename, self.reads, reslist[i])


        elif self.samplingMode == FOWLER_MODE:
            lastfilename = "%sH2RG_R01_M02_N%02d.fits" % (path2, self.reads)
            #filename = "FowlerResult.fits"
            #filename = "SDC%s_%s.fits" % (IAM[-1], folder_name)
            filename = "SDC%s_%s_%d.fits" % (IAM[-1], folder_name, next_idx)
            
            self.save_fitsfile_final(lastfilename, path, filename, self.reads, res)

        #-----------------------------------------------------------------------
        
        duration = ti.time() - startime
        tmp = "fowler calculation time: %.3f" % duration
        self.log.send(self._iam, INFO, tmp)
        
        self.measured_durationT = ti.time() - self.measured_startT
        
        '''
        if local and self.showfits:
            ds9 = WORKING_DIR + 'DCS/ds9'
            #subprocess.run([ds9, '-b', lastfilename, '-o', 'newfile'], shell = True)
            #subprocess.run([ds9, filename], shell = True)
            resfile = "%sResult/%s" % (path, filename)
            subprocess.Popen(['sudo', ds9, resfile])
            self.log.send(self._iam, INFO, ds9)
        '''

        return folder_name, path + "Result/" + filename

    
    def WriteFitsFile_window(self):
        self.log.send(self._iam, INFO, "Write Fits file now (ROI)....")
    
        _t = datetime.datetime.utcnow()

        cur_datetime = [_t.year, _t.month, _t.day, _t.hour, _t.minute, _t.second, _t.microsecond]

        path = "%s/Data/" % self.exe_path
        self.createFolder(path)

        path += "ROI/"
        self.createFolder(path)

        #str = time.strftime("%Y%m%d_%H%M%S", time.localtime(time.time()))
        folder_name = "%04d%02d%02d_%02d%02d%02d" % (cur_datetime[0], cur_datetime[1], cur_datetime[2], cur_datetime[3], cur_datetime[4], cur_datetime[5])
        path += folder_name + "/"
        self.createFolder(path)

        filename = "%sH2RG_R01_M01_N01.fits" % path
        sts = self.save_fitsfile_sub(0, filename, cur_datetime, 1, 1, 1)

        if sts != MACIE_OK:
            self.log.send(self._iam, ERROR, self.GetErrMsg())
            return -1
        else:
            self.log.send(self._iam, INFO, filename)

        return folder_name
            


    def save_fitsfile_sub(self, idx, filename, cur_datetime, ramp, group, read):
        
        header_array = MACIE_FitsHdr * FITS_HDR_CNT
        pHeaders = header_array()

        header_cnt = 0

        y, m, d = cur_datetime[0], cur_datetime[1], cur_datetime[2]
        _h, _m, _s, _ms = cur_datetime[3], cur_datetime[4], cur_datetime[5], cur_datetime[6]
        obs_datetime = "%04d-%02d-%02dT%02d:%02d:%02d.%03d" % (y, m, d, _h, _m, _s, _ms)

        t = Time(obs_datetime, format='isot', scale='utc')
        julian = t.to_value('jd', 'long')

        julian_time = "%f" % julian
        pHeaders[header_cnt] = MACIE_FitsHdr(key="ACQTIME".encode(), valType=HDR_STR, sVal=julian_time.encode(), comment="UTC Julian time".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="ACQTIME1".encode(), valType=HDR_STR, sVal=obs_datetime.encode(), comment="UTC time (YYYY-MM-DDTHH:MM:SS.MS)".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="UNITS".encode(), valType=HDR_STR, sVal="ADU".encode(), comment="ADC digital steps".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="MUXTYPE".encode(), valType=HDR_INT, iVal=MUX_TYPE, comment="1- H1RG; 2- H2RG; 4- H4RG".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="NOUTPUTS".encode(), valType=HDR_INT, iVal=32, comment="Number of detector outputs".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="FRMMODE".encode(), valType=HDR_INT, iVal=self.ROIMode, comment="0- full frame mode; 1- window mode".encode())
        header_cnt += 1

        if self.samplingMode == UTR_MODE:
            val = 0
        else:
            val = 1
        pHeaders[header_cnt] = MACIE_FitsHdr(key="EXPMODE".encode(), valType=HDR_INT, iVal=val, comment="0-Ramp mode; 1- Fowler sampling mode".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="NRESETS".encode(), valType=HDR_INT, iVal=self.resets, comment="Number of resets before integration".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="FRMTIME".encode(), valType=HDR_FLOAT, fVal=T_frame, comment="Frame time".encode())        
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="EXPTIME".encode(), valType=HDR_FLOAT, fVal=self.expTime, comment="sec, Exposure Time".encode())
        header_cnt += 1
        
        pHeaders[header_cnt] = MACIE_FitsHdr(key="FOWLTIME".encode(), valType=HDR_FLOAT, fVal=self.fowlerTime, comment="sec, Fowler Time".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="ASICGAIN".encode(), valType=HDR_INT, iVal=self.preampGain, comment="8 (12dB, large Cin)".encode())
        header_cnt += 1

        val = "0x%04x" % self.preampInputVal
        pHeaders[header_cnt] = MACIE_FitsHdr(key="AMPINPUT".encode(), valType=HDR_STR, sVal=val.encode(), comment="Preamp input".encode())
        header_cnt += 1

        #-------------------------------------------------------------------------------------
        #information
        _name = IAM[-1]
        pHeaders[header_cnt] = MACIE_FitsHdr(key="BAND".encode(), valType=HDR_STR, sVal=_name.encode(), comment="Band name".encode())
        header_cnt += 1
        
        _num = self.pCard[0].contents.macieSerialNumber
        pHeaders[header_cnt] = MACIE_FitsHdr(key="SERIALN".encode(), valType=HDR_INT, iVal=_num, comment="MACIE serial number".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="FIRMSLOT".encode(), valType=HDR_STR, sVal=self.pCard[0].contents.firmwareSlot1, comment="MACIE slot id".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="SWVER".encode(), valType=HDR_FLOAT, fVal=lib.MACIE_LibVersion(), comment="Software version number".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="CONNECT".encode(), valType=HDR_INT, iVal=2, comment="0: NONE; 1: USB; 2: GigE; 3: UART".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="FASTMODE".encode(), valType=HDR_INT, iVal=0, comment="1: FastMode; 0: SlowMode".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="ASICSET".encode(), valType=HDR_STR, sVal=self.asic_file.encode(), comment="ASIC firmware file".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="MACIESET".encode(), valType=HDR_STR, sVal=self.macie_file.encode(), comment="MACIE register file".encode())
        header_cnt += 1

        #-------------------------------------------------------------------------------------
        #temperature
        if self.dewar_info:
            pressure = "%.2e" % self.dewar_dict["pressure"]
            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_PRESSU".encode(), valType=HDR_STR, sVal=pressure.encode(), comment="Dewar Vacuum Pressure".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_BENCH".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["bench"], comment="Dewar Temp. Optical Bench".encode())
            header_cnt += 1
            
            pHeaders[header_cnt] = MACIE_FitsHdr(key="SP_BENCH".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["bench_sp"], comment="Dewar SetP. Optical Bench".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_GRATIN".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["grating"], comment="Dewar Temp. Immersion Grating".encode())
            header_cnt += 1  
            
            pHeaders[header_cnt] = MACIE_FitsHdr(key="SP_GRATI".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["grating_sp"], comment="Dewar SetP. Immersion Grating".encode())
            header_cnt += 1       

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_DETS".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["detS"], comment="Dewar Temp. Det S".encode())
            header_cnt += 1
            
            pHeaders[header_cnt] = MACIE_FitsHdr(key="SP_DETS".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["detS_sp"], comment="Dewar SetP. Det S".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_DETK".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["detK"], comment="Dewar Temp. Det K".encode())
            header_cnt += 1
            
            pHeaders[header_cnt] = MACIE_FitsHdr(key="SP_DETK".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["detK_sp"], comment="Dewar SetP. Det K".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_CAMH".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["camH"], comment="Dewar Temp. Cam H".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_DETH".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["detH"], comment="Dewar Temp. Det H".encode())
            header_cnt += 1
            
            pHeaders[header_cnt] = MACIE_FitsHdr(key="SP_DETH".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["detH_sp"], comment="Dewar SetP. Det H".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_BENCEN".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["benchcenter"], comment="Dewar Temp. Bench center".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_COLDH1".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["coldhead01"], comment="Dewar Temp. 1st ColdHead".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_COLDH2".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["coldhead02"], comment="Dewar Temp. 2nd ColdHead".encode())
            header_cnt += 1
            
            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_COLDST".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["coldstop"], comment="Dewar Temp. Cold stop".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_CARBOX".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["charcoalBox"], comment="Dewar Temp. Charcoal Box".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_CAMK".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["camK"], comment="Dewar Temp. Cam K".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_SHTOP".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["shieldtop"], comment="Dewar Temp. Rad. Shield".encode())
            header_cnt += 1

            pHeaders[header_cnt] = MACIE_FitsHdr(key="T_AIR".encode(), valType=HDR_FLOAT, fVal=self.dewar_dict["air"], comment="Dewar Temp. Rack".encode())
            header_cnt += 1

        #-------------------------------------------------------------------------------------
        
        pHeaders[header_cnt] = MACIE_FitsHdr(key="SEQNUM_R".encode(), valType=HDR_INT, iVal=ramp, comment="Ramp number".encode())
        header_cnt += 1

        pHeaders[header_cnt] = MACIE_FitsHdr(key="SEQNUM_N".encode(), valType=HDR_INT, iVal=read, comment="Sample number within group".encode())
        header_cnt += 1

        if self.samplingMode != UTR_MODE:
            base_fomula = T_frame * read + self.fowlerTime * (read-1)
            integratingT = 0
            if self.samplingMode == CDS_MODE:
                integratingT = base_fomula

            elif self.samplingMode == CDSNOISE_MODE:
                if ramp == 1:
                    integratingT = base_fomula
                else:
                    integratingT = (T_frame * 2 + self.fowlerTime) + base_fomula

            elif self.samplingMode == FOWLER_MODE:
                if group == 1:
                    integratingT = T_frame * read
                else:
                    integratingT = (T_frame * self.reads + self.fowlerTime) + T_frame * read
            
            pHeaders[header_cnt] = MACIE_FitsHdr(key="INTTIME".encode(), valType=HDR_FLOAT, fVal=integratingT, comment="integration time".encode())
            header_cnt += 1
            
        pHeaders[header_cnt] = MACIE_FitsHdr(key="SEQNUM_M".encode(), valType=HDR_INT, iVal=group, comment="1- before exposure; 2- after exposure.".encode())
        header_cnt += 1

        str = "R%02d_M%02d" % (ramp, group)
        pHeaders[header_cnt] = MACIE_FitsHdr(key="SEQNNAME".encode(), valType=HDR_STR, sVal=str.encode(), comment="Ramp and Group String".encode())
        header_cnt += 1

        if self.ROIMode:
            arr = np.array(self.loadimg, dtype=np.int16)
            data = arr.ctypes.data_as(POINTER(c_ushort))
            sts = lib.MACIE_WriteFitsFile(c_char_p(filename.encode()), self.x_stop - self.x_start + 1, self.y_stop - self.y_start + 1, data, header_cnt, pHeaders)
        else:
            arr = np.array(self.loadimg[idx], dtype=np.int16)
            data = arr.ctypes.data_as(POINTER(c_ushort))
            sts = lib.MACIE_WriteFitsFile(c_char_p(filename.encode()), FRAME_X, FRAME_Y, data, header_cnt, pHeaders)
        
        # for tunning test
        if self.samplingMode == UTR_MODE:
            offset, active = tn.cal_mean(filename)
            tunning = "%s: %.4f, %.4f" % (self.V_refmain, offset, active)
            self.log.send(self._iam, DEBUG, tunning)

        return sts


    def save_fitsfile_final(self, lastfilename, fullpath, filename, sampling, data):

        self.createFolder(fullpath)

        buf = []
        buf = data[0:FRAME_X * FRAME_Y]  
        #img = np.zeros([FRAME_X, FRAME_Y], dtype=np.float32)
        img = np.array(buf, dtype=np.float32)
        img = img.reshape(FRAME_X, FRAME_Y)
        
        #hdu = fits.PrimaryHDU(img)
        #hdul = fits.HDUList([hdu])

        hdulist = fits.open(lastfilename)   #read last fits file for getting header

        new_header = hdulist[0].header[:-5]
        
        new_header["SAMPLING"] = (sampling, "Sample number")
        
        new_header["COMMENT1"] = "This FITS file may contain long string keyword values that are"
        new_header["COMMENT2"] = "continued over multiple keywords.  This convention uses the  '&'"
        new_header["COMMENT3"] = "character at the end of a string which is then continued"
        new_header["COMMENT4"] = "on subsequent keywords whose name = 'CONTINUE"

        new_header["FITSFILE"] = fullpath
        new_header["CONTINUE"] = filename                

        fits.writeto(fullpath+filename, img, header=new_header) #, img, header, output_verify='ignore', overwrite=True)

        #hdul.close()


    def GetTelemetry(self):
        if self.handle == 0:
            return False

        #tlm = [0.0 for _ in range(79)]
        arr_list = [0.0 for _ in range(79)]
        arr = np.array(arr_list)
        tlm = arr.ctypes.data_as(POINTER(c_float))
        if lib.MACIE_GetTelemetryAll(self.handle, self.slctMACIEs, tlm) != MACIE_OK:
            self.log.send(self._iam, ERROR, self.GetErrMsg())
            return False
        else:
            self.log.send(self._iam, INFO, "MACIE_GetTelemetryAll " +  RET_OK)
            for i in range(79):
                msg = "%f" % tlm[i]
                self.log.send(self._iam, INFO, msg)

        return True 

    

if __name__ == "__main__":
    
    dc = DC()

    #del dc
