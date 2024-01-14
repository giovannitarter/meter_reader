#ifndef _READ_DHT__
#define _READ_DHT__




class DHT_reader {

    public:
        DHT_reader();
        void begin();
        float read();
        int8_t get_raw(uint8_t * buff, size_t len);

};


#endif