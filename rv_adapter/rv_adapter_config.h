///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����rv_adapter_config.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��19��
// ����������radvsion ARTPЭ��ջ������ -- �����ļ�
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef RADVISION_ADAPTER_CONFIG_
#define RADVISION_ADAPTER_CONFIG_

//�汾����
#define RV_ADAPTER_VER1		0
#define RV_ADAPTER_VER2		0
#define RV_ADAPTER_VER3		12

//#define DUMP_RTP_FILE
/*
	���ڵ���Ŀ�Ŀ�����ʱ�رտ���ȫ����artpЭ��ջ�����������ܵ���Ч�Ժ��ȶ��ԣ�
	���RV_CORE_ENABLE = 0�󣬸���׮��������ֵ�μӴ��붨�壬һ��Ϊ�ɹ�·��
*/
#ifndef NDEBUG
#define RV_CORE_ENABLE	1	
#else
#define RV_CORE_ENABLE	1	
#endif

/*
	���������Ż�Ŀ����release�汾��ִ�к�����ڲ��������ƣ���ߺ���ִ���ٶ�
	ǰ������debug�汾����û�����������øû���
*/
#ifndef NDEBUG
#define RV_ADAPTER_PARAM_CHECK	1
#else
#define RV_ADAPTER_PARAM_CHECK	0
#endif

//���̹߳���ͷ��ʿ�ʱ���˺�������á�
//�����ȷ������������ڣ���������Ż��Ƕȿ��Կ��ǹرոú�
#define RV_ADAPTER_SHARE_LOCK	1

//�첽д��RTP��ʱ��ģʽ1�Ļ��������ȹ滮
#define RV_ADAPTER_ASYNC_WRITE_BUFFER_SIZE	2048

#endif

