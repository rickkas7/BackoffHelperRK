# BackoffHelper library

When a connection failure occurs you should use a back-off algorithm to avoid excessive
reconnection to the cellular network. Aggressive reconnection behavior can result in your
SIM being blocked by your mobile provider.

The recommended times are: 5, 10, 15, 20, 30, then 60 minutes.

In battery powered situations you'd typically go into `SLEEP_MODE_DEEP` for the waiting
period. 

If you have a constantly running application, you'd typically use `SYSTEM_MODE(SEMI_AUTOMATIC)`
and use `Cellular.on()` and `Cellular.off()` to stop connecting during the back-off period.

This library keeps track of the number of tries in an 8-byte retained memory block so it
is maintained when using `SLEEP_MODE_DEEP` to easily implement the suggested back-off.

This library is intended for use in fixed locations. If you have an application that is used in 
a moving vehicle you may need to use a different algorithm that takes into account the exact 
type of failure. For example, if there is no tower visible at all, you don't need to back off.

Also, while the pre-programmed settings are designed for cellular back-off, you can supply a 
custom table for use with any back-off algorithm.

## Usage

Add the BackoffHelper library and include the header file in your main source file:

```
#include "BackoffHelperRK.h"
```

If you successfully connect to the cloud, call `BackoffHelper.success()` to reset
the failure tries counter. The `BackoffHelper` object is declared as a global object 
in BackoffHelperRK.h so you not need declare it. 

If you fail to connect to the cloud, after a period of time (4-6 minutes recommended) 
call `BackoffHelper.getFailureSleepTimeSecs()` to find out how long you should
sleep (in seconds).

```
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

    Log.info("failed to connect, going to sleep for %d seconds", sleepSecs);
    state = STATE_SLEEP;
}
```

A full example app is in the 1-usage example.

