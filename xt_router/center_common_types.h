#ifndef _CENTER_COMMON_TYPES_H_INCLUDED
#define _CENTER_COMMON_TYPES_H_INCLUDED

//��������ص�һЩ���� ԭ������MDef.h��

//����ָ��
enum CenterServerCtrlType
{
    CT_STOPDB		= 0,        // ֹͣ�㲥
    CT_STARTDB		= 1,        // �����㲥
    CT_LOGIN		= 2,        // �豸��½
    CT_STOPALLDB	= 10,       // ֹͣ���е㲥
    CT_UPDATEIDS	= 20,       // IDS����
};

enum CenterServerOperationType
{
    OP_PLAY = 0x1,              //��ʼ�㲥����
    OP_STOP = 0x2,              //ֹͣ�㲥����
    OP_INIT_SERVER = 0x3,       //��ʼ������������
    OP_LOGIN = 0x4              //��¼��EDVR��
};

enum CenterServerErrorCode
{
    ERR_ALREADY_PLAY = 0xe01,           //�Ѿ��㲥
    ERR_SERVER_NO_ANSWER = 0xe02,       //������δ��Ӧ
    ERR_NO_SERVER_ID = 0xe03,           //û�п��Է���ķ�����ͨ����
    ERR_NO_PLAY_INFO = 0xe04,           //��ǰ�㲥��Ϣ������
    ERR_PLAY_FAIL = 0xe05,              //�㲥ʧ��
    ERR_NOT_LOGIN = 0xe06,              //δ��¼��EDVR��
    ERR_E_DVR = 0xe07,                  //EDVR�豸����
    ERR_SERVER_IP = 0xe08,              //��������IP����
    ERR_SERVER_NUM = 0xe09,             //��������ͨ��������
    ERR_MUL_SERVER_IP = 0xe0a,          //�������鲥IP����
    ERR_CHECK_LINK_HANDLE = 0xe0b,      //������Ӿ��
    ERR_LOGIN_ERROR = 0xe0c,            //��¼ʧ��
    ERR_ALREADY_CAPTURE = 0xe0d,        //�Ѿ����������ݲ���
    ERR_WAIT_RDATA = 0xe0e,             //���ӽ�����δ��ȡ�κ�����
    ERR_OVERTIME = 0xe0f,               //�ȴ����ݳ�ʱ
    ERR_DEVICE_NOT_FOUND = 0xe10,       //δ�ҵ����豸
    ERR_WAIT_RDATA_CLOSE = 0xe11,       //���ӽ�����δ��ȡ�κ�����,Ȼ�����������Ӳ���

    //�ɹ���Ϣ
    SUCCESS_PLAY_OK = 0xa01,                //�ɹ���Ϣ����
    SUCCESS_LOGIN_OK = 0xa02,               //��¼�ɹ�
    SUCCESS_ALREADY_LINK = 0xa03,           //�Ѿ�����������
    SUCCESS_STOP_OK = 0xa04,                 //ͣ��ɹ�
};

#endif //_CENTER_COMMON_TYPES_H_INCLUDED
