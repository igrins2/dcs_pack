# -*- coding: utf-8 -*-

"""
Created on Mar 4, 2022

Modified on May 8, 2023

@author: hilee
"""

import sys

import getpass
user = getpass.getuser()

try:
    dir = sys.argv[0].split('/')
    WORKING_DIR = "/home/%s/" % dir[2]
    IAM = dir[2].upper()

except:
    WORKING_DIR = "/home/%s/" % user
    IAM = user.upper()

#FUN_OK = 1
IAM = "DCSS"
FITS_HDR_CNT = 100

FieldNames = [('date', str), ('time', str),
              ('pressure', float),
              ('bench', float), ('bench_tc', float),
              ('grating', float), ('grating_tc', float),
              ('detS', float), ('detS_tc', float),
              ('detK', float), ('detK_tc', float),
              ('camH', float),
              ('detH', float), ('detH_tc', float),
              ('benchcenter', float), ('coldhead01', float), 
              ('coldhead02', float), ('coldstop', float), 
              ('charcoalBox', float), ('camK', float), 
              ('shieldtop', float), ('air', float), 
              ('alert_status', str)]

# result message
RET_OK = "succeeded"
RET_FAIL = "failed"

# LOG option
DEBUG = "DEBUG"
INFO = "INFO"
WARNING = "WARNING"
ERROR = "ERROR"

LOCAL = 0
ICS = 1
UPLOADER = 2
OBSAPP = 3


# ASIC addr
V_RESET = 0
D_SUB = 1
V_BIAS_GATE = 2
V_REF_MAIN = 3
INPUT = 4

MUX_TYPE = 2

# find dir/file
CONFIG_DIR = 0
MACIE_FILE = 1
ASIC_FILE = 2
IMG_DIR = 3

FRAME_X = 2048
FRAME_Y = 2048

BITSIZE_65536 = 65536

# sampling mode
UTR_MODE = 0    #single frame
CDS_MODE = 1
CDSNOISE_MODE = 2
FOWLER_MODE = 3

T_frame = 1.45479
T_exp = 1.623
T_minFowler = 0.168
T_br = 2
N_fowler_max = 16

MACIE_Reg = 0x0300
MACIE_Block_Reg = 0x321

ASICAddr_NResets = 0x4000
ASICAddr_NReads = 0x4001
ASICAddr_Config = 0x4002
ASICAddr_NRamps = 0x4003
ASICAddr_NGroups = 0x4004
ASICAddr_nNDrops = 0x4005

ASICAddr_TfowlerLower = 0x400a
ASICAddr_TfowlerUpper = 0x400b
ASICAddr_TexpLower = 0x400c
ASICAddr_TexpUpper = 0x400d
    
ASICAddr_HxRGVal = 0x4010
ASICAddr_HxRGNumOutVal = 0x4011
ASICAddr_HxRGExpModeVal = 0x4018    #exposure mode
ASICAddr_ASICInputRefVal = 0x4019
ASICAddr_ASICPreAmpGainVal = 0x401a
ASICAddr_WinArr = [0x4020, 0x4021, 0x4022, 0x4023]

ASICAddr_PreAmpReg1Ch1ENAddr = 0x5100

ASICAddr = 0x6100
ASICAddr_State = 0x6900

UPLOAD_Q = "UploadDBQ"    #uploader

OBSAPP_BUSY = "ObsAppBusy"

CMD_SIMULATION = "Simulation"
CMD_VERSION = "LibVersion"
#CMD_EXIT = "Exit"

CMD_INITIALIZE1 = "Initialize1"
CMD_INITIALIZE2 = "Initialize2"
CMD_RESET = "ResetASIC"
#CMD_DOWNLOAD = "DownloadMCD"
CMD_SETDETECTOR = "SetDetector"
#CMD_ERRCOUNT = "GetErrorCounters"
CMD_SETFSMODE = "SETFSMODE"
CMD_SETWINPARAM = "SetWinParam"
CMD_SETRAMPPARAM = "SetRampParam"
CMD_SETFSPARAM = "SetFSParam"
CMD_ACQUIRERAMP = "ACQUIRERAMP"
CMD_STOPACQUISITION = "STOPACQUISITION"
#CMD_CONNECT_ICS_Q = "ConnectToICSQ"
CMD_ASICLOAD = "LOADASIC"
CMD_WRITEASICREG = "writeASICreg"
CMD_READASICREG = "readASICreg"
CMD_GETTELEMETRY = "GetTelemetry"

ALIVE = "Alive"

CMD_INIT2_DONE = "Initialize2_Done"
CMD_INITIALIZE2_ICS = "Initialize2_ics"
CMD_SETFSPARAM_ICS = "SetFSParam_ics"
CMD_ACQUIRERAMP_ICS = "ACQUIRERAMP_ics"
