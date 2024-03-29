/* 
	Editor: http://www.visualmicro.com
	        visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
	        the contents of the Visual Micro sketch sub folder can be deleted prior to publishing a project
	        all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
	        note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: LinkIt ONE, Platform=arm, Package=LinkIt
*/

#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
#define printf iprintf
#define F_CPU 84000000L
#define ARDUINO 10607
#define ARDUINO_MTK_ONE
#define ARDUINO_ARCH_ARM
#define __COMPILER_GCC__
#define __LINKIT_ONE__
#define __LINKIT_ONE_RELEASE__
#define USB_VID 0x0E8D
#define USB_PID 0x0023
#define USBCON
#define __cplusplus 201103L
#define GCC_VERSION 40803
#define __inline__
#define __asm__(x)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__


#define __ICCARM__
#define __ASM
#define __INLINE
#define __builtin_va_list void
//#define _GNU_SOURCE 
//#define __GNUC__ 0
//#undef  __ICCARM__
//#define __GNU__

typedef long Pio;
typedef long Efc;
typedef long Adc;
typedef long Pwm;
typedef long Rtc;
typedef long Rtt;
typedef long pRtc;
typedef long Spi;
typedef long spi;
typedef long Ssc;
//typedef long p_scc;
typedef long Tc;
//typedef long pTc;
typedef long Twi;
typedef long Wdt;
//typedef long pTwi;
typedef long Usart;
typedef long Pdc;
typedef long Rstc;

extern const int ADC_MR_TRGEN_DIS = 0;
extern const int ADC_MR_TRGSEL_ADC_TRIG0 = 0;
extern const int ADC_MR_TRGSEL_Pos = 0;

extern const int ADC_MR_TRGSEL_Msk = 0;
extern const int ADC_MR_TRGEN = 0;
extern const int ADC_TRIG_TIO_CH_0 = 0;
extern const int ADC_MR_TRGSEL_ADC_TRIG1 = 0;
extern const int ADC_TRIG_TIO_CH_1 = 0;
extern const int ADC_MR_TRGSEL_ADC_TRIG2 = 0;
extern const int ADC_MR_TRGSEL_ADC_TRIG3 = 0;

#define __ARMCC_VERSION 400678
#define __attribute__(noinline)

#define prog_void
#define PGM_VOID_P int


            
typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {;}



#include <arduino.h>
#include <pins_arduino.h> 
#include <variant.h> 
#undef F
#define F(string_literal) ((const PROGMEM char *)(string_literal))
#undef PSTR
#define PSTR(string_literal) ((const PROGMEM char *)(string_literal))
#undef cli
#define cli()
#define pgm_read_byte(address_short)
#define pgm_read_word(address_short)
#define pgm_read_word2(address_short)
#define digitalPinToPort(P)
#define digitalPinToBitMask(P) 
#define digitalPinToTimer(P)
#define analogInPinToBit(P)
#define portOutputRegister(P)
#define portInputRegister(P)
#define portModeRegister(P)
#include <..\Wearable_Device\Wearable_Device.ino>
#include <..\Wearable_Device\ADXL345.h>
#include <..\Wearable_Device\BTBinding.cpp>
#include <..\Wearable_Device\BTBinding.h>
#include <..\Wearable_Device\Ports.h>
#include <..\Wearable_Device\StatisticItem.cpp>
#include <..\Wearable_Device\StatisticItem.h>
#include <..\Wearable_Device\ThreadStarter.h>
#include <..\Wearable_Device\cJSON.c>
#include <..\Wearable_Device\cJSON.h>
#include <..\Wearable_Device\cJSON_Utils.c>
#include <..\Wearable_Device\cJSON_Utils.h>
#include <..\Wearable_Device\command.cpp>
#include <..\Wearable_Device\command.h>
#include <..\Wearable_Device\counter.cpp>
#include <..\Wearable_Device\counter.h>
#include <..\Wearable_Device\fall.cpp>
#include <..\Wearable_Device\fall.h>
#include <..\Wearable_Device\notification.cpp>
#include <..\Wearable_Device\notification.h>
#include <..\Wearable_Device\priority.h>
#include <..\Wearable_Device\sleep.cpp>
#include <..\Wearable_Device\sleep.h>
#include <..\Wearable_Device\stateJudge.cpp>
#include <..\Wearable_Device\stateJudge.h>
#include <..\Wearable_Device\timestamp.h>
#include <..\Wearable_Device\timestamp_compare.c>
#include <..\Wearable_Device\timestamp_format.c>
#include <..\Wearable_Device\timestamp_parse.c>
#include <..\Wearable_Device\timestamp_tm.c>
#include <..\Wearable_Device\timestamp_valid.c>
#endif
