#!/bin/bash

if [[ ! -e $HOME/DCS ]]; then
    mkdir $HOME/DCS
    echo "mkdir"
elif [[ ! -d $HOME/DCS ]]; then
    echo $HOME + "/DCS already exists but is not a directory" 1>&2
fi


# set DCS directory including configuration file
cp -r $HOME/dcs_pack/installation/DCS $HOME


# set macie library
cp -r $HOME/dcs_pack/installation/macie_v5.3_centos $HOME/macie_v5.3_centos

sudo cp $HOME/dcs_pack/installation/macie_v5.3_centos/MacieApp/libMACIE.so /lib
sudo cp $HOME/dcs_pack/installation/macie_v5.3_centos/MacieApp/51-ftd3xx.rules /etc/udev/rules.d
sudo udevadm control --reload-rules

sudo sysctl -w net.core.rmem_max="134000000"
sudo firewall-cmd --zone=trusted --change-interface=ens160

export LD_LIBRARY_PATH=$HOME/macie_v5.3_centos/MacieApp/
export PATH=$PATH:$HOME/macie_v5.3_centos/MacieApp/


# install python library
cd $HOME/dcs_pack/installation
bash Miniconda3-latest-Linux-x86_64.sh
export PATH=$HOME/miniconda3/bin:$PATH
source ~/.bash_profile

conda update conda
conda create -n dcs python=3.9
conda activate dcs

pip install numpy
pip install astropy
pip install pyside6==6.4.2
pip install pyqt5
pip install pika


# ds9
#cp $HOME/dcs_pack/installation/ds9 $HOME/DCS/


# make Folwer Sampling library
cd $HOME/dcs_pack/code/FowlerCalculation
rm -f sampling_cal.o sampling_cal.so
g++ -fPIC -c sampling_cal.c
g++ -shared -o libsampling_cal.so sampling_cal.o


# start core
sudo chmod 744 $HOME/dcs_pack/installation/run_dc_core.sh
sudo cp $HOME/dcs_pack/installation/dc-core.service /etc/systemd/system/



