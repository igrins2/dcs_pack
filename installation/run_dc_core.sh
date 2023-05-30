#!/bin/bash

sysctl -w net.core.rmem_max="134000000"
firewall-cmd --zone=trusted --change-interface=eno1

HOME=/home/dcss

export LD_LIBRARY_PATH=$HOME/macie_v5.3_centos/MacieApp/
export PATH=$PATH:$HOME/macie_v5.3_centos/MacieApp/

PYTHONBIN=$HOME/miniconda3/envs/dcs/bin/python

source ~/.bash_profile
conda activate dcs

$PYTHONBIN $HOME/dcs_pack/code/DetCtrl/DC_core.py


