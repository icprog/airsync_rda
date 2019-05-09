#ifndef BLECONFIG_H
#define BLECONFIG_H

#include "rda58xx.h"

class BleConfig
{
public:
    bool isReady(void);
    void setUartMode(void);
    void setBtMode(void);
    void switchToBleMode(void);
    void startBleConfigService(void);
    void startBleConfigService(unsigned char * adv);
    rda58xx_mode getMode(void);
    void setBtAddr(const unsigned char addr[12]);
    void leSetIndication(const char* state);
    void leSetIndication(char * state,int nLen);
    void leSetFeature(const char* state);
    int leGetConfigState(void);
    int leGetIndState(void);
    void leGetConfigInfo(char *configInfo,int &nLen);
    int leClearConfigInfo(void);

private:

};

#endif
