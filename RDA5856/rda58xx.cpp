/** \file rda58xx.c
 * Functions for interfacing with the codec chip.
 */
#include "rda58xx.h"
#include "rda58xx_dbg.h"
#include "rda58xx_int_types.h"
#include "mbed_interface.h"

#define CHAR_CR             (0x0D)  // carriage return
#define CHAR_LF             (0x0A)  // line feed

#define MUPLAY_PROMPT       "AT+MUPLAY="
#define TX_DATA_PROMPT      "AT+MURAWDATA="
#define RUSTART_PROMPT      "AT+RUSTART="

#define MUSTOP_CMD          "AT+MUSTOP="
#define MUSTOP_CMD2         "AT+MUSTOP\r\n"
#define RUSTOP_CMD          "AT+RUSTOP\r\n"
#define VOLADD_CMD          "AT+CVOLADD\r\n"
#define VOLSUB_CMD          "AT+CVOLSUB\r\n"
#define SET_VOL_CMD         "AT+CVOLUME="
#define MUPAUSE_CMD         "AT+MUPAUSE\r\n"
#define MURESUME_CMD        "AT+MURESUME\r\n"
#define SET_MIC_GAIN_CMD    "AT+MUSETMICGAIN="
#define FT_CMD              "AT+FTCODEC="
#define GET_STATUS_CMD      "AT+MUGETSTATUS\r\n"
#define GET_VERSION_CMD     "AT+CVERSION\r\n"
#define UART_MODE_CMD       "AT+CMODE=UART_PLAY\r\n"
#define BT_MODE_CMD         "AT+CMODE=BT\r\n"
#define BT_BR_MODE_CMD      "AT+BTSWITCH=0\r\n"
#define BT_LE_MODE_CMD      "AT+BTSWITCH=1\r\n"
#define GET_BT_RSSI_CMD     "AT+BTGETRSSI\r\n"
#define LEGETCONFIGSTATE_CMD "AT+LEGETCONFIGSTATE\r\n"
#define LESWIFISCS_CMD      "AT+LESWIFISCS\r\n"
#define LEGETCONFIGINFO_CMD "AT+LEGETCONFIGINFO\r\n"
#define LESETINDICATION_CMD "AT+LESETINDICATION=\r\n"
#define LESETFEATURE_CMD    "AT+LESETFEATURE=\r\n" 
#define LEGETINDSTATE_CMD   "AT+LEGETINDSTATE\r\n" 

#define RX_VALID_RSP        "\r\nOK\r\n"
#define RX_VALID_RSP2       "OK\r\n"
#define AT_READY_RSP        "AT Ready\r\n"
#define AST_POWERON_RSP     "AST_POWERON\r\n"
#define CODEC_READY_RSP     "CODEC_READY\r\n"

#define TIMEOUT_MS 5000

#define RDA58XX_DEBUG

#define RX_BUFFER_LEN 128

uint8_t tmpBuffer[64] = {0};
uint8_t tmpIdx = 0;
uint8_t res_str[100] = {0};

#define r_GPIO_DOUT (*((volatile uint32_t *)0x40001008))
#define USE_TX_INT
int32_t atAckHandler(int32_t status);

void stringTohex(unsigned char *destValue, const char* src, int stringLen)
{
    int i = 0;
    int j = 0;

	for(; i<stringLen; i+=2, j++)
	{
		if(src[i] >= '0' && src[i] <= '9')
			destValue[j] = (src[i] - 0x30) << 4;
		else if(src[i] >= 'A' && src[i] <= 'F')
			destValue[j] = (src[i] - 0x37) << 4;
		else if(src[i] >= 'a' && src[i] <= 'f')
			destValue[j] = (src[i] - 0x57) << 4;

		if(src[1+i] >= '0' && src[1+i] <= '9')
			destValue[j] |= src[1+i]-0x30;
		else if(src[1+i] >= 'A' && src[1+i] <= 'F')
			destValue[j] |= src[1+i]-0x37;
		else if(src[1+i] >= 'a' && src[1+i] <= 'f')
			destValue[j] |= src[1+i]-0x57;
	}
}


void hexToString(char *destValue, const unsigned char* src, int hexLen)
{
    int i = 0; 
    int j = 0;
    int temp;
    for(; i<hexLen; i++,j+=2)
    {
        temp = (src[i]&0xf0)>>4;
        if(temp >=0 && temp <=9)
        {
            destValue[j] = 0x30+temp;
        }
        else
        {
            destValue[j] = temp+0x37;
        }
        
        temp = (src[i]&0x0f);
        if(temp >=0 && temp <=9)
        {
            destValue[j+1] = 0x30+temp;
        }
        else
        {
            destValue[j+1] = temp+0x37;
        }
    }

}



/** Constructor of class RDA58XX. */
rda58xx::rda58xx(PinName TX, PinName RX, PinName HRESET):
    _serial(TX, RX),
    _baud(3200000),
    _rxsem(0),
    _HRESET(HRESET),
    _mode(UART_MODE),
    _status(UNREADY),
    _rx_buffer_status(EMPTY),
    _rx_buffer_size(640),
    _tx_buffer_size(0),
    _rx_idx(0),
    _tx_idx(0),
    _rx_buffer(NULL),
    _tx_buffer(NULL),
    _with_parameter(false),
    _at_status(NACK),
    _ready(false),
    _power_on(false),
    _ats(0),
    _atr(0)
{
    _serial.baud(_baud);
    _serial.attach(this, &rda58xx::rx_handler, RawSerial::RxIrq);
#if defined(USE_TX_INT)
    _serial.attach(this, &rda58xx::tx_handler, RawSerial::TxIrq);
    RDA_UART1->IER = RDA_UART1->IER & (~(1 << 1));//disable UART1 TX interrupt
    //RDA_UART1->FCR = 0x31;//set UART1 TX empty trigger FIFO 1/2 Full(0x21 1/4 FIFO)
#endif
    _HRESET = 1;
    _rx_buffer = new uint8_t [_rx_buffer_size * 2];
    _parameter.buffer = new uint8_t[RX_BUFFER_LEN];
    _parameter.bufferSize = 0;
    atHandler = atAckHandler;
}

rda58xx::~rda58xx()
{
    if (_rx_buffer != NULL)
        delete [] _rx_buffer;
    if (_parameter.buffer != NULL)
        delete [] _parameter.buffer;
}

void rda58xx::hardReset(void)
{
    _HRESET = 0;
    wait_ms(1);
    _mode = UART_MODE;
    _status = UNREADY;
    _rx_buffer_status = EMPTY;
    _tx_buffer_size = 0;
    _rx_idx = 0;
    _tx_idx = 0;
    _with_parameter = false;
    _at_status = NACK;
    _ready = false;
    _power_on = false;
    _ats=0;
    _atr=0;
    _HRESET = 1;
}

/** Send raw data to RDA58XX  */
rda58xx_at_status rda58xx::sendRawData(uint8_t *databuf, uint16_t n)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    if ((NULL == databuf) || (0 == n))
        return VACK;

    // Send AT CMD: MURAWDATA
    len = sprintf((char *)cmd_str, "%s%d\r\n", TX_DATA_PROMPT, n);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;
    if (VACK != _at_status) {
        CODEC_LOG(ERROR, "AT ACK=%d\n", _at_status);
        return _at_status;
    }

    // Raw Data bytes
    CODEC_LOG(INFO, "SendData\r\n");
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;

#if defined(USE_TX_INT)
    core_util_critical_section_enter();
    _tx_buffer = databuf;
    _tx_buffer_size = n;
    _tx_idx = 0;
    RDA_UART1->IER = RDA_UART1->IER | (1 << 1);//enable UART1 TX interrupt
#else
    while(n--) {
        _serial.putc(*databuf);
        databuf++;
    }
#endif
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

void rda58xx::setBaudrate(int32_t baud)
{
    _serial.baud(baud);
}

void rda58xx::setStatus(rda58xx_status status)
{
    _status = status;
}

rda58xx_status rda58xx::getStatus(void)
{
    return _status;
}

/* Set mode, BT mode or Codec mode. In case of switching BT mode to Codec mode,
   set _status to MODE_SWITCHING and wait for "UART_READY" after receiving "OK" */
rda58xx_at_status rda58xx::setMode(rda58xx_mode mode)
{
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    if (UART_MODE== mode) {
        // Send AT CMD: UART_MODE
        CODEC_LOG(INFO, UART_MODE_CMD);
        _status = MODE_SWITCHING;
        _ready = false;
        core_util_critical_section_enter();
        _serial.puts((const char *)UART_MODE_CMD);
        _ats++;
        core_util_critical_section_exit();
    } else if (BT_MODE== mode) {
        // Send AT CMD: BT_MODE
        CODEC_LOG(INFO, BT_MODE_CMD);
        core_util_critical_section_enter();
        _serial.puts((const char *)BT_MODE_CMD);
        _ats++;
        core_util_critical_section_exit();
    }
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;
    if (VACK == _at_status) {
        _mode = mode;
        if (BT_MODE == mode)
            _ready = true;
    }

    return _at_status;
}

rda58xx_mode rda58xx::getMode(void)
{
    return _mode;
}

rda58xx_at_status rda58xx::setBtBrMode(void)
{
    // Send AT CMD: BT_BR_MODE
    CODEC_LOG(INFO, BT_BR_MODE_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)BT_BR_MODE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::setBtLeMode(void)
{
    // Send AT CMD: BT_LE_MODE
    CODEC_LOG(INFO, BT_LE_MODE_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)BT_LE_MODE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::lesWifiScs(const unsigned char *adv)
{
	char cmd[120] = {0};
    // Send AT CMD: LESWIFISCS
	char advString[63] = {0};
	
	hexToString(advString, adv, 31);
    CODEC_LOG(INFO, LESWIFISCS_CMD);
	sprintf(cmd, "AT+LESWIFISCS=%s\r\n", advString);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::leGetConfigState(int8_t *configState)
{
    CODEC_LOG(INFO, LEGETCONFIGSTATE_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)LEGETCONFIGSTATE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.value = 0;
        for (uint32_t i =0; i < _parameter.bufferSize; i++) {
            if ((_parameter.buffer[i] >= '0') && (_parameter.buffer[i] <= '9')) {
                _parameter.value *= 10;
                _parameter.value += _parameter.buffer[i] - '0';
            }
        }
        _parameter.bufferSize = 0;
        *configState = (int8_t)(_parameter.value & 0xFF);
		//CODEC_LOG(INFO, "LEGETCONFIGSTATE:%d\n", *configState);
    }

    return _at_status;
}

rda58xx_at_status rda58xx::leGetIndState(int8_t *indState)
{

    CODEC_LOG(INFO, LEGETINDSTATE_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)LEGETINDSTATE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.value = 0;
        for (uint32_t i =0; i < _parameter.bufferSize; i++) {
            if ((_parameter.buffer[i] >= '0') && (_parameter.buffer[i] <= '9')) {
                _parameter.value *= 10;
                _parameter.value += _parameter.buffer[i] - '0';
            }
        }
        _parameter.bufferSize = 0;
        *indState = (int8_t)(_parameter.value & 0xFF);
		CODEC_LOG(INFO, "LEGETINDSTATE:%d\n", *indState);
    }

    return _at_status;
}


rda58xx_at_status rda58xx::leGetConfigInfo(char *configInfo,int &nLen)
{
	char value[256] = {0};
    CODEC_LOG(INFO, LEGETCONFIGINFO_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)LEGETCONFIGINFO_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.buffer[_parameter.bufferSize] = '\0';
        memcpy(value, &_parameter.buffer[0], (_parameter.bufferSize + 1));
        _parameter.bufferSize = 0;
    }

	stringTohex((unsigned char*)configInfo, value, strlen(value));
	nLen = strlen(value)/2;	
    return _at_status;
}

rda58xx_at_status rda58xx::leClearConfigInfo(void)
{
	char cmd[60] ="AT+LECLEARCONFIGINFO\r\n";

    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}


rda58xx_at_status rda58xx::leSetIndication(const char* state)
{
	char cmd[60] ={0};
	char tmp[60] ={0};
	hexToString(tmp, (const unsigned char*)state, strlen(state));
	sprintf(cmd, "AT+LESETINDICATION=%s\r\n", tmp);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}


rda58xx_at_status rda58xx::leSetIndication(const char* state,int nLen)
{
 	char cmd[128] ={0};
	char tmp[128] ={0};
	hexToString(tmp, (const unsigned char*)state, nLen);
	sprintf(cmd, "AT+LESETINDICATION=%s\r\n", tmp);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;   
}

rda58xx_at_status rda58xx::SetAddr(const unsigned char* addr)
{
	char cmd[60] ={0};

	sprintf(cmd, "AT+BTSETADDR=%s\r\n", addr);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}



rda58xx_at_status rda58xx::leSetFeature(const char* features)
{
	char cmd[120] ={0};
	char temp[120] ={0};

	hexToString(temp, (const unsigned char*)features, strlen(features));
	
	sprintf(cmd, "AT+LESETFEATURE=%s\r\n", temp);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}


bool rda58xx::isReady(void)
{
    return _ready;
}

bool rda58xx::isPowerOn(void)
{
    return _power_on;
}

int32_t atAckHandler(int32_t status)
{
    if (VACK != (rda58xx_at_status) status) {
        switch ((rda58xx_at_status) status) {
        case NACK:
            CODEC_LOG(ERROR, "No ACK for AT command!\n");
            break;
        case IACK:
            CODEC_LOG(ERROR, "Invalid ACK for AT command!\n");
            break;
        default:
            CODEC_LOG(ERROR, "Unknown ACK for AT command!\n");
            break;
        }
        Thread::wait(osWaitForever);
    }

    return 0;
}

int32_t rda58xx::setAtHandler(int32_t (*handler)(int32_t status))
{
    if (NULL != handler) {
        atHandler = handler;
        return 0;
    } else {
        CODEC_LOG(ERROR, "Handler is NULL!\n");
        return -1;
    }
}

void rda58xx::rx_handler(void)
{
    uint8_t count = (RDA_UART1->FSR >> 9) & 0x7F;

    count = (count <= (_rx_buffer_size - _rx_idx)) ? count : (_rx_buffer_size - _rx_idx);
    while (count--) {
        tmpBuffer[tmpIdx] = (uint8_t) (RDA_UART1->RBR & 0xFF);
        tmpIdx++;
    }

    memcpy(&_rx_buffer[_rx_idx], tmpBuffer, tmpIdx);
    _rx_idx += tmpIdx;

    switch (_status) {
    case RECORDING:
        if (_rx_buffer_size == _rx_idx) {
            memcpy(&_rx_buffer[_rx_buffer_size], &_rx_buffer[0], _rx_buffer_size);
            _rx_buffer_status = FULL;
            _rx_idx = 0;
        }
        break;
        
    case STOP_RECORDING:
#ifdef RDA58XX_DEBUG
        mbed_error_printf("%d\n", _rx_idx);
#endif
        if ((_rx_idx >= 4) && (_rx_buffer[_rx_idx - 1] == '\n')) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 4], 4);
            res_str[4] = '\0';
            if (strstr(RX_VALID_RSP2, (char *)res_str) != NULL) {
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
                _rx_idx = 0;
                _status = PLAY;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
            }
        }
        if (_rx_buffer_size == _rx_idx) {
            memcpy(&_rx_buffer[0], &_rx_buffer[_rx_buffer_size - 10], 10);
            _rx_idx = 10;
        }
        break;

    case UNREADY:
        if ((_rx_idx >= 6) && ('\n' == _rx_buffer[_rx_idx - 1])) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 6], 6);
            res_str[6] = '\0';
            if (strstr(CODEC_READY_RSP, (char *)res_str) != NULL) {
                _ready = true;
                _power_on = true;
                _rx_idx = 0;
                _status = STOP;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
            } else if (strstr(AST_POWERON_RSP, (char *)res_str) != NULL) {
                _power_on = true;
                _rx_idx = 0;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif                
            } else if (strstr(RX_VALID_RSP, (char *)res_str) != NULL) {
                if (_with_parameter) {
                    _parameter.bufferSize = ((_rx_idx - 10) < RX_BUFFER_LEN) ? (_rx_idx - 10) : RX_BUFFER_LEN;
                    memcpy(&_parameter.buffer[0], &_rx_buffer[2], _parameter.bufferSize);
                }
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
                _rx_idx = 0;
            } else {
#if 1
                _rx_buffer[_rx_idx] = '\0';
                mbed_error_printf("%s\n", _rx_buffer);
#else
                for (uint32_t i = 0; i < _rx_idx; i++) {
                    mbed_error_printf("%02X#", _rx_buffer[i]);
                }
#endif
            }
        }
        break;

    case MODE_SWITCHING:
        if ((_rx_idx >= 4) && ('\n' == _rx_buffer[_rx_idx - 1])) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 4], 4);
            res_str[4] = '\0';
            if (strstr(RX_VALID_RSP2, (char *)res_str) != NULL) {
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
                _status = UNREADY;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
                _rx_idx = 0;
            } else {
#if 1
                _rx_buffer[_rx_idx] = '\0';
                mbed_error_printf("%s", _rx_buffer);
#else
                for (uint32_t i = 0; i < _rx_idx; i++) {
                    mbed_error_printf("%02X#", _rx_buffer[i]);
                }
#endif
                _at_status = IACK;
                _status = STOP;
            }
        }
        break;
    
    default:
        if ((_rx_idx >= 4) && ('\n' == _rx_buffer[_rx_idx - 1])) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 4], 4);
            res_str[4] = '\0';
            if (strstr(RX_VALID_RSP2, (char *)res_str) != NULL) {
                if (_with_parameter) {
                    _parameter.bufferSize = ((_rx_idx - 10) < RX_BUFFER_LEN) ? (_rx_idx - 10) : RX_BUFFER_LEN;
                    memcpy(&_parameter.buffer[0], &_rx_buffer[2], _parameter.bufferSize);
                }
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
                _rx_idx = 0;
            } else {
#if 1
                _rx_buffer[_rx_idx] = '\0';
                mbed_error_printf("%s", _rx_buffer);
#else
                for (uint32_t i = 0; i < _rx_idx; i++) {
                    mbed_error_printf("%02X#", _rx_buffer[i]);
                }
#endif
                _at_status = IACK;
            }
        }
        break;
    }

    tmpIdx = 0;

    if (_rx_idx >= _rx_buffer_size)
        _rx_idx = 0;
}

void rda58xx::tx_handler(void)
{
    if ((_tx_buffer != NULL) || (_tx_buffer_size != 0)) {
        uint8_t n = ((_tx_buffer_size - _tx_idx) < 64) ? (_tx_buffer_size - _tx_idx) : 64;
        while (n--) {
            RDA_UART1->THR = _tx_buffer[_tx_idx];
            _tx_idx++;
        }

        if (_tx_idx == _tx_buffer_size) {
            RDA_UART1->IER = (RDA_UART1->IER) & (~(0x01L << 1));//disable UART1 TX interrupt
            _tx_buffer = NULL;
            _tx_buffer_size = 0;
            _tx_idx = 0;
        }
    } else {
        RDA_UART1->IER = (RDA_UART1->IER) & (~(0x01L << 1));//disable UART1 TX interrupt
        _tx_buffer_size = 0;
        _tx_idx = 0;
        return;
    }
}

