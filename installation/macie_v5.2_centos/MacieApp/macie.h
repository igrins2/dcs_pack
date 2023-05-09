#ifndef MACIE_H
#define MACIE_H


#if defined(_WIN32) || defined(_WIN64)

    #include <windows.h>
    #ifdef MACIE_LIBRARY
        #define MACIESHARED_EXPORT __declspec(dllexport)
    #else
        #define MACIESHARED_EXPORT __declspec(dllimport)
    #endif
#else /* _WIN32 || _WIN64 */

    #include <stddef.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <stdio.h>

    #define WINAPI
    #ifndef MACIE_LIBRARY
        #define MACIESHARED_EXPORT
    #else /* !MACIE_LIBRARY */
        #define MACIESHARED_EXPORT __attribute__((visibility("default")))
    #endif /* MACIE_LIBRARY */
#endif


#ifdef __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------
// Constant:
//------------------------------------------------------------------------
#define MACIE_ERROR_COUNTERS 33


//------------------------------------------------------------------------
// MACIE communication interface transfer definitions
//------------------------------------------------------------------------
typedef enum
{
    MACIE_NONE,
    MACIE_USB,
    MACIE_GigE,
    MACIE_UART
} MACIE_Connection;


typedef enum
{
    MACIE_OK,
    MACIE_FAIL
} MACIE_STATUS;

typedef enum
{
    MACIE_DAC_MACIE_DAC_VREF1,    // Vref1,  0 - 4.095V, LSB = 1.00mV
    MACIE_DAC_VDDAHIGH1,          // VDDAHigh1, 0V - 4.089V, LSB = 1.08 mV
    MACIE_DAC_VDDAHIGH1_VL,       // VDDAHigh1 Overvoltage limit, 0 - 4.761V, LSB = 1.16mV
    MACIE_DAC_VDDAHIGH1_CL,       // VDDAHigh1 Current limit, 0 - 1023.8mA, LSB = 250µA
    MACIE_DAC_VDDALOW1,           // VDDALow1, 0V - 4.089V, LSB = 1.08 mV
    MACIE_DAC_VDDALOW1_VL,        // VDDALow1 Overvoltage limit, 0 - 4.761V, LSB = 1.16mV
    MACIE_DAC_VDDALOW1_CL,        // VDDALow1 Current limit, 0 - 1023.8mA, LSB = 250µA
    MACIE_DAC_VDDHIGH1,           // VDDHigh1, 0V - 4.089V, LSB = 1.08 mV
    MACIE_DAC_VDDHIGH1_VL,        // VDDHigh1 Overvoltage limit, 0 - 4.761V, LSB = 1.16mV
    MACIE_DAC_VDDHIGH1_CL,        // VDDHigh1 Current limit, 0 - 511.9mA, LSB = 125µA
    MACIE_DAC_VDDLOW1,            // VDDLow1, h000 = 4.089V, hed0 = 0V, LSB = 1.08 mV
    MACIE_DAC_VDDLOW1_VL,         // VDDLow1 Overvoltage limit, 0 - 4.761V, LSB = 1.16mV
    MACIE_DAC_VDDLOW1_CL,         // VDDLow1 Current limit, 0 - 511.9mA, LSB = 125µA
    MACIE_DAC_VDDIO1,             // VDDIO1, h000 = 4.089V, hed0 = 0V, LSB = 1.08 mV
    MACIE_DAC_VDDIO1_VL,          // VDDIO1 Overvoltage limit, 0 - 4.761V, LSB = 1.16mV
    MACIE_DAC_VDDIO1_CL,          // VDDIO1 Current limit, 0 - 511.9mA, LSB = 125µA
    MACIE_DAC_VSSIO1,             // VSSIO1, 0 - 4.095V, LSB = 1.00mV
    MACIE_DAC_VSSIO1_VL,          // VSSIO1 Overvoltage limit, 0 - 4.095V, LSB = 1.00mV
    MACIE_DAC_VDDAUX1,            // VDDAUX1, h000 = 4.089V, hed0 = 0V, LSB = 1.08 mV
    MACIE_DAC_VREF2,              // Same as VREF1
    MACIE_DAC_VDDAHIGH2,          // Same as VDDAHIGH1
    MACIE_DAC_VDDAHIGH2_VL,       // Same as VDDAHIGH1_VL
    MACIE_DAC_VDDAHIGH2_CL,       // Same as VDDAHIGH1_CL
    MACIE_DAC_VDDALOW2,           // Same as VDDALOW1
    MACIE_DAC_VDDALOW2_VL,        // Same as VDDALOW1_VL
    MACIE_DAC_VDDALOW2_CL,        // Same as VDDALOW1_CL
    MACIE_DAC_VDDHIGH2,           // Same as VDDHIGH1
    MACIE_DAC_VDDHIGH2_VL,        // Same as VDDHIGH1_VL
    MACIE_DAC_VDDHIGH2_CL,        // Same as VDDHIGH1_CL
    MACIE_DAC_VDDLOW2,            // Same as VDDLOW1
    MACIE_DAC_VDDLOW2_VL,         // Same as VDDLOW1_VL
    MACIE_DAC_VDDLOW2_CL,         // Same as VDDLOW1_CL
    MACIE_DAC_VDDIO2,             // Same as VDDIO1
    MACIE_DAC_VDDIO2_VL,          // Same as VDDIO1_VL
    MACIE_DAC_VDDIO2_CL,          // Same as VDDIO1_CL
    MACIE_DAC_VSSIO2,             // Same as VSSIO1
    MACIE_DAC_VSSIO2_VL,          // Same as VSSIO1_VL
    MACIE_DAC_VDDAUX2             // Same as VDDAUX1
} MACIE_PWR_DAC;

typedef enum
{
    MACIE_CTRL_5V_ASIC,
    MACIE_CTRL_GIGE,
    MACIE_CTRL_GIGE_OVERRIDE,
    MACIE_CTRL_DGND_FILTER_BYPASS,
    MACIE_CTRL_USB_FILTER_BYPASS,
    MACIE_CTRL_AGND_CLEAN_FILTER_BYPASS,
    MACIE_CTRL_AGND_DIRTY_FILTER_BYPASS,
    MACIE_CTRL_VDDAUX1,
    MACIE_CTRL_VDDAUX2,
    MACIE_CTRL_VDDAHIGH1,
    MACIE_CTRL_VDDALOW1,
    MACIE_CTRL_VREF1,
    MACIE_CTRL_SENSE_VREF1_GNDA,
    MACIE_CTRL_SENSE_VDDAHIGH1_GNDA,
    MACIE_CTRL_SENSE_VDDAHIGH1,
    MACIE_CTRL_SENSE_VDDALOW1_GNDA,
    MACIE_CTRL_SENSE_VDDALOW1,
    MACIE_CTRL_VDDHIGH1,
    MACIE_CTRL_VDDLOW1,
    MACIE_CTRL_VDDIO1,
    MACIE_CTRL_VSSIO1,
    MACIE_CTRL_SENSE_VDDHIGH1_GND,
    MACIE_CTRL_SENSE_VDDHIGH1,
    MACIE_CTRL_SENSE_VDDLOW1_GND,
    MACIE_CTRL_SENSE_VDDLOW1,
    MACIE_CTRL_VDDAHIGH2,
    MACIE_CTRL_VDDALOW2,
    MACIE_CTRL_VREF2,
    MACIE_CTRL_SENSE_VREF2_GNDA,
    MACIE_CTRL_SENSE_VDDAHIGH2_GNDA,
    MACIE_CTRL_SENSE_VDDAHIGH2,
    MACIE_CTRL_SENSE_VDDALOW2_GNDA,
    MACIE_CTRL_SENSE_VDDALOW2,
    MACIE_CTRL_VDDHIGH2,
    MACIE_CTRL_VDDLOW2,
    MACIE_CTRL_VDDIO2,
    MACIE_CTRL_VSSIO2,
    MACIE_CTRL_SENSE_VDDHIGH2_GND,
    MACIE_CTRL_SENSE_VDDHIGH2,
    MACIE_CTRL_SENSE_VDDLOW2_GND,
    MACIE_CTRL_SENSE_VDDLOW2
} MACIE_PWR_CTRL;

typedef enum
{
    MACIE_TLM_16p7_Hz,
    MACIE_TLM_20_Hz,
    MACIE_TLM_83p3_Hz,
    MACIE_TLM_167_Hz
} MACIE_TLM_SAMPLE_RATE;

typedef enum
{
    MACIE_TLM_AVG_1,
    MACIE_TLM_AVG_2,
    MACIE_TLM_AVG_4,
    MACIE_TLM_AVG_8
} MACIE_TLM_AVERAGE;

typedef enum
{
    MACIE_TLM_REF_GND, //GND, 1 = DGND, 2 = AGND_CLEAN, 3 = AGND_DIRTY
    MACIE_TLM_REF_DGND,
    MACIE_TLM_REF_AGND_CLEAN,
    MACIE_TLM_REF_AGND_DIRTY,
    MACIE_TLM_REF_AUTO_GROUND
} MACIE_TLM_GROUND_REFERENCE;


typedef enum
{
    MACIE_TLM_V_VDDAHIGH1,
    MACIE_TLM_V_VDDAHIGH2,
    MACIE_TLM_V_VDDALOW1,
    MACIE_TLM_V_VDDALOW2,
    MACIE_TLM_V_VREF1,
    MACIE_TLM_V_VREF2,
    MACIE_TLM_V_VDDHIGH1,
    MACIE_TLM_V_VDDHIGH2,
    MACIE_TLM_V_VDDLOW1,
    MACIE_TLM_V_VDDLOW2,
    MACIE_TLM_V_VDDIO1,
    MACIE_TLM_V_VDDIO2,
    MACIE_TLM_V_VSSIO1,
    MACIE_TLM_V_VSSIO2,
    MACIE_TLM_V_VDDAUX1,
    MACIE_TLM_V_VDDAUX2,
    MACIE_TLM_V_GNDA1,
    MACIE_TLM_V_GNDA2,
    MACIE_TLM_V_GND1,
    MACIE_TLM_V_GND2,
    MACIE_TLM_V_ASIC_5V,
    MACIE_TLM_V_FPGA_5V,
    MACIE_TLM_V_DVDD_3P3V,
    MACIE_TLM_V_DVDD_2P5V,
    MACIE_TLM_V_DVDD_1P8V,
    MACIE_TLM_V_DVDD_1P2V,
    MACIE_TLM_V_GIGE_3P3V,
    MACIE_TLM_V_USB_5V,
    MACIE_TLM_V_USB_3P3V,
    MACIE_TLM_V_VDDALOW1_ASIC,
    MACIE_TLM_V_VDDALOW2_ASIC,
    MACIE_TLM_V_SENSE_VDDAHIGH1,
    MACIE_TLM_V_SENSE_VDDAHIGH2,
    MACIE_TLM_V_SENSE_VDDALOW1,
    MACIE_TLM_V_SENSE_VDDALOW2,
    MACIE_TLM_V_SENSE_GNDA1,
    MACIE_TLM_V_SENSE_GNDA2,
    MACIE_TLM_V_VDDLOW1_ASIC,
    MACIE_TLM_V_VDDLOW2_ASIC,
    MACIE_TLM_V_SENSE_VDDHIGH1,
    MACIE_TLM_V_SENSE_VDDHIGH2,
    MACIE_TLM_V_SENSE_VDDLOW1,
    MACIE_TLM_V_SENSE_VDDLOW2,
    MACIE_TLM_V_SENSE_GND1,
    MACIE_TLM_V_SENSE_GND2,
    MACIE_TLM_V_VREF1_ASIC,
    MACIE_TLM_V_VREF2_ASIC,
    MACIE_TLM_V_AGND_CLEAN,
    MACIE_TLM_V_AGND_DIRTY,
    MACIE_TLM_V_DGND,
    MACIE_TLM_I_VDDAHIGH1,
    MACIE_TLM_I_VDDAHIGH2,
    MACIE_TLM_I_VDDALOW1,
    MACIE_TLM_I_VDDALOW2,
    MACIE_TLM_I_VREF1,
    MACIE_TLM_I_VREF2,
    MACIE_TLM_I_VDDHIGH1,
    MACIE_TLM_I_VDDHIGH2,
    MACIE_TLM_I_VDDLOW1,
    MACIE_TLM_I_VDDLOW2,
    MACIE_TLM_I_VDDIO1,
    MACIE_TLM_I_VDDIO2,
    MACIE_TLM_I_VSSIO1,
    MACIE_TLM_I_VSSIO2,
    MACIE_TLM_I_VDDAUX1,
    MACIE_TLM_I_VDDAUX2,
    MACIE_TLM_I_GNDA1,
    MACIE_TLM_I_GNDA2,
    MACIE_TLM_I_GND1,
    MACIE_TLM_I_GND2,
    MACIE_TLM_I_ASIC_5V,
    MACIE_TLM_I_FPGA_5V,
    MACIE_TLM_I_DVDD_3P3V,
    MACIE_TLM_I_DVDD_2P5V,
    MACIE_TLM_I_DVDD_1P8V,
    MACIE_TLM_I_DVDD_1P2V,
    MACIE_TLM_I_GIGE_3P3V,
    MACIE_TLM_I_USB_5V,
    MACIE_TLM_I_USB_3P3V
} MACIE_TLM_ITEM;

//------------------------------------------------------------------------
// MACIE Card information structure
//------------------------------------------------------------------------
typedef struct
{
    unsigned short  macieSerialNumber;
    bool            bUART;
    bool            bGigE;
    bool            bUSB;
    unsigned char   ipAddr[4];
    unsigned short  gigeSpeed;
    char            serialPortName[10];
    char            usbSerialNumber[16];
    char            firmwareSlot1[100];
    char            firmwareSlot2[100];
    unsigned short  usbSpeed;
} MACIE_CardInfo;

typedef enum
{
    HDR_INT,
    HDR_FLOAT,
    HDR_STR
} Fits_HdrType;

//Each fits header unit contains a sequence of fixed-length 80-character keyword records which have the general form:
//Example: KEYNAME = value / comment string
//Important Note: total number of characters of the sequence is 80
typedef struct
{
    char key[9];
    Fits_HdrType valType;
    int   iVal;
    float fVal;
    char  sVal[72];
    char  comment[72];
} MACIE_FitsHdr;


typedef struct
{
    unsigned char   ipAddr[4];
} MACIE_IpAddr;


//------------------------------------------------------------------------
// API functions
//------------------------------------------------------------------------
MACIESHARED_EXPORT float MACIE_LibVersion();

MACIESHARED_EXPORT MACIE_STATUS MACIE_Init();

MACIESHARED_EXPORT MACIE_STATUS MACIE_Free();

MACIESHARED_EXPORT char* MACIE_Error();

MACIESHARED_EXPORT MACIE_STATUS MACIE_CheckInterfaces( unsigned short gigeCommandPort,
                                                       MACIE_IpAddr *pIpAddrList,
                                                       unsigned short nIpAddr,
                                                       unsigned short *numCards,
                                                       MACIE_CardInfo **pCardInfo );
MACIESHARED_EXPORT MACIE_STATUS MACIE_SetGigeTimeout(unsigned short timeout);

MACIESHARED_EXPORT unsigned long MACIE_GetHandle( unsigned short MACIESerialNumber,
                                                  MACIE_Connection connection );

MACIESHARED_EXPORT unsigned char MACIE_GetAvailableMACIEs( unsigned long handle );

MACIESHARED_EXPORT unsigned char MACIE_GetAvailableASICs( unsigned long handle, int asicType );

MACIESHARED_EXPORT MACIE_STATUS MACIE_ReadMACIEReg( unsigned long  handle,
                                                    unsigned char  slctMACIEs,
                                                    unsigned short address,
                                                    unsigned int   *value );

MACIESHARED_EXPORT MACIE_STATUS MACIE_WriteMACIEReg( unsigned long  handle,
                                                     unsigned char  slctMACIEs,
                                                     unsigned short addrress,
                                                     unsigned int   value );

MACIESHARED_EXPORT MACIE_STATUS MACIE_WriteMACIEBlock( unsigned long  handle,
                                                       unsigned char  slctMACIEs,
                                                       unsigned short address,
                                                       unsigned int   *valueArray,
                                                       int            arrSize );

MACIESHARED_EXPORT MACIE_STATUS MACIE_ReadMACIEBlock( unsigned long  handle,
                                                      unsigned char  slctMACIEs,
                                                      unsigned short address,
                                                      unsigned int   *valueArray,
                                                      int            arrSize );

MACIESHARED_EXPORT MACIE_STATUS MACIE_loadMACIEFirmware( unsigned long handle,
                                                         unsigned char slctMACIEs,
                                                         bool          bSlot1,
                                                         unsigned int  *pResult);

MACIESHARED_EXPORT MACIE_STATUS MACIE_DownloadMACIEFile( unsigned long handle,
                                                         unsigned char slctMACIEs,
                                                         const char*   regFile );

MACIESHARED_EXPORT MACIE_STATUS MACIE_WriteASICReg( unsigned long  handle,
                                                    unsigned char  slctASICs,
                                                    unsigned short address,
                                                    unsigned int   value,
                                                    bool           bOption);

MACIESHARED_EXPORT MACIE_STATUS MACIE_ReadASICReg( unsigned long  handle,
                                                   unsigned char  slctASICs,
                                                   unsigned short address,
                                                   unsigned int   *value,
                                                   bool           b24bit,
                                                   bool           bOption );

MACIESHARED_EXPORT MACIE_STATUS MACIE_WriteASICBlock( unsigned long  handle,
                                                      unsigned char  slctASICs,
                                                      unsigned short address,
                                                      unsigned int   *valueArray,
                                                      int            arrSize,
                                                      bool           bOption );

MACIESHARED_EXPORT MACIE_STATUS  MACIE_ReadASICBlock( unsigned long  handle,
                                                      unsigned char  slctASICs,
                                                      unsigned short address,
                                                      unsigned int   *valueArray,
                                                      int            arrSize,
                                                      bool           b24bit,
                                                      bool           bOption );

MACIESHARED_EXPORT MACIE_STATUS MACIE_DownloadASICFile( unsigned long  handle,
                                                        unsigned char  slctASICs,
                                                        const char     *regFile,
                                                        bool           bOption );

MACIESHARED_EXPORT MACIE_STATUS MACIE_ClosePort( unsigned long  handle );


MACIESHARED_EXPORT MACIE_STATUS MACIE_ResetErrorCounters( unsigned long handle,
                                                        unsigned char slctMACIEs );


MACIESHARED_EXPORT MACIE_STATUS MACIE_SetMACIEPhaseShift( unsigned long handle,
                                                          unsigned char slctMACIEs,
                                                          unsigned short clkPhase );

MACIESHARED_EXPORT MACIE_STATUS MACIE_GetMACIEPhaseShift( unsigned long handle,
                                                          unsigned char slctMACIEs,
                                                          unsigned short *clkPhase );

MACIESHARED_EXPORT MACIE_STATUS MACIE_DownloadLoadfile( unsigned long  handle,
                                                        unsigned char  slctMACIEs,
                                                        unsigned char  slctASICs,
                                                        const char     *regFile,
                                                        bool           bOption );


MACIESHARED_EXPORT MACIE_STATUS MACIE_GetErrorCounters( unsigned long  handle,
                                                        unsigned char  slctMACIEs,
                                                        unsigned short *counterArray);

MACIESHARED_EXPORT MACIE_STATUS MACIE_ConfigureCamLinkInterface( unsigned long handle,
                                                                 unsigned char slctMACIEs,
                                                                 unsigned short mode,
                                                                 const char    *dcfFFile,
                                                                 unsigned short timeout,
                                                                 unsigned short frameX,
                                                                 unsigned short frameY,
                                                                 short          *nBuffers );


MACIESHARED_EXPORT MACIE_STATUS MACIE_ConfigureGigeScienceInterface( unsigned long handle,
                                                                     unsigned char slctMACIEs,
                                                                     unsigned short mode,
                                                                     int           frameSize,
                                                                     unsigned short remotePort,
                                                                     int            *bufSize );

MACIESHARED_EXPORT MACIE_STATUS MACIE_ConfigureUSBScienceInterface( unsigned long handle,
                                                                    unsigned char slctMACIEs,
                                                                    unsigned short mode,
                                                                    int           frameSize,
                                                                    short         nBuffers );

MACIESHARED_EXPORT unsigned long MACIE_AvailableScienceData( unsigned long handle );

MACIESHARED_EXPORT unsigned long MACIE_AvailableScienceFrames( unsigned long handle );

MACIESHARED_EXPORT unsigned short* MACIE_ReadGigeScienceFrame( unsigned long handle,
                                                               unsigned short timeout );

MACIESHARED_EXPORT unsigned short* MACIE_ReadCamlinkScienceFrame( unsigned long handle,
                                                                  const char *tifFileName,
                                                                  unsigned short timeout );

MACIESHARED_EXPORT unsigned short* MACIE_ReadUSBScienceFrame( unsigned long handle,
                                                              unsigned short timeout );

MACIESHARED_EXPORT MACIE_STATUS MACIE_WriteFitsFile( char        *fileName,
                                                     unsigned short frameX,
                                                     unsigned short frameY,
                                                     unsigned short *pData,
                                                     unsigned short nHeaders,
                                                     MACIE_FitsHdr  *pHeaders );

MACIESHARED_EXPORT int MACIE_ReadGigeScienceData( unsigned long handle,
                                                  unsigned short timeout,
                                                  long           n,
                                                  unsigned short *pData );

MACIESHARED_EXPORT int MACIE_ReadUSBScienceData(  unsigned long handle,
                                                  unsigned short timeout,
                                                  long           n,
                                                  unsigned short *pData );

MACIESHARED_EXPORT MACIE_STATUS MACIE_CloseCamlinkScienceInterface( unsigned long handle,
                                                                 unsigned char slctMACIEs);

MACIESHARED_EXPORT MACIE_STATUS MACIE_CloseGigeScienceInterface( unsigned long handle,
                                                                 unsigned char slctMACIEs);

MACIESHARED_EXPORT MACIE_STATUS MACIE_CloseUSBScienceInterface( unsigned long handle,
                                                                unsigned char slctMACIEs);

MACIESHARED_EXPORT MACIE_STATUS MACIE_SetVoltage( unsigned long handle,
                                                  unsigned char slctMACIEs,
                                                  MACIE_PWR_DAC powerId,
                                                  float         powerValue);

MACIESHARED_EXPORT MACIE_STATUS MACIE_GetVoltage( unsigned long handle,
                                                  unsigned char slctMACIEs,
                                                  MACIE_PWR_DAC powerId,
                                                  float*        powerValue);

MACIESHARED_EXPORT MACIE_STATUS MACIE_EnablePower( unsigned long handle,
                                                   unsigned char  slctMACIEs,
                                                   MACIE_PWR_CTRL* pwrCtrlIdArr,
                                                   short          n);

MACIESHARED_EXPORT MACIE_STATUS MACIE_DisablePower( unsigned long handle,
                                                    unsigned char  slctMACIEs,
                                                    MACIE_PWR_CTRL* pwrCtrlIdArr,
                                                    short          n);

MACIESHARED_EXPORT MACIE_STATUS MACIE_SetPower( unsigned long handle,
                                                unsigned char  slctMACIEs,
                                                MACIE_PWR_CTRL pwrCtrlId,
                                                bool           bEnablePower);

MACIESHARED_EXPORT MACIE_STATUS MACIE_GetPower( unsigned long handle,
                                                unsigned char  slctMACIEs,
                                                MACIE_PWR_CTRL pwrCtrl,
                                                bool*          bEnablePower);

MACIESHARED_EXPORT MACIE_STATUS MACIE_SetTelemetryConfiguration( unsigned long handle,
                                                                 unsigned char  slctMACIEs,
                                                                 MACIE_TLM_SAMPLE_RATE vSampleRate,
                                                                 MACIE_TLM_AVERAGE     vAverage,
                                                                 MACIE_TLM_SAMPLE_RATE iSampleRate,
                                                                 MACIE_TLM_AVERAGE     iAverage,
                                                                 MACIE_TLM_GROUND_REFERENCE groundRef);

MACIESHARED_EXPORT MACIE_STATUS MACIE_GetTelemetryConfiguration( unsigned long handle,
                                                                 unsigned char  slctMACIEs,
                                                                 MACIE_TLM_SAMPLE_RATE* vSampleRate,
                                                                 MACIE_TLM_AVERAGE*     vAverage,
                                                                 MACIE_TLM_SAMPLE_RATE* iSampleRate,
                                                                 MACIE_TLM_AVERAGE*     iAverage,
                                                                 MACIE_TLM_GROUND_REFERENCE* groundRef);

MACIESHARED_EXPORT MACIE_STATUS MACIE_GetTelemetry( unsigned long handle,
                                                    unsigned char  slctMACIEs,
                                                    MACIE_TLM_ITEM tlmId,
                                                    float*         tlmValue);


MACIESHARED_EXPORT MACIE_STATUS MACIE_GetTelemetrySet( unsigned long handle,
                                                       unsigned char  slctMACIEs,
                                                       MACIE_TLM_ITEM* tlmIdArr,
                                                       short           n,
                                                       float*         tlmValArr);


MACIESHARED_EXPORT MACIE_STATUS MACIE_GetTelemetryAll( unsigned long handle,
                                                       unsigned char  slctMACIEs,
                                                       float*       tlmValArr);

MACIESHARED_EXPORT MACIE_STATUS MACIE_GetAcadiaAddressIncrement( unsigned long handle,
                                                                 unsigned char  slctMACIEs,
                                                                 bool*       bAutoAddrInc);
MACIESHARED_EXPORT MACIE_STATUS MACIE_SetAcadiaAddressIncrement( unsigned long handle,
                                                                 unsigned char  slctMACIEs,
                                                                 bool       bAutoAddrInc);

#ifdef __cplusplus
}
#endif

#endif // MACIE_H
