#ifndef _READ_DHT__
#define _READ_DHT__




class DHT_reader {

    public:
        DHT_reader();
        void begin();
        float read();

};


#endif