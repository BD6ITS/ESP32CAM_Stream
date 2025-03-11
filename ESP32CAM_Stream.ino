#include "esp_camera.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <ArduinoJson.h>  // 添加这行

// 相机引脚定义 - AI Thinker ESP32-CAM
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

// LED引脚定义
#define LED_PIN           4
#define LED_CHANNEL       7  // 使用PWM通道7

// WiFi设置
const char* ssid = "ESP32CAM_AP";
const char* password = "12345678";

// WebSocket服务器
WebSocketsServer webSocket = WebSocketsServer(81);

// HTTP服务器
WebServer server(80);

// 相机配置
void configInitCamera(){
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

  // 设置帧大小和质量
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;  //0-63 数字越小质量越好
  config.fb_count = 2;

  // 初始化相机
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

// WebSocket事件处理
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected!\n", num);
      break;
    case WStype_TEXT: {
      String message = String((char*)payload);
      Serial.println("收到消息: " + message);  // 调试输出
      
      if (message.indexOf("\"type\":\"config\"") > -1) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, message);
        
        if (!error) {
          const char* frameSize = doc["frameSize"];
          int quality = doc["quality"];
          
          sensor_t * s = esp_camera_sensor_get();
          if (s) {
            if (strcmp(frameSize, "QVGA") == 0) {
              s->set_framesize(s, FRAMESIZE_QVGA);
            } else if (strcmp(frameSize, "VGA") == 0) {
              s->set_framesize(s, FRAMESIZE_VGA);
            } else if (strcmp(frameSize, "SVGA") == 0) {
              s->set_framesize(s, FRAMESIZE_SVGA);
            } else if (strcmp(frameSize, "XGA") == 0) {
              s->set_framesize(s, FRAMESIZE_XGA);
            }
            s->set_quality(s, quality);
            Serial.printf("已更新设置：分辨率=%s, 质量=%d\n", frameSize, quality);
          }
        }
      }
      break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // WiFi配置
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  delay(1000);  // 增加延时确保AP完全启动
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // 初始化LED和相机等设备
  ledcSetup(LED_CHANNEL, 5000, 8);  // 5KHz PWM, 8位分辨率
  ledcAttachPin(LED_PIN, LED_CHANNEL);
  ledcWrite(LED_CHANNEL, 0);  // 初始LED关闭
  
  // 初始化相机
  configInitCamera();
  
  // 初始化文件系统
  if(!LittleFS.begin(true)){
      Serial.println("LittleFS Mount Failed");
      return;
  }
  
  // 检查文件系统
  if(!LittleFS.exists("/index.html")) {
    Serial.println("index.html 文件不存在");
    return;
  }
  
  // 测试文件读取
  File testFile = LittleFS.open("/index.html", "r");
  if(!testFile) {
    Serial.println("无法打开测试文件");
    return;
  }
  
  char testBuffer[1];
  size_t testBytes = testFile.readBytes(testBuffer, 1);
  testFile.close();
  Serial.printf("测试读取字节数: %d\n", testBytes);
  
  if(testBytes == 0) {
    Serial.println("LittleFS 文件系统可能损坏，建议重新格式化");
    return;
  }
  
  // 增加更详细的文件系统信息输出
  Serial.println("LittleFS 信息:");
  Serial.printf("总空间: %d bytes\n", LittleFS.totalBytes());
  Serial.printf("已用空间: %d bytes\n", LittleFS.usedBytes());
  
  Serial.println("文件列表:");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while(file){
      Serial.printf("- %s (%d bytes)\n", file.name(), file.size());
      file = root.openNextFile();
  }
  
  // 设置ESP32为AP模式
  // 删除重复的WiFi配置
  // 删除这段代码:
  // WiFi.softAP(ssid, password);
  // Serial.println();
  // Serial.print("AP IP address: ");
  // Serial.println(WiFi.softAPIP());
  
  server.on("/", HTTP_GET, []() {
    Serial.println("收到根目录请求");
    if(LittleFS.exists("/index.html")) {
      File file = LittleFS.open("/index.html", "r");
      if(!file) {
        Serial.println("无法打开index.html");
        server.send(500, "text/plain", "Internal Server Error");
        return;
      }
      
      // 使用 streamFile 方法
      server.sendHeader("Content-Type", "text/html");
      server.sendHeader("Connection", "close");
      
      size_t sent = server.streamFile(file, "text/html");
      file.close();
      
      Serial.printf("发送字节数: %d\n", sent);
      if(sent > 0) {
        Serial.println("成功发送index.html");
      } else {
        Serial.println("文件发送失败");
      }
    } else {
      Serial.println("找不到index.html文件");
      server.send(404, "text/plain", "File not found");
    }
  });

  server.on("/test", HTTP_GET, []() {
    Serial.println("收到测试请求");
    server.send(200, "text/plain", "Test OK");
  });

  // 启动HTTP服务器
  server.begin();
  Serial.println("HTTP server started on port 80");
  
  // 启动WebSocket服务器
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  // 优化循环处理
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();
  
  server.handleClient();  // 处理HTTP请求
  webSocket.loop();      // 处理WebSocket
  
  // 每100ms处理一次图像
  if (currentTime - lastTime > 100) {
    lastTime = currentTime;
    
    camera_fb_t * fb = esp_camera_fb_get();
    if(fb) {
      webSocket.broadcastBIN(fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
  }
  
  yield();  // 让出CPU时间给系统处理其他任务
}