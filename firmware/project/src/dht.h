// 
//    FILE: dht11.h
// VERSION: 0.4.1
// PURPOSE: DHT11 Temperature & Humidity Sensor library for Arduino
// LICENSE: GPL v3 (http://www.gnu.org/licenses/gpl.html)
//
// DATASHEET: http://www.micro4you.com/files/sensor/DHT11.pdf
//
//     URL: http://playground.arduino.cc/Main/DHT11Lib
//
// HISTORY:
// George Hadjikyriacou - Original version
// see dht.cpp file
// 

#ifndef dht_h
#define dht_h

#include <Arduino.h>

#define DHTLIB_OK 0
#define DHTLIB_ERROR_CHECKSUM 1
#define DHTLIB_ERROR_TIMEOUT 2
#define DHTLIB_ERROR_CFG 3
#define DHTLIB_ERROR_VALUE 4
#define DHTLIB_ERROR_NOT_INIT 5

#define SENS_DHT22 4
#define SENS_DHT12 5

#define MIN_READ_INTERVAL 2500 //ms
#define SENS_BOOT_TIME 1000 //ms

#define DTH_READ_TIMEOUT 5 //ms

class DHT
{
    private:
    
        uint8_t data_pin, power_pin;
        uint32_t lastReadTime; 
        uint8_t booted;

    public:

        DHT(uint8_t power_pin, uint8_t data_pin, uint8_t type_p);

        void begin();
        void end();
        uint8_t read_dht();
        
        uint8_t type;
        
        uint8_t hum;
	    uint8_t temp;
        uint8_t hum_dec;
        uint8_t temp_dec;

        uint8_t lastRes;
};



#endif
//
// END OF FILE
//
