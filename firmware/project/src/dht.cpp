//
// FILE: dht.cpp
// PURPOSE: DHT Temperature & Humidity Sensor library for Arduino
// LICENSE: GPL v3 (http://www.gnu.org/licenses/gpl.html)
//
// DATASHEET: http://www.micro4you.com/files/sensor/DHT11.pdf
//
// HISTORY:
// George Hadjikyriacou - Original version (??)
// Mod by SimKard - Version 0.2 (24/11/2010)
// Mod by Rob Tillaart - Version 0.3 (28/03/2011)
// + added comments
// + removed all non DHT11 specific code
// + added references
// Mod by Rob Tillaart - Version 0.4 (17/03/2012)
// + added 1.0 support
// Mod by Rob Tillaart - Version 0.4.1 (19/05/2012)
// + added error codes
//

#include "dht.h"


DHT::DHT(uint8_t power_pin, uint8_t data_pin, uint8_t type) {
    this->data_pin = data_pin;
    this->power_pin = power_pin;
    this->type = type;
    this->lastReadTime = 0;
    this->lastRes = DHTLIB_ERROR_NOT_INIT;

    this->hum = 127;
    this->temp = 127;
    this->hum_dec = 127;
    this->temp_dec = 127;

}


void DHT::begin() {

    //pinMode(this->power_pin, OUTPUT);
    //digitalWrite(this->power_pin, 1);

    this->booted = 0;
    this->lastReadTime = millis();
}


void DHT::end() {

    //digitalWrite(this->power_pin, 0);

    pinMode(data_pin, OUTPUT);
    digitalWrite(this->data_pin, 1);
}

// Return values:
// DHTLIB_OK
// DHTLIB_ERROR_CHECKSUM
// DHTLIB_ERROR_TIMEOUT
uint8_t DHT::read_dht()
{

    if (this->booted == 0) {
        while (millis() < this->lastReadTime + SENS_BOOT_TIME);
        this->booted = 1;
    }

    unsigned long ctime = millis();

    if (
            (ctime - lastReadTime) < MIN_READ_INTERVAL
            && lastRes != DHTLIB_ERROR_NOT_INIT
       )
    {
        return lastRes;
    }

    // BUFFER TO RECEIVE
    char res = DHTLIB_OK;
    uint8_t cnt = 7;
    uint8_t idx = 0;

    uint8_t sum;
    uint16_t tmp;
    int16_t tmp2;

    int16_t loopCnt;
    unsigned long pmillis, pmicros;
    int i;

    // EMPTY BUFFER
    for (i=0; i< 5; i++) {
        bits[i] = 0;
    }

    // REQUEST SAMPLE
    pinMode(data_pin, OUTPUT);
    digitalWrite(data_pin, LOW);
    delay(18);
    digitalWrite(data_pin, HIGH);
    delayMicroseconds(40);
    pinMode(data_pin, INPUT);


    //Wait that dht data_pin is low
    pmillis = millis();
    while(digitalRead(data_pin) == LOW) {
        if (millis() - pmillis > DTH_READ_TIMEOUT) {
            res = DHTLIB_ERROR_TIMEOUT;
            break;
        }
    }

    //Wait that dht data_pin becomes high
    if (res == DHTLIB_OK) {
        pmillis = millis();
        while(digitalRead(data_pin) == HIGH) {
            if (millis() - pmillis > DTH_READ_TIMEOUT) {
                res = DHTLIB_ERROR_TIMEOUT;
                break;
            }
        }
    }

    if (res == DHTLIB_OK) {
        // READ OUTPUT - 40 BITS => 5 BYTES or TIMEOUT
        for (i=0; i<40; i++)
        {

            pmillis = millis();
            while(digitalRead(data_pin) == LOW) {
                if (millis() - pmillis > DTH_READ_TIMEOUT) {
                    res = DHTLIB_ERROR_TIMEOUT;
                    break;
                }
            }

            pmicros = micros();
            pmillis = millis();
            while(digitalRead(data_pin) == HIGH) {
                if (millis() - pmillis > DTH_READ_TIMEOUT) {
                    res = DHTLIB_ERROR_TIMEOUT;
                    break;
                }
            }

            if ((micros() - pmicros) > 40) {
                bits[idx] |= (1 << cnt);
            }

            if (cnt == 0) {
                cnt = 7;    // restart at MSB
                idx++;      // next byte!
            }
            else cnt--;
        }

        sum = bits[0] + bits[1] + bits[2] + bits[3];
        if (bits[4] != sum) {
            lastRes = DHTLIB_ERROR_CHECKSUM;
            res = DHTLIB_ERROR_CHECKSUM;
        }
    }

    if (res == DHTLIB_OK) {

        if (type == SENS_DHT12) {
            hum = bits[0];
            hum_dec = bits[1];
            temp = bits[2];
            temp_dec = bits[3];
        }
        else if (type == SENS_DHT22) {

            tmp2 = (int16_t)(bits[2]<<8 | bits[3]) * 0.1;

            tmp = bits[0];
            tmp = tmp << 8;
            tmp += bits[1];
            hum = tmp / 10;
            hum_dec = tmp % 10;

            tmp = bits[2];
            tmp = tmp << 8;
            tmp += bits[3];
            temp = tmp / 10;
            temp_dec = tmp % 10;
        }
        else {
            res = DHTLIB_ERROR_CFG;
        }

        if (
            temp < -30
            || temp > 80
            || temp_dec < 0
            || temp_dec > 100
            || hum < 1
            || hum > 100
            || hum_dec < 0
            || hum_dec > 99
            )
        {
            res = DHTLIB_ERROR_VALUE;
        }


    }

    lastReadTime = millis();
    lastRes = res;
    return res;
}
