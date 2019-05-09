
#ifndef MPBLEDEMO2
#define MPBLEDEMO2

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "epb_MmBp.h"
#include "ble_wechat_util.h"



#define CMD_NULL 0
#define CMD_AUTH 1
#define CMD_INIT 2
#define CMD_SENDDAT 3

#define DEVICE_TYPE "gh_1bafe245c2cb"
#define DEVICE_ID "WeChatBluetoothDevice"


#define PROTO_VERSION 0x010004
#define AUTH_PROTO 1

#define MAC_ADDRESS_LENGTH 6

#define EAM_md5AndNoEnrypt 1     //��֤��ʽֻ�ܶ������е�һ��
//#define EAM_md5AndAesEnrypt 1
//#define EAM_macNoEncrypt 2


#define DEVICE_KEY {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};

#ifdef EAM_macNoEncrypt
	#define AUTH_METHOD EAM_macNoEncrypt
	#define MD5_TYPE_AND_ID_LENGTH 0
	#define CIPHER_TEXT_LENGTH 0
#endif

#ifdef EAM_md5AndAesEnrypt
	#define AUTH_METHOD EAM_md5AndAesEnrypt
	#define MD5_TYPE_AND_ID_LENGTH 16
	#define CIPHER_TEXT_LENGTH 16
#endif
#ifdef EAM_md5AndNoEnrypt
	#define AUTH_METHOD EAM_md5AndNoEnrypt
	#define MD5_TYPE_AND_ID_LENGTH 16
	#define CIPHER_TEXT_LENGTH 0
#endif

#define CHALLENAGE_LENGTH 4

#define MPBLEDEMO2_MAGICCODE_H 0xfe
#define MPBLEDEMO2_MAGICCODE_L 0xcf
#define MPBLEDEMO2_VERSION 0x01
//#define SEND_HELLO_WECHAT "Hello, WeChat!"
#define SEND_HELLO_WECHAT "Hello, WeChat!"

/* Hardware Resources define */
#define MPBLEDEMO2_LIGHT 19
#define MPBLEDEMO2_BUTTON_1 17

typedef enum
{
	errorCodeUnpackAuthResp = 0x9990,
	errorCodeUnpackInitResp = 0x9991,
	errorCodeUnpackSendDataResp = 0x9992,
	errorCodeUnpackCtlCmdResp = 0x9993,
	errorCodeUnpackRecvDataPush = 0x9994,
	errorCodeUnpackSwitchViewPush = 0x9995,
	errorCodeUnpackSwitchBackgroundPush = 0x9996,
	errorCodeUnpackErrorDecode = 0x9997,
}mpbledemo2UnpackErrorCode;
typedef enum
{
	errorCodeProduce = 0x9980,
}mpbledemo2PackErrorCode;
typedef enum
{
	sendTextReq = 0x01,
	sendTextResp = 0x1001,
	openLightPush = 0x2001,
	closeLightPush = 0x2002,
}BleDemo2CmdID;

typedef struct
{
	uint8_t m_magicCode[2];
	uint16_t m_version;
	uint16_t m_totalLength;
	uint16_t m_cmdid;
	uint16_t m_seq;
	uint16_t m_errorCode;
}BlueDemoHead;

typedef struct 
{
	int cmd;
	CString send_msg;
} mpbledemo2_info;

 typedef struct 
{
	bool wechats_switch_state; //�����˺��л���ǰ̨��״̬
	bool indication_state;
	bool auth_state;
	bool init_state;
	bool auth_send;
	bool init_send;
	unsigned short send_data_seq;
	unsigned short push_data_seq;
	unsigned short seq; 
}mpbledemo2_state;
extern data_handler mpbledemo2_data_handler;
#endif
