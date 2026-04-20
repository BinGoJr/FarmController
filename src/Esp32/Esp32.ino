#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>


const char* ssid     = "*********";
const char* password = "*********";



// uS_TO_S_FACTOR 1000000ULL — для перевода секунд в микросекунды
#define uS_TO_S_FACTOR 1000000ULL
// Спать 30 минут (60 * 60 секунд)
#define TIME_TO_SLEEP   (60 * 60)
#define CAMERA_NAME "ESP32_CAM_01"

const char* serverUrl = "********************"; 

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"


void setupCamera() {
  camera_config_t config;
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_FHD;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);

  sensor_t * s = esp_camera_sensor_get();
  int pid = s->id.PID;
  Serial.printf("sensor PID: 0x%02X\n", pid);

  if (err != ESP_OK) {
    Serial.printf("Ошибка камеры: 0x%x\n", err);
    ESP.restart();
  }
  Serial.println("Камера готова");
}


void sendPhotoToServer(camera_fb_t *fb) {
  if (fb == NULL) {
    Serial.println("Нет данных фото для отправки");
    return;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("X-Camera-Name", CAMERA_NAME);
  
  int httpResponseCode = http.POST(fb->buf, fb->len);
  
  if (httpResponseCode > 0) {
    Serial.printf("Фото отправлено! Код ответа: %d\n", httpResponseCode);
  } else {
    Serial.printf("Ошибка отправки: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  
  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000);


  WiFi.begin(ssid, password);
  Serial.print("Подключение к Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi подключен");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());


  setupCamera();


  Serial.println("Делаю снимок...");
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Ошибка захвата фото");
  } else {

    sendPhotoToServer(fb);
    esp_camera_fb_return(fb);
    Serial.println("Фото обработано");
  }


  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);


  Serial.printf("Уход в глубокий сон на %d секунд...\n", TIME_TO_SLEEP);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {

}