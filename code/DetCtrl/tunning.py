# -*- coding:utf-8 -*-

"""
Created on Sep 27, 2022

Modified on 

@author: hilee
"""

# tunning: CDS Noise

import astropy.io.fits as fits
import numpy as np

def cal_mean(path):
    #print("Get a mean value")

    head = fits.PrimaryHDU()
    frm = fits.open(path)
    data = frm[0].data
    head = frm[0].header

    img = []
    img = np.array(data, dtype="f")
    #_mean = np.mean(img)

    ref_img = []
    for row in range(2048):
        for col in range(2048):
            if row < 4 or row >= 2044:
                ref_img.append(img[row][col])
            else:
                if col < 4 or col >= 2044: 
                    ref_img.append(img[row][col])
                
    ref_img_mean = np.mean(ref_img)

    active_img = img[4:2044, 4:2044]
    active_img_mean = np.mean(active_img)

    #print("mean:", ref_img_mean, active_img_mean)

    ref, active = [], []
    for row in range(2048):
        for col in range(2048):
            if row < 4 or row >= 2044:
                ref.append(img[row][col])
            else:
                if col < 4 or col >= 2044: 
                    ref.append(img[row][col])
                else:
                    active.append(img[row][col])

    ref_aver = np.mean(ref)
    active_aver = np.mean(active)
    
    #print("mean 2:", ref_aver, active_aver)

    '''
    ref_sum, active_sum = 0, 0
    for row in range(2048):  
        if row < 4 or row >= 2044:
            ref_sum += np.sum(img[row][:])
        else:
            ref_sum += np.sum(img[row][0:4])
            ref_sum += np.sum(img[row][2044:2048])
            active_sum += np.sum(img[row][4:2044])

    ref_img_aver = ref_sum / ((2048*8)+(2040*8))
    active_img_aver = active_sum / (2040 * 2040)
    
    print("aver:", ref_img_aver, active_img_aver)

    ref, active = 0, 0
    for row in range(2048):
        for col in range(2048):
            if row < 4 or row >= 2044:
                ref += img[row][col]
            else:
                if col < 4 or col >= 2044: 
                    ref += img[row][col]
                else:
                    active += img[row][col]
    ref_aver = ref / (2048*8 + 2040*8)
    active_aver = active / (2040 * 2040)
    
    print("aver 2:", ref_aver, active_aver)
    '''

    return ref_img_mean, active_img_mean 
    

#path = "/DCS/Data/CDSNoise/20220927_024513/Result/CDSNoise.fits"
#cal_mean(path)