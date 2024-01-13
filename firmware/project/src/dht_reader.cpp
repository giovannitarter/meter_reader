
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

#endif