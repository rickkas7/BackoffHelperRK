// Cellular back-off with no sleep example

// Public domain (CC0) 
// Can be used in open or closed-source commercial projects and derivative works without attribution.

// Tested with Device OS 1.4.4
// - Electron U260
// - Boron LTE

#include "Particle.h"

#include "BackoffHelperRK.h"

// This example uses threading enabled and SEMI_AUTOMATIC mode
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

// Use the USB serial port for debugging logs
Serial1LogHandler logHandler(9600);

// This is the maximum amount of time to wait for the cloud to be connected in
// milliseconds. This should be at least 5 minutes. If you set this limit shorter,
// on Gen 2 devices the modem may not get power cycled which may help with reconnection.
// If you go into SLEEP_MODE_DEEP on failure, you can set this to be a little shorter,
// 4 to 4.5 minutes.
const unsigned long CONNECT_MAX_MS = 6 * 60 * 1000;

// These are the states in the finite state machine, handled in loop()
enum State {
    STATE_WAIT_CONNECTED = 0,
    STATE_RUNNING,
    STATE_WAIT_RETRY
};
State state = STATE_WAIT_CONNECTED;
unsigned long stateTime;
unsigned long retryMs = 0;


void setup() {
    // It's only necessary to turn cellular on and connect to the cloud. Stepping up
    // one layer at a time with Cellular.connect() and wait for Cellular.ready() can
    // be done but there's little advantage to doing so.
    Cellular.on();
    Particle.connect();
    stateTime = millis();
}

void loop() {
    switch(state) {
        case STATE_WAIT_CONNECTED:
            // Wait for the connection to the Particle cloud to complete
            if (Particle.connected()) {
                Log.info("connected to the cloud in %lu ms", millis() - stateTime);

                // Successfully connected, clear the cellular backoff timer
                BackoffHelper.success();

                state = STATE_RUNNING; 
                stateTime = millis(); 
            }
            else
            if (millis() - stateTime >= CONNECT_MAX_MS) {
                // Took too long to connect, go to stop connecting using a back-off 
                // of 5, 10, 15, 20, 30, then 60 minutes.
                retryMs = (unsigned long) BackoffHelper.getFailureSleepTimeSecs() * 1000;

                Log.info("failed to connect, turning off cellular, retrying in %lu ms", retryMs);
                
                Cellular.off();
                state = STATE_WAIT_RETRY;
                stateTime = millis();
            }
            break;

        case STATE_RUNNING:
            if (!Particle.connected()) {
                state = STATE_WAIT_CONNECTED;
                stateTime = millis();
            }
            break;

        case STATE_WAIT_RETRY:
            if (millis() - stateTime >= retryMs) {
                // Try to connect again
                Log.info("retrying connection");
                Cellular.on();
                Particle.connect();

                state = STATE_WAIT_CONNECTED;
                stateTime = millis();
            }
            break;
    }
}
