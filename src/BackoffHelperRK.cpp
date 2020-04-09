#include "BackoffHelperRK.h"

// Global retained data for the global BackoffHelper object. This uses 8 bytes of retained RAM.
static retained BackoffHelperRetained builtInRetainedData;

// Global BackoffHelper object. This is declared extern in the .h file.
BackoffHelperClass BackoffHelper(&builtInRetainedData);


const uint8_t BackoffHelperClass::standardBackoffTable[] = { 5, 10, 15, 20, 30, 60 };

BackoffHelperClass::BackoffHelperClass(BackoffHelperRetained *retainedData) :
    backoffTable(standardBackoffTable), backoffTableNumElem(sizeof(standardBackoffTable)), retainedData(retainedData) {

}

BackoffHelperClass::~BackoffHelperClass() {
  
}

BackoffHelperClass &BackoffHelperClass::withTable(const uint8_t *backoffTable, size_t backoffTableNumElem) {
    this->backoffTable = backoffTable;
    this->backoffTableNumElem = backoffTableNumElem;

    return *this;
}

BackoffHelperClass &BackoffHelperClass::withDefaultTable() {

    this->backoffTable = standardBackoffTable;
    this->backoffTableNumElem = sizeof(standardBackoffTable);

    return *this;
}


void BackoffHelperClass::success() {
    validate();
    retainedData->tries = 0;
}

int BackoffHelperClass::getFailureSleepTimeSecs() {
    int result;

    validate();
    if (retainedData->tries < backoffTableNumElem) {
        result = (int)(backoffTable[retainedData->tries] * 60);
    }
    else {
        result = (int)(backoffTable[backoffTableNumElem - 1] * 60);
    }
    retainedData->tries++;
    return result;
}

uint16_t BackoffHelperClass::getNumTries() {
    validate();
    return retainedData->tries;
}


void BackoffHelperClass::validate() {
    if (retainedData->magic != BACKOFFHELPER_RETAINED_MAGIC ||
        retainedData->version != BACKOFFHELPER_RETAINED_VERSION) {
        retainedData->magic = BACKOFFHELPER_RETAINED_MAGIC;
        retainedData->version = BACKOFFHELPER_RETAINED_VERSION;
        retainedData->tries = 0;
        retainedData->reserved = 0; 
    }
}


