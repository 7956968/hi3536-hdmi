#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_venc.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vda.h"
#include "hi_comm_region.h"
#include "hi_comm_adec.h"
#include "hi_comm_aenc.h"
#include "hi_comm_ai.h"
#include "hi_comm_ao.h"
#include "hi_defines.h"

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vpss.h"
#include "mpi_vdec.h"
#include "mpi_vda.h"
#include "mpi_region.h"
#include "mpi_adec.h"
#include "mpi_aenc.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_hdmi.h"

#include "video_hdmi.h"

void * generateFrame(void *pArgs)
{
	char *apcPathTable[] = {
		"/mnt/1080p0.h264",
		"/mnt/1080p1.h264",
		"/mnt/1080p2.h264",
		"/mnt/1080p3.h264",
		"/mnt/1080p4.h264",
		"/mnt/1080p5.h264"
	};
	
	VDEC_STREAM_S *pstStream = (VDEC_STREAM_S *)pArgs;
	FILE *apstFile[6] = {NULL};
	VDEC_STREAM_S astStream[6];
	HI_BOOL abFindStart[6];
	HI_BOOL abFindEnd[6];
	int iRet;
	int iCnt;
	int aiStart[6] = {0};
	int aiUsedBytes[6] = {0};
	int aiReadLen[6] = {0};
//	unsigned long long int adwPts[6] = {0};
	unsigned long long int dwPts = 0;
	int iChnnCnt;
	unsigned int dwInterval = 30000;
	SIZE_S stSize;
    HI_S32 s32MinBufSize;

	iRet = Video_getPicSize(VIDEO_ENCODING_MODE_PAL, PIC_HD1080, &stSize);
    if (HI_SUCCESS !=iRet)
    {
        printf("get picture size failed!\n");
        return HI_FAILURE;
    }

    s32MinBufSize = (stSize.u32Width * stSize.u32Height * 3) >> 1;

	iRet = HI_MPI_SYS_GetCurPts(&dwPts);
	if (HI_SUCCESS != iRet)
	{
		printf("HI_MPI_SYS_GetCurPts error with %x\n", iRet);
		return (void *)-1;
	}

	dwPts += 10000000;

	for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
	{
		apstFile[iChnnCnt] = fopen(apcPathTable[iChnnCnt], "rb");
		if(apstFile[iChnnCnt] == NULL)
		{
			printf("Can't open file %s in send stream thread\n", apcPathTable[iChnnCnt]);
			return (void *)HI_FAILURE;
		} 
	}
	fflush(stdout);
	
	while(1)
	{		
		for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
		{
			abFindStart[iChnnCnt] = HI_FALSE;  
			abFindEnd[iChnnCnt]   = HI_FALSE;

			iRet = fseek(apstFile[iChnnCnt], aiUsedBytes[iChnnCnt], SEEK_SET);
			if (iRet)
			{
				printf("fseek error: %x\n", iRet);
				return (void *)HI_FAILURE;
			}

			aiReadLen[iChnnCnt] = fread(pstStream[iChnnCnt].pu8Addr, 1, s32MinBufSize, apstFile[iChnnCnt]);
			if (aiReadLen[iChnnCnt] == 0)
			{
				aiUsedBytes[iChnnCnt] = 0;
				iRet = fseek(apstFile[iChnnCnt], aiUsedBytes[iChnnCnt], SEEK_SET);
				if (iRet)
				{
					printf("fseek error: %x\n", iRet);
					return (void *)HI_FAILURE;
				}

				aiReadLen[iChnnCnt] = fread(pstStream[iChnnCnt].pu8Addr, 1, s32MinBufSize, apstFile[iChnnCnt]);
			}
					
			for (iCnt = 0; iCnt < aiReadLen[iChnnCnt]-8; iCnt++)
			{
				int tmp = pstStream[iChnnCnt].pu8Addr[iCnt+3] & 0x1F;
				if (  pstStream[iChnnCnt].pu8Addr[iCnt] == 0 && 
						pstStream[iChnnCnt].pu8Addr[iCnt+1] == 0 && 
						pstStream[iChnnCnt].pu8Addr[iCnt+2] == 1 && 
						(
							((tmp == 5 || tmp == 1) && ((pstStream[iChnnCnt].pu8Addr[iCnt+4]&0x80) == 0x80)) ||
							(tmp == 20 && (pstStream[iChnnCnt].pu8Addr[iCnt+7]&0x80) == 0x80)
						)
					) 		   
				{
					abFindStart[iChnnCnt] = HI_TRUE;
					iCnt += 8;
					break;
				}
			}

			for (; iCnt < aiReadLen[iChnnCnt]-8; iCnt++)
			{
				int tmp = pstStream[iChnnCnt].pu8Addr[iCnt+3] & 0x1F;
				if (  pstStream[iChnnCnt].pu8Addr[iCnt] == 0 && 
						pstStream[iChnnCnt].pu8Addr[iCnt+1] == 0 && 
						pstStream[iChnnCnt].pu8Addr[iCnt+2] == 1 && 
							(
									tmp == 15 || tmp == 7 || tmp == 8 || tmp == 6 || 
									((tmp == 5 || tmp == 1) && ((pstStream[iChnnCnt].pu8Addr[iCnt+4]&0x80) == 0x80)) ||
									(tmp == 20 && (pstStream[iChnnCnt].pu8Addr[iCnt+7]&0x80) == 0x80)
								)
					) 				  
				{				   
					abFindEnd[iChnnCnt] = HI_TRUE;
					break;
				}
			}

			if(iCnt > 0) aiReadLen[iChnnCnt] = iCnt;
			if (abFindStart[iChnnCnt] == HI_FALSE)
			{
				printf("Chn %d can not find start code!s32ReadLen %d, s32UsedBytes %d. \n", 
											iChnnCnt, aiReadLen[iChnnCnt], aiUsedBytes[iChnnCnt]);
			}
			else if (abFindEnd[iChnnCnt] == HI_FALSE)
			{
				aiReadLen[iChnnCnt] = iCnt + 8;
			}
		}

		for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
		{
			pstStream[iChnnCnt].u64PTS	= dwPts;
			pstStream[iChnnCnt].pu8Addr = pstStream[iChnnCnt].pu8Addr + aiStart[iChnnCnt];
			pstStream[iChnnCnt].u32Len	= aiReadLen[iChnnCnt]; 
	
			aiUsedBytes[iChnnCnt] = aiUsedBytes[iChnnCnt] + aiReadLen[iChnnCnt] + aiStart[iChnnCnt]; 		   
			dwPts += dwInterval;			
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int iRet;
	char cInput;
	HDMI_HOTPLUG_EVENT_S stHdmiHotplugEvent;
//	VDEC_STREAM_PARAM_S astVdecStreamParam[6];
	VDEC_STREAM_S astStream[6];
	pthread_t VdecThread;

	memset(&stHdmiHotplugEvent, 0, sizeof(HDMI_HOTPLUG_EVENT_S));
	stHdmiHotplugEvent.enPicSize = PIC_HD1080;

	/******************************************
    mpp system init. 
    ******************************************/
    iRet = Video_SysInit(stHdmiHotplugEvent.enPicSize);
    if (HI_SUCCESS != iRet)
    {
        printf("system init failed with %d!\n", iRet);
		Video_SysExit();
    }

	Video_HdmiCreate(&stHdmiHotplugEvent);

	Video_StreamParamaConf(&stHdmiHotplugEvent, astStream);

//	generateFrame(astVdecStreamParam);
	iRet = pthread_create(&VdecThread, NULL, generateFrame, (void *)astStream);
	if(iRet)
	{
		printf("pthread_create error: %x\n", iRet);
		return HI_FAILURE;
	}

	Video_VdecStartSendStream(astVdecStreamParam);

	while(1)
    {	
        printf("press 'q' to exit this sample.\n");        
        cInput = getchar();
        if (10 == cInput)
        {
            continue;
        }
        getchar();        
        if ('q' == cInput)
        {
            break;
        }
        else
        {
            printf("the input is invaild! please try again.\n");
            continue;
        }
    }

//	Video_VdecStopSendStream(astVdecStreamParam);

	for (int iCnt = 0; iCnt < 6; iCnt++)
	{
		if (astStream[iCnt].pu8Addr != NULL)
		{
			free(astStream[iCnt].pu8Addr);
		}
	}

	if (stHdmiHotplugEvent.bHdmiConnected)
	{
		Video_Exit();
	}
	
	Video_HdmiDestroy(&stHdmiHotplugEvent);

    Video_SysExit();
	
	return 0;
}

