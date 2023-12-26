
#include "Arduino.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include "dht_reader.h"


DHT_Unified _dht(GPIO_NUM_33, DHT22);


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