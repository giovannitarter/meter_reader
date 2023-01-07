#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include "WiFi.h"
#include <esp32fota.h>

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


const uint16_t port = SRVPORT;
const char * host = SRVNAME; // ip or dns
const uint8_t sleep_time = SLEEP_TIME;

char tmp_url[40];
String url("");
char chip_id[13];

camera_config_t config;
esp32FOTA esp32FOTA("esp32-fota-http", CVERSION, false);

#define LED_STRIP_EN 14
#define LED_STRIP_COMM 2


int camera_setup() {

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    config.frame_size = FRAMESIZE_VGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;

    // Init Camera
    esp_err_t err = 0x255;

    int timeout = 3;
    while (err != ESP_OK && timeout > 0) {
      err = esp_camera_init(&config);
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


void sleep() {

    Serial.println("Sleeping!");
    turn_off_light();

    digitalWrite(4, LOW);
    gpio_hold_en(GPIO_NUM_4);

    digitalWrite(LED_STRIP_EN, HIGH);
    gpio_hold_dis(GPIO_NUM_14);
    //gpio_deep_sleep_hold_en();

    //esp_sleep_enable_timer_wakeup(30 * 6e7);
    esp_sleep_enable_timer_wakeup(sleep_time * 6e7);
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


void turn_on_light() {

    digitalWrite(LED_STRIP_EN, LOW);
    delay(5);

    //Serial.println("Turn on Light");
    //pixels.begin();
    //pixels.setPixelColor(0, pixels.Color(100, 100, 100));
    //pixels.setPixelColor(1, pixels.Color(100, 100, 100));
    //pixels.setPixelColor(2, pixels.Color(100, 100, 100));
    //pixels.show();

    //digitalWrite(4, HIGH);
    //delay(10);
}

void turn_off_light() {
    Serial.println("Turn off Light");
    digitalWrite(LED_STRIP_EN, HIGH);
}


void setup() {

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

    //rtc_gpio_hold_dis(GPIO_NUM_4);
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);

    pinMode(LED_STRIP_EN, OUTPUT);
    digitalWrite(LED_STRIP_EN, HIGH);

    snprintf(tmp_url, 40, "http://%s:%d/fota/manifest", SRVNAME, SRVPORT);
    url.concat(String(tmp_url));
    esp32FOTA.setManifestURL(url);

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    Serial.printf("VERSION: %d\r\n", CVERSION);
    Serial.printf("manifest_url: %s\r\n", tmp_url);

    get_chip_id(chip_id, 13);
    Serial.printf("ESP32 Chip ID = %s\r\n", chip_id);

    Serial.printf("sleep_time: %d\r\n", sleep_time);

}


int post_image(WiFiClient * client, const char * host, camera_fb_t * fb) {

    String boundary = "--7F7B922A48CEF516930FEC95902F1881";
    String head = "Content-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String head2 = "Content-Disposition: form-data; name=\"espid\"\r\n\r\n";

    String start_bnd = String("\r\n--");
    start_bnd.concat(boundary);
    start_bnd.concat("\r\n");

    String tail = String("\r\n--");
    tail.concat(boundary);
    tail.concat(String("--\r\n"));

    uint16_t imageLen = fb->len;
    uint16_t extraLen = start_bnd.length()
        + head.length()
        + start_bnd.length()-2 //count starts after the first blank line
        + head2.length()
        + 12
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

    client->print(tail);
    return 0;
}


void loop() {

    int timeout;
    WiFi.begin(STASSID, STAPSK);

    timeout = millis() + 5000;
    while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {

        Serial.println("");
        Serial.println("WiFi connected");

        Serial.print("Connecting to ");
        Serial.println(host);

        bool updatedNeeded = esp32FOTA.execHTTPcheck();
        if (updatedNeeded)
        {
            esp32FOTA.execOTA();
            return;
        }


        // Use WiFiClient class to create TCP connections
        WiFiClient client;

        if (client.connect(host, port)) {

            Serial.println("Connected!");

            //FLASH should be turned on before camera_setup!

            turn_on_light();

            if (camera_setup() != ESP_OK) {
                client.stop();
                sleep();
                return;
            }

            camera_fb_t * fb = NULL;
            fb = esp_camera_fb_get();
            turn_off_light();

            if(!fb) {
                Serial.println("Camera capture failed");
            }
            else {

                post_image(&client, host, fb);
                esp_camera_fb_return(fb);

                timeout = millis() + 5000;
                //wait for the server's reply to become available

                while (!client.available() && millis() < timeout)
                {
                    delay(500);
                }

                if (client.available() > 0)
                {
                    //read back one line from the server
                    String line = client.readStringUntil('\r');
                    Serial.println(line);
                }
                else
                {
                    Serial.println("client.available() timed out ");
                }
            }

            Serial.println("Closing TCP connection.");

            client.flush();
            client.stop();
        }

        else {
            Serial.println("TCP Connection failed.");
        }
    }

    WiFi.disconnect();
    sleep();
}
