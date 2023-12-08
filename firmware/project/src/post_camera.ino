#include "esp_camera.h"
#include "SPIFFS.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include "WiFi.h"
#include <esp32fota.h>
#include "dht.h"

#include "parameters.h"

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


esp_sleep_wakeup_cause_t wakeup_reason;

struct esp_config {

    uint8_t stassid[20];
    uint8_t stapsk[20];

    uint8_t srvname[20];
    uint16_t port;

    uint32_t max_sleep_time;
};
typedef struct esp_config esp_config;

esp_config ec;

char tmp_url[40];
String url("");
char chip_id[13];
uint32_t sleep_time = SLEEP_TIME;

camera_config_t espcam_config;

WiFiClient client;
esp32FOTA esp32FOTA("esp32-fota-http", CVERSION, false);

#define LED_STRIP_EN GPIO_NUM_14
#define LED_ONBOARD GPIO_NUM_4

#define DHT_POW_PIN GPIO_NUM_32
#define DHT_DATA_PIN GPIO_NUM_33
DHT dht(DHT_POW_PIN, DHT_DATA_PIN, SENS_DHT22);

#define TEMP_LEN 7
uint8_t temp[TEMP_LEN];

#define TEMP_RAW_LEN 30
uint8_t temp_raw[TEMP_RAW_LEN];

#define WK_REASON_LEN 20
uint8_t wk_reason_par[WK_REASON_LEN];


DynamicJsonDocument doc(1024);


esp_err_t camera_setup() {

    espcam_config.ledc_channel = LEDC_CHANNEL_0;
    espcam_config.ledc_timer = LEDC_TIMER_0;
    espcam_config.pin_d0 = Y2_GPIO_NUM;
    espcam_config.pin_d1 = Y3_GPIO_NUM;
    espcam_config.pin_d2 = Y4_GPIO_NUM;
    espcam_config.pin_d3 = Y5_GPIO_NUM;
    espcam_config.pin_d4 = Y6_GPIO_NUM;
    espcam_config.pin_d5 = Y7_GPIO_NUM;
    espcam_config.pin_d6 = Y8_GPIO_NUM;
    espcam_config.pin_d7 = Y9_GPIO_NUM;
    espcam_config.pin_xclk = XCLK_GPIO_NUM;
    espcam_config.pin_pclk = PCLK_GPIO_NUM;
    espcam_config.pin_vsync = VSYNC_GPIO_NUM;
    espcam_config.pin_href = HREF_GPIO_NUM;
    espcam_config.pin_sscb_sda = SIOD_GPIO_NUM;
    espcam_config.pin_sscb_scl = SIOC_GPIO_NUM;
    espcam_config.pin_pwdn = PWDN_GPIO_NUM;
    espcam_config.pin_reset = RESET_GPIO_NUM;
    espcam_config.xclk_freq_hz = 20000000;
    espcam_config.pixel_format = PIXFORMAT_JPEG;

    espcam_config.frame_size = FRAMESIZE_VGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    espcam_config.jpeg_quality = 10;
    espcam_config.fb_count = 2;

    // Init Camera
    esp_err_t err = 0x255;

    int timeout = 3;
    while (err != ESP_OK && timeout > 0) {
      err = esp_camera_init(&espcam_config);
      Serial.printf("Camera init returned 0x%x\n", err);
      delay(500);
      timeout--;
    }

    if (err == ESP_OK) {
        sensor_t * s = esp_camera_sensor_get();
        s->set_brightness(s, 1);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_gain_ctrl(s, 1);                       // auto gain on
        s->set_exposure_ctrl(s, 1);                   // auto exposure on
        s->set_awb_gain(s, 0);                        // Auto White Balance enable (0 or 1)
        s->set_aec_value(s, 600);
    }

    return err;
}


//sleep time in seconds
void meter_sleep(uint32_t sleep_time) {

    Serial.println("Sleeping for %ds!", sleep_time);
    turn_off_light();

    digitalWrite(LED_ONBOARD, LOW);
    gpio_hold_en(LED_ONBOARD);

    digitalWrite(LED_STRIP_EN, HIGH);
    gpio_hold_en(LED_STRIP_EN);

    digitalWrite(DHT_POW_PIN, LOW);
    gpio_hold_en(DHT_POW_PIN);

    digitalWrite(DHT_DATA_PIN, HIGH);
    gpio_hold_en(DHT_DATA_PIN);

    sleep_time = sleep_time * 1e6;

    if (sleep_time > ec.max_sleep_time) {
        sleep_time = ec.max_sleep_time;
    }

    esp_sleep_enable_timer_wakeup(sleep_time);
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
}


void get_chip_id(char * text_id, size_t len) {

    uint64_t chipid;
    chipid = ESP.getEfuseMac();
    snprintf(text_id, len,
        "%04X%08X",
        (uint16_t)(chipid >> 32),
        (uint32_t)chipid
    );
}


void setup_light() {

    pinMode(LED_STRIP_EN, OUTPUT);
    digitalWrite(LED_STRIP_EN, HIGH);
    gpio_hold_dis(LED_STRIP_EN);

    pinMode(LED_ONBOARD, OUTPUT);
    digitalWrite(LED_ONBOARD, LOW);
    gpio_hold_dis(LED_ONBOARD);
}


void turn_on_light() {

    Serial.println("Turn on Light");
    digitalWrite(LED_STRIP_EN, LOW);
    delay(5);
}


void turn_off_light() {
    Serial.println("Turn off Light");
    digitalWrite(LED_STRIP_EN, HIGH);
}


void decode_wakeup_reason(esp_sleep_wakeup_cause_t wakeup_reason){

    switch(wakeup_reason) {

      case ESP_SLEEP_WAKEUP_EXT0:
        strncpy((char *)wk_reason_par, "EXT0", WK_REASON_LEN);
        break;

      case ESP_SLEEP_WAKEUP_EXT1:
        strncpy((char *)wk_reason_par, "EXT1", WK_REASON_LEN);
        break;

      case ESP_SLEEP_WAKEUP_TIMER:
        strncpy((char *)wk_reason_par, "TIMER", WK_REASON_LEN);
        break;

      case ESP_SLEEP_WAKEUP_TOUCHPAD:
        strncpy((char *)wk_reason_par, "TOUCHPAD", WK_REASON_LEN);
        break;

      case ESP_SLEEP_WAKEUP_ULP:
        strncpy((char *)wk_reason_par, "ULP", WK_REASON_LEN);
        break;

      default:
        strncpy((char *)wk_reason_par, "UNDEFINED", WK_REASON_LEN);
        break;
    }
}


uint8_t parse_config(esp_config * cfg, File * fd) {

    deserializeJson(doc, *fd);

    strlcpy((char *)cfg->stassid, doc["stassid"] | STASSID, 20);
    strlcpy((char *)cfg->stapsk, doc["stapsk"] | STAPSK, 20);
    strlcpy((char *)cfg->srvname, doc["srvname"] | SRVNAME, 20);
    cfg->max_sleep_time = doc["max_sleep_time"] | MAX_SLEEP_TIME;
    cfg->port = doc["srvport"].as<uint16_t>() | SRVPORT;

    return 1;
}


uint8_t load_config(esp_config * res) {

    memset(res, 0, sizeof(esp_config));

    if (SPIFFS.begin() == 0) {
        Serial.printf("Cannot mount SPIFFS\n\r");
        SPIFFS.format();
        Serial.printf("Formatting\r\n");

        if (SPIFFS.begin() == 0) {
            Serial.printf("Cannot mount even after format\r\n");
            return 0;
        }
        if (! SPIFFS.exists("/espconfig.json")) {

            Serial.printf("initializing config\r\n");

            doc["stassid"] = STASSID;
            doc["stapsk"] = STAPSK;
            doc["srvname"] = SRVNAME;
            doc["srvport"] = SRVPORT;
            doc["max_sleep_time"] = MAX_SLEEP_TIME;

            File fd = SPIFFS.open("/espconfig.json", "w");
            serializeJson(doc, fd);
            fd.close();
        }
    };

    File fd = SPIFFS.open("/espconfig.json");
    parse_config(res, &fd);
    fd.close();

    return 1;
}


void setup() {

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    load_config(&ec);

    //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

    wakeup_reason = esp_sleep_get_wakeup_cause();

    setup_light();
    gpio_hold_dis(DHT_POW_PIN);
    gpio_hold_dis(DHT_DATA_PIN);

    snprintf(
        tmp_url,
        40,
        "http://%s:%d/fota/manifest",
        ec.srvname,
        ec.port
        );
    url.concat(String(tmp_url));
    esp32FOTA.setManifestURL(url);

    Serial.printf("VERSION: %d\r\n", CVERSION);
    Serial.printf("STASSID: %s\r\n", ec.stassid);
    Serial.printf("STAPSK : **********\r\n", ec.stapsk);
    //Serial.printf("STAPSK : %s\r\n", ec.stapsk);
    Serial.printf("manifest_url: %s\r\n", tmp_url);

    get_chip_id(chip_id, 13);
    Serial.printf("ESP32 Chip ID = %s\r\n", chip_id);

    Serial.printf("ec.max_sleep_time: %u\r\n", ec.max_sleep_time);

    decode_wakeup_reason(wakeup_reason);
    Serial.printf("wk_reason: %s\r\n", wk_reason_par);


  //Serial.println("Starting SD Card");
  //if(!SD_MMC.begin()){
  //  Serial.println("SD Card Mount Failed");
  //  //return;
  //}
  //else {
  //  Serial.println("SD Card Mount Ok");
  //}

  //dht.begin();
  //delay(1000);

}


int post_image(WiFiClient * client, const char * host, camera_fb_t * fb) {

    String boundary = "--7F7B922A48CEF516930FEC95902F1881";
    String head = "Content-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String head2 = "Content-Disposition: form-data; name=\"espid\"\r\n\r\n";
    String head3 = "Content-Disposition: form-data; name=\"temp\"\r\n\r\n";
    String head4 = "Content-Disposition: form-data; name=\"wkreason\"\r\n\r\n";
    String head5 = "Content-Disposition: form-data; name=\"rawtemp\"\r\n\r\n";

    String start_bnd = String("\r\n--");
    start_bnd.concat(boundary);
    start_bnd.concat("\r\n");

    String tail = String("\r\n--");
    tail.concat(boundary);
    tail.concat(String("--\r\n"));

    uint16_t imageLen = fb->len;
    uint16_t temp_len = strnlen((char *)temp, TEMP_LEN);
    uint16_t wkreason_len = strnlen((char *)wk_reason_par, WK_REASON_LEN);
    uint16_t temp_raw_len = strnlen((char *)temp_raw, TEMP_RAW_LEN);

    uint16_t extraLen = start_bnd.length()
        + head.length()
        + start_bnd.length()-2 //count starts after the first blank line
        + head2.length()
        + 12

        + start_bnd.length()
        + head3.length()
        + temp_len

        + start_bnd.length()
        + head4.length()
        + wkreason_len

        + start_bnd.length()
        + head5.length()
        + temp_raw_len

        + tail.length();

    uint16_t totalLen = imageLen + extraLen;

    client->println("POST /sendphoto HTTP/1.1");
    client->println("Host: " + String(host));
    client->println("Content-Length: " + String(totalLen));

    client->print("Content-Type: multipart/form-data; boundary=");
    client->print(boundary);
    client->print("\r\n");

    client->print(start_bnd);
    client->print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        client->write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen%1024;
        client->write(fbBuf, remainder);
      }
    }

    client->print(start_bnd);
    client->print(head2);
    client->print(chip_id);

    client->print(start_bnd);
    client->print(head3);
    client->write(temp, temp_len);

    client->print(start_bnd);
    client->print(head4);
    client->write(wk_reason_par, wkreason_len);

    client->print(start_bnd);
    client->print(head5);
    client->write(temp_raw, temp_raw_len);

    client->print(tail);
    return 0;
}


void loop() {

    uint32_t start_time = millis();
    turn_on_light();

    if (camera_setup() != ESP_OK) {
        Serial.println("Camera setup failed");
    }

    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    turn_off_light();

    if(!fb) {
        Serial.println("Camera capture failed");
        esp_camera_deinit();
        meter_sleep(45);
    }

    dht.begin();

    int timeout;
    WiFi.begin((char *)ec.stassid, (char *)ec.stapsk);
    timeout = millis() + 5000;

    while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
        delay(500);
        Serial.print(".");
    }

    dht.read_dht();
    snprintf((char *)temp, 7, "%d.%d", dht.temp, dht.temp_dec);

    uint8_t i = 0, j = 0;
    for(j; j<5 && i < TEMP_RAW_LEN; j++) {
        i = i + snprintf(
            (char *)&(temp_raw[i]),
            TEMP_RAW_LEN - i,
            "%d,",
            dht.bits[j]
            );
    }
    dht.end();

    if (WiFi.status() == WL_CONNECTED) {

        Serial.println("");
        Serial.println("WiFi connected");

        start_time = millis();

        Serial.printf("Connecting to %s\n", ec.srvname);
        bool updatedNeeded = esp32FOTA.execHTTPcheck();

        if (updatedNeeded)
        {
            esp32FOTA.execOTA();
            return;
        }

        //Serial.printf("%s:%d\n", (char *)ec.srvname, ec.port);

        // Use WiFiClient class to create TCP connections
        if (client.connect((char *)ec.srvname, ec.port)) {

            Serial.println("Connected!");

            Serial.printf("temp: %s\n", temp);
            Serial.printf("temp_raw: %s\n", temp_raw);

            post_image(&client, (const char *)ec.srvname, fb);
            esp_camera_fb_return(fb);

            timeout = millis();
            //wait for the server's reply to become available

            Serial.print("Waiting reply");
            while (!client.available() && millis() < timeout + 10000)
            {
                Serial.print(".");
                delay(50);
            }
            Serial.printf("Reply took %d\n", millis() - timeout);

            Serial.printf("\nHttp reply (%d bytes):\n", client.available());
            Serial.print("\n----------------------\n");
            uint8_t tmp[51], len, line;
            tmp[0] = 0;
            tmp[50] = 0;
            line = 0;

            while (client.available() > 0)
            {
                len = client.readBytesUntil('\n', tmp, 50);
                tmp[len] = 0;

                if (len > 0) {
                    if (len == 1 && tmp[len-1] == '\r') {
                        break;
                    }
                    tmp[len-1] = 0;
                }

                Serial.printf("\n%d - %s", line, tmp);
                line++;
            }
            Serial.print("\n----------------------");
            Serial.println("\nClosing TCP connection.");

            deserializeJson(doc, client);

            sleep_time = doc["sleeptime"] | ec.max_sleep_time;
            Serial.printf("sleeptime: %u\n", sleep_time);
            Serial.printf("ctime: %u\n", doc["ctime"].as<uint32_t>());

            client.flush();
            client.stop();
        }

        else {
            Serial.println("TCP Connection failed.");
            sleep_time = ec.max_sleep_time;
        }

    }

    Serial.printf("elapsed since boot: %d\n", millis());

    //WiFi.disconnect();
    meter_sleep(sleep_time);
}
