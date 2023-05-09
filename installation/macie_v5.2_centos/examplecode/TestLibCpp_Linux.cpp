//============================================================================
// TestLibCpp_Linux.cpp
//		Example code
//  
//  This code provides some examples for possible implementations of the MACIE
//  library functions. When running the code, it provides a list of options 
//  that can be executed by entering the number of the specific option.
//
//  Implemented functions are (enter the number to execute):
//  1. Check Interfaces:  Detects and reports available MACIE cards and communication interface
//
//  20-30. Get Handle for the selected MACIE card: 20 get handle for GigE interface; 21 get handle 
//  for camlink interface; 22 get handle for USB interface. If only one MACIE card is detected in the 
//  CheckInterface function, it is the selected MACIE card for handler; if more than one MACIE cards are
//  detected, the second MACIE is selected for the handler (for testing purpose)
//  Obtain the handle for the first available MACIE if only one MACIE is detected 
//
//  30. Detect Available MACIE cards:  Reports how many MACIE cards are connected at the GigE port
//     that is associated with the handle obtained under option 2.
//  40-50. Initialize system (MACIE & ASIC): Executes a number of steps to get the MACIE card and the
//     connected ASIC ready for operation. This includes loading the desired MACIE firmware, downloading
//     MACIE registers to configure operation and enable power, download SIDECAR microcode, verify if
//     ASIC is configured (Check Available ASICs).
//     40: Execute the sequence with HxRG_Main.mcd for GigE, slow mode 
//     41: Execute the sequence with DevBrd_H2RG_12bit_32output_5MHz_coldASIC_aaaa.mcd for camlink, fast mode
//     42: Execute the sequence with HxRG_Main.mcd for USB, slow mode 
//     43: Execute the sequence with DevBrd_H2RG_12bit_32output_5MHz_coldASIC_aaaa.mcd for USB, fast mode 
//     44: Execute the sequence for Camlink fast mode - SFI support
//
//  50-60. Capture single frame: Configures the ASIC for generating a single read frame, sets up the MACIE card and
//  computer for data acquisition, triggers ASIC to start the ramp, captures the frame, and write it to a fits file.
//  50: Slow mode Capture through GigE interface; 51: Fast mode Capture through Camlink interface;
//  52: Slow mode Capture through USB interface; 53: Slow mode window readout through USB interface;
//  54: Fast mode Capture through USB interface; 55: falst mode readout with external triggering

//  6. Read and report MACIE error counters
//  7. Download load file: Download a generic configuration file to MACIE and/or ASIC (not needed for normal
//     operation), just for demonstration purposes. Please change the path of LoadFile_Example.mcf if needed.
//  8. Download load file: Download a generic configuraiton file to MACIE, just for demonstration purposes.
//     Please change the path if needed.
//
//	60. Get full list of telemetry measurements. Total 79 items.
//  61. Get / set a set of telemetry measurements for the given array of telemetry items listed in the enum structure of MACIE_TLM_ITEM.
//  62. Get single telemetry item
//  63. Get / set telemetry configuration
//  64. Get the power control status for the given power control item listed in the structure of MACIE_PWR_CTRL. 
//      The status is indicated by the output parameter bEnablePower. 
//      Also enable / disable power for the givien power control id
//  65. Get and set MACIE voltage/current
//
//  70 Acquisition from Multi MACIEs and Multi ASICs
//  Please note: In order to execute a certain option, all prior options have to be executed in order first. 
//  (except for number 6 and 7, which only require to first execute option 1 and 2)
//============================================================================

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "macie.h"


using namespace std;


void delay(int ms)
{
	clock_t t0 = clock();
	while ((clock() - t0) * 1000 / CLOCKS_PER_SEC < ms)
		;
}


bool InitializeASIC(unsigned long handle, unsigned char slctMACIEs, short mode)
{
	unsigned int val = 0;

	cout << "Initialize with handle  " << handle << endl;

	if ((unsigned short)slctMACIEs == 0)
	{
		cout << "MACIE0 is not available" << endl;
		return false;
	}

	// step 1: load MACIE firmware from slot 1 or slot 2

	if (MACIE_loadMACIEFirmware(handle, slctMACIEs, false, &val) != MACIE_OK)
	{
		cout << "Load MACIE firmware failed: " << MACIE_Error() << endl;
		return false;
	}
	if (val != 0xac1e) // return value
	{
		cout << "Verification of MACIE firmware load failed: readback of hFFFB=" << val << endl;
		return false;
	}
	cout << "Load MACIE firmware succeeded" << endl;

	// step 2: download MACIE registers
	const char *filename = (mode != 0) ? "/home/jing/MyProjects/MACIE/MACIE_CONTROL/lbin/debug/LoadFiles/msac/MACIE_Registers_Fast.mrf" :
										"/home/jing/MyProjects/MACIE/MACIE_CONTROL/lbin/debug/LoadFiles/msac/MACIE_Registers_Slow.mrf";

	if (MACIE_DownloadMACIEFile(handle, slctMACIEs, filename) != MACIE_OK)
	{
		cout << "Download MACIE register file Failed: " << endl;
		cout << MACIE_Error() << endl;
		return false;
	}
	cout << "Download MACIE register file succeeded" << endl;
	
	unsigned int *pData = new unsigned int[5];
	MACIE_STATUS test = MACIE_ReadMACIEBlock(handle, slctMACIEs, 0x0321, pData, 5);
	if ( test != MACIE_OK)
		unsigned int  x = test;
	else
	{
		for (int a = 0; a < 5; a++)
			cout << " val = " << pData[a] << endl;
		unsigned int x = pData[1];
	}

	// step 3 reset science data error counters
	if (MACIE_ResetErrorCounters(handle, slctMACIEs) != MACIE_OK)
	{
		cout << "Reset MACIE error counters failed" << endl;
		return false;
	}
	cout << "Reset error counters succeeded" << endl;

	// step 4 download ASIC file, for example ASIC mcd file
	char *ASICFile0 = "/home/jing/MyProjects/MACIE/MACIE_CONTROL/lbin/debug/LoadFiles/msac/HxRG_Main.mcd";
	char *ASICFile1 = "/home/jing/MyProjects/MACIE/MACIE_CONTROL/lbin/debug/LoadFiles/msac/DevBrd_H2RG_12bit_32output_5MHz_coldASIC_aaaa.mcd";
	char *ASICFile2 = "/home/jing/MyProjects/MACIE/MACIE_CONTROL/lbin/debug/LoadFiles/msac/H2RG_5MHz_Ramp_VertWindow_coldASIC.mcd";

	MACIE_STATUS status;
	if (mode == 0)
		status = MACIE_DownloadASICFile(handle, slctMACIEs, ASICFile0, true);
	else if (mode == 1)
		status = MACIE_DownloadASICFile(handle, slctMACIEs, ASICFile1, true);
	else
		status = MACIE_DownloadASICFile(handle, slctMACIEs, ASICFile2, true);
	if (status != MACIE_OK) //m_asicIds
	{
		cout << "Download ASIC firmware failed: " << MACIE_Error() << endl;
		return false;
	}
	cout << "Download ASIC firmware succeeded" << endl;

	if (MACIE_ResetErrorCounters(handle, slctMACIEs) != MACIE_OK)
	{
		cout << "Reset MACIE error counters failed" << endl;
		return false;
	}
	cout << "Reset error counters succeeded" << endl;

	unsigned char avaiMACIE = MACIE_GetAvailableMACIEs(handle);
	unsigned char avaiASICs = MACIE_GetAvailableASICs(handle, 0);
	cout << MACIE_Error() << endl;
	if ((unsigned short)avaiASICs == 0)
	{
		cout << "MACIE_GetAvailableASICs failed" << endl;
		return false;
	}
	cout << "Available ASICs=" << (unsigned short)avaiASICs << endl;
	cout << "Initialization succeeded " << endl;
	return true;
}

// Acquire Image:
bool Acquire_SlowMode(unsigned long handle, unsigned char slctMACIEs, unsigned char slctASIC)
{
	cout << "Acquire Science Data..." << endl;
	unsigned int val = 0;

	// step1: ASIC configuration, see detail in SIDECAR ASIC firmware manual
	unsigned int preampInputScheme = 2, preampInputVal = 0xaaaa, preampGain = 1;
	MACIE_STATUS r1 = MACIE_WriteASICReg(handle, slctASIC, 0x4019, preampInputScheme, true);
	MACIE_STATUS r2 = MACIE_WriteASICReg(handle, slctASIC, 0x5100, preampInputVal, true);
	MACIE_STATUS r3 = MACIE_WriteASICReg(handle, slctASIC, 0x401a, preampGain, true);
	MACIE_STATUS r4 = MACIE_WriteASICReg(handle, slctASIC, 0x4001, 1, true);
	MACIE_STATUS r6 = MACIE_WriteASICReg(handle, slctASIC, 0x4018, 2, true); // window mode
	MACIE_STATUS r5 = MACIE_WriteASICReg(handle, slctASIC, 0x6900, 0x8002, true);
	if (( r1 != MACIE_OK) || ( r2 != MACIE_OK) || ( r3 != MACIE_OK)	|| ( r4 != MACIE_OK) || ( r5 != MACIE_OK) )
	{
		cout << "ASIC configuration failed - write ASIC registers" << endl;
		return false;
	}

	// wait for configuration: better to optimize the delay according to SIDECAR assembly code manual
	delay(1500);
	MACIE_STATUS status1 = MACIE_ReadASICReg(handle, slctASIC, 0x6900, &val, false, true);
	if (((val & 1) != 0) || (status1 != MACIE_OK))
	{
		cout << "ASIC configuration for shorted preamp inputs failed" << endl;
		return false;
	}
	cout << "Configuration succeeded" << endl;

	// step 2: science interface (GigE vs. Camlink vs.USB) configuration
	int buf = 0;
	int frameX = 2048, frameY = 2048;
	int frameSize = frameX * frameY;

	short connType = (handle & 0x0000000F);
	if (connType == short(MACIE_USB))
	{
		buf = 20;
		status1 = MACIE_ConfigureUSBScienceInterface(handle, slctMACIEs, 0, frameSize, buf);
	}
	else if (connType == short(MACIE_GigE))
	{
		status1 = MACIE_ConfigureGigeScienceInterface(handle, slctMACIEs, 0, frameSize, 42037, &buf);
	}
	else
	{
		cout << "Only GigE or USB is supported for slow mode operation" << endl;
		return false;
	}
	if (status1 != MACIE_OK)
	{
		cout << "Science interface configuration failed. buf = " << buf << endl;
		return false;
	}
	cout << "Science interface configuration succeeded. buf (KB) = " << buf << endl;

	// step 3: trigger ASIC to read science data
	cout << "Trigger image acquisition... " << endl;

	unsigned short frametime = 1500, nResets = 1, idleReset = 1, moreDelay = 2000;
	unsigned long n = 0;
	unsigned short triggerTimeout = frametime * (nResets + idleReset) + moreDelay;
	char fileName[] = "./test_image/testImage0.fits";

	// make sure h6900 bit<0> is 0 before triggering. 
	if (connType != short(MACIE_USB))
	{
		if (MACIE_ReadASICReg(handle, slctASIC, 0x6900, &val, false, true) != MACIE_OK)
		{
			cout << "Read ASIC h6900 failed" << endl;
			return false;
		}
		if ((val & 1) != 0)
		{
			cout << "Configure idle mode by writing ASIC h6900 failed" << endl;
			return false;
		}
        }
	if (MACIE_WriteASICReg(handle, slctASIC, 0x6900, 0x8001, false) != MACIE_OK)
	{
		cout << " Triggering failed" << endl;
		return false;
	}
	cout << " Triggering succeeded" << endl;

	delay(200);
	// Wait for available science data bytes
	for (int i = 0; i < 100; i++)
	{
		n = MACIE_AvailableScienceData(handle);
		if (n > 0)
		{
			cout << "After trigering: available science data = " << n << " bytes, Loop = " << i << endl;
			if (n >= 100 * 100)
			break;
		}
		cout << "Wait...." << endl;
		delay(triggerTimeout / 100);
	}
	if (n <= 0)
	{
		cout << "trigger timeout: no available science data" << endl;
		return false;
	}
	// reading
	unsigned short *pData = NULL;
	if (connType == short(MACIE_USB))
		pData = MACIE_ReadUSBScienceFrame(handle, 1500 + 5000);
	else
		pData = MACIE_ReadGigeScienceFrame(handle, 1500 + 5000);

	if (!pData)
	{
		cout << "Null frame " << endl;
		return false;
	}
	cout << "Write Fits file now..." << endl;
	MACIE_FitsHdr pHeaders[3];

	strncpy(pHeaders[0].key, "ASICGAIN", sizeof(pHeaders[0].key));
	pHeaders[0].iVal = 1;
	pHeaders[0].valType = HDR_INT;
	strncpy(pHeaders[0].comment, "SIDECAR Preamp gain setting", sizeof(pHeaders[0].comment));

	strncpy(pHeaders[1].key, "FrmTime", sizeof(pHeaders[1].key));
	pHeaders[1].fVal = 1.47;
	pHeaders[1].valType = HDR_FLOAT;
	strncpy(pHeaders[1].comment, "Frame time at slow mode", sizeof(pHeaders[1].comment));

	strncpy(pHeaders[2].key, "AcqDate", sizeof(pHeaders[2].key));
	strncpy(pHeaders[2].sVal, "Jan 20, 2018", sizeof(pHeaders[2].sVal));
	pHeaders[2].valType = HDR_STR;
	strncpy(pHeaders[2].comment, "Test comments here", sizeof(pHeaders[2].comment));

	MACIE_STATUS status = MACIE_WriteFitsFile(fileName, 2048, 2048, pData, 3, pHeaders);
	if (connType == MACIE_USB)
		MACIE_CloseUSBScienceInterface(handle, slctMACIEs);
	else
		MACIE_CloseGigeScienceInterface(handle, slctMACIEs);

	if (status != MACIE_OK)
	{
		cout << "Write fits file failed: " << MACIE_Error() << endl;
		return false;
	}
	cout << "Succeeded" << endl;
	return true;
}

bool Acquire_SlowMode_MultiMACIE(unsigned long handle, unsigned char slctMACIEs, unsigned char slctASICs, bool bProcessImage)
{
	cout << "Acquire Science Data..." << endl;
	unsigned int val = 0;

	// step1: ASIC configuration, see detail in SIDECAR ASIC firmware manual
	unsigned int preampInputScheme = 2, preampInputVal = 0xaaaa, preampGain = 1;
	MACIE_STATUS r1 = MACIE_WriteASICReg(handle, slctASICs, 0x4019, preampInputScheme, true);
	MACIE_STATUS r2 = MACIE_WriteASICReg(handle, slctASICs, 0x5100, preampInputVal, true);
	MACIE_STATUS r3 = MACIE_WriteASICReg(handle, slctASICs, 0x401a, preampGain, true);
	MACIE_STATUS r4 = MACIE_WriteASICReg(handle, slctASICs, 0x4001, 1, true);
	MACIE_STATUS r5 = MACIE_WriteASICReg(handle, slctASICs, 0x6900, 0x8002, true);
	if ((r1 != MACIE_OK) || (r2 != MACIE_OK) || (r3 != MACIE_OK) || (r4 != MACIE_OK) || (r5 != MACIE_OK))
	{
		cout << "ASIC configuration failed - write ASIC registers" << endl;
		return false;
	}

	// wait for configuration: better to optimize the delay according to SIDECAR assembly code manual
	delay(1500);
	MACIE_STATUS status1 = MACIE_ReadASICReg(handle, slctASICs, 0x6900, &val, false, true);
	if (((val & 1) != 0) || (status1 != MACIE_OK))
	{
		cout << "ASIC configuration for shorted preamp inputs failed" << endl;
		return false;
	}
	cout << "Configuration succeeded" << endl;
	
	// step 2: science interface (GigE vs. Camlink vs.USB) configuration
	int buf = 0;
	int frameX = 2048, frameY = 2048;
	int nASICs = 2;  // in actual application: calculate it based on the input slctASICs
	int frameSize = frameX * frameY * nASICs;

	// first: Multi-FIFO configuration
	if (MACIE_WriteMACIEReg(handle, slctMACIEs, 0x01b6, frameX - 1) != MACIE_OK)
	{
		cout << "FIFO configuration for MultiFIFO_Mode failed. " << endl;
		return false;
	}
	if (MACIE_WriteMACIEReg(handle, slctMACIEs, 0x01b5, 0x10ff) != MACIE_OK)
	{
		cout << "FIFO configuration for Multi_ASIC_BlockSize failed. " << endl;
		return false;
	}
	unsigned int multiBlkSize = 0;
	if (MACIE_ReadMACIEReg(handle, slctMACIEs, 0x01b6, &multiBlkSize) != MACIE_OK)
	{
		cout << "FIFO configuration for Multi_ASIC_BlockSize failed. " << endl;
		return false;
	}
	if ((multiBlkSize + 1) != frameX)
	{
		cout << "FIFO configuration for Multi_ASIC_BlockSize failed. " << endl;
		return false;
	}
	multiBlkSize += 1;
	cout << "Configuration for Multi-ASIC FIFO Mode with BlockSize of " << endl;

	// then configure science interface
	short connType = (handle & 0x0000000F);
	if (connType == short(MACIE_USB))
	{
		buf = 20;
		status1 = MACIE_ConfigureUSBScienceInterface(handle, slctMACIEs, 0, frameSize, buf);
	}
	else if (connType == short(MACIE_GigE))
	{
		status1 = MACIE_ConfigureGigeScienceInterface(handle, slctMACIEs, 0, frameSize, 42037, &buf);
	}
	else
	{
		cout << "Only GigE or USB is supported for multi-MACIE / multi-ASIC slow mode operation" << endl;
		return false;
	}
	if (!status1 == MACIE_OK)
	{
		cout << "Science interface configuration failed. buf = " << buf << endl;
		return false;
	}
	cout << "Science interface configuration succeeded. buf (KB) = " << buf << endl;

	// step 3: trigger ASIC to read science data
	cout << "Trigger image acquisition... " << endl;

	unsigned short frametime = 1500, nResets = 1, idleReset = 1, moreDelay = 2000;
	unsigned long n = 0;
	unsigned short triggerTimeout = frametime * (nResets + idleReset) + moreDelay;

	// make sure h6900 bit<0> is 0 before triggering. 
	if (connType != short(MACIE_USB))
	{
		if (MACIE_ReadASICReg(handle, slctASICs, 0x6900, &val, false, true) != MACIE_OK)
		{
			cout << "Read ASIC h6900 failed" << endl;
			return false;
		}
		if ((val & 1) != 0)
		{
			cout << "Configure idle mode by writing ASIC h6900 failed" << endl;
			return false;
		}
	}
	if (MACIE_WriteASICReg(handle, slctASICs, 0x6900, 0x8001, true) != MACIE_OK)
	{
		cout << " Triggering failed" << endl;
		return false;
	}
	cout << " Triggering succeeded" << endl;

	delay(200);
	// Wait for available science data bytes
	for (int i = 0; i < 100; i++)
	{
		n = MACIE_AvailableScienceData(handle);
		if (n > 0)
		{
			cout << "Available science data = " << n << " bytes, Loop = " << i << endl;
			break;
		}
		cout << "Wait...." << endl;
		delay(triggerTimeout / 100);
	}
	if (n <= 0)
	{
		cout << "trigger timeout: no available science data" << endl;
		return false;
	}
	// reading
	unsigned short *pData = NULL;
	if (connType == short(MACIE_USB))
		pData = MACIE_ReadUSBScienceFrame(handle, 1500 + 5000);
	else
		pData = MACIE_ReadGigeScienceFrame(handle, 1500 + 5000);
	if (!pData)
	{
		cout << "Null frame " << endl;
		return false;
	}
	cout << "Write Fits file now..." << endl;
	MACIE_FitsHdr pHeaders[3];

	strncpy(pHeaders[0].key, "ASICGAIN", sizeof(pHeaders[0].key));
	pHeaders[0].iVal = 1;
	pHeaders[0].valType = HDR_INT;
	strncpy(pHeaders[0].comment, "SIDECAR Preamp gain setting", sizeof(pHeaders[0].comment));

	strncpy(pHeaders[1].key, "FrmTime", sizeof(pHeaders[1].key));
	pHeaders[1].fVal = 1.47;
	pHeaders[1].valType = HDR_FLOAT;
	strncpy(pHeaders[1].comment, "Frame time at slow mode", sizeof(pHeaders[1].comment));

	strncpy(pHeaders[2].key, "AcqDate", sizeof(pHeaders[2].key));
	strncpy(pHeaders[2].sVal, "Jan 20, 2018", sizeof(pHeaders[2].sVal));
	pHeaders[2].valType = HDR_STR;
	strncpy(pHeaders[2].comment, "Test comments here", sizeof(pHeaders[2].comment));
	MACIE_STATUS status = MACIE_OK;
	if ((nASICs > 1) && (bProcessImage))
	{
		unsigned int subImgSize = frameSize / nASICs;
		unsigned short **ary = new unsigned short*[nASICs];
		for (int i = 0; i < nASICs; ++i)
			ary[i] = new unsigned short[subImgSize];

		int nBlocks = frameSize / (multiBlkSize* nASICs);
		for (int i = 0; i < nBlocks; i++)
		{
			for (int j = 0; j < nASICs; j++)
				memcpy(&ary[j][i*multiBlkSize], &pData[i*multiBlkSize* nASICs + j*multiBlkSize], multiBlkSize * 2);
		}
		for (int j = 0; j < nASICs; j++)
		{
			char fileName[80];
			char tmp[2];
			itoa(j, tmp, 10);
			strcpy(fileName, "c:\\data\\testImage");
			strcat(fileName, tmp);
			strcat(fileName, ".fits");
			if (MACIE_WriteFitsFile(fileName, frameX, frameY, ary[j], 3, pHeaders) != MACIE_OK)
			{
				cout << "Write fits file failed: " << fileName << endl;
				status = MACIE_FAIL;
			} 
			delete[] ary[j];
		}
		delete[] ary;
	}
	else
	{
		char fileName[80];
		strcpy(fileName, "c:\\data\\testImage.fits");
		status = MACIE_WriteFitsFile(fileName, 2048 * nASICs, 2048, pData, 3, pHeaders);
	}
	if (connType == MACIE_USB)
		MACIE_CloseUSBScienceInterface(handle, slctMACIEs);
	else
		MACIE_CloseGigeScienceInterface(handle, slctMACIEs);
	if (status != MACIE_OK)
	{
		cout << "Write fits file failed: " << MACIE_Error() << endl;
		return false;
	} 
	cout << "Succeeded" << endl;
	return true;
}

bool Acquire_SlowWinRamp(unsigned long handle, unsigned char slctMACIEs, unsigned char slctASIC)
{
	cout << "Acquire Science Data..." << endl;
	unsigned int val = 0;

	// step1: ASIC configuration, see detail in SIDECAR ASIC firmware manual
	unsigned int preampInputScheme = 2, preampInputVal = 0xaaaa, preampGain = 1;
	MACIE_STATUS r1 = MACIE_WriteASICReg(handle, slctASIC, 0x4019, preampInputScheme, true);
	MACIE_STATUS r2 = MACIE_WriteASICReg(handle, slctASIC, 0x5100, preampInputVal, true);
	MACIE_STATUS r3 = MACIE_WriteASICReg(handle, slctASIC, 0x401a, preampGain, true);
	MACIE_STATUS r4 = MACIE_WriteASICReg(handle, slctASIC, 0x4001, 1, true);

	//win mode
	MACIE_STATUS r5 = MACIE_WriteASICReg(handle, slctASIC, 0x4018, 2, true);
	unsigned int winarr[4] = { 0, 1023, 0, 311 };
	MACIE_STATUS r6 = MACIE_WriteASICBlock(handle, slctASIC, 0x4020, winarr, 4, true);

	// triggle Config
	MACIE_STATUS r7 = MACIE_WriteASICReg(handle, slctASIC, 0x6900, 0x8002, true);
	if (( r1 != MACIE_OK) || ( r2 != MACIE_OK) || ( r3 != MACIE_OK)	|| ( r4 != MACIE_OK) || ( r5 != MACIE_OK) || (r6 != r5) || (r7 != r5))
	{
		cout << "ASIC configuration failed - write ASIC registers" << endl;
		return false;
	}

	// wait for configuration: better to optimize the delay according to SIDECAR assembly code manual
	delay(1500);
	MACIE_STATUS status1 = MACIE_ReadASICReg(handle, slctASIC, 0x6900, &val, false, true);
	if (((val & 1) != 0) || (status1 != MACIE_OK))
	{
		cout << "ASIC configuration for shorted preamp inputs failed" << endl;
		return false;
	}
	cout << "Configuration succeeded" << endl;
	
	// step 2: science interface (GigE vs. Camlink vs.USB) configuration
	int buf = 0;
	//int frameX = 2048, frameY = 2048;
	int frameX = 1024, frameY = 311;
	int frameSize = frameX * frameY;

	short connType = (handle & 0x0000000F);
	if(connType == short(MACIE_USB) )
	{
		buf = 20;
		status1 = MACIE_ConfigureUSBScienceInterface(handle, slctMACIEs, 0, frameSize, buf);
	}
	else if (connType == short(MACIE_GigE) )
	{
		status1 = MACIE_ConfigureGigeScienceInterface(handle, slctMACIEs, 0, frameSize, 42037, &buf);
	}
	else
	{
		cout << "Only GigE or USB is supported for slow mode operation" << endl;
		return false;
	}
	if (!status1 == MACIE_OK)
	{
		cout << "Science interface configuration failed. buf = " << buf << endl;
		return false;
	}
	cout << "Science interface configuration succeeded. buf (KB) = " << buf << endl;

	// step 3: trigger ASIC to read science data
	cout << "Trigger image acquisition... " << endl;

	unsigned short frametime = 1500, nResets = 1, idleReset = 1, moreDelay = 4000;
	unsigned long n = 0;
	unsigned short triggerTimeout = frametime * (nResets + idleReset) + moreDelay; 
	char fileName[] = "c:\\data\\testImage0.fits";

	// make sure h6900 bit<0> is 0 before triggering. 
	if (connType != short(MACIE_USB) )
	{
		if (MACIE_ReadASICReg(handle, slctASIC, 0x6900, &val, false, true) != MACIE_OK)
		{
			cout << "Read ASIC h6900 failed" << endl;
			return false;
		}
		if ((val & 1) != 0)
		{
			cout << "Configure idle mode by writing ASIC h6900 failed" << endl;
			return false;
		}
    }

	// configure nRead
	if (MACIE_WriteASICReg(handle, slctASIC, 0x4001, 15, true) != MACIE_OK)
	{
		cout << "write ASIC h4001 failed" << endl;
		return false;
	}
	if (MACIE_WriteASICReg(handle, slctASIC, 0x6900, 0x8001, true) != MACIE_OK)
	{
		cout << " Triggering failed" << endl;
		return false;
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
		return false;
	}
	// reading
	
	for (int i = 0; i < 15; i++)
	{
		unsigned short *pData = NULL;
		if (connType == short(MACIE_USB))
			pData = MACIE_ReadUSBScienceFrame(handle, 1500 + 5000);
		else
			pData = MACIE_ReadGigeScienceFrame(handle, 1500 + 5000);

		if (!pData)
		{
			cout << "Null frame " << endl;
			return false;
		}
		cout << "Write Fits file now..." << endl;
		MACIE_FitsHdr pHeaders[3];

		strncpy(pHeaders[0].key, "ASICGAIN", sizeof(pHeaders[0].key));
		pHeaders[0].iVal = 1;
		pHeaders[0].valType = HDR_INT;
		strncpy(pHeaders[0].comment, "SIDECAR Preamp gain setting", sizeof(pHeaders[0].comment));

		strncpy(pHeaders[1].key, "FrmTime", sizeof(pHeaders[1].key));
		pHeaders[1].fVal = 1.47;
		pHeaders[1].valType = HDR_FLOAT;
		strncpy(pHeaders[1].comment, "Frame time at slow mode", sizeof(pHeaders[1].comment));

		strncpy(pHeaders[2].key, "AcqDate", sizeof(pHeaders[2].key));
		strncpy(pHeaders[2].sVal, "Jan 20, 2018", sizeof(pHeaders[2].sVal));
		pHeaders[2].valType = HDR_STR;
		strncpy(pHeaders[2].comment, "Test comments here", sizeof(pHeaders[2].comment));
		//char fileName[] = "c:\\data\\testImage0.fits";
		char fileName[] = "c:\\data\\testImage"; // 0.fits";
		char tmp[3];
	//	itoa(i, tmp, 10);
		sprintf (tmp, "%d", i);
		strncat(fileName, tmp, 3);
		strncat(fileName, ".fits", 5);

		MACIE_STATUS status = MACIE_WriteFitsFile(fileName, frameX, frameY, pData, 3, pHeaders);
		if (status != MACIE_OK)
		{
			cout << "Write fits file failed: " << MACIE_Error() << endl;
			return false;
		}
		else
		    cout << fileName << endl;
	}
	if(connType == MACIE_USB )
		MACIE_CloseUSBScienceInterface(handle, slctMACIEs);
	else
		MACIE_CloseGigeScienceInterface(handle, slctMACIEs);

	cout << "Succeeded" << endl;
	return true;
}

bool Acquire_LucyUSB(unsigned long handle, unsigned char slctMACIEs, unsigned char slctASIC)
{
	cout << "Acquire Science Data..." << endl;
	unsigned int val = 0;

	MACIE_STATUS r1 = MACIE_WriteASICReg(handle, slctASIC, 0x6903, 0, true);
	delay(1500);
	MACIE_STATUS r2 = MACIE_WriteASICReg(handle, slctASIC, 0x4001, 0x3000, true);
	delay(2000);
	MACIE_STATUS r3 = MACIE_WriteASICReg(handle, slctASIC, 0x4001, 0x4800, true);
	delay(1000);
	
	// step 2: science interface (GigE vs. Camlink vs.USB) configuration
	int buf = 0;
	int frameX = 1478, frameY = 1024;
	int frameSize = frameX * frameY;
	MACIE_STATUS status1;
	short connType = (handle & 0x0000000F);
	if (connType == short(MACIE_USB))
	{
		buf = 20;
		status1 = MACIE_ConfigureUSBScienceInterface(handle, slctMACIEs, 0, frameSize, buf);
	}
	else
	{
		cout << "Only USB is supported for slow mode operation" << endl;
		return false;
	}
	if (!status1 == MACIE_OK)
	{
		cout << "Science interface configuration failed. buf = " << buf << endl;
		return false;
	}
	cout << "Science interface configuration succeeded. buf (KB) = " << buf << endl;

	r3 = MACIE_WriteASICReg(handle, slctASIC, 0x4001, 0x4900, true);

	// step 3: trigger ASIC to read science data
	cout << "Trigger image acquisition... " << endl;

	unsigned short frametime = 1500;
	unsigned short triggerTimeout = 7000;
	char fileName[] = "c:\\data\\testImage0.fits";

	// make sure h6900 bit<0> is 0 before triggering. 
	if (MACIE_WriteASICReg(handle, slctASIC, 0x4001, 0x4001, true) != MACIE_OK)
	{
		cout << " Triggering failed" << endl;
		return false;
	}
	cout << " Triggering succeeded" << endl;
	long n = 0;
	// Wait for available science data bytes
	for (int i = 0; i < 500; i++)
	{
		n = MACIE_AvailableScienceData(handle);
		if (n > 0)
		{
			cout << "Available science data = " << n << " bytes, Loop = " << i << endl;
			break;
		}
		cout << "Wait...." << endl;
		delay(triggerTimeout / 100);
	}
	if (n <= 0)
	{
		cout << "trigger timeout: no available science data" << endl;
		return false;
	}
	// reading
	//unsigned short *pData = MACIE_ReadUSBScienceFrame(handle, 1500 + 5000);

	unsigned short *pData = new unsigned short[frameSize];

	long test = MACIE_ReadUSBScienceData(handle, 1500 + 5000, frameSize, pData);
	
	if (!pData)
	{
		cout << "Null frame " << endl;
		return false;
	}
	else
	{
		unsigned short test[5000];
		for (int x = 0; x < 5000; x++)
			test[x] = pData[x];
		bool b = true;
	}
	cout << "Write Fits file now..." << endl;
	MACIE_FitsHdr pHeaders[3];

	strncpy(pHeaders[0].key, "ASICGAIN", sizeof(pHeaders[0].key));
	pHeaders[0].iVal = 1;
	pHeaders[0].valType = HDR_INT;
	strncpy(pHeaders[0].comment, "SIDECAR Preamp gain setting", sizeof(pHeaders[0].comment));

	strncpy(pHeaders[1].key, "FrmTime", sizeof(pHeaders[1].key));
	pHeaders[1].fVal = 1.47;
	pHeaders[1].valType = HDR_FLOAT;
	strncpy(pHeaders[1].comment, "Frame time at slow mode", sizeof(pHeaders[1].comment));

	strncpy(pHeaders[2].key, "AcqDate", sizeof(pHeaders[2].key));
	strncpy(pHeaders[2].sVal, "Jan 20, 2018", sizeof(pHeaders[2].sVal));
	pHeaders[2].valType = HDR_STR;
	strncpy(pHeaders[2].comment, "Test comments here", sizeof(pHeaders[2].comment));

	MACIE_STATUS status = MACIE_WriteFitsFile(fileName, 2048, 2048, pData, 3, pHeaders);
	if (connType == MACIE_USB)
		MACIE_CloseUSBScienceInterface(handle, slctMACIEs);
	else
		MACIE_CloseGigeScienceInterface(handle, slctMACIEs);

	if (status != MACIE_OK)
	{
		cout << "Write fits file failed: " << MACIE_Error() << endl;
		return false;
	}
	cout << "Succeeded" << endl;
	return true;
}

bool Acquire_FastMode(unsigned long handle, unsigned char slctMACIEs, unsigned char slctASIC, bool externalTrigger)
{
	cout << "Acquire Science Data - fast mode ..." << endl;
	unsigned int val = 0;
	
	// step 1: science interface (GigE vs. Camlink vs.USB) configuration
	short buf = 0;
	int frameX = 2048, frameY = 2048;
	int frameSize = frameX * frameY;

	MACIE_STATUS status1;
	short connType = (handle & 0x0000000F);
	if(connType == MACIE_UART )
	{
		const char dcf[500] = "/home/jing/MyProjects/MACIE/MACIE_CONTROL/lbin/debug/MacieApp/load files/FastMode_2048.dcf";
		status1 = MACIE_ConfigureCamLinkInterface(handle, slctMACIEs, 0, dcf, 1000, frameX, frameY, &buf);
	}
	else if (connType == MACIE_USB)
	{
		buf = 20;
		status1 = MACIE_ConfigureUSBScienceInterface(handle, slctMACIEs, 3, frameSize, buf);
	}
	else
	{
		cout << "This interface for image acquisition is not added to the example code" << endl;
		return false;
	}

	if (!status1 == MACIE_OK)
	{
		cout << "Science interface configuration failed. buf = " << buf << endl;
		return false;
	}
	cout << "Science interface configuration succeeded. buf = " << buf << endl;

	// step 2: trigger ASIC to read science data
	cout << "Trigger image acquisition... " << endl;
	unsigned short frametime = 1500, nResets = 1, idleReset = 1, moreDelay = 2000;
	unsigned long n = 0;
	unsigned short triggerTimeout = frametime * (nResets + idleReset) + moreDelay;
	
	if (connType != MACIE_USB)
	{
		// make sure h6900 bit<0> is 0 before triggering. 
		if (MACIE_ReadASICReg(handle, slctASIC, 0x6900, &val, false, true) != MACIE_OK)
		{
			cout << "Read ASIC h6900 failed" << endl;
			return false;
		}
		if ((val & 1) != 0)
		{
			cout << "Configure idle mode by writing ASIC h6900 failed" << endl;
			return false;
		}
	}
	if (externalTrigger)
	{
		unsigned int val = 0x8001;
		bool bLineBoundar = true, bFrameBoundary = false;
		if (bLineBoundar)
			val = val | 0x0002;
		if (bFrameBoundary)
			val = val | 0x0004;

		if (MACIE_WriteMACIEReg(handle, slctMACIEs, 0x01f0,1) != MACIE_OK)
		{
			cout << "Writing MACIE 0x01f0 with value 1 failed";
			return false;
		}
		if (MACIE_WriteMACIEReg(handle, slctMACIEs, 0x01f1, 0x6900) != MACIE_OK)
		{
			cout << "Writing ASIC register address 0x6900 to MACIE 0x01f1 failed";
			return false;
		}
		if (MACIE_WriteMACIEReg(handle, slctMACIEs, 0x01f2, val) != MACIE_OK)
		{
			cout << "Writing MACIE 0x01f2 failed";
			return false;
		}
	}
	else
	{
		if (MACIE_WriteASICReg(handle, slctASIC, 0x6900, 0x8001, true) != MACIE_OK)
		{
			cout << " Triggering failed" << endl;
			return false;
		}
	}
	cout << " Triggering succeeded" << endl;
	
	// Wait for available science data bytes
	for (int i = 0; i < 100; i++)
	{
		if(connType == MACIE_UART )
			n = MACIE_AvailableScienceFrames(handle);
		else
			n = MACIE_AvailableScienceData(handle);
		if (n > 0)
		{
			cout << "Available science data = " << n << " bytes, Loop = " << i << endl;
			break;
		}
		cout << "Wait...." << endl;
		delay(triggerTimeout / 200);
	}
	if (n <= 0)
	{
		cout << "trigger timeout: no available science data" << endl;
		return false;
	} 
	const char tifFileName[100] = "./test_image/TestImage0.tif";
	unsigned short timeout = 1500;
	unsigned short *pData = NULL;

	if (connType == MACIE_UART )
		pData = MACIE_ReadCamlinkScienceFrame(handle,  tifFileName, timeout);
	else
		pData = MACIE_ReadUSBScienceFrame(handle, timeout);

	MACIE_WriteMACIEReg(handle, slctMACIEs, 0x01f0, 0);

	if (!pData)
	{
		cout << "Null frame " << endl;
		return false;
	}
	cout << "Write Fits file now..." << endl;
	MACIE_FitsHdr pHeaders[3];

	strncpy(pHeaders[0].key, "ASICGAIN", sizeof(pHeaders[0].key));
	pHeaders[0].iVal = 1;
	pHeaders[0].valType = HDR_INT;
	strncpy(pHeaders[0].comment, "SIDECAR Preamp gain setting", sizeof(pHeaders[0].comment));

	strncpy(pHeaders[1].key, "FrmTime", sizeof(pHeaders[1].key));
	pHeaders[1].fVal = 1.47;
	pHeaders[1].valType = HDR_FLOAT;
	strncpy(pHeaders[1].comment, "Frame time at slow mode", sizeof(pHeaders[1].comment));

	strncpy(pHeaders[2].key, "AcqDate", sizeof(pHeaders[2].key));
	strncpy(pHeaders[2].sVal, "Jan 20, 2018", sizeof(pHeaders[2].sVal));
	pHeaders[2].valType = HDR_STR;
	strncpy(pHeaders[2].comment, "Test comments here", sizeof(pHeaders[2].comment));
	char fileName1[] = "./test_image/testImage0.fits";
	MACIE_STATUS status = MACIE_WriteFitsFile(fileName1, 2048, 2048, pData, 3, pHeaders);

	if (connType == MACIE_UART )
		MACIE_CloseCamlinkScienceInterface(handle, slctASIC);
	else
		MACIE_CloseUSBScienceInterface(handle, slctASIC);
	if (status != MACIE_OK)
	{
		cout << "Write fits file failed: " << MACIE_Error() << endl;
		return false;
	}  
	cout << "Succeeded" << endl;
	return true;
} 



int main()
{
	cout << "start test C++" << endl;
	unsigned short numCards = 0;
	MACIE_CardInfo *pCard = NULL;
	unsigned long handle = 0;
	MACIE_Connection connection = MACIE_NONE;
	unsigned char avaiMACIEs = 0, slctMACIEs = 1;
	unsigned char avaiASICs = 0, slctASIC = 0;
	unsigned int val = 0;

	int option;
	cout << "\nEnter an option, -1 to end: ";
	cin >> option;
	float mVersion  = MACIE_LibVersion(); //MACIE_LibVersion
	cout << "MACIE_LibVersion = " << mVersion << endl;
	MACIE_STATUS bRet = MACIE_Init();
	short slctCard = 0;

	while (option != -1)
	{
		if (option == 1)
		{
			cout << "Check Interface...." << endl;
			bRet = MACIE_CheckInterfaces(0, NULL, 0, &numCards, &pCard);
			cout << "MACIE_CheckInterfaces " << ((bRet == MACIE_OK) ? "succeeded" : "failed") << endl;
			cout << "numCards= " << numCards << endl;
			for (int i = 0; i < numCards; i++)
			{
				cout << "macieSerialNumber=" << pCard[i].macieSerialNumber << endl;
				cout << "bUART=" << ((pCard[i].bUART == true) ? "true" : "false") << endl;
				cout << "bGigE=" << ((pCard[i].bGigE == true) ? "true" : "false") << endl;
				cout << "bUSB=" << ((pCard[i].bUSB == true) ? "true" : "false") << endl;
				cout << "ipAddr=" << static_cast<int>((pCard[i].ipAddr)[0]) << "." << static_cast<int>((pCard[i].ipAddr)[1]) <<
					"." << static_cast<int>((pCard[i].ipAddr)[2]) << "." << static_cast<int>((pCard[i].ipAddr)[3]) << endl;
				cout << "gigeSpeed=" << pCard[i].gigeSpeed << endl;
				cout << "serialPortName=" << pCard[i].serialPortName << endl;
				cout << "\nusbSerialNumber=" << pCard[i].usbSerialNumber << endl;
				cout << "USBSpeed=" << pCard[i].usbSpeed << endl;
				cout << "Firmware 2=" << pCard[i].firmwareSlot2 << endl;
			}
			if (numCards > 1)
				slctCard = 1;
		}
		else if ((option >=20) && (option <30)) //else if ((option == 2) || (option == 12) || (option == 22) || (option == 32) )
		{
			if (option == 20)
			{
				cout << "Get handle for the interface of MACIE " << pCard[slctCard].macieSerialNumber << ", GigE connection " << endl;
				connection = MACIE_GigE;
			}
			else if (option == 21)
			{
				cout << "Get handle for the interface of MACIE " << pCard[slctCard].macieSerialNumber << ", Camlink Serial port connection " << endl;
				connection = MACIE_UART;
			}
			else if (option == 22)
			{
				cout << "Get handle for the interface of MACIE " << pCard[slctCard].macieSerialNumber << ", USB port connection " << endl;
				connection = MACIE_USB;
			}		
			handle = MACIE_GetHandle(pCard[slctCard].macieSerialNumber, connection);
			cout << " Handle = " << handle << endl;
			if (handle == 0)
				cout << MACIE_Error() << endl;
		}
		else if (option == 30) //((option == 3) || (option == 13) || (option == 23) || (option == 33))
		{
			cout << "Get available MACIE cards..." << endl;
			avaiMACIEs = MACIE_GetAvailableMACIEs(handle);
			cout << "MACIE_GetAvailableMACIEs= " << short(avaiMACIEs) << endl;
			slctMACIEs = ((unsigned short)avaiMACIEs) & 1;
			if (slctMACIEs == 0)
				cout << "Select MACIE = " << slctMACIEs << " invalid" << endl;
			else if (MACIE_ReadMACIEReg(handle, avaiMACIEs, 0x0300, &val) != MACIE_OK)
				cout << "MACIE read 0x300 failed: " << MACIE_Error() << endl;
			else
				cout << "MACIE h0300=" << val << endl;
		}
		else if  ((option >=40) && (option < 50)) //((option == 4) || (option == 14) || (option == 24) || (option == 28) || (option == 34))
		{
			if (option == 40)
				InitializeASIC(handle, slctMACIEs, 0); // GigE slow mode
			else if (option == 41)
				InitializeASIC(handle, slctMACIEs, 1); // Camlink fast mode
			else if (option == 42)
				InitializeASIC(handle, slctMACIEs, 0); // USB slow mode
			else if (option == 43)
				InitializeASIC(handle, slctMACIEs, 1); // USB false mode
			else if (option == 44)
				InitializeASIC(handle, slctMACIEs, 2); // Camlink fast mode - SFI support

			avaiASICs = MACIE_GetAvailableASICs(handle, false);
			slctASIC = avaiASICs & 1; // ASIC1
			if ((unsigned short)avaiASICs == 0)
				cout << "MACIE_GetAvailableASICs failed" << endl;
			else
			{
				MACIE_ReadASICReg(handle, slctASIC, 0x6100, &val, false, true);
				cout << "ASIC h6100=" << val << endl;
				unsigned int *pData = new unsigned int[5];

				MACIE_ReadASICBlock(handle, slctASIC, 0x4000, pData, 5, false, true);
				for (int a = 0; a < 5; a++)
					cout << "val=" << pData[a] << endl;
			}
		}
		else if ((option >=50) && (option < 60)) //((option == 5) || (option == 15) || (option == 25) || (option == 29) || (option == 35) )
		{
			if (option == 50)
				Acquire_SlowMode(handle, slctMACIEs, slctASIC); // single frm -GigE
			else if (option == 51)
				Acquire_FastMode(handle, slctMACIEs, slctASIC, false);  // single frm - Camlink, fast mode
			else if (option == 52)
				Acquire_SlowMode(handle, slctMACIEs, slctASIC);  // single frm - USB, slow mode Acquire_SlowMode
			else if (option == 53)
				Acquire_SlowWinRamp(handle, slctMACIEs, slctASIC);  // window mode ramp - USB, slow mode Acquire_SlowWinRamp
			else if (option == 54)
				Acquire_FastMode(handle, slctMACIEs, slctASIC, false);  // single frm - USB, fast mode
			else if (option == 55)
				Acquire_FastMode(handle, slctMACIEs, slctASIC, true);
		}
		else if (option == 6)
		{
			unsigned short errArr[MACIE_ERROR_COUNTERS];
			if (MACIE_GetErrorCounters(handle,slctMACIEs, errArr) != MACIE_OK)
			{
				cout << "Read MACIE error counter failed";
			}
			else
			{
				cout << "Error counters: ";
				for (int i = 0; i < MACIE_ERROR_COUNTERS; i++)
					cout << errArr[i] << "  ";
				cout << endl;
			}
		}
		else if (option == 7)
		{
			char LoadFile[200] = "/home/jing/MyProjects/MACIE/MACIE_CONTROL/lbin/debug/MacieApp/load files/LoadFile_Example.mcf";
			if (MACIE_DownloadLoadfile(handle, slctMACIEs, slctASIC, LoadFile, true) != MACIE_OK)
				cout << "Load file failed: " << MACIE_Error() << endl;
			else
				cout << "Load file succeeded" << endl;

			MACIE_ReadASICReg(handle, slctASIC, 0x6000, &val, false, true);
			cout << "h6000 = " << val << endl;
		}
		else if (option == 8)
		{
			char LoadFile[200] = "/home/jing/MyProjects/MACIE/MACIE_CONTROL/lbin/debug/MacieApp/load files/Init_MACIE_SIDECAR.mcf";
			if (MACIE_DownloadLoadfile(handle, 0, slctASIC, LoadFile, true) != MACIE_OK)
				cout << "Load file failed: " << MACIE_Error() << endl;
			else
				cout << "Load file succeeded" << endl;

			MACIE_ReadMACIEReg(handle, slctMACIEs, 0x0300, &val);
			cout << "h0300 = " << val << endl;
		}
		else if (option == 60)
		{
			float tlm[79];
			if (MACIE_GetTelemetryAll(handle, slctMACIEs, tlm) != MACIE_OK)
				cout << "MACIE_GetTelemetryAll failed: " << MACIE_Error() << endl;
			else
			{
				cout << "MACIE_GetTelemetryAll succeeded" << endl;
				for (int i = 0; i < 79; i++)
					cout << tlm[i] << endl;
			}
		}
		else if (option == 61)
		{
			MACIE_TLM_ITEM addr[4] = { MACIE_TLM_V_VDDAHIGH1, MACIE_TLM_V_VDDAHIGH2, MACIE_TLM_I_VDDHIGH1, MACIE_TLM_I_VDDHIGH2 };
			float tlm[4];
			if (MACIE_GetTelemetrySet(handle, slctMACIEs, addr, 4, tlm) != MACIE_OK)
				cout << "MACIE_GetTelemetryAll failed: " << MACIE_Error() << endl;
			else
			{
				cout << "MACIE_GetTelemetryAll succeeded" << endl;
				for (int i = 0; i < 4; i++)
					cout << tlm[i] << endl;
			}
		}
		else if (option == 62)
		{
			MACIE_TLM_ITEM addr = MACIE_TLM_I_VDDHIGH1;
			float tlm;
			if (MACIE_GetTelemetry(handle, slctMACIEs, addr, &tlm) != MACIE_OK)
				cout << "MACIE_GetTelemetry failed: " << MACIE_Error() << endl;
			else
			{
				cout << "MACIE_GetTelemetry succeeded" << endl;
				cout << tlm << endl;
			}
		}
		else if (option == 63)
		{
			MACIE_TLM_SAMPLE_RATE vSampleRate;
			MACIE_TLM_AVERAGE     vAverage;
			MACIE_TLM_SAMPLE_RATE iSampleRate;
			MACIE_TLM_AVERAGE    iAverage;
			MACIE_TLM_GROUND_REFERENCE groundRef;

			if (MACIE_GetTelemetryConfiguration(handle, slctMACIEs, &vSampleRate, &vAverage, &iSampleRate, &iAverage, &groundRef) != MACIE_OK)
				cout << "MACIE_GetTelemetryConfiguration failed: " << MACIE_Error() << endl;
			else
				cout << "vSampleRate= " << vSampleRate << "  vAverage=" << vAverage << "  iSampleRate=" << iSampleRate  << " iAverag=" << iAverage << " groundRef=" << groundRef << endl;

			vSampleRate = MACIE_TLM_167_Hz;
			vAverage = MACIE_TLM_AVG_2;
			iAverage = MACIE_TLM_AVG_2;
			iSampleRate = MACIE_TLM_167_Hz;
			groundRef = MACIE_TLM_REF_AGND_CLEAN;

			if (MACIE_SetTelemetryConfiguration(handle, slctMACIEs, vSampleRate, vAverage, iSampleRate, iAverage, groundRef) != MACIE_OK)
				cout << "MACIE_GetTelemetryConfiguration failed: " << MACIE_Error() << endl;
			else
			{
				MACIE_ReadMACIEReg(handle, slctMACIEs, 0x0480, &val);
				cout << "h0480=" << val << endl;
			}

			if (MACIE_GetTelemetryConfiguration(handle, slctMACIEs, &vSampleRate, &vAverage, &iSampleRate, &iAverage, &groundRef) != MACIE_OK)
				cout << "MACIE_GetTelemetryConfiguration failed: " << MACIE_Error() << endl;
			else
				cout << "vSampleRate= " << vSampleRate << "  vAverage=" << vAverage << "  iSampleRate=" << iSampleRate << " iAverag=" << iAverage << " groundRef=" << groundRef << endl;

		}
		else if (option == 64)
		{
			bool pArr[39], bEn = false;
			for (int i = 0; i < 39; i++)
			{
				if (MACIE_GetPower(handle, slctMACIEs, MACIE_PWR_CTRL(i), &bEn) != MACIE_OK)
					cout << "MACIE_GetPower failed: " << MACIE_Error() << endl;
				else
					pArr[i] = bEn;
			}
			MACIE_ReadMACIEReg(handle, slctMACIEs, 0x0300, &val);
			cout << "h0300=" << val << endl;
			MACIE_ReadMACIEReg(handle, slctMACIEs, 0x0320, &val);
			cout << "h0320=" << val << endl;
			MACIE_ReadMACIEReg(handle, slctMACIEs, 0x0340, &val);
			cout << "h0340=" << val << endl;

			MACIE_SetPower(handle, slctMACIEs, MACIE_CTRL_VDDAUX1, true);
			MACIE_SetPower(handle, slctMACIEs, MACIE_CTRL_VDDAUX2, true);
			
			MACIE_ReadMACIEReg(handle, slctMACIEs, 0x0300, &val);
			cout << "h0300=" << val << endl;

			MACIE_PWR_CTRL ids[2] = { MACIE_CTRL_VDDAUX1 , MACIE_CTRL_VDDAUX2 };
			MACIE_DisablePower( handle, slctMACIEs, ids, 2);
			MACIE_ReadMACIEReg(handle, slctMACIEs, 0x0300, &val);
			cout << "h0300=" << val << endl;

			MACIE_EnablePower(handle, slctMACIEs, ids, 2);
			MACIE_ReadMACIEReg(handle, slctMACIEs, 0x0300, &val);
			cout << "h0300=" << val << endl;

		}
		else if (option == 45)
		{
			float fval = 0.0;
			float vArr[38] = { 0 };
			for (int i = 0; i < 38; i++)
			{
				MACIE_GetVoltage(handle, slctMACIEs, MACIE_PWR_DAC(i), &fval);
				vArr[i] = fval;
			}
			
			fval = 1.6; //VDDIO1
			MACIE_SetVoltage(handle, slctMACIEs, MACIE_DAC_VDDIO1, fval);
			fval = 3.0;
			MACIE_SetVoltage(handle, slctMACIEs, MACIE_DAC_MACIE_DAC_VREF1, fval);

			for (int i = 0; i < 38; i++)
			{
				MACIE_GetVoltage(handle, slctMACIEs, MACIE_PWR_DAC(i), &fval);
				vArr[i] = fval;
			}
		}
		else if (option == 70)
		{
			InitializeASIC(handle, 15, 0); // USB slow mode
			char LoadFile[200] = "/home/jing/MyProjects/MACIE/MACIE_CONTROL/lbin/debug/MacieApp/load files/Init_2xMACIE_SIDECAR.mcf";
			if (MACIE_DownloadLoadfile(handle, 0, 0, LoadFile, true) != MACIE_OK)
				cout << "Load file failed: " << MACIE_Error() << endl;
			else
				cout << "Load file succeeded" << endl;

			cout << "Get available MACIE cards..." << endl;
			avaiMACIEs = MACIE_GetAvailableMACIEs(handle);
			cout << "MACIE_GetAvailableMACIEs= " << short(avaiMACIEs) << endl;
			avaiASICs = MACIE_GetAvailableASICs(handle, 0);

			unsigned char multiASICs = 0x41, multiMACIEs = 0x9;
			if (((avaiASICs & 0x41) != 0x41) || ((avaiMACIEs & 0x09) != 0x09))
				cout << "Muti-ASICs (1 & 2) are not available" << endl;
			else
				Acquire_SlowMode_MultiMACIE(handle, multiMACIEs, multiASICs, true);
		}

		cout << "\nEnter an option, -1 to end: ";
		cin >> option;
	}
	MACIE_Free();
	cout << "Test done " << endl; 
    return 0;
}

