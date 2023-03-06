
#include <Arduino.h>

// Config file contains Wi-Fi connection details and FluidNC hostname/port
#include "Config.h"

#include <Wifi.h>
#include <FluidNC_WebSocket.h>

const char *fluidnc_host = "fluidnc.local";
const uint16_t fluidnc_port = 81;

FluidNC_WS myCNC = FluidNC_WS();

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.flush();
    delay(3000);

    WiFi.begin(ssid, password);
 
    while ( WiFi.status() != WL_CONNECTED ) {
      delay ( 500 );
      Serial.print ( "." );
    }
    Serial.print("\n[SETUP] Local IP: "); Serial.println(WiFi.localIP());

    bool conn = myCNC.connect(fluidnc_host, fluidnc_port);
    if (!conn)
    {
        Serial.println("[SETUP] Not connected.");
    }
    else 
    {
        Serial.printf("[SETUP] Connected! ID:%d\n", myCNC.activeID());
        // Test Command
        cSF(setup_test,1,"\n");
        cSF (myReply, 5);
        myCNC.cmd2(setup_test, myReply);
        Serial.printf("[SETUP] ID:%d CNC Reply: ", myCNC.activeID());
        Serial.println(myReply);
    }
}

void loop() {
    myCNC.getGrblState(true);
    Serial.printf("Work coordinate X=%d\n", myCNC.wX());
    delay (1000);
}