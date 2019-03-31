#ifndef __VIDEO_HDMI_H__
#define __VIDEO_HDMI_H__

#include "hi_type.h"
#include "hi_common.h"
#include "hi_comm_hdmi.h"
#include "video_common.h"

int Video_SysInit(PIC_SIZE_E enPicSize);
void Video_SysExit(void);
int Video_HdmiCreate(HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent);
int Video_HdmiDestroy(HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent);
int Video_Init(HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent);
void Video_Exit(void);
int Video_StreamParamaConf(HDMI_HOTPLUG_EVENT_S *pstHdmiHotplugEvent, VDEC_STREAM_PARAM_S *pstVdecStreamParam);
int Video_VdecStartSendStream(VDEC_STREAM_PARAM_S *pstVdecStreamParam);
void Video_VdecStopSendStream(VDEC_STREAM_PARAM_S *pstVdecStreamParam);

#endif /* __VIDEO_HDMI_H__ */

