//20230802 for ROI mode by hilee

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "/home/dcss/macie_v5.3_centos/MacieApp/macie.h"

using namespace std;

//rm -f roi_mode.o libroi_mode.so
//g++ -fPIC -c roi_mode.cpp
//g++ -shared -o libroi_mode.so /home/dcss/macie_v5.3_centos/MacieApp/libMACIE.so roi_mode.o

void delay(int ms)
{
	clock_t t0 = clock();
	while ((clock() - t0) * 1000 / CLOCKS_PER_SEC < ms)
		;
}

extern "C"
{
    unsigned short* Acquire_SlowWinRamp(unsigned long handle, unsigned char slctMACIEs, unsigned char slctASIC, 
                                        unsigned int preampInputScheme, unsigned int preampInputVal, unsigned int preampGain, 
                                        unsigned int x_start, unsigned int x_stop, unsigned int y_start, unsigned int y_stop)
    {
        cout << "Acquire Science Data..." << endl;
        unsigned int val = 0;

        // step1: ASIC configuration, see detail in SIDECAR ASIC firmware manual
        //unsigned int preampInputScheme = 2, preampInputVal = 0xaaaa, preampGain = 1;
        //unsigned int preampInputScheme = 2, preampInputVal = 0x4502, preampGain = 1;
        MACIE_STATUS r[7];

        //printf("%d\r\n", preampInputScheme);
        //printf("%d\r\n", preampInputVal);
        //printf("%d\r\n", preampGain);

        r[0] = MACIE_WriteASICReg(handle, slctASIC, 0x4019, preampInputScheme, true);
        r[1] = MACIE_WriteASICReg(handle, slctASIC, 0x5100, preampInputVal, true);
        r[2] = MACIE_WriteASICReg(handle, slctASIC, 0x401a, preampGain, true);
        r[3] = MACIE_WriteASICReg(handle, slctASIC, 0x4001, 1, true);

        //win mode
        r[4] = MACIE_WriteASICReg(handle, slctASIC, 0x4018, 2, true);
        //unsigned int winarr[4] = { 0, 300, 0, 200 };
        unsigned int winarr[4] = {x_start, x_stop, y_start, y_stop};
        //for(int i = 0; i<4; i++)
        //    printf("%d\r\n", winarr[i]);

        r[5] = MACIE_WriteASICBlock(handle, slctASIC, 0x4020, winarr, 4, true);

        // triggle Config
        r[6] = MACIE_WriteASICReg(handle, slctASIC, 0x6900, 0x8002, true);
        for(int i; i < 7; i++){
            if(r[i] != MACIE_OK){
                cout << "ASIC configuration failed - write ASIC registers" << endl;
                return NULL;
            }

        }

        // wait for configuration: better to optimize the delay according to SIDECAR assembly code manual
        delay(1500);
        MACIE_STATUS status1 = MACIE_ReadASICReg(handle, slctASIC, 0x6900, &val, false, true);
        if (((val & 1) != 0) || (status1 != MACIE_OK))
        {
            cout << "ASIC configuration for shorted preamp inputs failed" << endl;
            return NULL;
        }
        cout << "Configuration succeeded" << endl;
        
        // step 2: science interface (GigE vs. Camlink vs.USB) configuration
        
        int buf = 0;
        int frameX = (winarr[1] - winarr[0] + 1);
        int frameY = (winarr[3] - winarr[2] + 1);
        int frameSize = frameX * frameY;

        //printf("%d, %d\r\n", frameX, frameY);
        status1 = MACIE_ConfigureGigeScienceInterface(handle, slctMACIEs, 0, frameSize, 42037, &buf);
        if (!status1 == MACIE_OK)
        {
            cout << "Science interface configuration failed. buf = " << buf << endl;
            return NULL;
        }
        cout << "Science interface configuration succeeded. buf (KB) = " << buf << endl;

        // step 3: trigger ASIC to read science data
        cout << "Trigger image acquisition... " << endl;

        unsigned short frametime = 1500, nResets = 1, idleReset = 1, moreDelay = 4000;
        unsigned long n = 0;
        unsigned short triggerTimeout = frametime * (nResets + idleReset) + moreDelay; 
        char fileName[] = "/home/dcss/macie_v5.3_centos/examplecode/testImage.fits";

        // make sure h6900 bit<0> is 0 before triggering. 
        
        if (MACIE_ReadASICReg(handle, slctASIC, 0x6900, &val, false, true) != MACIE_OK)
        {
            cout << "Read ASIC h6900 failed" << endl;
            return NULL;
        }
        if ((val & 1) != 0)
        {
            cout << "Configure idle mode by writing ASIC h6900 failed" << endl;
            return NULL;
        }

        // configure nRead
        /*if (MACIE_WriteASICReg(handle, slctASIC, 0x4001, 15, true) != MACIE_OK)
        {
            cout << "write ASIC h4001 failed" << endl;
            return NULL;
        }*/
        if (MACIE_WriteASICReg(handle, slctASIC, 0x6900, 0x8001, true) != MACIE_OK)
        {
            cout << " Triggering failed" << endl;
            return NULL;
        }
        cout << " Triggering succeeded" << endl;
        
        delay(200);
        // Wait for available science data bytes
        for (int i = 0; i < 20; i++)
        {
            n = MACIE_AvailableScienceData(handle);
            if (n > 0)
            {
                cout << "Available science data = " << n << " bytes, Loop = " << i << endl;
                break;
            }
            cout << "Wait...." << endl;
            delay(triggerTimeout / 10);
        }
        if (n <= 0)
        {
            cout << "trigger timeout: no available science data" << endl;
            return NULL;
        }
        // reading
        unsigned short *pData = NULL;
        //unsigned short *pData = new float[frameX*frameY];
        pData = MACIE_ReadGigeScienceFrame(handle, 1500 + 5000);

        if (!pData)
        {
            cout << "Null frame " << endl;
            return NULL;
        }
        cout << "Write Fits file now..." << endl;

        //printf("%s", pData);
        
        MACIE_CloseGigeScienceInterface(handle, slctMACIEs);

        cout << "Succeeded" << endl;
        
        return pData;
    }
}

