#include "sleep.h"

struct queue
{
	s16* m_values;
	u16 m_maxlen;
	u16 m_start;
	u16 m_count;
	s32 m_sum;
};

static void queue_init(struct queue* p_queue,s16* p_buffer,u16 p_maxlen)
{
	p_queue->m_values=p_buffer;
	p_queue->m_maxlen=p_maxlen;
	p_queue->m_start=0;
	p_queue->m_count=0;
	p_queue->m_sum=0;
}

static void queue_put(struct queue* p_queue,s16 p_value)
{
	p_queue->m_sum+=p_value;
	if(p_queue->m_count<p_queue->m_maxlen)
	{
		u16 t_index=p_queue->m_start+p_queue->m_count;
		if(t_index>=p_queue->m_maxlen)
			t_index-=p_queue->m_maxlen;
		p_queue->m_values[t_index]=p_value;
		p_queue->m_count++;
	}
	else
	{
		p_queue->m_sum-=p_queue->m_values[p_queue->m_start];
		p_queue->m_values[p_queue->m_start]=p_value;
		p_queue->m_start++;
		if(p_queue->m_start==p_queue->m_maxlen)
			p_queue->m_start=0;
	}
}

static s16 queue_get(struct queue* p_queue,u16 p_index)
{
	u16 t_index=(p_queue->m_start+p_index)%p_queue->m_maxlen;
	return p_queue->m_values[t_index];
}

static s16 queue_get_avg(struct queue* p_queue)
{
	return (s16)(p_queue->m_sum/p_queue->m_count);
}

struct two_avg
{
	struct queue m_short;
	struct queue m_long;
};

static void two_avg_init(struct two_avg* p_two_avg,s16* p_buffer,u16 p_short_len,u16 p_long_len)
{
	queue_init(&(p_two_avg->m_short),p_buffer,p_short_len);
	queue_init(&(p_two_avg->m_long),p_buffer+p_short_len,p_long_len);
}

static s16 two_avg_trans(struct two_avg* p_two_avg,s16 p_value)
{
	queue_put(&(p_two_avg->m_short),p_value);
	queue_put(&(p_two_avg->m_long),p_value);
	s16 t_short_avg=queue_get_avg(&(p_two_avg->m_short));
	s16 t_long_avg=queue_get_avg(&(p_two_avg->m_long));
	return t_short_avg-t_long_avg;
}

static u16 sg_hz;
static u16 sg_webster,sg_rovel,sg_head;
static struct two_avg sg_accs[3];
static u16* sg_ks;
static u16 sg_ks_offset;
static struct queue sg_an,sg_dn;
static u32 sg_area_sum;
static u16 sg_area_count;
static struct queue sg_states;
static u16 sg_max_acc;
static u16 sg_states_period;
static s16 sg_buffer[SLEEP_ALLOC_BUF_LEN];

bool Sleep::sleep_init(struct sleep_conf *p_config)
{
	if((p_config->m_short_period+p_config->m_long_period)*3
		+p_config->m_ks.m_len*2+p_config->m_max_sleep_minc*2>SLEEP_ALLOC_BUF_LEN)
		return false;
	sg_hz=p_config->m_hz;
	sg_webster=p_config->m_threhold.m_webster;
	sg_rovel=p_config->m_threhold.m_rovel;
	sg_head=p_config->m_threhold.m_head;
	s16* t_buffer=sg_buffer;
	u16 t_i;
	for(t_i=0;t_i<3;t_i++)
	{
		two_avg_init(sg_accs+t_i,t_buffer,p_config->m_short_period,p_config->m_long_period);
		t_buffer+=p_config->m_short_period+p_config->m_long_period;
	}
	sg_ks=(u16 *)t_buffer;
	t_buffer+=p_config->m_ks.m_len;
	sg_ks_offset=p_config->m_ks.m_offset;
	for(t_i=0;t_i<p_config->m_ks.m_len;t_i++)
		sg_ks[t_i]=p_config->m_ks.m_values[t_i];
	queue_init(&sg_an,t_buffer,p_config->m_ks.m_len);
	t_buffer+=p_config->m_ks.m_len;
	queue_init(&sg_dn,t_buffer,p_config->m_max_sleep_minc);
	t_buffer+=p_config->m_max_sleep_minc;
	sg_area_sum=0;
	sg_area_count=0;
	queue_init(&sg_states,t_buffer,p_config->m_max_sleep_minc);
	t_buffer+=p_config->m_max_sleep_minc;
	sg_max_acc=0;
	sg_states_period=p_config->m_states_period;
	return true;
}

static u32 abs_int(s32 p_n)
{
	if(p_n<0)
		return -p_n;
	return p_n;
}

static u32 sqrt_int(u32 p_n)
{
	u32 t_left=0,t_right=(1<<16);
	while(t_left<t_right)
	{
		u32 t_mid=(t_left+t_right)/2;
		u32 t_mid2=t_mid*t_mid;
		if(t_mid2==p_n)
			return t_mid;
		else if(t_mid2>p_n)
			t_right=t_mid-1;
		else
			t_left=t_mid+1;
	}
	u32 t_ans=(t_left+t_right)/2;
	t_left=t_ans-1;
	t_right=t_ans+1;
	if(abs_int(t_left*t_left-p_n)<abs_int(t_ans*t_ans-p_n))
		t_ans=t_left;
	if(abs_int(t_right*t_right-p_n)<abs_int(t_ans*t_ans-p_n))
		t_ans=t_right;
	return t_ans;
}

void Sleep::sleep_on_data(s16 p_acc_x,s16 p_acc_y,s16 p_acc_z)
{
	s32 t_x=two_avg_trans(sg_accs+0,p_acc_x);
	s32 t_y=two_avg_trans(sg_accs+1,p_acc_y);
	s32 t_z=two_avg_trans(sg_accs+2,p_acc_z);
	u32 t_acc=sqrt_int(t_x*t_x+t_y*t_y+t_z*t_z);
	sg_area_sum+=t_acc;
	sg_area_count++;
	if(t_acc>sg_max_acc)
		sg_max_acc=t_acc;
	if(sg_area_count==sg_hz*60)
	{
		u16 t_area=(u16)(sg_area_sum/sg_hz);
		queue_put(&sg_an,t_area);
		sg_area_sum=0;
		sg_area_count=0;
		if(sg_an.m_count==sg_an.m_maxlen)
		{
			u32 t_sum=0;
			u16 t_i;
			for(t_i=0;t_i<sg_an.m_count;t_i++)
				t_sum+=sg_ks[t_i]*queue_get(&sg_an,t_i);
			t_sum/=10;
			queue_put(&sg_dn,(u16)t_sum);
			u8 t_state;
			if(t_sum>sg_webster)
				t_state=0;
			else
			{
				if(sg_max_acc>sg_rovel)
					t_state=1;
				else if(sg_max_acc>sg_head)
					t_state=2;
				else
					t_state=3;
			}
			queue_put(&sg_states,t_state);
			sg_max_acc=0;
		}
	}
}

u16 Sleep::sleep_get_count()
{
	return sg_dn.m_count;
}

u16 Sleep::sleep_get_mid_result(u16 p_index)
{
	return queue_get(&sg_dn,p_index);
}

u8 Sleep::sleep_get_state(u16 p_index)
{
	u16 t_min=4;
	u8 t_i;
	for(t_i=0;t_i<sg_states_period;t_i++)
	{
		if(t_i>p_index)
			break;
		u16 t_state=queue_get(&sg_states,p_index-t_i);
		if(t_state<t_min)
			t_min=t_state;
	}
	return t_min;
}