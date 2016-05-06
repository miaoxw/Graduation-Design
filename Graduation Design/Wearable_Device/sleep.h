#ifndef _SLEEP_h
#define _SLEEP_h

//#if defined(ARDUINO) && ARDUINO >= 100
//	#include "arduino.h"
//#else
//	#include "WProgram.h"
//#endif

#define SLEEP_ALLOC_BUF_LEN 2048

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

namespace Sleep
{
	struct sleep_conf
	{
		u16 m_hz;
		u16 m_short_period;
		u16 m_long_period;
		u16 m_max_sleep_minc;

		struct
		{
			u16* m_values;
			u16  m_len;
			u16 m_offset;
		}
		m_ks;

		struct
		{
			u16 m_webster;
			u16 m_rovel;
			u16 m_head;
		}
		m_threhold;

		u16 m_states_period;
	};

	bool sleep_init(struct sleep_conf *p_config);
	void sleep_on_data(s16 p_acc_x,s16 p_acc_y,s16 p_acc_z);
	u16 sleep_get_count();
	u16 sleep_get_mid_result(u16 p_index);
	u8 sleep_get_state(u16 p_index);
}

#endif

