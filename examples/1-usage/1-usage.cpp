// Wake Publish Sleep Cellular Example

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
SerialLogHandler logHandler;

// This is the maximum amount of time to wait for the cloud to be connected in
// milliseconds. This should be at least 5 minutes. If you set this limit shorter,
// on Gen 2 devices the modem may not get power cycled which may help with reconnection.
// If you go into SLEEP_MODE_DEEP on failure, you can set this to be a little shorter,
// 4 to 4.5 minutes.
const unsigned long CONNECT_MAX_MS = 4 * 60 * 1000;

// This is the minimum amount of time to stay connected to the cloud. You can set this
// to zero and the device will sleep as fast as possible, however you may not get 
// firmware updates and device diagnostics won't go out all of the time. Setting this
// to 5 seconds is a good starting place.
const unsigned long CLOUD_MIN_MS = 5 * 1000;

// How long to sleep in seconds. This code uses SLEEP_MODE_DEEP and is best suited
// for sleeping longer than 15 minutes. If less, see the stop mode sleep with 
// SLEEP_NETWORK_STANDBY for more efficient operation.
// Using a time of less than 10 minutes may result in your SIM being banned from
// your mobile provider for aggressive reconnection behavior.
const long SLEEP_SECS = 15 * 60;

// Maximum amount of time to wait for a user firmware download in milliseconds
// before giving up and just going back to sleep
const unsigned long FIRMWARE_UPDATE_MAX_MS = 5 * 60 * 60;

// These are the states in the finite state machine, handled in loop()
enum State {
    STATE_WAIT_CONNECTED = 0,
    STATE_PUBLISH,
    STATE_PRE_SLEEP,
    STATE_SLEEP,
    STATE_FIRMWARE_UPDATE
};
State state = STATE_WAIT_CONNECTED;
unsigned long stateTime;
bool firmwareUpdateInProgress = false;
int sleepSecs = SLEEP_SECS;

void readSensorAndPublish(); // forward declaration
void firmwareUpdateHandler(system_event_t event, int param); // forward declaration

void setup() {
    FuelGauge fuel;
    if (fuel.getSoC() < 15) {
        // If battery is too low, don't try to connect to cellular, just go back into
        // sleep mode.
        Log.info("low battery, going to sleep immediately");
        state = STATE_SLEEP;
        return;
    }

    System.on(firmware_update, firmwareUpdateHandler);

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

                // Successfully connected, reset default sleep value and clear the 
                // cellular backoff timer
                sleepSecs = SLEEP_SECS;
                BackoffHelper.success();

                state = STATE_PUBLISH; 
                stateTime = millis(); 
            }
            else
            if (millis() - stateTime >= CONNECT_MAX_MS) {
                // Took too long to connect, go to sleep using a back-off 
                // of 5, 10, 15, 20, 30, then 60 minutes.
                sleepSecs = BackoffHelper.getFailureSleepTimeSecs();

                Log.info("failed to connect, going to sleep");
                state = STATE_SLEEP;
            }
            break;

        case STATE_PUBLISH:
            readSensorAndPublish();

            if (millis() - stateTime < CLOUD_MIN_MS) {
                Log.info("waiting %lu ms before sleeping", CLOUD_MIN_MS - (millis() - stateTime));
                state = STATE_PRE_SLEEP;
            }
            else {
                state = STATE_SLEEP;
            }
            break;

        case STATE_PRE_SLEEP:
            // This delay is used to make sure firmware updates can start and diagnostics go out
            // It can be eliminated by setting CLOUD_MIN_MS to 0 and sleep will occur as quickly
            // as possible. 
            if (millis() - stateTime >= CLOUD_MIN_MS) {
                state = STATE_SLEEP;
            }
            break;

        case STATE_SLEEP:
            if (firmwareUpdateInProgress) {
                Log.info("firmware update detected");
                state = STATE_FIRMWARE_UPDATE;
                stateTime = millis();
                break;
            }

            Log.info("going to sleep for %lu seconds", sleepSecs);
#if HAL_PLATFORM_NRF52840
            // Gen 3 (nRF52840) does not suppport SLEEP_MODE_DEEP with a time in seconds
            // to wake up. This code uses stop mode sleep instead. 
            System.sleep(WKP, RISING, sleepSecs);
            System.reset();
#else
            System.sleep(SLEEP_MODE_DEEP, sleepSecs);
            // This is never reached; when the device wakes from sleep it will start over 
            // with setup()
#endif
            break; 

        case STATE_FIRMWARE_UPDATE:
            if (!firmwareUpdateInProgress) {
                Log.info("firmware update completed");
                state = STATE_SLEEP;
            }
            else
            if (millis() - stateTime >= FIRMWARE_UPDATE_MAX_MS) {
                Log.info("firmware update timed out");
                state = STATE_SLEEP;
            }
            break;
    }
}

void readSensorAndPublish() {
    // This is just a placeholder for code that you're write for your actual situation
    int a0 = analogRead(A0);

    // Create a simple JSON string with the value of A0
    char buf[256];
    snprintf(buf, sizeof(buf), "{\"a0\":%d}", a0);

    bool result = Particle.publish("sensorTest", buf, PRIVATE | WITH_ACK);

    Log.info("published %s (result=%d)", buf, result);
}

void firmwareUpdateHandler(system_event_t event, int param) {
    switch(param) {
        case firmware_update_begin:
            firmwareUpdateInProgress = true;
            break;

        case firmware_update_complete:
        case firmware_update_failed:
            firmwareUpdateInProgress = false;
            break;
    }
}
