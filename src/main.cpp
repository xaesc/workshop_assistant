#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <lvgl.h>
#include <SPI.h>
#include <Arduino_GFX_Library.h>

#include "config.h"
#include "ui.h"

#define TFT_BL 45
Arduino_DataBus *bus = new Arduino_ESP32SPI(38, 39, 40, 41);
Arduino_GFX *gfx = new Arduino_ESP32S3_RGB_HX8357C(bus);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * 40];

WiFiClient espClient;
PubSubClient mqtt(espClient);

unsigned long lastCurrentUpdate = 0;
float currentValue = 0.0;
bool relaisState[NB_RELAIS] = {false};

void displayFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
    lv_disp_flush_ready(disp);
}

void touchRead(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint16_t x, y;
    if (gfx->getTouch(&x, &y)) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    char msg[128];
    if (length >= sizeof(msg)) length = sizeof(msg) - 1;
    memcpy(msg, payload, length);
    msg[length] = '\0';

    if (strcmp(topic, MQTT_TOPIC_CURRENT) == 0) {
        currentValue = atof(msg);
        uiUpdateCurrent(currentValue);
    } else if (strcmp(topic, MQTT_TOPIC_RELAIS_STATE) == 0) {
        StaticJsonDocument<256> doc;
        DeseralizeError err = deserializeJson(doc, msg);
        if (!err) {
            for (int i = 0; i < NB_RELAIS; i++) {
                char key[4];
                snprintf(key, sizeof(key), "%d", i + 1);
                if (doc.containsKey(key)) {
                    relaisState[i] = doc[key];
                    uiUpdateRelais(i, relaisState[i]);
                }
            }
        }
    }
}

void mqttReconnect() {
    while (!mqtt.connected()) {
        String clientId = "atelier-esp32-" + String(random(0xffff), HEX);
        if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
            mqtt.subscribe(MQTT_TOPIC_CURRENT);
            mqtt.subscribe(MQTT_TOPIC_RELAIS_STATE);
            mqtt.publish(MQTT_TOPIC_STATUS, "online", true);
        } else {
            delay(5000);
        }
    }
}

void sendRelaisCommand(int relais, bool state) {
    StaticJsonDocument<64> doc;
    doc["relais"] = relais + 1;
    doc["state"] = state;
    char out[64];
    serializeJson(doc, out);
    mqtt.publish(MQTT_TOPIC_RELAIS_CMD, out);
}

void sendScenarioCommand(const char *scenario) {
    mqtt.publish(MQTT_TOPIC_SCENARIO, scenario);
}

void setup() {
    Serial.begin(115200);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    gfx->begin();
    gfx->fillScreen(BLACK);
    gfx->setTouchRotation(1);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 40);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = displayFlush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchRead;
    lv_indev_drv_register(&indev_drv);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(500);

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(mqttCallback);

    uiInit(sendRelaisCommand, sendScenarioCommand);
}

void loop() {
    if (!mqtt.connected()) mqttReconnect();
    mqtt.loop();
    lv_timer_handler();
    delay(5);
}
