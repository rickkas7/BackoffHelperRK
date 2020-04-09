#ifndef __BACKOFFHELPERRK_H
#define __BACKOFFHELPERRK_H

// Github: https://github.com/rickkas7/BackoffHelperRK
// License: MIT

#include "Particle.h"

/**
 * @brief This structure is stored in retained memory
 */
typedef struct { // 8 bytes
    uint32_t    magic;
    uint8_t     version;
    uint8_t     reserved;
    uint16_t    tries;
} BackoffHelperRetained;


/**
 * @brief Class to implement a cellular connection failure backoff algorithm
 * 
 * The default behavior is 5, 10, 15, 20, 30, then 60 minutes but you can
 * supply a custom table if desired.
 * 
 * You normally do not instantiate this object. A global object `BackoffHelper`
 * is created in the library and exposed, so you can use the Wiring API style
 * like `BackoffHelper.success();`.
 * 
 * You can, however, create your own retained BackoffHelperRetained structure
 * as a global and pass it to the constructor. This allow for multiple separate
 * back-off counters, if you need that feature.
 */
class BackoffHelperClass {
public:
    /**
     * @brief Constructs the object with default settings
     * 
     * @param retainedData pointer to a global BackoffHelperRetained structure in retained memory.
     * 
     * You will normally use the global `BackoffHelper` object and not construct
     * your own object.
     * 
     * The default backoff times are: 5, 10, 15, 20, 30, then 60 minutes. You can change this using withTable().
     */
    BackoffHelperClass(BackoffHelperRetained *retainedData);

    /**
     * @brief Destructor. 
     * 
     * This class is usually a global object and not deleted.
     */
    virtual ~BackoffHelperClass();

    /**
     * @brief Use a custom backoff table
     * 
     * @param backoffTable pointer to an array of uint8_t variables containing wait periods in minutes.
     * Thus the maximum wait time is 255 minutes, but that's 4.25 hours and should be sufficient for
     * most cases.
     * 
     * @param backoffTableNumElem size of the table in elements. Since elements are uint8_t, it's 
     * also the number of bytes so you can conveniently use sizeof(backoffTable) in your code.
     */
    BackoffHelperClass &withTable(const uint8_t *backoffTable, size_t backoffTableNumElem);

    /**
     * @brief Sets the backoff table to the default table
     * 
     * You don't need to call this unless you use withTable() and then want to set the default back.
     */
    BackoffHelperClass &withDefaultTable();

    /**
     * @brief Call this to clear the tries counter so the next failure will start off with a short delay
     */
    void success();

    /**
     * @brief Call this on failure to get the amount of time to sleep (or wait) in seconds
     * 
     * @return sleep or wait time in seconds
     * 
     * Note that the table is in minute, but the value returned by this function is in seconds since you
     * usually pass it to System.sleep() which takes seconds.
     */
    int getFailureSleepTimeSecs();

    /**
     * @brief Get the current number of tries
     * 
     * This is zero if the last call was success and increases as the number of times getFailureSleepTimeSecs()
     * is called. This will increase beyond backoffTableNumElem even though the value of getFailureSleepTimeSecs()
     * stops increasing at the last element of backoffTable.
     */
    uint16_t getNumTries();

    /**
     * @brief Used internally to validate the retained memory
     * 
     * This is called from success(), getFailureSleepTimeSecs(), and getNumTries(). If the retained memory is not
     * valid then the number of tries is set to 0.
     */
    void validate();

    /**
     * @brief Random magic bytes used to see if the retained memory is valid
     */
    static const uint32_t BACKOFFHELPER_RETAINED_MAGIC = 0x5d7ec708;

    /**
     * @brief Version number of the retained data structure
     */
    static const uint8_t BACKOFFHELPER_RETAINED_VERSION = 1;

    /**
     * @brief  Backoff times in minutes, used when the default contructor is used
     * 
     * Default values are: { 5, 10, 15, 20, 30, 60 }
     */
    static const uint8_t standardBackoffTable[];

protected:
    /**
     * @brief Pointer to an array of int delay times in minutes 
     * 
     * Default constructor sets this to standardBackoffTable which is 5, 10, 15, 20, 30, then 60 minutes.
     * 
     * Note that getFailureSleepTimeSecs() returns seconds but the table is in minutes!
     */
    const uint8_t *backoffTable;

    /**
     * @brief Number of elements in the backoffTable. Default is 6.
     * 
     * Since the table is uint8_t (1 byte), the number of elements is the same as the number of bytes.
     */
    size_t backoffTableNumElem;

    /**
     * @brief This is the data stored in retained memory (8 bytes)
     */
    BackoffHelperRetained *retainedData;
};

extern BackoffHelperClass BackoffHelper;

#endif /* __BACKOFFHELPERRK_H */
