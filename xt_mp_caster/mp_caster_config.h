///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����mp_caster_config.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��23��
// ����������ý�����ݹ㲥�� -- �����ļ�
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MP_CASTER_CONFIG_
#define MP_CASTER_CONFIG_

#ifndef _WIN32_WINNT            // ָ��Ҫ������ƽ̨�� Windows Vista��
#define _WIN32_WINNT 0x0501     // ����ֵ����Ϊ��Ӧ��ֵ���������� Windows �������汾��
#endif

//�汾����
#define MP_CASTER_VER1	0
#define MP_CASTER_VER2	0
#define MP_CASTER_VER3	15

/*
���������Ż�Ŀ����release�汾��ִ�к�����ڲ��������ƣ���ߺ���ִ���ٶ�
ǰ������debug�汾����û�����������øû���
*/
#ifndef NDEBUG
#define MP_CASTER_PARAM_CHECK	1
#else
#define MP_CASTER_PARAM_CHECK	1
#endif

//���̹߳���ͷ��ʿ�ʱ���˺�������á�
//�����ȷ������������ڣ���������Ż��Ƕȿ��Կ��ǹرոú�
#define MP_CASTER_SHARE_LOCK	1

//Ĭ��MP���ݳ����� -- ������ߴ�
#define MP_BLOCK_POOL_SIZE		2048
//Ĭ��MP���ݳ����� -- �Զ���չ����
#define MP_BLOCK_POOL_EXPAND	10
//Ĭ��MP���ݳ����� -- Ԥ��������
#define MP_BLOCK_POOL_DEFAULT	10

//Ĭ��֡��ʽ�����αװý���������� 96 -- H.263 90000Hz Clock
//1ms = 1 * 90, 30ms = 2700, 40ms = 3600ms
#define MP_PSEUDO_PAYLOAD_TYPE	96
#define MP_PSEUDO_RTP_MAX_SIZE	1400
#define MP_PSEUDO_RTP_HEAD_SIZE	16
#define MP_PSEUDO_TS_CLOCK		90

//Caster Engine�ڲ���ʱ����С����ֵ����ͬ����ϵͳ��ֵ����������
#define CASTER_ENGINE_TIMER_SLICE	10

#define MP_MSSRC_TASK_LOCK_TM		1
#define MP_MSINK_TASK_LOCK_TM		1


//FIFO��������
/*
MSSRC_FRAME		��120fps��ƻ���ڵ���
MSSRC_RTP		��30M������� ������ MP_PSEUDO_RTP_MAX_SIZE
MSSRC_RV_RTP	��30M������� ������ MP_PSEUDO_RTP_MAX_SIZE
*/
//#define MSSRC_FRAME_FIFO_SIZE			120
//#define MSSRC_FRAME_FIFO_OVERLAPPED		true
#define MSSRC_FRAME_FIFO_SIZE			0
#define MSSRC_FRAME_FIFO_OVERLAPPED		false

//#define MSSRC_RTP_FIFO_SIZE				256
//#define MSSRC_RTP_FIFO_OVERLAPPED		true
#define MSSRC_RTP_FIFO_SIZE				0
#define MSSRC_RTP_FIFO_OVERLAPPED		false

//#define MSSRC_RV_RTP_FIFO_SIZE			256
//#define MSSRC_RV_RTP_FIFO_OVERLAPPED	true
#define MSSRC_RV_RTP_FIFO_SIZE			0
#define MSSRC_RV_RTP_FIFO_OVERLAPPED	false

//����mssrc_rv_rtpʵ���rtcp sr�������󻺴�����
//����mssrc_rv_rtpʵ���rtcp sr������Ƿ���ø�����ʷ��ʽ���
#define MSSRC_RV_RTP_SR_FIFO_SIZE		2
#define MSSRC_RV_RTP_SR_FIFO_OVERLAPPED	true
//����mssrc_rv_rtpʵ���rtcp rr�������󻺴�����
//����mssrc_rv_rtpʵ���rtcp rr������Ƿ���ø�����ʷ��ʽ���
#define MSSRC_RV_RTP_RR_FIFO_SIZE		2
#define MSSRC_RV_RTP_RR_FIFO_OVERLAPPED	true

#ifdef TI_368
//����mssrc_rv_rtpʵ���socket���ջ������ߴ�
#define MSSRC_RV_RTP_RECEIVE_BUFFER_SIZE	1 * 1024  *1024
#else
//����mssrc_rv_rtpʵ���socket���ջ������ߴ�
#define MSSRC_RV_RTP_RECEIVE_BUFFER_SIZE	5 * 1024  *1024
#endif

/*
MSINK_MEMORY	��30M������� ������ MP_PSEUDO_RTP_MAX_SIZE
MSINK_RV_RTP	��30M������� ������ MP_PSEUDO_RTP_MAX_SIZE
*/
//#define MSINK_MEMORY_FIFO_SIZE		256
//#define MSINK_MEMORY_FIFO_OVERLAPPED	true
#define MSINK_MEMORY_FIFO_SIZE			0
#define MSINK_MEMORY_FIFO_OVERLAPPED	false

//#define MSINK_RV_RTP_FIFO_SIZE		256
//#define MSINK_RV_RTP_FIFO_OVERLAPPED	true
#define MSINK_RV_RTP_FIFO_SIZE			0
#define MSINK_RV_RTP_FIFO_OVERLAPPED	false

//����msink_rv_rtpʵ���rtcp sr�������󻺴�����
//����msink_rv_rtpʵ���rtcp sr������Ƿ���ø�����ʷ��ʽ���
#define MSINK_RV_RTP_SR_FIFO_SIZE		2
#define MSINK_RV_RTP_SR_FIFO_OVERLAPPED	true
//����msink_rv_rtpʵ���rtcp rr�������󻺴�����
//����msink_rv_rtpʵ���rtcp rr������Ƿ���ø�����ʷ��ʽ���
#define MSINK_RV_RTP_RR_FIFO_SIZE		1024
#define MSINK_RV_RTP_RR_FIFO_OVERLAPPED	true

#ifdef TI_368
//����msink_rv_rtpʵ���socket���ͻ������ߴ�
#define MSINK_RV_RTP_TRANSMIT_BUFFER_SIZE	1 * 1024 * 1024
#else
//����msink_rv_rtpʵ���socket���ͻ������ߴ�
#define MSINK_RV_RTP_TRANSMIT_BUFFER_SIZE	20 * 1024 * 1024
#endif

//msink_rv_rtp����ģʽ
#define MSINK_RV_RTP_WRITE_BYPASS			0
#define MSINK_RV_RTP_SYNC_WRITE_SAFE		1
#define MSINK_RV_RTP_ASYNC_WRITE_SAFE		2
#define MSINK_RV_RTP_SYNC_WRITE_FAST		3
#define MSINK_RV_RTP_ASYNC_WRITE_FAST		4
#define MSINK_RV_RTP_WRITE_MODE				MSINK_RV_RTP_WRITE_BYPASS

#include "caster_config.h"
#endif
