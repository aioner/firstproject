
#ifndef GB28181_PS_H
#define GB28181_PS_H


#define PS_HDR_LEN  14  
#define SYS_HDR_LEN 18  
#define PSM_HDR_LEN 24  
#define PES_HDR_LEN 19  
#define RTP_HDR_LEN 12
#define PS_I_ALL_LEN (PS_HDR_LEN+SYS_HDR_LEN+PSM_HDR_LEN+PES_HDR_LEN)
#define PS_P_ALL_LEN (PS_HDR_LEN+PES_HDR_LEN)
#define PS_PES_PAYLOAD_SIZE 65000


enum H264_nalu_type
{
    NALU_Unspecified = 0,
    NALU_NON_IDR = 1,
    NALU_DataPartitionA,
    NALU_DataPartitionB,
    NALU_DataPartitionC,
    NALU_IDR = 5,
    NALU_SEI = 6,
    NALU_SPS = 7,
    NALU_PPS = 8,
    NALU_Delimiter = 9,
    NALU_SequenceEnd,
    NALU_StreamEnd,
    NALU_FillerData,
    NALU_SPSExetrnsion = 13,
    NALU_Prefix,
    NALU_Subset_SPS
};

typedef struct bits_buffer
{
    unsigned char i_size;
    unsigned char i_data;
    unsigned char i_mask;
    unsigned char *p_data;

}bits_buffer_s;

/*** 
*@remark:  讲传入的数据按地位一个一个的压入数据 
*@param :  buffer   [in]  压入数据的buffer 
*          count    [in]  需要压入数据占的位数 
*          bits     [in]  压入的数值 
*/  
inline void bits_write(bits_buffer_s *buffer, int count, unsigned long bits)  
{  
    bits_buffer_s *p_buffer = (buffer);  
    int i_count = (count);  
    unsigned long i_bits = (bits);  
    while( i_count > 0 )  
    {  
        i_count--;  
        if( ( i_bits >> i_count )&0x01 ) 
        {  
            p_buffer->p_data[p_buffer->i_data] |= p_buffer->i_mask;  
        }  
        else  
        { 
            p_buffer->p_data[p_buffer->i_data] &= ~p_buffer->i_mask;  
        }  
        p_buffer->i_mask >>= 1;         /*操作完一个字节第一位后，操作第二位*/  
        if( p_buffer->i_mask == 0 )     /*循环完一个字节的8位后，重新开始下一位*/  
        {  
            p_buffer->i_data++;  
            p_buffer->i_mask = 0x80;  
        }  
    }  
}

bool is_Iframe(unsigned char nal_type);

bool is_NALU_SPS(unsigned char nal_type);

bool is_NALU_PPS(unsigned char nal_type);

bool is_NALU_SEI(unsigned char nal_type);

bool is_NALU_IDR(unsigned char nal_type);

/*** 
*@remark:   ps头的封装,里面的具体数据的填写已经占位，可以参考标准 
*@param :   pData  [in] 填充ps头数据的地址 
*           s64Src [in] 时间戳 
*@return:   0 success, others failed 
*/  
int gb28181_make_ps_header(char *pData, unsigned long long s64Scr);

/*** 
*@remark:   sys头的封装,里面的具体数据的填写已经占位，可以参考标准 
*@param :   pData  [in] 填充ps头数据的地址 
*@return:   0 success, others failed 
*/  
int gb28181_make_sys_header(char *pData);

/*** 
*@remark:   psm头的封装,里面的具体数据的填写已经占位，可以参考标准 
*@param :   pData  [in] 填充ps头数据的地址 
*@return:   0 success, others failed 
*/  
int gb28181_make_psm_header(char *pData);

/*** 
*@remark:   pes头的封装,里面的具体数据的填写已经占位，可以参考标准 
*@param :   pData      [in] 填充ps头数据的地址 
*           stream_id  [in] 码流类型 
*           paylaod_len[in] 负载长度 
*           pts        [in] 时间戳 
*           dts        [in] 
*@return:   0 success, others failed 
*/  
int gb28181_make_pes_header(char *pData, int stream_id, int payload_len, unsigned long long pts, unsigned long long dts);




class mp_28181_ps
{
public:
    mp_28181_ps();
    ~mp_28181_ps();
private:
    int a;

};

#endif
