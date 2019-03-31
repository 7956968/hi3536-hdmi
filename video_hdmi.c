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

pthread_t g_VdecThread;

/**
 * Video_getPicSize:
 * Get picture size(w*h), according to @enVideoNorm and @enPicSize
 */
int Video_getPicSize(VIDEO_NORM_E enVideoNorm, PIC_SIZE_E enPicSize, SIZE_S *pstSize)
{
    switch(enPicSize)
    {
        case PIC_QCIF:
            pstSize->u32Width = D1_WIDTH / 4;
            pstSize->u32Height = (VIDEO_ENCODING_MODE_PAL == enVideoNorm) ? 144 : 120;
            break;
        case PIC_CIF:
            pstSize->u32Width = D1_WIDTH / 2;
            pstSize->u32Height = (VIDEO_ENCODING_MODE_PAL == enVideoNorm) ? 288 : 240;
            break;
        case PIC_D1:
            pstSize->u32Width = D1_WIDTH;
            pstSize->u32Height = (VIDEO_ENCODING_MODE_PAL == enVideoNorm) ? 576 : 480;
            break;
        case PIC_960H:
            pstSize->u32Width = 960;
            pstSize->u32Height = (VIDEO_ENCODING_MODE_PAL == enVideoNorm) ? 576 : 480;
            break;			
        case PIC_2CIF:
            pstSize->u32Width = D1_WIDTH / 2;
            pstSize->u32Height = (VIDEO_ENCODING_MODE_PAL == enVideoNorm) ? 576 : 480;
            break;
        case PIC_QVGA:    /* 320 * 240 */
            pstSize->u32Width = 320;
            pstSize->u32Height = 240;
            break;
        case PIC_VGA:     /* 640 * 480 */
            pstSize->u32Width = 640;
            pstSize->u32Height = 480;
            break;
        case PIC_XGA:     /* 1024 * 768 */
            pstSize->u32Width = 1024;
            pstSize->u32Height = 768;
            break;
        case PIC_SXGA:    /* 1400 * 1050 */
            pstSize->u32Width = 1400;
            pstSize->u32Height = 1050;
            break;
        case PIC_UXGA:    /* 1600 * 1200 */
            pstSize->u32Width = 1600;
            pstSize->u32Height = 1200;
            break;
        case PIC_QXGA:    /* 2048 * 1536 */
            pstSize->u32Width = 2048;
            pstSize->u32Height = 1536;
            break;
        case PIC_WVGA:    /* 854 * 480 */
            pstSize->u32Width = 854;
            pstSize->u32Height = 480;
            break;
        case PIC_WSXGA:   /* 1680 * 1050 */
            pstSize->u32Width = 1680;
            pstSize->u32Height = 1050;
            break;
        case PIC_WUXGA:   /* 1920 * 1200 */
            pstSize->u32Width = 1920;
            pstSize->u32Height = 1200;
            break;
        case PIC_WQXGA:   /* 2560 * 1600 */
            pstSize->u32Width = 2560;
            pstSize->u32Height = 1600;
            break;
        case PIC_HD720:   /* 1280 * 720 */
            pstSize->u32Width = 1280;
            pstSize->u32Height = 720;
            break;
        case PIC_HD1080:  /* 1920 * 1080 */
            pstSize->u32Width = 1920;
            pstSize->u32Height = 1080;
            break;
		case PIC_UHD4K:  /* 3840 * 2160 */
            pstSize->u32Width = 3840;
            pstSize->u32Height = 2160;
            break;
        default:
            return HI_FAILURE;
    }
    return HI_SUCCESS;
}

/**
 * Video_calcPicBlkSize: 
 * Calculate VB Block size of picture.
 */
int Video_calcPicBlkSize(VIDEO_NORM_E enVideoNorm, PIC_SIZE_E enPicSize, PIXEL_FORMAT_E enPixFmt, int iAlignWidth)
{
	int iRet = HI_FAILURE;
    SIZE_S stSize;
    unsigned int wWidth = 0;
    unsigned int wHeight = 0;
    unsigned int wBlkSize = 0;

    iRet = Video_getPicSize(enVideoNorm, enPicSize, &stSize);
    if (HI_SUCCESS != iRet)
    {
        printf("get picture size[%d] failed!\n", enPicSize);
        return HI_FAILURE;
    }

    if (PIXEL_FORMAT_YUV_SEMIPLANAR_422 != enPixFmt && PIXEL_FORMAT_YUV_SEMIPLANAR_420 != enPixFmt)
    {
        printf("pixel format[%d] input failed!\n", enPixFmt);
        return HI_FAILURE;
    }

    if (16 != iAlignWidth && 32 != iAlignWidth && 64 != iAlignWidth)
    {
        printf("system align width[%d] input failed!\n",\
               iAlignWidth);
        return HI_FAILURE;
    }
    if (704 == stSize.u32Width)
    {
        stSize.u32Width = 720;
    }    

    wWidth  = CEILING_2_POWER(stSize.u32Width, iAlignWidth);
    wHeight = CEILING_2_POWER(stSize.u32Height, iAlignWidth);
    
    if (PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixFmt)
    {
        wBlkSize = wWidth * wHeight * 2;
    }
    else
    {
        wBlkSize = wWidth * wHeight * 3 / 2;
    }
    
    return wBlkSize;
}

/**
 * Video_SysInit: 
 * VB init and MPI system init
 */
int Video_SysInit(PIC_SIZE_E enPicSize)
{
    VB_CONF_S stVbConf;
    MPP_SYS_CONF_S stSysConf = {0};
    unsigned int wBlkSize;
    int iRet = HI_FAILURE;
    int iCnt;

    memset(&stVbConf,0,sizeof(VB_CONF_S));     
    wBlkSize = Video_calcPicBlkSize(VIDEO_ENCODING_MODE_PAL, enPicSize, PIXEL_FORMAT_YUV_SEMIPLANAR_420, VIDEO_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    stVbConf.astCommPool[0].u32BlkSize = wBlkSize;
    stVbConf.astCommPool[0].u32BlkCnt  = 20;
    stVbConf.astCommPool[1].u32BlkSize = 3840*2160*2;
    stVbConf.astCommPool[1].u32BlkCnt  = 5;

    HI_MPI_SYS_Exit();
    
    for(iCnt = 0; iCnt < VB_MAX_USER; iCnt++)
    {
         HI_MPI_VB_ExitModCommPool(iCnt);
    }
    for(iCnt = 0; iCnt < VB_MAX_POOLS; iCnt++)
    {
         HI_MPI_VB_DestroyPool(iCnt);
    }
    HI_MPI_VB_Exit();

    iRet = HI_MPI_VB_SetConf(&stVbConf);
    if (HI_SUCCESS != iRet)
    {
        printf("HI_MPI_VB_SetConf failed!\n");
        return HI_FAILURE;
    }

    iRet = HI_MPI_VB_Init();
    if (HI_SUCCESS != iRet)
    {
        printf("HI_MPI_VB_Init failed!\n");
        return HI_FAILURE;
    }

    stSysConf.u32AlignWidth = VIDEO_ALIGN_WIDTH;
    iRet = HI_MPI_SYS_SetConf(&stSysConf);
    if (HI_SUCCESS != iRet)
    {
        printf("HI_MPI_SYS_SetConf failed\n");
        return HI_FAILURE;
    }

    iRet = HI_MPI_SYS_Init();
    if (HI_SUCCESS != iRet)
    {
        printf("HI_MPI_SYS_Init failed!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/**
 * Video_SysExit: 
 * VB exit and MPI system exit
 */
void Video_SysExit(void)
{

    int iCnt;

    HI_MPI_SYS_Exit();
    for(iCnt = 0; iCnt < VB_MAX_USER; iCnt++)
    {
         HI_MPI_VB_ExitModCommPool(iCnt);
    }
 
    for(iCnt = 0; iCnt < VB_MAX_POOLS; iCnt++)
    {
         HI_MPI_VB_DestroyPool(iCnt);
    }	

    HI_MPI_VB_Exit();

	return;
}

#if 0
/**
 * Video_VdecSendFrame:
 *
 */
static void * Video_VdecSendFrame(void *pArgs)
{
	char *apcPathTable[] = {
		"/mnt/1080p0.h264",
		"/mnt/1080p1.h264",
		"/mnt/1080p2.h264",
		"/mnt/1080p3.h264",
		"/mnt/1080p4.h264",
		"/mnt/1080p5.h264"
	};

	VDEC_STREAM_PARAM_S *pstVdecStreamParam = (VDEC_STREAM_PARAM_S *)pArgs;
	FILE *apstFile[6] = {NULL};
	VDEC_STREAM_S astStream[6];
	HI_BOOL abFindStart[6];
	HI_BOOL abFindEnd[6];
	int iRet;
	int iCnt;
	int aiStart[6] = {0};
	int aiUsedBytes[6] = {0};
	int aiReadLen[6] = {0};
	unsigned long long int adwPts[6] = {0};
	unsigned long long int dwPts = 0;
	int iChnnCnt;
	unsigned int dwInterval = 30000;

	iRet = HI_MPI_SYS_GetCurPts(&dwPts);
	if (HI_SUCCESS != iRet)
	{
		printf("HI_MPI_SYS_GetCurPts error with %x\n", iRet);
		return (void *)(-1);
	}

	for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
	{
		apstFile[iChnnCnt] = fopen(apcPathTable[iChnnCnt], "rb");
		if(apstFile[iChnnCnt] == NULL)
		{
			printf("Can't open file %s in send stream thread\n", apcPathTable[iChnnCnt]);
			return (void *)(HI_FAILURE);
		} 

		//adwPts[iChnnCnt] = pstVdecThreadParam[iChnnCnt].u64PtsInit;
		adwPts[iChnnCnt] = dwPts + 10000000;
	}
	fflush(stdout);
	
	while (1)
	{
		if (pstVdecStreamParam[0].eCtrlSinal == VDEC_CTRL_STOP)
		{
			printf("Stop sending frame ......\n");
			break;
		}
		else if(pstVdecStreamParam[0].eCtrlSinal == VDEC_CTRL_PAUSE)
		{
			printf("Pause sending frame ......\n");
		   	continue;
		}

		if ( (pstVdecStreamParam[0].s32StreamMode == VIDEO_MODE_FRAME) && (pstVdecStreamParam[0].enType == PT_H264) )
		{
			for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
			{
				abFindStart[iChnnCnt] = HI_FALSE;  
				abFindEnd[iChnnCnt]   = HI_FALSE;

				iRet = fseek(apstFile[iChnnCnt], aiUsedBytes[iChnnCnt], SEEK_SET);
				if (iRet)
				{
					printf("fseek error: %x\n", iRet);
					return (void *)(HI_FAILURE);
				}

				aiReadLen[iChnnCnt] = fread(pstVdecStreamParam[iChnnCnt].pu8Buffer, 1, pstVdecStreamParam[iChnnCnt].s32MinBufSize, apstFile[iChnnCnt]);
				if (aiReadLen[iChnnCnt] == 0)
				{
					if (1)
					{
						aiUsedBytes[iChnnCnt] = 0;
						iRet = fseek(apstFile[iChnnCnt], aiUsedBytes[iChnnCnt], SEEK_SET);
						if (iRet)
						{
							printf("fseek error: %x\n", iRet);
							return (void *)(HI_FAILURE);
						}

						aiReadLen[iChnnCnt] = fread(pstVdecStreamParam[iChnnCnt].pu8Buffer, 1, pstVdecStreamParam[iChnnCnt].s32MinBufSize, apstFile[iChnnCnt]);
					}
					else
					{
						pstVdecStreamParam[iChnnCnt].eCtrlSinal == VDEC_CTRL_STOP;
						break;
					}
				}
						
				for (iCnt = 0; iCnt < aiReadLen[iChnnCnt]-8; iCnt++)
				{
					int tmp = pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt+3] & 0x1F;
					if (  pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt] == 0 && 
							pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt+1] == 0 && 
							pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt+2] == 1 && 
							(
								((tmp == 5 || tmp == 1) && ((pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt+4]&0x80) == 0x80)) ||
								(tmp == 20 && (pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt+7]&0x80) == 0x80)
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
					int tmp = pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt+3] & 0x1F;
					if (  pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt] == 0 && 
							pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt+1] == 0 && 
							pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt+2] == 1 && 
								(
										tmp == 15 || tmp == 7 || tmp == 8 || tmp == 6 || 
										((tmp == 5 || tmp == 1) && ((pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt+4]&0x80) == 0x80)) ||
										(tmp == 20 && (pstVdecStreamParam[iChnnCnt].pu8Buffer[iCnt+7]&0x80) == 0x80)
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
												pstVdecStreamParam[iChnnCnt].s32ChnId, aiReadLen[iChnnCnt], aiUsedBytes[iChnnCnt]);
				}
				else if (abFindEnd[iChnnCnt] == HI_FALSE)
				{
					aiReadLen[iChnnCnt] = iCnt + 8;
				}
			}
		}
	   
	   else
	   {
			for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
		   	{
				iRet = fseek(apstFile[iChnnCnt], aiUsedBytes[iChnnCnt], SEEK_SET);
				if (iRet)
				{
					printf("fseek error: %x\n", iRet);
					return (void *)(HI_FAILURE);
				}

				aiReadLen[iChnnCnt] = fread(pstVdecStreamParam[iChnnCnt].pu8Buffer, 1, pstVdecStreamParam[iChnnCnt].s32MinBufSize, apstFile[iChnnCnt]);
				if (aiReadLen[iChnnCnt] == 0)
				{
					if (1)
					{
						aiUsedBytes[iChnnCnt] = 0;
						iRet = fseek(apstFile[iChnnCnt], aiUsedBytes[iChnnCnt], SEEK_SET);
						if (iRet)
						{
							printf("fseek error: %x\n", iRet);
							return (void *)(HI_FAILURE);
						}

						aiReadLen[iChnnCnt] = fread(pstVdecStreamParam[iChnnCnt].pu8Buffer, 1, pstVdecStreamParam[iChnnCnt].s32MinBufSize, apstFile[iChnnCnt]);
					}
					else
					{
						pstVdecStreamParam[iChnnCnt].eCtrlSinal == VDEC_CTRL_STOP;
						break;
					}
				}
			}
	   }

		for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
		{
			astStream[iChnnCnt].u64PTS		  	= adwPts[iChnnCnt];
			astStream[iChnnCnt].pu8Addr  	  	= pstVdecStreamParam[iChnnCnt].pu8Buffer + aiStart[iChnnCnt];
			astStream[iChnnCnt].u32Len		  	= aiReadLen[iChnnCnt]; 
			astStream[iChnnCnt].bEndOfFrame  	= (pstVdecStreamParam[iChnnCnt].s32StreamMode == VIDEO_MODE_FRAME) ? HI_TRUE : HI_FALSE;
			astStream[iChnnCnt].bEndOfStream 	= HI_FALSE;	   
			iRet = HI_MPI_VDEC_SendStream(pstVdecStreamParam[iChnnCnt].s32ChnId, &astStream[iChnnCnt], pstVdecStreamParam[iChnnCnt].s32MilliSec);
			if (HI_SUCCESS != iRet)
			{
				printf("Channel%d: HI_MPI_VDEC_SendStream error with %x\n", pstVdecStreamParam[iChnnCnt].s32ChnId, iRet);
				usleep(100);
			}
			else
			{
					aiUsedBytes[iChnnCnt] = aiUsedBytes[iChnnCnt] + aiReadLen[iChnnCnt] + aiStart[iChnnCnt]; 		   
					adwPts[iChnnCnt] += dwInterval;			
			}
//			usleep(1);
		}
	}

	/* send the flag of stream end */
	for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
	{
		memset(&astStream[iChnnCnt], 0, sizeof(VDEC_STREAM_S) );
		astStream[iChnnCnt].bEndOfStream = HI_TRUE;

		iRet = HI_MPI_VDEC_SendStream(pstVdecStreamParam[iChnnCnt].s32ChnId, &astStream[iChnnCnt], pstVdecStreamParam[iChnnCnt].s32MilliSec);
		if (HI_SUCCESS != iRet)
		{
			printf("Channel%d: HI_MPI_VDEC_SendStream error with %x\n", pstVdecStreamParam[iChnnCnt].s32ChnId, iRet);
			usleep(100);
		}

		fflush(stdout);

		fclose(apstFile[iChnnCnt]);
	}

	printf("Thread over!\n");

	return (void *)HI_SUCCESS;
}

/**
 * Video_VdecSendFrame:
 *
 */
static void * Video_VdecSendFrame(void *pArgs)
{
	VDEC_STREAM_PARAM_S *pstVdecStreamParam = (VDEC_STREAM_PARAM_S *)pArgs;
	VDEC_STREAM_S astStream[6];
	int iRet;
	int iCnt;
	int iChnnCnt;

    while(1)
    {
        if (pstVdecStreamParam->eCtrlSinal == VDEC_CTRL_STOP)
        {
            break;
        }
        else if (pstVdecStreamParam->eCtrlSinal == VDEC_CTRL_PAUSE)
        {
            continue;
        }
        else if(pstVdecStreamParam->eCtrlSinal == VDEC_CTRL_START)
        {
            for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
            {
                astStream[iChnnCnt].u64PTS		  	= pstVdecStreamParam[iChnnCnt].u64PtsInit;
                astStream[iChnnCnt].pu8Addr  	  	= pstVdecStreamParam[iChnnCnt].pu8Buffer;
                astStream[iChnnCnt].u32Len		  	= pstVdecStreamParam[iChnnCnt].u32ReadLen; 
                astStream[iChnnCnt].bEndOfFrame  	= HI_TRUE;
                astStream[iChnnCnt].bEndOfStream 	= HI_FALSE;	   
                iRet = HI_MPI_VDEC_SendStream(pstVdecStreamParam[iChnnCnt].s32ChnId, &astStream[iChnnCnt], pstVdecStreamParam[iChnnCnt].s32MilliSec);
                printf("channel=%d\n", pstVdecStreamParam[iChnnCnt].s32ChnId);
                if (HI_SUCCESS != iRet)
                {
                    printf("Channel%d: HI_MPI_VDEC_SendStream error with %x\n", pstVdecStreamParam[iChnnCnt].s32ChnId, iRet);
                    usleep(100);
                }
            }

            /* send the flag of stream end */
        /*	for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
            {
                memset(&astStream[iChnnCnt], 0, sizeof(VDEC_STREAM_S) );
                astStream[iChnnCnt].bEndOfStream = HI_TRUE;

                iRet = HI_MPI_VDEC_SendStream(pstVdecStreamParam[iChnnCnt].s32ChnId, &astStream[iChnnCnt], pstVdecStreamParam[iChnnCnt].s32MilliSec);
                if (HI_SUCCESS != iRet)
                {
                    printf("Channel%d: HI_MPI_VDEC_SendStream error with %x\n", pstVdecStreamParam[iChnnCnt].s32ChnId, iRet);
                    usleep(100);
                }

                fflush(stdout);

                fclose(apstFile[iChnnCnt]);
            }*/
        }
    }
    
	printf("Thread over!\n");

	return (void *)HI_SUCCESS;
}
#endif

/**
 * Video_StreamParamaConf:
 *
 */
int Video_StreamParamaConf(HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent, VDEC_STREAM_S *pstStream)
{
    VDEC_CHN VdChn = 6;
    SIZE_S stSize;
    HI_S32 s32MinBufSize;
    int iChnnCnt;
    int iRet;

    iRet = Video_getPicSize(VIDEO_ENCODING_MODE_PAL, pstHdmiHotplugEvent->enPicSize, &stSize);
    if (HI_SUCCESS !=iRet)
    {
        printf("get picture size failed!\n");
        return HI_FAILURE;
    }

    s32MinBufSize = (stSize.u32Width * stSize.u32Height * 3) >> 1;

    for (iChnnCnt = 0; iChnnCnt < VdChn; iChnnCnt++)
	{
        pstStream[iChnnCnt].u64PTS		 = 0;
        pstStream[iChnnCnt].u32Len		 = 0; 
        pstStream[iChnnCnt].bEndOfFrame  = HI_TRUE;
        pstStream[iChnnCnt].bEndOfStream = HI_FALSE;
        pstStream[iChnnCnt].pu8Addr  	 = malloc(s32MinBufSize);
        if(pstStream[iChnnCnt].pu8Addr == NULL)
	    {
		    printf("Can't alloc memory for stream\n");
		    return HI_FAILURE;
	    }
	}

    return HI_SUCCESS;
}

/**
 * Video_VdecSendStream:
 *
 */
int Video_VdecSendStream(VDEC_STREAM_PARAM_S *pstVdecStreamParam)
{
	VDEC_STREAM_S astStream[6];
	int iRet;
	int iCnt;
	int iChnnCnt;

    for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
    {
        astStream[iChnnCnt].u64PTS		  	= pstVdecStreamParam[iChnnCnt].u64PtsInit;
        astStream[iChnnCnt].pu8Addr  	  	= pstVdecStreamParam[iChnnCnt].pu8Buffer;
        astStream[iChnnCnt].u32Len		  	= pstVdecStreamParam[iChnnCnt].u32ReadLen; 
        astStream[iChnnCnt].bEndOfFrame  	= HI_TRUE;
        astStream[iChnnCnt].bEndOfStream 	= HI_FALSE;	   
        iRet = HI_MPI_VDEC_SendStream(pstVdecStreamParam[iChnnCnt].s32ChnId, &astStream[iChnnCnt], pstVdecStreamParam[iChnnCnt].s32MilliSec);
        printf("channel=%d\n", pstVdecStreamParam[iChnnCnt].s32ChnId);
        if (HI_SUCCESS != iRet)
        {
            printf("Channel%d: HI_MPI_VDEC_SendStream error with %x\n", pstVdecStreamParam[iChnnCnt].s32ChnId, iRet);
            usleep(100);
        }
    }

    /* send the flag of stream end */
/*	for (iChnnCnt = 0; iChnnCnt < 6; iChnnCnt++)
    {
        memset(&astStream[iChnnCnt], 0, sizeof(VDEC_STREAM_S) );
        astStream[iChnnCnt].bEndOfStream = HI_TRUE;

        iRet = HI_MPI_VDEC_SendStream(pstVdecStreamParam[iChnnCnt].s32ChnId, &astStream[iChnnCnt], pstVdecStreamParam[iChnnCnt].s32MilliSec);
        if (HI_SUCCESS != iRet)
        {
            printf("Channel%d: HI_MPI_VDEC_SendStream error with %x\n", pstVdecStreamParam[iChnnCnt].s32ChnId, iRet);
            usleep(100);
        }

        fflush(stdout);

        fclose(apstFile[iChnnCnt]);
    }*/

    return HI_SUCCESS;
}

#if 0
/**
 * Video_StreamParamaConf:
 *
 */
int Video_StreamParamaConf(HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent, VDEC_STREAM_PARAM_S *pstVdecStreamParam)
{
    VDEC_CHN VdChn = 6;
    SIZE_S stSize;
    PAYLOAD_TYPE_E enType = PT_H264;
    int iCnt;
    int iRet;

    iRet = Video_getPicSize(VIDEO_ENCODING_MODE_PAL, pstHdmiHotplugEvent->enPicSize, &stSize);
    if (HI_SUCCESS !=iRet)
    {
        printf("get picture size failed!\n");
        return HI_FAILURE;
    }

    for (iCnt = 0; iCnt < VdChn; iCnt++)
	{
        pstVdecStreamParam[iCnt].s32MilliSec      = 0;
	    pstVdecStreamParam[iCnt].s32ChnId         = iCnt;
	    pstVdecStreamParam[iCnt].u64PtsInit       = 0;
	    pstVdecStreamParam[iCnt].eCtrlSinal       = VDEC_CTRL_START;
	    pstVdecStreamParam[iCnt].enType           = enType;
	    pstVdecStreamParam[iCnt].s32MinBufSize    = (stSize.u32Width * stSize.u32Height * 3) >> 1;
	    pstVdecStreamParam[iCnt].s32StreamMode    = VIDEO_MODE_FRAME;
	    pstVdecStreamParam[iCnt].pu8Buffer        = malloc(pstVdecStreamParam[iCnt].s32MinBufSize);
	    if(pstVdecStreamParam[iCnt].pu8Buffer == NULL)
	    {
		    printf("Can't alloc memory for stream\n");
		    return HI_FAILURE;
	    }
	}
}

/**
 * Video_VdecStartSendStream:
 *
 */
int Video_VdecStartSendStream(VDEC_STREAM_PARAM_S *pstVdecStreamParam)
{
	int iRet;

	iRet = pthread_create(&g_VdecThread, NULL, Video_VdecSendFrame, (void *)pstVdecStreamParam);
	if(iRet)
	{
		printf("pthread_create error: %x\n", iRet);
		return HI_FAILURE;
	}

    return HI_SUCCESS;
}

/**
 * Video_VdecStopSendStream:
 *
 */
void Video_VdecStopSendStream(VDEC_STREAM_PARAM_S *pstVdecStreamParam)
{
	int iCnt;

	pstVdecStreamParam[0].eCtrlSinal = VDEC_CTRL_STOP;
	pthread_join(g_VdecThread, HI_NULL);
	
	for (iCnt = 0; iCnt < 6; iCnt++)
	{
		if (pstVdecStreamParam[iCnt].pu8Buffer != NULL)
		{
			free(pstVdecStreamParam[iCnt].pu8Buffer);
		}
	}
}
#endif
/**
 * Video_Init：
 * 
 */
int Video_Init(HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent)
{
	VB_CONF_S stVbConf;    
    VDEC_CHN VdChn = 6;
    PAYLOAD_TYPE_E enType = PT_H264;
    SIZE_S stSize; 
    VDEC_CHN_ATTR_S astVdecChnAttr[6];
    int iVpssGrpCnt = 6;
	int iVpssChnCnt = 1;    
    VPSS_GRP_ATTR_S tGrpAttr;     
    VO_DEV VoDev = VO_DEV_DHD0;
    VO_LAYER VoLayer = VO_LAYER_VHD0;
	E_VO_MODE enVoMode = VO_MODE_6MUX;
	unsigned int wWndNum = 6;
    VO_PUB_ATTR_S stVoPubAttr; 
    VO_VIDEO_LAYER_ATTR_S stLayerAttr; 
	int iCnt;
	int iRet = HI_SUCCESS;
	HI_HDMI_CALLBACK_FUNC_S stCallbackFunc;
	VO_INTF_SYNC_E enIntfSync;

	/******************************************
     step 1: init variable 
    ******************************************/    
    iRet = Video_getPicSize(VIDEO_ENCODING_MODE_PAL, pstHdmiHotplugEvent->enPicSize, &stSize);
    if (HI_SUCCESS !=iRet)
    {
        printf("get picture size failed!\n");
        return HI_FAILURE;
    }
    
    if (704 == stSize.u32Width)
    {
        stSize.u32Width = 720;
    }
    else if (352 == stSize.u32Width)
    {
        stSize.u32Width = 360;
    }
    else if (176 == stSize.u32Width)
    {
        stSize.u32Width = 180;
    }

	/******************************************
     step 2: start vdec 
    ******************************************/     
    memset(&stVbConf, 0, sizeof(VB_CONF_S));        
    Video_getVdecModCommPoolConf(&stVbConf, enType, &stSize, VdChn);	
    iRet = Video_VdecModCommPoolInit(&stVbConf);
    if(iRet != HI_SUCCESS)
    {	    	
        printf("init mod common vb fail for %#x!\n", iRet);
        return HI_FAILURE;
    }
    
    /**************create vdec chn****************************/    
    Video_createVdecChnAttr(VdChn, astVdecChnAttr, enType, &stSize);
    iRet = Video_VdecStart(VdChn, astVdecChnAttr);    
    if (HI_SUCCESS != iRet)
    {
        printf("Start Vdec failed!\n");
        goto END_2;
    } 

    for(iCnt = 0; iCnt < VdChn; iCnt++)
    {
        iRet = HI_MPI_VDEC_SetDisplayMode(iCnt, VIDEO_DISPLAY_MODE_PLAYBACK);
        if(HI_SUCCESS != iRet)
        {
            printf("HI_MPI_VDEC_SetDisplayMode failed!\n");
            goto END_2;
        }
    }

	/******************************************
     step 3: start vpss and vdec bind vpss 
    ******************************************/
    tGrpAttr.u32MaxW 	= stSize.u32Width;
    tGrpAttr.u32MaxH 	= stSize.u32Height;
    tGrpAttr.bIeEn 		= HI_FALSE;
    tGrpAttr.bNrEn 		= HI_TRUE;
    tGrpAttr.bHistEn 	= HI_FALSE;
    tGrpAttr.enDieMode 	= VPSS_DIE_MODE_NODIE;
    tGrpAttr.enPixFmt 	= PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    tGrpAttr.bDciEn 	= HI_FALSE;
    tGrpAttr.bEsEn 		= HI_TRUE;

    iRet = Video_VpssStart(iVpssGrpCnt, &stSize, iVpssChnCnt, &tGrpAttr);
    if (HI_SUCCESS != iRet)
    {
        printf("Start Vpss failed!\n");
        goto END_3;
    }
    for(iCnt = 0; iCnt < VdChn; iCnt++)
    {
        iRet = Video_VdecBindVpss(iCnt,iCnt);
        if (HI_SUCCESS != iRet)
        {
            printf("Video_VdecBindVpss failed!\n");
            goto END_4;
        }
    }

	/***************************************************
     step 4: start HD0 with 4 windows and bind to vpss 
    ***************************************************/    
    /**************start Dev****************************/
	Video_VoHdmiConvertSync(pstHdmiHotplugEvent->enVideoFmt, &enIntfSync);

    stVoPubAttr.enIntfSync = enIntfSync;	//VO_OUTPUT_1080P60
    stVoPubAttr.enIntfType = VO_INTF_HDMI; 
    stVoPubAttr.u32BgColor = 0x000000ff;
	
    iRet = Video_VoDevStart(VoDev, &stVoPubAttr);    
    if (HI_SUCCESS != iRet)
    {
        printf("Video_startVoDev failed!\n");
        goto END_4;
    }

    /**************start Layer****************************/ 
    stLayerAttr.bClusterMode = HI_FALSE;
    stLayerAttr.bDoubleFrame = HI_FALSE;
    stLayerAttr.enPixFormat  = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	
    iRet = Video_getVoWH(stVoPubAttr.enIntfSync, &stLayerAttr.stDispRect.u32Width, &stLayerAttr.stDispRect.u32Height, &stLayerAttr.u32DispFrmRt);
    if (iRet != HI_SUCCESS)
    {
        printf("Video_getVoWH failed with %#x!\n", iRet);
        goto END_5;
    }
    stLayerAttr.stImageSize.u32Width  = stLayerAttr.stDispRect.u32Width;
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height;
	stLayerAttr.u32DispFrmRt = 25;
	
    iRet = Video_VoLayerStart(VoLayer, &stLayerAttr);    
    if (HI_SUCCESS != iRet)
    {
       printf("Video_VoLayerStart failed!\n");
       goto END_6;
    }    
    
    /**************start Chn****************************/    
    iRet = Video_VoChnStart(VoLayer, enVoMode);    
    if (iRet != HI_SUCCESS)
    {
        printf("Video_VoChnStart failed with %#x!\n", iRet);
        goto END_6;
    }      
    
    /**************vo bind to vpss****************************/
    for(iCnt = 0; iCnt < wWndNum; iCnt++)
    {
    	iRet = Video_VpssBindVo(VoLayer, iCnt, iCnt, 0);
    	if (HI_SUCCESS != iRet)
    	{
    	   printf("Video_VpssBindVo failed!\n");
    	   goto END_6;
    	}    
    }

    return HI_SUCCESS;

	/******************************************
    processing errors area
    ******************************************/ 
END_6:    
    Video_VoChnStop(VoLayer,enVoMode);        
    Video_VoLayerStop(VoLayer);    
	   
    for(iCnt = 0;iCnt < wWndNum; iCnt++)
    {
        Video_VpssUnbindVo(VoLayer, iCnt, iCnt, VPSS_CHN0);
    }
    
END_5:          
    Video_VoHdmiExit(&stCallbackFunc);
    Video_VoDevStop(VoDev);
    
END_4:        
    for(iCnt = 0; iCnt < VdChn; iCnt++)
    {
        Video_VdecUnbindVpss(iCnt, iCnt);
    }
END_3:        
    Video_VpssStop(iVpssGrpCnt, iVpssChnCnt);
    
END_2:
    Video_VdecStop(VdChn);

	return HI_FAILURE;
}

/**
 * Video_Exit：
 * 
 */
void Video_Exit(void)
{  
    VDEC_CHN VdChn = 6;   
    int iVpssGrpCnt = 6;
	int iVpssChnCnt = 1;     
    VO_DEV VoDev = VO_DEV_DHD0;
    VO_LAYER VoLayer = VO_LAYER_VHD0;
	E_VO_MODE enVoMode = VO_MODE_6MUX;
	unsigned int wWndNum = 6;
	int iCnt;

    Video_VoChnStop(VoLayer,enVoMode);        
    Video_VoLayerStop(VoLayer);    
	   
    for(iCnt = 0;iCnt < wWndNum; iCnt++)
    {
        Video_VpssUnbindVo(VoLayer, iCnt, iCnt, VPSS_CHN0);
    }
             
    Video_VoDevStop(VoDev);
           
    for(iCnt = 0; iCnt < VdChn; iCnt++)
    {
        Video_VdecUnbindVpss(iCnt, iCnt);
    }
       
    Video_VpssStop(iVpssGrpCnt, iVpssChnCnt);
    
    Video_VdecStop(VdChn);
}

/**
 * Video_HdmiEventProc:
 *
 */
HI_VOID Video_HdmiEventProc(HI_HDMI_EVENT_TYPE_E event, HI_VOID *pPrivateData)
{
    HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent = (HDMI_HOTPLUG_EVENT_S *)pPrivateData;

	switch (event)
	{
		case HI_HDMI_EVENT_HOTPLUG:
            printf("......HDMI EVENT %x......\n", event);

            if (HI_SUCCESS != Video_VoHdmiConfig(pstHdmiHotplugEvent))
            {
                printf("Start Video_VoHdmiStart failed!\n");
                return;
            }
            printf("pstHdmiHotplugEvent->enPicSize=%d\n", pstHdmiHotplugEvent->enPicSize);
            Video_Init(pstHdmiHotplugEvent);
            //Video_VdecStartSendStream(pstHdmiHotplugEvent);
            pstHdmiHotplugEvent->bHdmiConnected = HI_TRUE;
			
			break;

		case HI_HDMI_EVENT_NO_PLUG:
            printf("......HDMI EVENT %x......\n", event);

            pstHdmiHotplugEvent->bHdmiConnected = HI_FALSE;

            Video_Exit();
			break;

		case HI_HDMI_EVENT_EDID_FAIL:
			break;

		case HI_HDMI_EVENT_HDCP_FAIL:
			break;

		case HI_HDMI_EVENT_HDCP_SUCCESS:
			break;

		case HI_HDMI_EVENT_HDCP_USERSETTING:
			break;

		default:
			printf("un-known event:%d\n",event);
	}

	return;	
}

/**
 * Video_HdmiCreate
 * 
 */
int Video_HdmiCreate(HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent)
{
    HI_HDMI_CALLBACK_FUNC_S stCallbackFunc;
	HDMI_HOTPLUG_EVENT_S stHdmiHotplugEvent;
    int iRet;

    memset(&stHdmiHotplugEvent, 0, sizeof(HDMI_HOTPLUG_EVENT_S));
   	stCallbackFunc.pfnHdmiEventCallback = Video_HdmiEventProc;
	printf("1: %p\n", Video_HdmiEventProc);
	stCallbackFunc.pPrivateData = pstHdmiHotplugEvent;

	iRet = Video_VoHdmiInit(&stCallbackFunc);
	if (iRet)
	{
		printf("Video_VoHdmiInit error: %x\n", iRet);
        return HI_FAILURE;
	}

    return HI_SUCCESS;
}

/**
 * Video_HdmiDestroy
 * 
 */
int Video_HdmiDestroy(HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent)
{
    int iRet;
    HI_HDMI_CALLBACK_FUNC_S stCallbackFunc;

    stCallbackFunc.pfnHdmiEventCallback = Video_HdmiEventProc;
    stCallbackFunc.pPrivateData = pstHdmiHotplugEvent;

    iRet = Video_VoHdmiExit(&stCallbackFunc);
    if (iRet)
	{
		printf("Video_VoHdmiExit error: %x\n", iRet);
        return HI_FAILURE;
	}

    return HI_SUCCESS;
}