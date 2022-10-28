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
char tmp_url[40];
String url("");

camera_config_t config;
esp32FOTA esp32FOTA("esp32-fota-http", CVERSION, false);


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
        s->set_awb_gain(s, 1);                        // Auto White Balance enable (0 or 1)
        s->set_brightness(s, 1);
    }

    return err;
}


void sleep() {

    Serial.println("Sleeping!");

    digitalWrite(4, LOW);
    rtc_gpio_hold_en(GPIO_NUM_4);
    gpio_deep_sleep_hold_en();

    //esp_sleep_enable_timer_wakeup(60000000);
    esp_sleep_enable_timer_wakeup(600000000);
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
}


void setup() {

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

    snprintf(tmp_url, 40, "http://%s:%d/fota/manifest", SRVNAME, SRVPORT);
    url.concat(String(tmp_url));
    esp32FOTA.setManifestURL(url);

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    Serial.printf("VERSION: %d\n", CVERSION);
    Serial.printf("manifest_url: %s\n", tmp_url);


}


int post_image(WiFiClient * client, const char * host, camera_fb_t * fb) {

    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;

    client->println("POST /sendphoto HTTP/1.1");
    client->println("Host: " + String(host));
    client->println("Content-Length: " + String(totalLen));
    client->println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    client->println();
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

    client->print(tail);
    return 0;
}


void loop() {

  int timeout = 20;
  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(500);
    Serial.print(".");
    timeout--;
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


    rtc_gpio_hold_dis(GPIO_NUM_4);
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (client.connect(host, port)) {


    Serial.println("Connected!");

    if (camera_setup() != ESP_OK) {
        client.stop();
        sleep();
        return;
    }

    camera_fb_t * fb = NULL;

    digitalWrite(4, HIGH);
    delay(50);
    fb = esp_camera_fb_get();
    delay(50);
    digitalWrite(4, LOW);

    if(!fb) {
      Serial.println("Camera capture failed");
    }
    else {

      post_image(&client, host, fb);
      esp_camera_fb_return(fb);

      timeout = 20;
      //wait for the server's reply to become available
      while (!client.available() && timeout > 0)
      {
        delay(500);
        timeout--;
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
        client.stop();
      }

      else {
        Serial.println("TCP Connection failed.");
      }
    }

    sleep();
}
