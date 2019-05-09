#include "mbed.h"
#include "bleconfig.h"
#include "WiFiStackInterface.h"
#include "pb_example.h"
BleConfig config;
Timer timer;


void SendReq(uint8_t * pProtoBuf,int nBufLen)
{
    const int nOnePackMaxLen = 20;
    uint8_t * pTempBuf= new uint8_t[nOnePackMaxLen];
    int nSendLen = 0;   
    printf("nBufLen = %d\n\r",nBufLen);
    int nSeq = 0;      
    while(nSendLen<nBufLen)
    {
        memset(pTempBuf,0,nOnePackMaxLen);     
        memcpy(pTempBuf,pProtoBuf+nSendLen,nSendLen + nOnePackMaxLen >nBufLen?nBufLen - nSendLen:nOnePackMaxLen);            
        printf("send package: %d \n",++nSeq);     
        for(int i =0;i< nOnePackMaxLen;i++)
        {
            printf("%x,",pTempBuf[i]);
        }
        printf("\n");                                  
        config.leSetIndication((char *)pTempBuf,nOnePackMaxLen);		            
        nSendLen += nOnePackMaxLen;
        Thread::wait(100);///(大坑)must
    } 
    delete []pTempBuf;  
}

void RecvRsp(uint8_t * pProtoBuf,int &nBufLen)
{
    while(1)
	{
		int configState = config.leGetConfigState();
		if(configState >= 1)
		{
		    printf("configState = %d \n",configState);			    			    
			config.leGetConfigInfo((char *)pProtoBuf,nBufLen);
			break;
		}
		Thread::wait(500);
	}
	config.leClearConfigInfo();    
}


int main()
{
	int ret = 0;
	int count = 0;
	int i = 0;
	
    timer.start();
	WiFiStackInterface wifi;	
	
	
/* * * * * * * *init 5856 * * * * * * * */	
    while(!config.isReady()) {
        printf("Codec Not Ready\n\r");
        Thread::wait(100);
    }    
    printf("1\n\r");
	config.setBtAddr((const unsigned char*)"998866001D76");
	//config.setBtAddr((const unsigned char*)"f4e6f41549f2");	
	Thread::wait(500);
	printf("2\n\r");
	
	
/* * * * * * * *send Adv * * * * * * * */		
	//config.startBleConfigService();
	//0x09, 0x09, 0x77, 0x69,0x66, 0x69, 0x5f, 0x73, 0x63, 0x73,      0x02, 0x01, 0x06
    unsigned char adv[31]=
    {
        /*
        #define BLE_GAP_ADV_FLAG_LE_LIMITED_DISC_MODE         (0x01)   < LE Limited Discoverable Mode. 
        #define BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE         (0x02)   < LE General Discoverable Mode. 
        #define BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED         (0x04)   < BR/EDR not supported. 
        #define BLE_GAP_ADV_FLAG_LE_BR_EDR_CONTROLLER         (0x08)   < Simultaneous LE and BR/EDR, Controller. 
        #define BLE_GAP_ADV_FLAG_LE_BR_EDR_HOST               (0x10)   < Simultaneous LE and BR/EDR, Host. 
        #define BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE   (BLE_GAP_ADV_FLAG_LE_LIMITED_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED)   < LE Limited Discoverable Mode, BR/EDR not supported. 
        #define BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE   (BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED)   < LE General Discoverable Mode, BR/EDR not supported. 
        */   
        //flags    
        //0x02,0x01,0x1a,///0x06,
        //UUID
        0x03,0x03,0xe7,0xfe,  
         //flags 
        0x02,0x01,0x02,//0x1a,///0x06,/// (大坑)rda must here         
        //MAC   f4e6f41549f2
        //0x09,0xff,0x00,0x00,0x0f4,0xe6,0xf4,0x15,0x49,0xf2,
        //MAC   998866001D76   
        #if 1 // 普通包
        0x09,0xff,0x00,0x00,0x99,0x88,0x66,0x00,0x1d,0x76,
        #else
        0x0c,0xff,0x00,0x00,0xfe,0x01,0x01,0x99,0x88,0x66,0x00,0x1d,0x76,
        #endif
    };   
    config.startBleConfigService(adv);
    printf("3\n\r");
	Thread::wait(500);
    if(BT_MODE != config.getMode())
    {
        config.setBtMode();
		while(config.getMode() != BT_MODE)
		{
		    printf("xxxx\n");
			Thread::wait(500);
		}
    }
    printf("4\n\r");
	Thread::wait(3500);	
	//Thread::wait(500);	
	config.switchToBleMode();
	//Thread::wait(3000);	
	//config.leSetFeature("778899001122");
    //config.leSetFeature("998866001D76");
    printf("5\n\r");
    
/* * * * * * * *send AuthReq * * * * * * * */	       
    Thread::wait(10000)  ;
    printf("begin send AuthReq\n");       
    do
    {  
        int nBufLen = 0;
        uint8_t * pProtoBuf = pack_auth_request(nBufLen);   
        SendReq(pProtoBuf,nBufLen);                        
        free(pProtoBuf);                                      
    }  while(0); 

/* * * * * * * *recv AuthRsp * * * * * * * */	    
    //Thread::wait(5000) ; 
    printf("begin recv AuthReq\n");  
	do
	{
	    unsigned char pProtoBuf[30];
	    int nBufLen = 0;
		memset(pProtoBuf, 0, sizeof(pProtoBuf));		
		RecvRsp(pProtoBuf,nBufLen);						
		printf("nBufLen = %d\n",nBufLen);
		for(int i = 0;i< 30;i++){printf("%x,",pProtoBuf[i]);}printf("\n");
		unpack_auth_response(pProtoBuf,nBufLen);
	}while(0);

/* * * * * * * *send InitRsq * * * * * * * */
    Thread::wait(1000)  ;
    printf("begin send InitReq\n");
	do
	{
		int nBufLen = 0;
        uint8_t * pProtoBuf = pack_init_request(nBufLen);   
        SendReq(pProtoBuf,nBufLen);                        
        free(pProtoBuf); 
	}while(0);
	
/* * * * * * * *recv InitRsp * * * * * * * */	
    //Thread::wait(5000) ; 
    printf("begin recv InitReq\n");  
	do
	{
	    unsigned char pProtoBuf[30];
	    int nBufLen = 0;
		memset(pProtoBuf, 0, sizeof(pProtoBuf));		
		RecvRsp(pProtoBuf,nBufLen);						
		printf("nBufLen = %d\n",nBufLen);
		for(int i = 0;i< 30;i++){printf("%x,",pProtoBuf[i]);}printf("\n");
		unpack_init_response(pProtoBuf,nBufLen);
	}while(0);
}

