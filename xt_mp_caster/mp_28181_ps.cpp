
//#include <iostream>
#include <stdio.h>
#include <string.h>
#include "mp_28181_ps.h"
//using namespace std;

#if 0
int main(void)
{
    unsigned char pData[4*1024];

    bits_buffer_s   bitsBuffer;  
    bitsBuffer.i_size = PS_HDR_LEN;   
    bitsBuffer.i_data = 0;  
    // �����ƣ�10000000 ������Ϊ�˺����һ���ֽڵ�ÿһλ���в����������С�˿��ֽ��������  
    bitsBuffer.i_mask = 0x80;

    bitsBuffer.p_data = (unsigned char *)(pData);  
    memset(bitsBuffer.p_data, 0, PS_HDR_LEN);  

    bits_write(&bitsBuffer, 32, 0x000001BA);

    for(int i=0;i<4;i++)
    {
        printf("pData[%d]::%0x\n", i, pData[i]);
    }


    return 0;
}

#endif

bool is_Iframe(unsigned char nal_type)
{
    return ((NALU_IDR == nal_type) || (NALU_SEI == nal_type) || (NALU_SPS == nal_type) || (NALU_PPS == nal_type));
}

bool is_NALU_SPS(unsigned char nal_type)
{
    return (NALU_SPS == nal_type);
}

bool is_NALU_PPS(unsigned char nal_type)
{
    return (NALU_PPS == nal_type);
}

bool is_NALU_SEI(unsigned char nal_type)
{
    return (NALU_SEI == nal_type);
}

bool is_NALU_IDR(unsigned char nal_type)
{
    return (NALU_IDR == nal_type);
}


/*@param :  pData      [in] ��Ҫ���͵�����Ƶ���� 
4.	 *          nFrameLen  [in] �������ݵĳ��� 
5.	 *          pPacker    [in] ���ݰ���һЩ��Ϣ������ʱ�����rtp����buff�����͵�socket�����Ϣ 
6.	 *          stream_type[in] �������� 0 ��Ƶ 1 ��Ƶ 
7.	 *@return:  0 success others failed 
8.	*/  
#if 0
int gb28181_streampackageForH264(char *pData, int nFrameLen, Data_Info_s* pPacker, int stream_type)  
{  
    char szTempPacketHead[256];  
    int  nSizePos = 0;  
    int  nSize = 0;       
    char *pBuff = NULL;  
    memset(szTempPacketHead, 0, 256);  
    // 1 package for ps header   
    gb28181_make_ps_header(szTempPacketHead + nSizePos, pPacker->s64CurPts);  
    nSizePos += PS_HDR_LEN;   
    //2 system header   
    if( pPacker->IFrame == 1 )  
    {  
        // �����I֡�Ļ��������ϵͳͷ  
        gb28181_make_sys_header(szTempPacketHead + nSizePos);  
        nSizePos += SYS_HDR_LEN;  
        //����ط����ǲ�����I֡����p֡��������map�ģ�ò��ֻ��I֡��Ҳû������  
        //      gb28181_make_psm_header(szTempPacketHead + nSizePos);  
        //nSizePos += PSM_HDR_LEN;  
    }  

    // psmͷ (Ҳ��map)  
    gb28181_make_psm_header(szTempPacketHead + nSizePos);  
    nSizePos += PSM_HDR_LEN;  
    //����rtp���ͳ�ȥ�������Ļ�����������ݾ�ֻҪ��Ƭ�ְ���ֻ�м���pesͷ��rtpͷ��  
    if(gb28181_send_rtp_pack(szTempPacketHead, nSizePos, 0, pPacker) != 0 )  
        return -1;    
    // ��������ƶ���Ϊ�˷��㿽��pesͷ  
    //������Ϊ�˼��ٺ�������Ƶ�����ݵĴ��������˷ѿռ䣬�������������ƶ�����ʵ�ʴ����ʱ��Ҫע���ַ�Ƿ�Խ���Լ����ǵ�����  
    pBuff = pData - PES_HDR_LEN;  
    while(nFrameLen > 0)  
    {  
        //ÿ��֡�ĳ��Ȳ�Ҫ����short���ͣ����˾͵÷�Ƭ��ѭ���з���  
        nSize = (nFrameLen > PS_PES_PAYLOAD_SIZE) ? PS_PES_PAYLOAD_SIZE : nFrameLen;  
        // ���pesͷ  
        gb28181_make_pes_header(pBuff, stream_type ? 0xC0:0xE0, nSize, (pPacker->s64CurPts / 100), (pPacker->s64CurPts/300));  

        //��������rtpͷ����������  
        if( gb28181_send_rtp_pack(pBuff, nSize + PES_HDR_LEN, ((nSize == nFrameLen)?1:0), pPacker) != 0 )  
        {  
            printf("gb28181_send_pack failed!\n");  
            return -1;  
        }  
        //��Ƭ��ÿ�η��͵������ƶ�ָ�����  
        nFrameLen -= nSize;  
        //����Ҳֻ�ƶ�nSize,��Ϊ��while����ƶ���pesͷ���ȣ������������pesͷ����  
        pBuff     += nSize;  

    }  
    return 0;  
}  

#endif


/*** 
*@remark:   psͷ�ķ�װ,����ľ������ݵ���д�Ѿ�ռλ�����Բο���׼ 
*@param :   pData  [in] ���psͷ���ݵĵ�ַ 
*           s64Src [in] ʱ��� 
*@return:   0 success, others failed 
*/  
int gb28181_make_ps_header(char *pData, unsigned long long s64Scr)  
{  
    unsigned long long lScrExt = (s64Scr) % 100;      
    s64Scr = s64Scr / 100;  
    // �������100������sdpЭ�鷵�ص�video��Ƶ����90000��֡����25֡/s������ÿ�ε���������3600,  
    // ����ʵ����Ӧ�ø������Լ��������ʱ����������Ա�֤ʱ���������Ϊ3600���ɣ�  
    //������ﲻ�ԵĻ����Ϳ��ܵ��¿���������  
    bits_buffer_s   bitsBuffer;  
    bitsBuffer.i_size = PS_HDR_LEN;   
    bitsBuffer.i_data = 0;  
    bitsBuffer.i_mask = 0x80; // �����ƣ�10000000 ������Ϊ�˺����һ���ֽڵ�ÿһλ���в����������С�˿��ֽ��������  
    bitsBuffer.p_data = (unsigned char *)(pData);  
    memset(bitsBuffer.p_data, 0, PS_HDR_LEN);  
    bits_write(&bitsBuffer, 32, 0x000001BA);            /*start codes*/  
    bits_write(&bitsBuffer, 2,  1);                     /*marker bits '01b'*/  
    bits_write(&bitsBuffer, 3,  (s64Scr>>30)&0x07);     /*System clock [32..30]*/  
    bits_write(&bitsBuffer, 1,  1);                     /*marker bit*/  
    bits_write(&bitsBuffer, 15, (s64Scr>>15)&0x7FFF);   /*System clock [29..15]*/  
    bits_write(&bitsBuffer, 1,  1);                     /*marker bit*/  
    bits_write(&bitsBuffer, 15, s64Scr&0x7fff);         /*System clock [29..15]*/  
    bits_write(&bitsBuffer, 1,  1);                     /*marker bit*/  
    bits_write(&bitsBuffer, 9,  lScrExt&0x01ff);        /*System clock [14..0]*/  
    bits_write(&bitsBuffer, 1,  1);                     /*marker bit*/  
    bits_write(&bitsBuffer, 22, (255)&0x3fffff);        /*bit rate(n units of 50 bytes per second.)*/  
    bits_write(&bitsBuffer, 2,  3);                     /*marker bits '11'*/  
    bits_write(&bitsBuffer, 5,  0x1f);                  /*reserved(reserved for future use)*/  
    bits_write(&bitsBuffer, 3,  0);                     /*stuffing length*/  
    return 0;  
}  

/*** 
*@remark:   sysͷ�ķ�װ,����ľ������ݵ���д�Ѿ�ռλ�����Բο���׼ 
*@param :   pData  [in] ���psͷ���ݵĵ�ַ 
*@return:   0 success, others failed 
*/  
int gb28181_make_sys_header(char *pData)  
{  
    bits_buffer_s   bitsBuffer;  
    bitsBuffer.i_size = SYS_HDR_LEN;  
    bitsBuffer.i_data = 0;  
    bitsBuffer.i_mask = 0x80;  
    bitsBuffer.p_data = (unsigned char *)(pData);  
    memset(bitsBuffer.p_data, 0, SYS_HDR_LEN);  
    /*system header*/  
    bits_write( &bitsBuffer, 32, 0x000001BB);   /*start code*/  
    bits_write( &bitsBuffer, 16, SYS_HDR_LEN-6);/*header_length ��ʾ���ֽں���ĳ��ȣ���������ͷҲ�Ǵ���˼*/  
    bits_write( &bitsBuffer, 1,  1);            /*marker_bit*/  
    bits_write( &bitsBuffer, 22, 50000);        /*rate_bound*/  //ģ��3959
    bits_write( &bitsBuffer, 1,  1);            /*marker_bit*/  
    bits_write( &bitsBuffer, 6,  1);            /*audio_bound*/  //ģ��1
    bits_write( &bitsBuffer, 1,  0);            /*fixed_flag */  //ģ��0
    bits_write( &bitsBuffer, 1,  1);            /*CSPS_flag */   //1
    bits_write( &bitsBuffer, 1,  1);            /*system_audio_lock_flag*/  //1
    bits_write( &bitsBuffer, 1,  1);            /*system_video_lock_flag*/  //1
    bits_write( &bitsBuffer, 1,  1);            /*marker_bit*/  
    bits_write( &bitsBuffer, 5,  1);            /*video_bound*/  //0
    bits_write( &bitsBuffer, 1,  0);            /*dif from mpeg1*/  //0
    bits_write( &bitsBuffer, 7,  0x7F);         /*reserver*/  //0x17
    /*audio stream bound*/  
    bits_write( &bitsBuffer, 8,  0xC0);         /*stream_id*/  //0xfe
    bits_write( &bitsBuffer, 2,  3);            /*marker_bit */  //00
    bits_write( &bitsBuffer, 1,  0);            /*PSTD_buffer_bound_scale*/  //0
    bits_write( &bitsBuffer, 13, 512);          /*PSTD_buffer_size_bound*/  //
    /*video stream bound*/  
    bits_write( &bitsBuffer, 8,  0xE0);         /*stream_id*/  
    bits_write( &bitsBuffer, 2,  3);            /*marker_bit */  
    bits_write( &bitsBuffer, 1,  1);            /*PSTD_buffer_bound_scale*/  
    bits_write( &bitsBuffer, 13, 2048);         /*PSTD_buffer_size_bound*/  

    return 0;  
}  

/*** 
*@remark:   psmͷ�ķ�װ,����ľ������ݵ���д�Ѿ�ռλ�����Բο���׼ 
*@param :   pData  [in] ���psͷ���ݵĵ�ַ 
*@return:   0 success, others failed 
*/  
 
int gb28181_make_psm_header(char *pData)  
{  
    bits_buffer_s   bitsBuffer;  
    bitsBuffer.i_size = PSM_HDR_LEN;   
    bitsBuffer.i_data = 0;  
    bitsBuffer.i_mask = 0x80;  
    bitsBuffer.p_data = (unsigned char *)(pData);  
    memset(bitsBuffer.p_data, 0, PSM_HDR_LEN);  
    bits_write(&bitsBuffer, 24,0x000001);   /*start code*/  
    bits_write(&bitsBuffer, 8, 0xBC);       /*map stream id*/  
    bits_write(&bitsBuffer, 16,18);         /*program stream map length*/   
    bits_write(&bitsBuffer, 1, 1);          /*current next indicator */  
    bits_write(&bitsBuffer, 2, 3);          /*reserved*/  
    bits_write(&bitsBuffer, 5, 0);          /*program stream map version*/  
    bits_write(&bitsBuffer, 7, 0x7F);       /*reserved */  
    bits_write(&bitsBuffer, 1, 1);          /*marker bit */  
    bits_write(&bitsBuffer, 16,0);          /*programe stream info length*/  
    bits_write(&bitsBuffer, 16, 8);         /*elementary stream map length  is*/  
    /*audio*/  
    bits_write(&bitsBuffer, 8, 0x90);       /*stream_type*/  
    bits_write(&bitsBuffer, 8, 0xC0);       /*elementary_stream_id*/  
    bits_write(&bitsBuffer, 16, 0);         /*elementary_stream_info_length is*/  
    /*video*/  
    bits_write(&bitsBuffer, 8, 0x1B);       /*stream_type*/  
    bits_write(&bitsBuffer, 8, 0xE0);       /*elementary_stream_id*/  
    bits_write(&bitsBuffer, 16, 0);         /*elementary_stream_info_length */  
    /*crc (2e b9 0f 3d)*/  
    bits_write(&bitsBuffer, 8, 0x45);       /*crc (24~31) bits*/  
    bits_write(&bitsBuffer, 8, 0xBD);       /*crc (16~23) bits*/  
    bits_write(&bitsBuffer, 8, 0xDC);       /*crc (8~15) bits*/  
    bits_write(&bitsBuffer, 8, 0xF4);       /*crc (0~7) bits*/  

    return 0;
}  


/*** 
*@remark:   pesͷ�ķ�װ,����ľ������ݵ���д�Ѿ�ռλ�����Բο���׼ 
*@param :   pData      [in] ���psͷ���ݵĵ�ַ 
*           stream_id  [in] �������� 
*           paylaod_len[in] ���س��� 
*           pts        [in] ʱ��� 
*           dts        [in] 
*@return:   0 success, others failed 
*/  
int gb28181_make_pes_header(char *pData, int stream_id, int payload_len, unsigned long long pts, unsigned long long dts)  
{  
    bits_buffer_s   bitsBuffer;  
    bitsBuffer.i_size = PES_HDR_LEN;  
    bitsBuffer.i_data = 0;  
    bitsBuffer.i_mask = 0x80;  
    bitsBuffer.p_data = (unsigned char *)(pData);  
    memset(bitsBuffer.p_data, 0, PES_HDR_LEN);  
    /*system header*/  
    bits_write( &bitsBuffer, 24,0x000001);  /*start code*/  
    bits_write( &bitsBuffer, 8, (stream_id));   /*streamID*/  
    bits_write( &bitsBuffer, 16,(payload_len)+13);  /*packet_len*/ //ָ��pes���������ݳ��Ⱥ͸��ֽں�ĳ��Ⱥ�  
    bits_write( &bitsBuffer, 2, 2 );        /*'10'*/  
    bits_write( &bitsBuffer, 2, 0 );        /*scrambling_control*/  
    bits_write( &bitsBuffer, 1, 0 );        /*priority*/  
    bits_write( &bitsBuffer, 1, 0 );        /*data_alignment_indicator*/  //1
    bits_write( &bitsBuffer, 1, 0 );        /*copyright*/  
    bits_write( &bitsBuffer, 1, 0 );        /*original_or_copy*/  
    bits_write( &bitsBuffer, 1, 1 );        /*PTS_flag*/  
    bits_write( &bitsBuffer, 1, 1 );        /*DTS_flag*/  
    bits_write( &bitsBuffer, 1, 0 );        /*ESCR_flag*/  
    bits_write( &bitsBuffer, 1, 0 );        /*ES_rate_flag*/  
    bits_write( &bitsBuffer, 1, 0 );        /*DSM_trick_mode_flag*/  
    bits_write( &bitsBuffer, 1, 0 );        /*additional_copy_info_flag*/  
    bits_write( &bitsBuffer, 1, 0 );        /*PES_CRC_flag*/  
    bits_write( &bitsBuffer, 1, 0 );        /*PES_extension_flag*/  
    bits_write( &bitsBuffer, 8, 10);        /*header_data_length*/   
    // ָ�������� PES ��������еĿ�ѡ�ֶκ��κ�����ֽ���ռ�õ����ֽ��������ֶ�֮ǰ  
    //���ֽ�ָ�������޿�ѡ�ֶΡ�  

    /*PTS,DTS*/   
    bits_write( &bitsBuffer, 4, 3 );                    /*'0011'*/  
    bits_write( &bitsBuffer, 3, ((pts)>>30)&0x07 );     /*PTS[32..30]*/  
    bits_write( &bitsBuffer, 1, 1 );  
    bits_write( &bitsBuffer, 15,((pts)>>15)&0x7FFF);    /*PTS[29..15]*/  
    bits_write( &bitsBuffer, 1, 1 );  
    bits_write( &bitsBuffer, 15,(pts)&0x7FFF);          /*PTS[14..0]*/  
    bits_write( &bitsBuffer, 1, 1 );  
    bits_write( &bitsBuffer, 4, 1 );                    /*'0001'*/  
    bits_write( &bitsBuffer, 3, ((dts)>>30)&0x07 );     /*DTS[32..30]*/  
    bits_write( &bitsBuffer, 1, 1 );  
    bits_write( &bitsBuffer, 15,((dts)>>15)&0x7FFF);    /*DTS[29..15]*/  
    bits_write( &bitsBuffer, 1, 1 );  
    bits_write( &bitsBuffer, 15,(dts)&0x7FFF);          /*DTS[14..0]*/  
    bits_write( &bitsBuffer, 1, 1 );  

    return 0;  
}  









