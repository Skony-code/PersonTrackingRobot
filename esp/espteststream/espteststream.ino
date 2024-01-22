#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"

//sensor pins
const int trig = 2;
const int echo = 4;

//motors pins
const int out1 = 15;
const int out2 = 13;
const int out3 = 12;

//motors controls
#define RIGHT_MOTORS 0
#define LEFT_MOTORS 1

#define FORWARD 0
#define BACKWARD 1
#define STOP 2

void rotateLeft(int speed/*0-255*/) {
  digitalWrite(out1,HIGH);
  digitalWrite(out2,LOW);
  digitalWrite(out3,HIGH);
}

void rotateRight(int speed/*0-255*/) {
  digitalWrite(out1,LOW);
  digitalWrite(out2,HIGH);
  digitalWrite(out3,HIGH);
}

void moveForward(int speed/*0-255*/) {
  digitalWrite(out1,HIGH);
  digitalWrite(out2,HIGH);
  digitalWrite(out3,HIGH);
}

void moveBackward(int speed/*0-255*/) {
  digitalWrite(out1,LOW);
  digitalWrite(out2,LOW);
  digitalWrite(out3,HIGH);
}

void stop() {
  digitalWrite(out3,LOW);
}

//Replace with your network credentials
const char* ssid = "robot_test_2400";
const char* password = "12345";

#define PART_BOUNDARY "123456789000000000000987654321"

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

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;
httpd_handle_t sensor_httpd = NULL;


static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[128];

    static int64_t last_frame = 0;
    if (!last_frame)
    {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
    {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "60");

    while (true)
    {
        fb = esp_camera_fb_get();
        if (!fb)
        {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
        }
        else
        {
            _timestamp.tv_sec = fb->timestamp.tv_sec;
            _timestamp.tv_usec = fb->timestamp.tv_usec;
                if (fb->format != PIXFORMAT_JPEG)
                {
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if (!jpeg_converted)
                    {
                        ESP_LOGE(TAG, "JPEG compression failed");
                        res = ESP_FAIL;
                    }
                }
                else
                {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if (res == ESP_OK)
        {
            size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if (fb)
        {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        }
        else if (_jpg_buf)
        {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG, "send frame failed failed");
            break;
        }
        int64_t fr_end = esp_timer_get_time();

        int64_t frame_time = fr_end - last_frame;
        frame_time /= 1000;
        ESP_LOGI(TAG, "MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)"
                 ,
                 (uint32_t)(_jpg_buf_len),
                 (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
                 avg_frame_time, 1000.0 / avg_frame_time
        );
    }

    return res;
}
int req_count = 0;

int SCREEN_WIDTH = 320;
int MOTOR_TIME_SHORT = 120;
int MOTOR_TIME_LONG = 240;
float CENTER_TOLERANCE = 0.2;

TaskHandle_t Task1;
char xPure[32];
char hPure[32];

static esp_err_t commands_handler(httpd_req_t *req) {
  esp_err_t res = ESP_OK;

  char *buf = NULL;

  if (parse_get(req, &buf) != ESP_OK) {
      return ESP_FAIL;
  }
  if (httpd_query_key_value(buf, "x", xPure, sizeof(xPure)) != ESP_OK ||
      httpd_query_key_value(buf, "h", hPure, sizeof(hPure)) != ESP_OK) {
      free(buf);
      httpd_resp_send_404(req);
      //return ESP_FAIL;
  }
  free(buf);

  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    &(xPure[0]),        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);  
  return res;
}

void Task1code( void * pvParameters ){

  int x = atoi((char*)pvParameters);
  int h = atoi(hPure);
  int dist_from_center = x-SCREEN_WIDTH/2;
  int abs_dist_center = abs(dist_from_center);
  if(abs_dist_center>CENTER_TOLERANCE*SCREEN_WIDTH/2) {
    if(dist_from_center>0) {
      Serial.println("prawo");
      rotateRight(100);
    } else if(dist_from_center<0) {
      Serial.println("lewo");
      rotateLeft(100);
    }
    int fun = abs_dist_center*2.4; 
    Serial.println(h);
    Serial.println(fun);
    delay(fun);
    stop();
  } else {
    int distance = measure_dist();

    if(distance>40) {
      Serial.println("prosto");
      moveForward(100);
      delay(100);
      stop();
    } else if(distance<20) {
      moveBackward(100);
      delay(80);
      stop();
    } else {
      Serial.println("stop");
      stop();
    }
  }
  vTaskDelete( NULL );
}

int measure_dist() {
    digitalWrite(trig, HIGH);
    delay(0.010);

    digitalWrite(trig, LOW);

    long time = pulseIn(echo, HIGH,300000);
    return time*0.034/2;
}

static esp_err_t sensor_handler(httpd_req_t *req) {
  int distance = measure_dist();

  char resp[16];
  itoa(distance, resp, 10);

  return httpd_resp_send(req, resp, 16);
}

static esp_err_t parse_get(httpd_req_t *req, char **obuf)
{
    char *buf = NULL;
    size_t buf_len = 0;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char *)malloc(buf_len);
        if (!buf) {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            *obuf = buf;
            return ESP_OK;
        }
        free(buf);
    }
    httpd_resp_send_404(req);
    return ESP_FAIL;
}

void startServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 16;
  config.server_port = 81;
  config.core_id = 1;

  httpd_uri_t get_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t sensor_uri = {
    .uri       = "/sensor",
    .method    = HTTP_GET,
    .handler   = sensor_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t post_uri = {
    .uri       = "/control",
    .method    = HTTP_POST,
    .handler   = commands_handler,
    .user_ctx  = NULL
  };
  
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &get_uri);
  }

  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&camera_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(camera_httpd, &post_uri);
  }

  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&sensor_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(sensor_httpd, &sensor_uri);
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
 
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  pinMode(out1, OUTPUT);
	pinMode(out2, OUTPUT);
	pinMode(out3, OUTPUT);

  //led
  pinMode(33, OUTPUT);
  digitalWrite(33, LOW);

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  digitalWrite(trig, LOW);
  delay(0.002);

  Serial.println();

  
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  //Camera start
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  //Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  Serial.print("Camera Stream http://");
  Serial.println(WiFi.localIP());
  
  //Start server
  startServer();
}

void loop() {

}