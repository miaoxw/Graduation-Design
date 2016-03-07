#ifndef _ADXL345_h
#define _ADXL345_h

namespace ADXL345
{
	const double SCALE = 64.0;

	const uint8_t address = 0x53;
	const uint8_t OFSX = 0x1E;
	const uint8_t OFSY = 0x1F;
	const uint8_t OFSZ = 0x20;
	const uint8_t BW_RATE = 0x2C;
	const uint8_t POWER_CTL = 0x2D;
	const uint8_t INT_ENABLE = 0x2E;
	const uint8_t INT_MAP = 0x2F;
	const uint8_t INT_SOURCE = 0x30;
	const uint8_t DATA_FORMAT = 0x31;

	const uint8_t X_low = 0x32;
	const uint8_t Y_low = 0x34;
	const uint8_t Z_low = 0x36;

	const uint8_t THRESHOLD_0G = 5u;
};

#endif
