#include "BackoffHelperRK.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler;

const uint32_t TESTRETAINED_MAGIC = 0x72cf7281;
typedef struct {
    uint32_t magic;
    uint16_t state;
    uint16_t reserved;
} TestRetained;

retained static TestRetained testRetained;

retained static BackoffHelperRetained testRetained2;

enum {
    STATE_START = 0,
    STATE_SLEEP1,
    STATE_TABLE1,
    STATE_WAIT
};

static const int expectedValue[] = { 5 * 60, 10 * 60, 15 * 60, 20 * 60, 30 * 60, 60 * 60 };

static const uint8_t table2[] = { 10, 20, 60 }; 
static const int expectedValue2[] = { 10 * 60, 20 * 60, 60 * 60 };

#define ASSERT_TRUE(expr) if (!(expr)) { Log.error("assertion failed line %u", __LINE__, (int)(expected)); }

#define ASSERT_INT(expected, value) if ((expected) != (value)) { Log.error("assertion failed line %u %d != %d", __LINE__, (int)(expected), (int)(value)); }

unsigned long stateTime = 0;

void setup() {
    // Wait for a USB serial connection for up to 15 seconds
    waitFor(Serial.isConnected, 15000);    

    // Wait a little longer
    delay(3000);

    if (testRetained.magic != TESTRETAINED_MAGIC) {
        Log.info("Resetting retained data");
        testRetained.magic = TESTRETAINED_MAGIC;
        testRetained.state = 0;
        testRetained.reserved = 0;
    }
}

void loop() {
    switch(testRetained.state) {
    case STATE_START:
        // Initialize to a known state
        Log.info("running STATE_START");

        BackoffHelper.success();

        ASSERT_INT(0, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[0], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(1, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[1], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(2, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[2], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(3, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[3], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(4, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[4], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(5, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[5], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(6, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[5], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(7, BackoffHelper.getNumTries());

        BackoffHelper.success();
        ASSERT_INT(0, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[0], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(1, BackoffHelper.getNumTries());

        testRetained.state = STATE_SLEEP1;

#if HAL_PLATFORM_NRF52840
        // Gen 3 (nRF52840) does not suppport SLEEP_MODE_DEEP with a time in seconds
        // to wake up. This code uses stop mode sleep instead. 
        System.sleep(WKP, RISING, 10);
        System.reset();
#else
        System.sleep(SLEEP_MODE_DEEP, 10);
        // This is never reached; when the device wakes from sleep it will start over 
        // with setup()
#endif

        break;

    case STATE_SLEEP1:
        Log.info("running STATE_SLEEP1");
        ASSERT_INT(1, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[1], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(2, BackoffHelper.getNumTries());

        BackoffHelper.withTable(table2, sizeof(table2));
        BackoffHelper.success();

        ASSERT_INT(0, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue2[0], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(1, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue2[1], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(2, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue2[2], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(3, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue2[2], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(4, BackoffHelper.getNumTries());

        BackoffHelper.withDefaultTable();
        BackoffHelper.success();

        ASSERT_INT(0, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[0], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(1, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[1], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(2, BackoffHelper.getNumTries());

        ASSERT_INT(expectedValue[2], BackoffHelper.getFailureSleepTimeSecs());
        ASSERT_INT(3, BackoffHelper.getNumTries());

        // Test using a separate custom table and separate counter
        {
            BackoffHelperClass test2(&testRetained2);
            static const uint8_t table2[] = { 1, 2, 4};

            test2.withTable(table2, sizeof(table2));

            ASSERT_INT(0, test2.getNumTries());
            ASSERT_INT(table2[0] * 60, test2.getFailureSleepTimeSecs());
            ASSERT_INT(1, test2.getNumTries());
            ASSERT_INT(3, BackoffHelper.getNumTries());

            ASSERT_INT(table2[1] * 60, test2.getFailureSleepTimeSecs());
            ASSERT_INT(2, test2.getNumTries());

            ASSERT_INT(table2[2] * 60, test2.getFailureSleepTimeSecs());
            ASSERT_INT(3, test2.getNumTries());

            ASSERT_INT(table2[2] * 60, test2.getFailureSleepTimeSecs());
            ASSERT_INT(4, test2.getNumTries());

            test2.success();
            ASSERT_INT(0, test2.getNumTries());
            ASSERT_INT(3, BackoffHelper.getNumTries());
        }     


        Log.info("tests complete!");
        testRetained.state = STATE_WAIT;
        stateTime = millis();
        break;

    case STATE_WAIT:
        if (millis() - stateTime >= 30000) {
            // After 30 seconds, run the test again
            Log.info("re-running tests");
            testRetained.state = STATE_START;
        }
        break;

    }
}
