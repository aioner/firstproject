#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#define DEBUG_OUTPUT	0						//�����Ƿ������
#define USE_MP_MANAGER	0						//�����Ƿ�ʹ��mp_manager������
#define WRITE_LOG		1						//�����Ƿ������־
#define WRITE_LOG_RTP   1
#define BYTE_POOL_BLOCK_SIZE	2048			//bytepool���ݿ��С
#define BYTE_POOL_EXPAND_SIZE	5				//bytepool�Զ���չ����
#define BYTE_POOL_INIT_SIZE		5				//bytepool��ʼ�����ݿ����
#define BYTE_BLOCK_FIFO_SIZE	0
#define MP_SOURCE_FIFO_MAX_SIZE	0				//mp source fifo��󳤶�,ѭ��ʹ��10*1024*1024
#define MP_SINK_FIFO_MAX_SIZE	0				//mp sink fifo��󳤶�,ѭ��ʹ��
#define MP_SOURCE_SN_END_JUDGE_OFFSET	65000	//rtp���ݰ�SN�Ž����仯�жϲ���
												//(���磺һ�����ݰ���SNΪ65530����һ��Ϊ3��
												//��ô65530-3 > 65000���жϸ�rtp���ݰ���SN��ѭ��һ��)
#define MP_SOURCE_SN_MAX_NUM	65536			//rtp���ݰ�SN�����ֵ
#define MP_SN_JUDGE_CONDITION	32768			//
#define MP_TS_JUDGE_CONDITION	0X7FFFFFFF
#define MP_SOURCE_MAX_TURNS		100				//rtp���ݰ�SNѭ�������
#define MP_RECEIVE_BUFFER_SIZE	20*1024*1024	//���ջ�������С
#define MP_SEND_BUFFER_SIZE		20*1024*1024	//���ͻ�������С
#define MP_RTCP_INTERVAL		5				//5s���ͼ��
#define MP_SND_OUT_BUFFER_SIZE	0				//���ͻ�������С
#define MP_MSSRC_TASK_LOCK_TM	1				//������ˮ��������ʱʱ��
#define MP_MSINK_TASK_LOCK_TM	1				//������ˮ��������ʱʱ��
#define MP_FORWORD_TASK_LOCK_TM	1				//�ļ���ˮ��������ʱʱ��
#define MP_VERSION_MAIN			0
#define MP_VERSION_SUB			0
#define MP_VERSION_TEMP			18
#define MP_RTP_MUTICAST_TTL		128

#define __DEBUG					0

#endif
