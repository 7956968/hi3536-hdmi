#ifndef __VIDEO_COMMON_H__
#define __VIDEO_COMMON_H__

#include "hi_comm_vb.h"
#include "hi_common.h"
#include "hi_comm_venc.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vo.h"


#define CHECK_CHN_RET(express,Chn,name)\
	do{\
		int Ret;\
		Ret = express;\
		if (HI_SUCCESS != Ret)\
		{\
			printf("\033[0;31m%s chn %d failed at %s: LINE: %d with %#x!\033[0;39m\n", name, Chn, __FUNCTION__, __LINE__, Ret);\
			fflush(stdout);\
			return Ret;\
		}\
	}while(0)

#define CHECK_RET(express,name)\
    do{\
        int Ret;\
        Ret = express;\
        if (HI_SUCCESS != Ret)\
        {\
            printf("\033[0;31m%s failed at %s: LINE: %d with %#x!\033[0;39m\n", name, __FUNCTION__, __LINE__, Ret);\
            return Ret;\
        }\
    }while(0)

#define ALIGN_UP(x, a)		((x+a-1)&(~(a-1)))
#define ALIGN_BACK(x, a)	((a) * (((x) / (a))))
		
#define VIDEO_ALIGN_WIDTH  16		
#define VO_BKGRD_BLUE           0x0000FF
		
#define HD_WIDTH                1920
#define HD_HEIGHT               1088
		
#define D1_WIDTH                720
#define D1_HEIGHT               576
		
#define VO_DEV_DHD0 0
#define VO_DEV_DHD1 1
#define VO_DEV_DSD0 2
#define VO_DEV_VIRT0 3
		
#define VO_LAYER_VHD0 0
#define VO_LAYER_VHD1 1
#define VO_LAYER_VSD0 3
#define VO_LAYER_VIRT0 4
#define VO_LAYER_VPIP 2

typedef enum eVoMode
{
	VO_MODE_1MUX  = 0,
	VO_MODE_4MUX = 1,
	VO_MODE_9MUX = 2,
	VO_MODE_16MUX = 3,	  
	VO_MODE_25MUX = 4,	  
	VO_MODE_36MUX = 5,
	VO_MODE_64MUX = 6,
	VO_MODE_6MUX = 7,
	VO_MODE_BUTT
}E_VO_MODE;

typedef enum hiVdecThreadCtrlSignal_E
{
	VDEC_CTRL_START,
	VDEC_CTRL_PAUSE,
	VDEC_CTRL_STOP,	
} VdecThreadCtrlSignal_E;

typedef struct hiVDEC_STREAM_PARAM_S
{
	HI_S32 s32ChnId;	
	PAYLOAD_TYPE_E enType;
	HI_S32 s32StreamMode;
	HI_S32 s32MilliSec;
	VdecThreadCtrlSignal_E eCtrlSinal;
	HI_U64	u64PtsInit;
	HI_S32 s32MinBufSize;
	HI_U8 *pu8Buffer;
	HI_U32 u32ReadLen;
} VDEC_STREAM_PARAM_S;

typedef struct hiHDMI_HOTPLUG_EVENT_S
{
	HI_HDMI_VIDEO_FMT_E enVideoFmt;
	VdecThreadCtrlSignal_E *penCtrlSignal;
	HI_BOOL bHdmiConnected;
	PIC_SIZE_E enPicSize;
} HDMI_HOTPLUG_EVENT_S;

void Video_getVdecModCommPoolConf(VB_CONF_S *ptModVbConf, PAYLOAD_TYPE_E eType, SIZE_S *ptSize, int iChnNum);

int Video_VdecModCommPoolInit(VB_CONF_S *ptModVbConf);

void Video_createVdecChnAttr(int iChnNum, VDEC_CHN_ATTR_S *ptVdecChnAttr, PAYLOAD_TYPE_E eType, SIZE_S *ptSize);

int Video_VdecStart(int iChnNum, VDEC_CHN_ATTR_S *pstAttr);

int Video_VdecStop(int iChnNum);

int Video_VpssStart(int iGrpCnt, SIZE_S *ptSize, int iChnCnt, VPSS_GRP_ATTR_S *ptVpssGrpAttr);

int Video_VpssStop(int iGrpCnt, int iChnCnt);

int Video_VdecBindVpss(VDEC_CHN VdChn, VPSS_GRP VpssGrp);

int Video_VdecUnbindVpss(VDEC_CHN VdChn, VPSS_GRP VpssGrp);

int Video_VoDevStart(VO_DEV VoDev, VO_PUB_ATTR_S *ptPubAttr);

int Video_VoDevStop(VO_DEV iVoDev);

int Video_VoHdmiConvertSync(HI_HDMI_VIDEO_FMT_E enVideoFmt, VO_INTF_SYNC_E *penIntfSync);

int Video_VoHdmiInit(HI_HDMI_CALLBACK_FUNC_S *pstCallbackFunc);

int Video_VoHdmiConfig(HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent);

int Video_VoHdmiExit(HI_HDMI_CALLBACK_FUNC_S *pstCallbackFunc);

int Video_getVoWH(VO_INTF_SYNC_E enIntfSync, HI_U32 *pu32W,HI_U32 *pu32H, HI_U32 *pu32Frm);

int Video_VoLayerStart(VO_LAYER iVoLayer,const VO_VIDEO_LAYER_ATTR_S *ptLayerAttr);

int Video_VoLayerStop(VO_LAYER iVoLayer);

int Video_VoChnStart(VO_LAYER iVoLayer, E_VO_MODE eMode);

int Video_VoChnStop(VO_LAYER iVoLayer, E_VO_MODE eMode);

int Video_VpssBindVo(VO_LAYER iVoLayer, VO_CHN iVoChn, VPSS_GRP iVpssGrp, VPSS_CHN iVpssChn);

int Video_VpssUnbindVo(VO_LAYER iVoLayer,VO_CHN iVoChn,VPSS_GRP iVpssGrp,VPSS_CHN iVpssChn);

int Video_VoHdmiChangeFormatStart(HI_HDMI_VIDEO_FMT_E enVideoFmt);

#endif /* __VIDEO_COMMON_H__ */

