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

#include "hi_type.h"
#include "hi_comm_sys.h"
#include "hi_comm_vi.h"
#include "hi_comm_vda.h"
#include "hi_comm_region.h"
#include "hi_comm_adec.h"
#include "hi_comm_aenc.h"
#include "hi_comm_ai.h"
#include "hi_comm_ao.h"
#include "hi_comm_hdmi.h"
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

#include "video_common.h"

/**
 * Video_VdecModCommPoolConf:
 * Configurate vdec common pool.
 */
void Video_getVdecModCommPoolConf(VB_CONF_S *pstModVbConf, 
    PAYLOAD_TYPE_E enType, SIZE_S *pstSize, int iChnNum)
{
    unsigned int wPicSize;
	unsigned int wPmvSize;
	
    memset(pstModVbConf, 0, sizeof(VB_CONF_S));
    pstModVbConf->u32MaxPoolCnt = 2;
	
    VB_PIC_BLK_SIZE(pstSize->u32Width, pstSize->u32Height, enType, wPicSize);	
    pstModVbConf->astCommPool[0].u32BlkSize = wPicSize;
    pstModVbConf->astCommPool[0].u32BlkCnt  = 10 * iChnNum;

    /* NOTICE: 			   
     * 1. if the VDEC channel is H264 channel and support to decode B frame, then you should allocate PmvBuffer 
     * 2. if the VDEC channel is MPEG4 channel, then you should allocate PmvBuffer.
     */
    if(PT_H265 == enType)
    {
        VB_PMV_BLK_SIZE(pstSize->u32Width, pstSize->u32Height, enType, wPmvSize);
        
        pstModVbConf->astCommPool[1].u32BlkSize = wPmvSize;
        pstModVbConf->astCommPool[1].u32BlkCnt  = 5 * iChnNum;
    }
}

/* Video_VdecInitModCommVb:
 * Initialize vb common pool.
 */
int Video_VdecModCommPoolInit(VB_CONF_S *pstModVbConf)
{
	HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);

	CHECK_RET(HI_MPI_VB_SetModPoolConf(VB_UID_VDEC, pstModVbConf), "HI_MPI_VB_SetModPoolConf");
	CHECK_RET(HI_MPI_VB_InitModCommPool(VB_UID_VDEC), "HI_MPI_VB_InitModCommPool");
	
	return HI_SUCCESS;
}

/**
 * Video_VdecChnAttr:
 *
 */
void Video_createVdecChnAttr(int iChnNum, VDEC_CHN_ATTR_S *pstVdecChnAttr, PAYLOAD_TYPE_E enType, SIZE_S *pstSize)
{
	int iCnt;

	for(iCnt = 0; iCnt < iChnNum; iCnt++)
	{
		pstVdecChnAttr[iCnt].enType	   	                        = enType;
		pstVdecChnAttr[iCnt].u32BufSize                         = 3 * pstSize->u32Width * pstSize->u32Height;
		pstVdecChnAttr[iCnt].u32Priority                        = 5;
		pstVdecChnAttr[iCnt].u32PicWidth                        = pstSize->u32Width;
		pstVdecChnAttr[iCnt].u32PicHeight                       = pstSize->u32Height;
        pstVdecChnAttr[iCnt].stVdecVideoAttr.enMode             = VIDEO_MODE_FRAME;
        pstVdecChnAttr[iCnt].stVdecVideoAttr.u32RefFrameNum     = 3;
        pstVdecChnAttr[iCnt].stVdecVideoAttr.bTemporalMvpEnable = 0; 
	}
}

/**
 * Video_VdecStart:
 * Start vdec.
 */
int Video_VdecStart(int iChnNum, VDEC_CHN_ATTR_S *pstAttr)
{
    int iCnt;

    for(iCnt = 0; iCnt < iChnNum; iCnt++)
    {	
        CHECK_CHN_RET(HI_MPI_VDEC_CreateChn(iCnt, &pstAttr[iCnt]), iCnt, "HI_MPI_VDEC_CreateChn");
        
        CHECK_CHN_RET(HI_MPI_VDEC_StartRecvStream(iCnt), iCnt, "HI_MPI_VDEC_StartRecvStream");
    }

    return HI_SUCCESS;
}

/**
 * Video_VdecStop:
 * Stop vdec.
 */
int Video_VdecStop(int iChnNum)
{
    int iCnt;	

    for(iCnt = 0; iCnt < iChnNum; iCnt++)
    {
        CHECK_CHN_RET(HI_MPI_VDEC_StopRecvStream(iCnt), iCnt, "HI_MPI_VDEC_StopRecvStream");       
        CHECK_CHN_RET(HI_MPI_VDEC_DestroyChn(iCnt), iCnt, "HI_MPI_VDEC_DestroyChn");
    }

    return HI_SUCCESS;
}

/**
 * Video_VpssStart: 
 * Start vpss. VPSS chn with frame
 */
int Video_VpssStart(int iGrpCnt, SIZE_S *pstSize, int iChnCnt, VPSS_GRP_ATTR_S *pstVpssGrpAttr)
{
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stGrpAttr = {0};
    VPSS_CHN_ATTR_S stChnAttr = {0};
    VPSS_GRP_PARAM_S stVpssParam = {0};
    int iRet;
    int i, j;
//    VPSS_FRAME_RATE_S stVpssFrameRate;
//    VPSS_CHN_MODE_S stVpssMode;

    /*** Set Vpss Grp Attr ***/

    if(NULL == pstVpssGrpAttr)
    {
        stGrpAttr.u32MaxW  	= pstSize->u32Width;
        stGrpAttr.u32MaxH  	= pstSize->u32Height;
        stGrpAttr.enPixFmt 	= PIXEL_FORMAT_YUV_SEMIPLANAR_420; 
        stGrpAttr.bIeEn 	= HI_FALSE;
        stGrpAttr.bNrEn 	= HI_TRUE;
        stGrpAttr.bDciEn 	= HI_FALSE;
        stGrpAttr.bHistEn 	= HI_FALSE;
        stGrpAttr.bEsEn 	= HI_FALSE;
        stGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    }
    else
    {
        memcpy(&stGrpAttr, pstVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S));
    }

    for(i=0; i < iGrpCnt; i++)
    {
        VpssGrp = i;
        /*** create vpss group ***/
        iRet = HI_MPI_VPSS_CreateGrp(VpssGrp, &stGrpAttr);
        if (iRet != HI_SUCCESS)
        {
            printf("HI_MPI_VPSS_CreateGrp failed with %#x!\n", iRet);
            return HI_FAILURE;
        }

        /*** set vpss param ***/
        iRet = HI_MPI_VPSS_GetGrpParam(VpssGrp, &stVpssParam);
        if (iRet != HI_SUCCESS)
        {
            printf("failed with %#x!\n", iRet);
            return HI_FAILURE;
        }
        
        stVpssParam.u32IeStrength = 0;
        iRet = HI_MPI_VPSS_SetGrpParam(VpssGrp, &stVpssParam);
        if (iRet != HI_SUCCESS)
        {
            printf("failed with %#x!\n", iRet);
            return HI_FAILURE;
        }

//        HI_MPI_VPSS_GetGrpFrameRate(VpssGrp, &stVpssFrameRate);
//        printf("SrcFrmRate=%d s32DstFrmRate=%d\n", stVpssFrameRate.s32SrcFrmRate, stVpssFrameRate.s32DstFrmRate);

        /*** enable vpss chn, with frame ***/
        for(j=0; j < iChnCnt; j++)
        {
            VpssChn = j;
/*
            iRet = HI_MPI_VPSS_GetChnMode(VpssGrp,VpssChn,&stVpssMode);
            if (iRet != HI_SUCCESS)
            {
                printf("HI_MPI_VPSS_GetChnMode failed with %#x\n", iRet);
                return iRet;
            }
            printf("mode=%d SrcFrmRate=%d DstFrmRate=%d\n", stVpssMode.enChnMode, stVpssMode.stFrameRate.s32SrcFrmRate, stVpssMode.stFrameRate.s32DstFrmRate);
            stVpssMode.enChnMode = VPSS_CHN_MODE_USER;
            stVpssMode.u32Width = 640;
            stVpssMode.u32Height = 360;
            stVpssMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
            stVpssMode.stFrameRate.s32SrcFrmRate  = 30;
            stVpssMode.stFrameRate.s32DstFrmRate  = 25;
            iRet = HI_MPI_VPSS_SetChnMode(VpssGrp,VpssChn,&stVpssMode);
            if(iRet != HI_SUCCESS)
            {
                printf("HI_MPI_VPSS_SetChnMode failed with %#x\n", iRet);
                return iRet;
            }*/

            /* Set Vpss Chn attr */
            stChnAttr.bSpEn 					= HI_FALSE;
            stChnAttr.bUVInvert 				= HI_FALSE;
            stChnAttr.bBorderEn 				= HI_TRUE;
            stChnAttr.stBorder.u32Color 		= 0xff00;
            stChnAttr.stBorder.u32LeftWidth 	= 2;
            stChnAttr.stBorder.u32RightWidth 	= 2;
            stChnAttr.stBorder.u32TopWidth 		= 2;
            stChnAttr.stBorder.u32BottomWidth 	= 2;
            
            iRet = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stChnAttr);
            if (iRet != HI_SUCCESS)
            {
                printf("HI_MPI_VPSS_SetChnAttr failed with %#x\n", iRet);
                return HI_FAILURE;
            }
    
            iRet = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
            if (iRet != HI_SUCCESS)
            {
                printf("HI_MPI_VPSS_EnableChn failed with %#x\n", iRet);
                return HI_FAILURE;
            }
        }
        
        /*** start vpss group ***/
        iRet = HI_MPI_VPSS_StartGrp(VpssGrp);
        if (iRet != HI_SUCCESS)
        {
            printf("HI_MPI_VPSS_StartGrp failed with %#x\n", iRet);
            return HI_FAILURE;
        }
    }
	
    return HI_SUCCESS;
}

/**
 * Video_VpssStop: 
 * Stop vpss.
 */
int Video_VpssStop(int iGrpCnt, int iChnCnt)
{
    int i, j;
    int iRet = HI_SUCCESS;
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;

    for(i = 0; i < iGrpCnt; i++)
    {
        VpssGrp = i;
        iRet = HI_MPI_VPSS_StopGrp(VpssGrp);
        if (iRet != HI_SUCCESS)
        {
            printf("failed with %#x!\n", iRet);
            return HI_FAILURE;
        }
        for(j = 0; j < iChnCnt; j++)
        {
            VpssChn = j;
            iRet = HI_MPI_VPSS_DisableChn(VpssGrp, VpssChn);
            if (iRet != HI_SUCCESS)
            {
                printf("failed with %#x!\n", iRet);
                return HI_FAILURE;
            }
        }
    
        iRet = HI_MPI_VPSS_DestroyGrp(VpssGrp);
        if (iRet != HI_SUCCESS)
        {
            printf("failed with %#x!\n", iRet);
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

/**
 * Video_VdecBindVpss:
 * 
 */
int Video_VdecBindVpss(VDEC_CHN VdChn, VPSS_GRP VpssGrp)
{
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = VdChn;

    stDestChn.enModId = HI_ID_VPSS;
    stDestChn.s32DevId = VpssGrp;
    stDestChn.s32ChnId = 0;

    CHECK_RET(HI_MPI_SYS_Bind(&stSrcChn, &stDestChn), "HI_MPI_SYS_Bind");

    return HI_SUCCESS;
}

/**
 * Video_VdecUnbindVpss:
 *
 */
int Video_VdecUnbindVpss(VDEC_CHN VdChn, VPSS_GRP VpssGrp)
{
	MPP_CHN_S stSrcChn;
	MPP_CHN_S stDestChn;

	stSrcChn.enModId = HI_ID_VDEC;
	stSrcChn.s32DevId = 0;
	stSrcChn.s32ChnId = VdChn;

	stDestChn.enModId = HI_ID_VPSS;
	stDestChn.s32DevId = VpssGrp;
	stDestChn.s32ChnId = 0;

	CHECK_RET(HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn), "HI_MPI_SYS_UnBind");

	return HI_SUCCESS;
}

/**
 * Video_startVoDev:
 *
 */
int Video_VoDevStart(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr)
{
    int iRet = HI_SUCCESS;
    
    iRet = HI_MPI_VO_SetPubAttr(VoDev, pstPubAttr);
    if (iRet != HI_SUCCESS)
    {
        printf("failed with %#x!\n", iRet);
        return HI_FAILURE;
    }

    iRet = HI_MPI_VO_Enable(VoDev);
    if (iRet != HI_SUCCESS)
    {
        printf("failed with %#x!\n", iRet);
        return HI_FAILURE;
    }
    
    return iRet;
}

/**
 * Video_stopVoDev:
 *
 */
int Video_VoDevStop(VO_DEV VoDev)
{
	int iRet = HI_SUCCESS;
	
	iRet = HI_MPI_VO_Disable(VoDev);
	if (iRet != HI_SUCCESS)
	{
		printf("HI_MPI_VO_Disable failed with %#x!\n", iRet);
		return HI_FAILURE;
	}

	return iRet;
}

/**
 * Video_VoHdmiConvertSync
 *
 */
int Video_VoHdmiConvertSync(HI_HDMI_VIDEO_FMT_E enVideoFmt, VO_INTF_SYNC_E *penIntfSync)
{
    switch (enVideoFmt)
    {
        case HI_HDMI_VIDEO_FMT_PAL:
            *penIntfSync = VO_OUTPUT_PAL;
            break;

        case HI_HDMI_VIDEO_FMT_NTSC:
            *penIntfSync = VO_OUTPUT_NTSC;
            break;

        case HI_HDMI_VIDEO_FMT_1080P_24:
            *penIntfSync = VO_OUTPUT_1080P24;
            break;

        case HI_HDMI_VIDEO_FMT_1080P_25:
            *penIntfSync = VO_OUTPUT_1080P25;
            break;

        case HI_HDMI_VIDEO_FMT_1080P_30:
            *penIntfSync = VO_OUTPUT_1080P30;
            break;

        case HI_HDMI_VIDEO_FMT_720P_50:
            *penIntfSync = VO_OUTPUT_720P50;
            break;

        case HI_HDMI_VIDEO_FMT_720P_60:
            *penIntfSync = VO_OUTPUT_720P60;
            break;

        case HI_HDMI_VIDEO_FMT_1080i_50:
            *penIntfSync = VO_OUTPUT_1080I50;
            break;

        case HI_HDMI_VIDEO_FMT_1080i_60:
            *penIntfSync = VO_OUTPUT_1080I60;
            break;

        case HI_HDMI_VIDEO_FMT_1080P_50:
            *penIntfSync = VO_OUTPUT_1080P50;
            break;

        case HI_HDMI_VIDEO_FMT_1080P_60:
            *penIntfSync = VO_OUTPUT_1080P60;
            break;

        case HI_HDMI_VIDEO_FMT_576P_50:
            *penIntfSync = VO_OUTPUT_576P50;
            break;

        case HI_HDMI_VIDEO_FMT_480P_60:
            *penIntfSync = VO_OUTPUT_480P60;
            break;

        case HI_HDMI_VIDEO_FMT_VESA_800X600_60:
            *penIntfSync = VO_OUTPUT_800x600_60;
            break;

        case HI_HDMI_VIDEO_FMT_VESA_1024X768_60:
            *penIntfSync = VO_OUTPUT_1024x768_60;
            break;

        case HI_HDMI_VIDEO_FMT_VESA_1280X1024_60:
            *penIntfSync = VO_OUTPUT_1280x1024_60;
            break;

        case HI_HDMI_VIDEO_FMT_VESA_1366X768_60:
            *penIntfSync = VO_OUTPUT_1366x768_60;
            break;

        case HI_HDMI_VIDEO_FMT_VESA_1440X900_60:
            *penIntfSync = VO_OUTPUT_1440x900_60;
            break;

        case HI_HDMI_VIDEO_FMT_VESA_1280X800_60:
            *penIntfSync = VO_OUTPUT_1280x800_60;
            break;

        case HI_HDMI_VIDEO_FMT_VESA_1920X1200_60:
            *penIntfSync = VO_OUTPUT_1920x1200_60;
            break; 

        case HI_HDMI_VIDEO_FMT_3840X2160P_30:
            *penIntfSync = VO_OUTPUT_3840x2160_30;
            break;

        case HI_HDMI_VIDEO_FMT_3840X2160P_60:
            *penIntfSync = VO_OUTPUT_3840x2160_60;    
            break;

        default :
            printf("Unkonw HI_HDMI_VIDEO_FMT value!\n");
            return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/**
 * Video_VoHdmiInit:
 *
 */
int Video_VoHdmiInit(HI_HDMI_CALLBACK_FUNC_S *pstCallbackFunc)
{
//    HI_HDMI_SINK_CAPABILITY_S *pstSinkCap = (HI_HDMI_SINK_CAPABILITY_S *)pstCallbackFunc->pPrivateData;
//    HI_HDMI_EDID_S stEdidData;
//    HI_HDMI_SINK_CAPABILITY_S stSinkCap;
    int iRet;

    /******************start hdmi***********************/
    iRet = HI_MPI_HDMI_Init();
    if (iRet)
    {
        printf("HI_MPI_HDMI_Init error: %x\n", iRet);
        return HI_FAILURE;
    }

    iRet = HI_MPI_HDMI_Open(HI_HDMI_ID_0);
    if (iRet)
    {
        printf("HI_MPI_HDMI_Open error: %x\n", iRet);
        return HI_FAILURE;
    }

    /**************register hdmi hot-plug event**************/
    printf("2: %p\n", pstCallbackFunc->pfnHdmiEventCallback);
	iRet = HI_MPI_HDMI_RegCallbackFunc(HI_HDMI_ID_0, pstCallbackFunc);
	if (iRet)
	{
		printf("HI_MPI_HDMI_RegCallbackFunc error: %x\n", iRet);
        return HI_FAILURE;
	}

    /**
     * check if hdmi is connected
     */

    iRet = HI_MPI_HDMI_Start(HI_HDMI_ID_0);
    if (iRet)
    {
        printf("HI_MPI_HDMI_Start error: %x\n", iRet);
        return HI_FAILURE;
    }

    /* get sink capability */
/*    memset(&stSinkCap, 0, sizeof(HI_HDMI_SINK_CAPABILITY_S));
    HI_MPI_HDMI_GetSinkCapability(HI_HDMI_ID_0, &stSinkCap);

    printf("bConnected=%d\n", stSinkCap.bConnected);
    printf("enNativeVideoFormat=%d\n", stSinkCap.enNativeVideoFormat);

    while(!stSinkCap.bConnected)
    {
        HI_MPI_HDMI_GetSinkCapability(HI_HDMI_ID_0, &stSinkCap);    // must be behind "HI_MPI_HDMI_Start(HI_HDMI_ID_0)"
    }
    printf("bConnected=%d\n", stSinkCap.bConnected);
    printf("enNativeVideoFormat=%d\n", stSinkCap.enNativeVideoFormat);

    printf("hdmi connect\n");
*/
    return HI_SUCCESS;
}

/**
 * Video_VoHdmiStart:
 *
 */
int Video_VoHdmiConfig(HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent)
{
    HI_HDMI_SINK_CAPABILITY_S stSinkCap;
    HI_HDMI_ATTR_S stAttr;
    int iRet;
#if 0	
    iRet = HI_MPI_HDMI_Init();
    if (iRet)
	{
		printf("HI_MPI_HDMI_Init error: %x\n", iRet);
        return HI_FAILURE;
	}

    iRet = HI_MPI_HDMI_Open(HI_HDMI_ID_0);
    if (iRet)
	{
		printf("HI_MPI_HDMI_Open error: %x\n", iRet);
        return HI_FAILURE;
	}
    
    /* get EDID */
    HI_MPI_HDMI_Force_GetEDID(HI_HDMI_ID_0, &stEdidData);

    /* get sink capability */
    HI_MPI_HDMI_GetSinkCapability(HI_HDMI_ID_0, pstSinkCap);
    
    /* wait for hdmmi connecting */
    while(!pstSinkCap->bConnected)
    {
        HI_MPI_HDMI_GetSinkCapability(HI_HDMI_ID_0, pstSinkCap);
    }
    printf("bConnected=%d\n", pstSinkCap->bConnected);
    printf("enNativeVideoFormat=%d\n", pstSinkCap->enNativeVideoFormat);

    printf("hdmi connect\n");
#endif
/*
    iRet = HI_MPI_HDMI_Start(HI_HDMI_ID_0);
    if (iRet)
	{
		printf("HI_MPI_HDMI_Start error: %x\n", iRet);
        return HI_FAILURE;
	}
*/
    /* get sink capability */
    memset(&stSinkCap, 0, sizeof(HI_HDMI_SINK_CAPABILITY_S));
    iRet = HI_MPI_HDMI_GetSinkCapability(HI_HDMI_ID_0, &stSinkCap); // must be behind "HI_MPI_HDMI_Start(HI_HDMI_ID_0)"
    if (iRet)
    {
        printf("HI_MPI_HDMI_GetSinkCapability error: %x\n", iRet);
        return HI_FAILURE;
    }

    pstHdmiHotplugEvent->enVideoFmt = stSinkCap.enNativeVideoFormat;

    iRet = HI_MPI_HDMI_Stop(HI_HDMI_ID_0);
    if (iRet)
    {
        printf("HI_MPI_HDMI_Stop error: %x\n", iRet);
        return HI_FAILURE;
    }

    iRet = HI_MPI_HDMI_GetAttr(HI_HDMI_ID_0, &stAttr);
    if (iRet)
	{
		printf("HI_MPI_HDMI_GetAttr error: %x\n", iRet);
        return HI_FAILURE;
	}

    stAttr.bEnableHdmi 			= HI_TRUE;   
    stAttr.bEnableVideo 		= HI_TRUE;
    stAttr.enVideoFmt 			= stSinkCap.enNativeVideoFormat;
    stAttr.enVidOutMode 		= HI_HDMI_VIDEO_MODE_YCBCR444;
    stAttr.enDeepColorMode 		= HI_HDMI_DEEP_COLOR_OFF;
    stAttr.bxvYCCMode 			= HI_FALSE;
    stAttr.enDefaultMode 		= HI_HDMI_FORCE_HDMI;
    stAttr.bEnableAudio 		= HI_FALSE;
    stAttr.enSoundIntf 			= HI_HDMI_SND_INTERFACE_I2S;
    stAttr.bIsMultiChannel 		= HI_FALSE;
    stAttr.enBitDepth 			= HI_HDMI_BIT_DEPTH_16;
    stAttr.bEnableAviInfoFrame 	= HI_TRUE;
    stAttr.bEnableAudInfoFrame 	= HI_TRUE;
    stAttr.bEnableSpdInfoFrame 	= HI_FALSE;
    stAttr.bEnableMpegInfoFrame = HI_FALSE;
    stAttr.bDebugFlag 			= HI_FALSE;          
    stAttr.bHDCPEnable 			= HI_FALSE;
    stAttr.b3DEnable 			= HI_FALSE;
    
    iRet = HI_MPI_HDMI_SetAttr(HI_HDMI_ID_0, &stAttr);
    if (iRet)
	{
		printf("HI_MPI_HDMI_SetAttr error: %x\n", iRet);
        return HI_FAILURE;
	}

    iRet = HI_MPI_HDMI_Start(HI_HDMI_ID_0);
    if (iRet)
	{
		printf("HI_MPI_HDMI_Start error: %x\n", iRet);
        return HI_FAILURE;
	}
	
    return HI_SUCCESS;
}

/**
 * Video_VoHdmiStop:
 *
 */
int Video_VoHdmiStop(HI_HDMI_ID_E enHdmiId)
{
	int iRet;
	
	iRet = HI_MPI_HDMI_Stop(enHdmiId);

	return iRet;
}

/**
 * Video_VoHdmiExit:
 *
 */
int Video_VoHdmiExit(HI_HDMI_CALLBACK_FUNC_S *pstCallbackFunc)
{
	int iRet;

    /*************************exit hdmi*****************************/
    iRet = HI_MPI_HDMI_Stop(HI_HDMI_ID_0);
    if (iRet)
    {
        printf("HI_MPI_HDMI_Stop error: %x\n", iRet);
        return HI_FAILURE;
    }

    /*cancel hdmi hot-plug event*/
	iRet = HI_MPI_HDMI_UnRegCallbackFunc(HI_HDMI_ID_0, pstCallbackFunc);
    printf("4: %p\n", pstCallbackFunc->pfnHdmiEventCallback);
	if (iRet)
	{
		printf("HI_MPI_HDMI_UnRegCallbackFunc error: %x\n", iRet);
	}

    iRet = HI_MPI_HDMI_Close(HI_HDMI_ID_0);
    if (iRet)
    {
        printf("HI_MPI_HDMI_Close error: %x\n", iRet);
        return HI_FAILURE;
    }

    iRet = HI_MPI_HDMI_DeInit();
    if (iRet)
    {
        printf("HI_MPI_HDMI_DeInit error: %x\n", iRet);
        return HI_FAILURE;
    }

	return HI_SUCCESS;
}

/**
 * Video_getVoWH:
 *
 */
int Video_getVoWH(VO_INTF_SYNC_E enIntfSync, HI_U32 *pu32W,HI_U32 *pu32H, HI_U32 *pu32Frm)
{
    switch (enIntfSync)
    {
        case VO_OUTPUT_PAL       	:  *pu32W = 720;  *pu32H = 576;  *pu32Frm = 25; break;
        case VO_OUTPUT_NTSC      	:  *pu32W = 720;  *pu32H = 480;  *pu32Frm = 30; break;        
        case VO_OUTPUT_576P50    	:  *pu32W = 720;  *pu32H = 576;  *pu32Frm = 50; break;
        case VO_OUTPUT_480P60    	:  *pu32W = 720;  *pu32H = 480;  *pu32Frm = 60; break;
        case VO_OUTPUT_800x600_60	:  *pu32W = 800;  *pu32H = 600;  *pu32Frm = 60; break;
        case VO_OUTPUT_720P50    	:  *pu32W = 1280; *pu32H = 720;  *pu32Frm = 50; break;
        case VO_OUTPUT_720P60    	:  *pu32W = 1280; *pu32H = 720;  *pu32Frm = 60; break;        
        case VO_OUTPUT_1080I50   	:  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 25; break;
        case VO_OUTPUT_1080I60   	:  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 30; break;
        case VO_OUTPUT_1080P24   	:  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 24; break;        
        case VO_OUTPUT_1080P25   	:  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 25; break;
        case VO_OUTPUT_1080P30   	:  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 30; break;
        case VO_OUTPUT_1080P50   	:  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 50; break;
        case VO_OUTPUT_1080P60   	:  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 60; break;
        case VO_OUTPUT_1024x768_60	:  *pu32W = 1024; *pu32H = 768;  *pu32Frm = 60; break;
        case VO_OUTPUT_1280x1024_60	:  *pu32W = 1280; *pu32H = 1024; *pu32Frm = 60; break;
        case VO_OUTPUT_1366x768_60	:  *pu32W = 1366; *pu32H = 768;  *pu32Frm = 60; break;
        case VO_OUTPUT_1440x900_60	:  *pu32W = 1440; *pu32H = 900;  *pu32Frm = 60; break;
        case VO_OUTPUT_1280x800_60	:  *pu32W = 1280; *pu32H = 800;  *pu32Frm = 60; break;        
        case VO_OUTPUT_1600x1200_60	:  *pu32W = 1600; *pu32H = 1200; *pu32Frm = 60; break;
        case VO_OUTPUT_1680x1050_60	:  *pu32W = 1680; *pu32H = 1050; *pu32Frm = 60; break;
        case VO_OUTPUT_1920x1200_60	:  *pu32W = 1920; *pu32H = 1200; *pu32Frm = 60; break;
        case VO_OUTPUT_3840x2160_30	:  *pu32W = 3840; *pu32H = 2160; *pu32Frm = 30; break;
        case VO_OUTPUT_3840x2160_60	:  *pu32W = 3840; *pu32H = 2160; *pu32Frm = 60; break;
        case VO_OUTPUT_USER    		:  *pu32W = 720;  *pu32H = 576;  *pu32Frm = 25; break;
        default: 
            printf("vo enIntfSync not support!\n");
            return HI_FAILURE;
    }
	
    return HI_SUCCESS;
}

/**
 * Video_startVoLayer:
 *
 */
int Video_VoLayerStart(VO_LAYER VoLayer,const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	int iRet = HI_SUCCESS;
	iRet = HI_MPI_VO_SetVideoLayerAttr(VoLayer, pstLayerAttr);
	if (iRet != HI_SUCCESS)
	{
		printf("failed with %#x!\n", iRet);
		return HI_FAILURE;
	}

	iRet = HI_MPI_VO_EnableVideoLayer(VoLayer);
	if (iRet != HI_SUCCESS)
	{
		printf("failed with %#x!\n", iRet);
		return HI_FAILURE;
	}
	
	return iRet;
}

/**
 * Video_stopVoLayer:
 *
 */
int Video_VoLayerStop(VO_LAYER VoLayer)
{
    int iRet = HI_SUCCESS;
    
    iRet = HI_MPI_VO_DisableVideoLayer(VoLayer);
    if (iRet != HI_SUCCESS)
    {
        printf("failed with %#x!\n", iRet);
        return HI_FAILURE;
    }
	
    return iRet;
}

/**
 * Video_startVoChn:
 *
 */
int Video_VoChnStart(VO_LAYER VoLayer, E_VO_MODE enMode)
{
    int iCnt;
    int iRet = HI_SUCCESS;
    unsigned int wWndNum = 0;
    unsigned int wSquare = 0;
    unsigned int wWidth = 0;
    unsigned int wHeight = 0;
    VO_CHN_ATTR_S stChnAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    
    switch (enMode)
    {
        case VO_MODE_1MUX:
            wWndNum = 1;
            wSquare = 1;
            break;
        case VO_MODE_4MUX:
            wWndNum = 4;
            wSquare = 2;
            break;
        case VO_MODE_9MUX:
            wWndNum = 9;
            wSquare = 3;
            break;
        case VO_MODE_16MUX:
            wWndNum = 16;
            wSquare = 4;
            break;            
        case VO_MODE_25MUX:
            wWndNum = 25;
            wSquare = 5;
            break;
        case VO_MODE_36MUX:
            wWndNum = 36;
            wSquare = 6;
            break;
        case VO_MODE_64MUX:
            wWndNum = 64;
            wSquare = 8;
            break;
        case VO_MODE_6MUX:
            wWndNum = 6;
            wSquare = 3;
            break;
        default:
            printf("failed with %#x!\n", iRet);
            return HI_FAILURE;
    }

    if((VoLayer == VO_LAYER_VHD1) && (wWndNum == 36))
        wWndNum = 32;
    
    if((VoLayer == VO_CAS_DEV_1+1) || (VoLayer == VO_CAS_DEV_2+1))
    {
        iRet = HI_MPI_VO_GetVideoLayerAttr(VO_DEV_DHD0, &stLayerAttr);
    }
    else
    {        
        iRet = HI_MPI_VO_GetVideoLayerAttr(VoLayer, &stLayerAttr);
    }
    if (iRet != HI_SUCCESS)
    {
        printf("failed with %#x!\n", iRet);
        return HI_FAILURE;
    }
	
    wWidth  = stLayerAttr.stImageSize.u32Width;
    wHeight = stLayerAttr.stImageSize.u32Height;

    printf("wWidth=%d, wHeight=%d, wWndNum=%d\n", wWidth, wHeight, wWndNum);
	
    for (iCnt = 0; iCnt < wWndNum; iCnt++)
    {
        if (enMode == VO_MODE_6MUX)
        {
            stChnAttr.stRect.s32X       = ALIGN_BACK((wWidth / wSquare) * (iCnt % wSquare), 2);
            stChnAttr.stRect.s32Y       = ALIGN_BACK((wHeight - 480 * 2) / 2 + 480 * (iCnt / wSquare), 2);
            stChnAttr.stRect.u32Width   = ALIGN_BACK(wWidth / wSquare, 2);
            stChnAttr.stRect.u32Height  = ALIGN_BACK(480, 2);
            stChnAttr.u32Priority       = 0;
            stChnAttr.bDeflicker        = HI_FALSE;
        }
        else
        {
            stChnAttr.stRect.s32X       = ALIGN_BACK((wWidth / wSquare) * (iCnt % wSquare), 2);
            stChnAttr.stRect.s32Y       = ALIGN_BACK((wHeight / wSquare) * (iCnt / wSquare), 2);
            stChnAttr.stRect.u32Width   = ALIGN_BACK(wWidth / wSquare, 2);
            stChnAttr.stRect.u32Height  = ALIGN_BACK(wHeight / wSquare, 2);
            stChnAttr.u32Priority       = 0;
            stChnAttr.bDeflicker        = HI_FALSE;
        }

        iRet = HI_MPI_VO_SetChnAttr(VoLayer, iCnt, &stChnAttr);
        if (iRet != HI_SUCCESS)
        {
            printf("%s(%d):failed with %#x!\n", __FUNCTION__,__LINE__,  iRet);
            return HI_FAILURE;
        }

        iRet = HI_MPI_VO_EnableChn(VoLayer, iCnt);
        if (iRet != HI_SUCCESS)
        {
            printf("failed with %#x!\n", iRet);
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

/**
 * Video_stopVoChn:
 *
 */
int Video_VoChnStop(VO_LAYER VoLayer, E_VO_MODE enMode)
{
	int iCnt;
	int iRet = HI_SUCCESS;
	int wWndNum = 0;

	switch (enMode)
	{
		case VO_MODE_1MUX:
		{
			wWndNum = 1;
			break;
		}

		case VO_MODE_4MUX:
		{
			wWndNum = 4;
			break;
		}

		case VO_MODE_9MUX:
		{
			wWndNum = 9;
			break;
		}

		case VO_MODE_16MUX:
		{
			wWndNum = 16;
			break;
		}
		
		case VO_MODE_25MUX:
		{
			wWndNum = 25;
			break;
		}

		case VO_MODE_36MUX:
		{
			wWndNum = 36;
			break;
		}

		case VO_MODE_64MUX:
		{
			wWndNum = 64;
			break;
		}
		case VO_MODE_6MUX:
            wWndNum = 6;
            break;
		default:
			printf("failed with %#x!\n", iRet);
			return HI_FAILURE;
	}

	for (iCnt = 0; iCnt < wWndNum; iCnt++)
	{
		iRet = HI_MPI_VO_DisableChn(VoLayer, iCnt);
		if (iRet != HI_SUCCESS)
		{
			printf("failed with %#x!\n", iRet);
			return HI_FAILURE;
		}
	}	 
	return iRet;
}

/**
 * Video_VpssBindVo:
 *
 */
int Video_VpssBindVo(VO_LAYER VoLayer, VO_CHN iVoChn, VPSS_GRP iVpssGrp, VPSS_CHN iVpssChn)
{
    int iRet = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId  = HI_ID_VPSS;
    stSrcChn.s32DevId = iVpssGrp;
    stSrcChn.s32ChnId = iVpssChn;

    stDestChn.enModId  = HI_ID_VOU;
    stDestChn.s32DevId = VoLayer;
    stDestChn.s32ChnId = iVoChn;

    iRet = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (iRet != HI_SUCCESS)
    {
        printf("failed with %#x!\n", iRet);
        return HI_FAILURE;
    }

    return iRet;
}

/**
 * Video_stopVoLayer:
 *
 */
int Video_VpssUnbindVo(VO_LAYER VoLayer,VO_CHN iVoChn,VPSS_GRP iVpssGrp,VPSS_CHN iVpssChn)
{
    int iRet = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId  = HI_ID_VPSS;
    stSrcChn.s32DevId = iVpssGrp;
    stSrcChn.s32ChnId = iVpssChn;

    stDestChn.enModId  = HI_ID_VOU;
    stDestChn.s32DevId = VoLayer;
    stDestChn.s32ChnId = iVoChn;

    iRet = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (iRet != HI_SUCCESS)
    {
        printf("failed with %#x!\n", iRet);
        return HI_FAILURE;
    }
	
    return iRet;
}

/**
 * Video_VoHdmiChangeFormatStart:
 *
 */
int Video_VoHdmiChangeFormatStart(HI_HDMI_VIDEO_FMT_E enVideoFmt)
{
    HI_HDMI_ATTR_S stAttr;
    
    HI_MPI_HDMI_GetAttr(HI_HDMI_ID_0, &stAttr);
    stAttr.bEnableHdmi  = HI_TRUE;
    stAttr.bEnableVideo = HI_TRUE;
    stAttr.enVideoFmt   = enVideoFmt;
//    Video_VoHdmiConvertSync(enIntfSync, &stAttr.enVideoFmt);
    HI_MPI_HDMI_SetAttr(HI_HDMI_ID_0, &stAttr);
    HI_MPI_HDMI_Start(HI_HDMI_ID_0);
    
    return HI_SUCCESS;
}


