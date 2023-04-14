# -*- coding: utf-8 -*-
"""
Modified from SetConfig.py from IGRINS on Oct 25, 2021

@author: hilee

"""

import configparser as cp


def LoadConfig(path=None, interpolation=True):

    if path == None:
        return
    # load config object

    if interpolation:
        cfg = cp.ConfigParser()
        #cfg = cp.SafeConfigParser()
    else:
        cfg = cp.RawConfigParser()

    # read the configure file(s)
    cfg.read(path)

    # read the device list string and list and time-out interval

    # save the parameters
    return cfg


def get_ini_files(env_name, default_file):
    import os

    ini_path = os.environ.get(env_name, None)

    # print('ini_path:', ini_path)

    if ini_path is None:
        ini_path = default_file
        print(
            "No %s env is defined. "
            "Default of '%s' will be used." % (env_name, ini_path)
        )
    else:
        ini_path = [_.strip() for _ in ini_path.split(",") if _.strip()]
        print("Reading config files from %s env : %s" % (env_name, ini_path))

    return ini_path


def SaveConfig(cfg=None, path=None):

    if cfg == None:
        return

    with open(path, "w") as cfgfile:
        cfg.write(cfgfile)

