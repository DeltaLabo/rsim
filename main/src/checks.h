#ifndef CHECKS_H
#define CHECKS_H

#include "adafruit-io.h"
#include "slm.h"
#include "battery-checker.h"

// Run the checks after Serial.begin()
bool runtimeChecks() {
    if (LEQ_PERIOD < 1.0) { // seconds
        Serial.println( "[ERROR] [SLM]: LEQ_PERIOD is too short. Values between 1 and 60 seconds are recommended.");
        return false;
    }

    if (LOGGING_PERIOD/LEQ_PERIOD < 10.0) {
        Serial.println( "[ERROR] [SLM]: LOGGING_PERIOD must be at least 10 times the LEQ_PERIOD.");
        return false;
    }

    if (MIC_OFFSET_DB < 0.0) {// dBA
        Serial.println( "[ERROR] [SLM]: MIC_OFFSET_DB can't be negative.");
        return false;
    }

    /*
    In theory, the only reason to limit LOGGING_PERIOD and LEQ_PERIOD is to avoid overflows
    when calculating sums of squares.
    This leads to very large upper limits.
    In practice, long measurement periods before calculating noise level values are not
    acceptable, since the device would update the current noise level and indicate it
    to users too infrequently to be of any use.

    The following are checks for absolute maximum limits of LOGGING_ and LEQ_PERIOD:

    double MAX_DOUBLE = 1.7e308;
    #define MIC_DATA_BITS 24
    float MAX_FLOAT_SQR = pow((pow(2, MIC_DATA_BITS) - 1), 2);

    if (LOGGING_PERIOD/LEQ_PERIOD >= MAX_DOUBLE/(3*MAX_FLOAT_SQR)) {
        Serial.println( "[ERROR] [SLM]: LOGGING_PERIOD is too long. Values between 1 and 10 minutes are recommended."
    }

    if (LEQ_PERIOD*SAMPLES_SHORT >= MAX_DOUBLE/(3*MAX_FLOAT_SQR)) {
        Serial.println( "[ERROR] [SLM]: LEQ_PERIOD is too long. Values between 1 and 60 seconds are recommended."
    }

    Instead, the following checks are used to keep the values within reasonable operational limits:
    */

    // Note that the allowed range is larger than the recommended one
    if ((LOGGING_PERIOD > 60.0*10.0) || (LOGGING_PERIOD < 5.0)) { // seconds
        Serial.println("[ERROR] [SLM]: LOGGING_PERIOD is out of range. Values between 1 and 10 minutes are recommended.");
        return false;
    }

    if ((LEQ_PERIOD > 60.0) || (LEQ_PERIOD < 0.125)) { // seconds
        Serial.println("[ERROR] [SLM]: LEQ_PERIOD is out of range. Values between 1 and 60 seconds are recommended.");
        return false;
    }

    #define NUMBER_OF_LOGGED_VARIABLES 3

    if (LOGGING_QUEUE_SIZE <= (NUMBER_OF_LOGGED_VARIABLES*HTTP_RESPONSE_TIMEOUT/(LOGGING_PERIOD*1000) + (HTTP_RESPONSE_TIMEOUT/BATTERY_CHECK_PERIOD + 1))) {
        Serial.println("[ERROR] [LOGGING]: LOGGING_QUEUE_SIZE is too small.");
        return false;
    }

    if ((BATTERY_CHECK_PERIOD > 180000) || (BATTERY_CHECK_PERIOD < 30000)) { // milliseconds
        Serial.println("[ERROR] [SLM]: BATTERY_CHECK_PERIOD is out of range. Values between 30 and 180 seconds are recommended.");
        return false;
    }

    return true; // No errors
}

#endif // CHECKS_H