#include <Arduino.h>

#if !defined( ESP32 )
	#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

//#define DEBUG

// These define's must be placed at the beginning before #include "ESP32_New_TimerInterrupt.h"
#define _TIMERINTERRUPT_LOGLEVEL_     1

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include "ESP32TimerInterrupt.h"
volatile uint32_t TimerCount = 0;
#define TIMER_TIMEOUT_INTERVAL_MS        10000

#include <FluidNC_WebSocket.h>

WebSocketsClient webSocket;
ESP32Timer ITimer(0);

volatile bool webSocket_connected = false;
volatile bool isProcessingDone = false;
volatile FluidNC_WS_API myCMD = None;
const char *fluidnc_host_default = "fluidnc.local";
const uint16_t fluidnc_port_default = 81;

#define MAX_LEN 65

#define DEBUG_SERIAL Serial

volatile bool _isConnected = false;
volatile int _activeID = -1;
machinestate _mState = Unknwn;
int _reportedSpindleSpeed;
int _setSpindleSpeed;
bool _spindleOn;
volatile float _ovRapid;
volatile float _ovFeed;
int _ovSpeed;
double _mX, _mY, _mZ; // Machine position
double _wX, _wY, _wZ; // Work positions

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
bool isConnected_internal(void);
void cmd_internal( SafeString &command );
void _getGrblState(bool full, const void *mem, uint32_t len);
bool IRAM_ATTR TimerHandler(void * timerNo);

FluidNC_WS::FluidNC_WS()
{
    #ifdef DEBUG
        Serial.setDebugOutput(true);
    #else
        Serial.setDebugOutput(false);
    #endif
}

FluidNC_WS::~FluidNC_WS() {}

bool FluidNC_WS::connect(void)
{
    return connect(fluidnc_host_default, fluidnc_port_default);
}

bool FluidNC_WS::connect(const char* host, uint16_t port)
{
    DEBUG_SERIAL.printf("[FluidNC_WS::connect] %s\n",ARDUINO_BOARD);
	DEBUG_SERIAL.printf("[FluidNC_WS::connect] %s\n",ESP32_TIMER_INTERRUPT_VERSION);
	DEBUG_SERIAL.printf("[FluidNC_WS::connect] CPU Frequency = %d MHz\n",F_CPU / 1000000);

    // Interval in microsecs
	if (ITimer.attachInterruptInterval(TIMER_TIMEOUT_INTERVAL_MS * 1000, TimerHandler))
	{
		DEBUG_SERIAL.printf("[FluidNC_WS::connect] Starting  ITimer OK, millis() = %d\n", millis());
	}
	else
		DEBUG_SERIAL.println("[FluidNC_WS::connect] Can't set ITimer. Select another freq. or timer");

    // server address, port and URL
    webSocket.begin(host, port, "/");
 
    // event handler
    webSocket.onEvent(webSocketEvent);

    bool retVal = false;
    if (! isConnected_internal() )
    {
        DEBUG_SERIAL.println("[FluidNC_WS::connect] Not connected.");
    }
    else
    {
        DEBUG_SERIAL.printf("[FluidNC_WS::connect] Connected! ID:%d\n", _activeID);
        retVal = true;
    }

    return retVal;
}

bool FluidNC_WS::isConnected()
{
    return _isConnected;
}

machinestate FluidNC_WS::mState()
{
    return _mState;
}

void FluidNC_WS::set_mState(machinestate newState) {_mState = newState;}
float FluidNC_WS::mX() {return _mX;}
float FluidNC_WS::mY() {return _mY;}
float FluidNC_WS::mZ() {return _mZ;}
float FluidNC_WS::wX() {return _wX;}
float FluidNC_WS::wY() {return _wY;}
float FluidNC_WS::wZ() {return _wZ;}
float FluidNC_WS::reportedSpindleSpeed() {return _reportedSpindleSpeed;}
void  FluidNC_WS::set_reportedSpindleSpeed(float newSpeed) {_reportedSpindleSpeed=newSpeed;}
float FluidNC_WS::isSpindleOn() {return _spindleOn;}
int FluidNC_WS::ovSpeed() {return _ovSpeed;}
int FluidNC_WS::ovFeed() {return _ovFeed;}
int FluidNC_WS::ovRapid() {return _ovRapid;}

bool isConnected_internal()
{
    unsigned long millisCheckInterval = 200;
    unsigned long lastUpdate = millis();

    TimerCount=0;
    while (TimerCount==0)
    {
        webSocket.loop();
        if (webSocket_connected && (lastUpdate+millisCheckInterval<millis())){
            if (_isConnected) break;
            lastUpdate = millis();
        }
    }
    if (TimerCount>0)
        DEBUG_SERIAL.printf("[FluidNC_WS::isConnected_internal] Timed out. TimerCount=%d\n", TimerCount);
    
    return _isConnected;
}

int FluidNC_WS::activeID()
{
    return _activeID;
}

void FluidNC_WS::fluidCMD(const char c)
{
    if ( this->isConnected() ) {
      cSF(safe_cmd, 2);
      safe_cmd.concat(c).concat("\n");
      cSF (myReply, 65);
      this->cmd2(safe_cmd, myReply);
    }
}

void FluidNC_WS::fluidCMD(const char *cmd)
{
    if ( this->isConnected() ) {
      cSF(safe_cmd, 65);
      safe_cmd.concat(cmd).concat("\n");
      cSF (myReply, 65);
      this->cmd2(safe_cmd, myReply);
    }
}

void FluidNC_WS::getGrblState(bool full)
{
    myCMD = full ? getGrblFullStat : getGrblStat;
    cSF(cmd,2,"?\n");
    cmd_internal(cmd);
}

void FluidNC_WS::cmd2( SafeString &command, SafeString &reply )
{
    myCMD = Raw;
    DEBUG_SERIAL.printf("[FluidNC_WS::cmd2] Sending command: ");
    DEBUG_SERIAL.println(command);
    cmd_internal(command);
}

void FluidNC_WS::cmd( SafeString &command, SafeString &reply )
{
    cmd_internal(command);
    noInterrupts();
//    reply.print(cmd_out);
    interrupts();
}

void cmd_internal( SafeString &command )
{
    unsigned long millisCheckInterval = 500;
    unsigned long lastUpdate = millis();

    if (webSocket_connected)
    {
        //DEBUG_SERIAL.printf("[FluidNC_WS::cmd] SENT CMD: %s\n", command);
        isProcessingDone = false;
        char cmd[65];
        //cmd = (const char *)command.c_str();
        webSocket.sendTXT( command.c_str() );
    }

    lastUpdate = millis();
    TimerCount=0;
    while (TimerCount==0)
    {
        webSocket.loop();
        if (webSocket_connected && (lastUpdate+millisCheckInterval<millis()))
        {
            if (isProcessingDone)
            {
                //DEBUG_SERIAL.println("[FluidNC_WS::cmd] READING LAST CMD REPLY.");
                //DEBUG_SERIAL.printf("[FluidNC_WS::cmd] Reply: %s\n", cmd_out);
                isProcessingDone = false;
                break;
            }
            lastUpdate = millis();
        }
    }
    if (TimerCount>0)
        DEBUG_SERIAL.printf("[FluidNC_WS::cmd_internal] Timed out. TimerCount=%d\n", TimerCount);    
}

void get_payload (const void *mem, uint32_t len, SafeString &reply)
{
    const uint8_t* src = (const uint8_t*) mem;
    uint32_t buf_len=0;
    uint32_t max_len = len;
    if (max_len>MAX_LEN-1) max_len=MAX_LEN-1;

    for(uint32_t i = 0; i < max_len; i++) {
        buf_len = i;
        if (*src=='\n') {break;}
//        DEBUG_SERIAL.printf("%02X ", *src);
        reply += (char)*src;
        src++;
    }
    reply += '\0';
}

void _executeFluidNC_Command(const void *mem, uint32_t len)
{
    uint32_t max_len = len;
    if (max_len>MAX_LEN-1) max_len=MAX_LEN-1;
    createSafeString(cmdReply, max_len);
    get_payload(mem, len, cmdReply);

    DEBUG_SERIAL.print("[_FluidNC_WS::_executeFluidNC_Command] CMD Reply: ");
    DEBUG_SERIAL.println(cmdReply);
}

void FluidNC_WS::SpindleOff() {this->fluidCMD((const char *)"M5");}
void FluidNC_WS::SoftReset() {this->fluidCMD(18);}
void FluidNC_WS::StatusQuery() {this->fluidCMD((const char *)"?");}
void FluidNC_WS::FeedHold() {this->fluidCMD((const char *)"!");}
void FluidNC_WS::CycleStartResume() {this->fluidCMD((const char *)"~");}
void FluidNC_WS::SafetyDoor() {this->fluidCMD(84);}
void FluidNC_WS::JogCancel() {this->fluidCMD(85);}
void FluidNC_WS::Unlock(bool async) {}
void FluidNC_WS::Unlock() {this->fluidCMD((const char *)"$X");}
void FluidNC_WS::Home(bool async) {}
void FluidNC_WS::Home() {this->fluidCMD((const char *)"$H");}
void FluidNC_WS::Reset() {this->fluidCMD(24);}

void FluidNC_WS::SpindleOnOff() {this->SpindleOnOff(3000);}
void FluidNC_WS::SpindleOn() {this->SpindleOn(3000);}

void FluidNC_WS::SpindleOnOff(int speed)
{
    if (this->isSpindleOn()) this->SpindleOff();
    else this->SpindleOn(speed);
}

void FluidNC_WS::SpindleOn(int speed)
{
    char sbuf[20];
    sprintf(sbuf, "M3 S%d", speed);
    DEBUG_SERIAL.printf("[FluidNC_WS::SpindleOn] set spindle speed command: %s.\n", sbuf);
    this->fluidCMD(sbuf);
}

void print_payload (const void *mem, uint32_t len)
{
    const uint8_t* src = (const uint8_t*) mem;
    for(uint32_t i = 0; i < len; i++) {
//        DEBUG_SERIAL.printf("%02X ", *src);
        DEBUG_SERIAL.printf("%c", *src);
        src++;
    }
    DEBUG_SERIAL.printf("\n");
}

static bool chkVerb(String verb, String msg)
{
    return (msg.length() > verb.length()) && msg.startsWith( verb );
}

static void check_if_connected(const char* message)
{
    String verbs[] = {"CURRENT_ID:", "ACTIVE_ID:", "PING:"};
    String cnc_MSG(message);
    cnc_MSG.trim();
    for (int i=0; i < 3; i++)
    {
        if ( chkVerb(verbs[i],cnc_MSG) )
        {
            String s_ID = String(cnc_MSG[cnc_MSG.length()-1]);
            int tmp = s_ID.toInt(); 
            String recon = verbs[i] + String(tmp);
            if (recon.equals(cnc_MSG))
            {
                _isConnected = true;
                _activeID = tmp;
                break;
            }
        }
    }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
 
    switch(type) {
        case WStype_DISCONNECTED:
            DEBUG_SERIAL.printf("[webSocketEvent] Disconnected!\n");
            webSocket_connected = false;
            break;
        case WStype_CONNECTED:
            DEBUG_SERIAL.printf("[webSocketEvent] Connected to url: %s\n", payload);
            webSocket_connected = true;
            break;
        case WStype_TEXT:
            //DEBUG_SERIAL.printf("[webSocketEvent] TXT RESPONSE: %s\n", payload);
            check_if_connected((const char*)payload);
            break;
        case WStype_BIN:
            //DEBUG_SERIAL.printf("[webSocketEvent] get binary length: %u\n", length);
//            hexdump(payload, length);
//            print_payload(payload, length);
            //get_payload(payload, length);
            switch (myCMD)
                {
                    case getGrblStat:
                        _getGrblState(false, payload, length);
                        break;
                    case getGrblFullStat:
                        _getGrblState(true, payload, length);
                        break;
                    case Raw:
                        _executeFluidNC_Command(payload, length);
                        break;
                }
            isProcessingDone = true;
            myCMD = None;
            break;
        case WStype_PING:
            // pong will be send automatically
            DEBUG_SERIAL.printf("[webSocketEvent] get ping\n");
            break;
        case WStype_PONG:
            // answer to a ping we send
            DEBUG_SERIAL.printf("[webSocketEvent] get pong\n");
            break;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
        break;
    }
}

bool IRAM_ATTR TimerHandler(void * timerNo)
{
	TimerCount++;
	return true;
}

#include "GRBL_Parse.h"
