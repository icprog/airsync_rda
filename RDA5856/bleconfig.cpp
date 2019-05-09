#include "bleconfig.h"
#include "rda58xx.h"

//extern rda58xx rda5856;//UART pins: tx, rx, xreset //RDA5991H_HDK_V1.0,5981A/C_HDK_V1.1
rda58xx rda5856(PD_3, PD_2, PC_9);//UART pins: tx, rx, xreset //RDA5991H_HDK_V1.0,5981A/C_HDK_V1.1




/*  This function checks if the codec is ready  */
bool BleConfig::isReady(void)
{
    return rda5856.isReady();
}

void BleConfig::setUartMode(void)
{
    rda58xx_at_status ret;
    ret = rda5856.setMode(UART_MODE);
    rda5856.atHandler(ret);
}

void BleConfig::setBtMode(void)
{
    rda58xx_at_status ret;
    ret = rda5856.setMode(BT_MODE);
    rda5856.atHandler(ret);
}

void BleConfig::switchToBleMode(void)
{
    rda58xx_at_status ret;
    ret = rda5856.setBtLeMode();
    rda5856.atHandler(ret);
}

void BleConfig::startBleConfigService(void)
{
    // len = 9 type = 9£¨set_local_name£© wifi_scs
    // len = 2 type = 1 (flags) 128bitµÄUUID
	unsigned char adv[31]={0x09, 0x09, 0x77, 0x69,0x66, 0x69, 0x5f, 0x73, 0x63, 0x73,      0x02, 0x01, 0x06};
    rda58xx_at_status ret;
    ret = rda5856.lesWifiScs(adv);
    rda5856.atHandler(ret);
}


void BleConfig::startBleConfigService(unsigned char * adv)
{
	//unsigned char adv[31]={0x09, 0x09, 0x77, 0x69,0x66, 0x69, 0x5f, 0x73, 0x63, 0x73, 0x02, 0x01, 0x06};
    rda58xx_at_status ret;
    ret = rda5856.lesWifiScs(adv);
    rda5856.atHandler(ret);
}

void BleConfig::setBtAddr(const unsigned char addr[12])
{
    rda58xx_at_status ret;
    ret = rda5856.SetAddr(addr);
    rda5856.atHandler(ret);
}


rda58xx_mode BleConfig::getMode(void)
{
    return rda5856.getMode();
}

void BleConfig::leGetConfigInfo(char *configInfo,int &nLen)
{
    rda58xx_at_status ret;   
    ret = rda5856.leGetConfigInfo(configInfo,nLen);
    rda5856.atHandler(ret);
}

int BleConfig::leGetConfigState(void)
{
    rda58xx_at_status ret;
	int8_t value = 0;
    ret = rda5856.leGetConfigState(&value);
    rda5856.atHandler(ret);
	return value;
}

int BleConfig::leGetIndState(void)
{
    rda58xx_at_status ret;
	int8_t value = 0;
    ret = rda5856.leGetIndState(&value);
    rda5856.atHandler(ret);
	return value;
}

int BleConfig::leClearConfigInfo(void)
{
    rda58xx_at_status ret;
	int8_t value = 0;
    ret = rda5856.leClearConfigInfo();
    rda5856.atHandler(ret);
	return value;
}


void BleConfig::leSetFeature(const char* state)
{
    rda58xx_at_status ret;
    ret = rda5856.leSetFeature(state);
    rda5856.atHandler(ret);
}


void BleConfig::leSetIndication(const char* state)
{
    rda58xx_at_status ret;
    ret = rda5856.leSetIndication(state);
    rda5856.atHandler(ret);
}

//add by lijun
void BleConfig::leSetIndication(char * state,int nLen)
{
    rda58xx_at_status ret;
    ret = rda5856.leSetIndication(state, nLen);
    rda5856.atHandler(ret);    
}
