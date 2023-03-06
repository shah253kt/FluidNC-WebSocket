#include <Arduino.h>

#ifndef FLUIDNC_WS_H_
#define FLUIDNC_WS_H_

#include <WebSocketsClient.h>
#include <SafeString.h>

    enum machinestate {
        Unknwn,
        Alarm,
        Idle,
        Jog,
        Home,
        Check,
        Run,
        Cycle,
        Hold,
        Door,
        Sleep
};

    enum FluidNC_WS_API {
        None,
        Raw,
        getGrblStat,
        getGrblFullStat
    };

class FluidNC_WS;

class FluidNC_WS
{
    public:
        FluidNC_WS();
        ~FluidNC_WS();

        bool connect(void);
        bool connect(const char* host, uint16_t port);
        bool isConnected();
        int activeID();
        void cmd( SafeString &command, SafeString &reply );
        void cmd(SafeString &command, SafeString &reply, bool async);
        void cmd2( SafeString &command, SafeString &reply );
        void getGrblState(bool full);

        void fluidCMD(const char c);
        void fluidCMD(const char *cmd);
        void SpindleOn();
        void SpindleOn(int speed);
        void SpindleOff();
        void SpindleOnOff();
        void SpindleOnOff(int speed);
        void SoftReset();
        void StatusQuery();
        void FeedHold();
        void CycleStartResume();
        void SafetyDoor();
        void JogCancel();
        
        void Unlock();
        void Unlock(bool async);
        void Home();
        void Home(bool async);
        void Reset();
        void Reset(bool async);

        machinestate mState();
        void set_mState(machinestate newState);

        float mX();
        float mY();
        float mZ();
        float wX();
        float wY();
        float wZ();

        float reportedSpindleSpeed();
        void  set_reportedSpindleSpeed(float newSpeed);

        float isSpindleOn();
        int ovSpeed();
        int ovFeed();
        int ovRapid();
};

#endif /* FLUIDNC_WS_H_ */