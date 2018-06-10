/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILED                           */
/*-----------------------------------------------------------------------*/
#include "rvrtspsample.h"
#include "rvtypes.h"
#include <stdio.h>
#include "rvsdp.h"

RVAPI
void RVCALLCONV PrintCB(IN char* str)
{
	printf(str);
}


/**************************************************************************
 * main
 * ------------------------------------------------------------------------
 * General: main function, calls the testing functions.
 *
 * Return Value:	0 on completion.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	argc	- not used.
 *				argv	- not used.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN)
int mainSample(void)
#else
int main(int argc, char** argv)
#endif /* (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN) */
{
#if 0
char buffer[2048]="v=0\n\
o=- 1109162014219182 0 IN IP4 0.0.0.0\n\
s=HIK Media Server (null)\n\
i=HIK Media Server Session Description : standard\n\
e=NONE\n\
c=IN c=IN IP4 0.0.0.0\n\
t=0 0\n\
a=control:*\n\
a=range:npt=now-\n\
m=video 0 RTP/AVP 96\n\
i=Video Media\n\
a=rtpmap:96 H264/90000\n\
a=fmtp:96 profile-level-id=4D0014;packetization-mode=0\n\
a=control:trackID=video\n\
m=audio 0 RTP/AVP 98\n\
i=Audio Media\n\
a=rtpmap:98 G7221/16000\n\
a=control:trackID=audio\n\
a=Media_header:MEDIAINFO=494D4B48010100000200000121720110007D0000007D000000000000000000000000000000000000;\n\a=appversion:1.0\n\
";
	RvSdpMsg *pMsg = (RvSdpMsg*)malloc(sizeof(RvSdpMsg));
	RvAlloc *pSdpAlloc = (RvAlloc *)malloc(sizeof(RvAlloc));
	RvSdpParseStatus eStat;
	RvSdpMediaDescr *media;
	int pBufSize=strlen(buffer);
	int mediaSize;
	int index,i;
	int num;
	RvStatus status;
	RvSdpAttribute * att;
	int num_rtp_map=0;
	RvSdpRtpMap*	rtpmap;

	

	status=RvSdpMgrConstruct();
	if(status !=RV_OK)
		printf("RvSdpMgrConstruct %d\n",status);
	pMsg=rvSdpMsgConstruct(pMsg);
	if(pMsg ==NULL)
		printf("rvSdpMsgConstruct \n");
	pMsg=rvSdpMsgConstructParse(pMsg,buffer, &pBufSize,&eStat);
	if(pMsg ==NULL)
		printf("rvSdpMsgConstruct \n");

	 mediaSize = rvSdpMsgGetNumOfMediaDescr(pMsg);
	for (index=0;index< mediaSize; ++index)
	{
		media = rvSdpMsgGetMediaDescr(pMsg,index);
		printf("---------------------------------------------------\n");
		printf("rvSdpMediaDescrGetNumOfFormats=%d\n",rvSdpMediaDescrGetNumOfFormats(media));
		printf("rvSdpMediaDescrGetFormat=%s\n",rvSdpMediaDescrGetFormat(media,0));
		printf("rvSdpMediaDescrGetNumOfPayloads=%d\n",rvSdpMediaDescrGetNumOfPayloads(media));
		printf("rvSdpMediaDescrGetPayload=%d\n",rvSdpMediaDescrGetPayload(media,0));
		printf("rvSdpMediaDescrGetMediaTypeStr=%s\n",rvSdpMediaDescrGetMediaTypeStr(media));
		printf("rvSdpMediaDescrGetProtocolStr=%s\n",rvSdpMediaDescrGetProtocolStr(media));
		printf("rvSdpMediaDescrGetPort=%d\n",rvSdpMediaDescrGetPort(media));
		
		//printf("rvSdpMediaDescrGetFrameRate=%s\n",rvSdpMediaDescrGetFrameRate(media));

		num_rtp_map=rvSdpMediaDescrGetNumOfRtpMap(media);
		for(i=0;i<num_rtp_map;i++)
		{
			rtpmap=rvSdpMediaDescrGetRtpMap(media,i);
			printf("----------------RTP MAP------------------\n");
			printf("rtpmap->iPayload=%d\n",rtpmap->iPayload);
			printf("rtpmap->iEncName=%s\n",rtpmap->iEncName);
			printf("rtpmap->iClockRate=%d\n",rtpmap->iClockRate);
			printf("rtpmap->iEncParameters=%s\n",rtpmap->iEncParameters);
		}

		num =rvSdpMediaDescrGetNumOfAttr2(media);
		for(i=0;i<num;i++)
		{
			att=rvSdpMediaDescrGetAttribute2(media,i);
			printf("rvSdpMediaDescrGetAttribute %s=%s\n",att->iAttrName,att->iAttrValue);
		}
	// Do something with media
	}
#endif

	initTest(PrintCB);

	while (testMainLoop(50) )
	{
	}

	RV_UNUSED_ARG(argv);
	RV_UNUSED_ARG(argc);
    return 0;
}



