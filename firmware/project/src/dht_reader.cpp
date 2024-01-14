
#include "Arduino.h"
#include "dht_reader.h"

#ifdef ADAFRUIT_DHT_LIB

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

DHT _dht(GPIO_NUM_33, DHT22);

DHT_reader::DHT_reader() {
}

void DHT_reader::begin() {
    _dht.begin();
}



float DHT_reader::read() {
    sensors_event_t event;
    _dht.temperature().getEvent(&event);
    return event.temperature;
}


int8_t DHT_reader::get_raw(uint8_t * buff, size_t len) {
    return 1;
};

#else

#include "dht.h"

DHT _dht = DHT(255, 33, SENS_DHT12);

DHT_reader::DHT_reader() {
}

void DHT_reader::begin() {
    _dht.begin();
}



float DHT_reader::read() {

    float res = NAN;

    if (_dht.read_dht() == DHTLIB_OK) {
        res = (float)_dht.temp_dec;
        res = res / 10;
        res += _dht.temp;
    }

    return res;
}


int8_t DHT_reader::get_raw(uint8_t * buff, size_t len) {

    uint8_t i = 0, j = 0;

    //print last res
    i += snprintf(
        (char *)&(buff[i]),
        len - i,
        "%d ",
        _dht.lastRes
    );

    //print bytes received from sensor
    for(j; j<5 && i < len; j++) {
        i += snprintf(
                (char *)&(buff[i]),
                len - i,
                "%d,",
                _dht.bits[j]
            );
    }

    //check if we exceeded len
    if (i < len) {
        //remove last ","
        buff[i-1] = ' ';

        //make sure char array is terminated
        buff[len-1] = '\0';
    }
    else {
        return 0;
    }


    return 1;
}

#endif