#include "esp_camera.h"
#include <WiFi.h>

// Adicionado para receber um post
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);


//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15 
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD
//#define CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3 // Has PSRAM
//#define CAMERA_MODEL_DFRobot_Romeo_ESP32S3 // Has PSRAM
#include "camera_pins.h"

#include "PinDefinitionsAndMore.h" //Define macros for input and output pin etc.
#include <IRremote.hpp>

// ===========================
// Enter your WiFi credentials
// ===========================
const char* ssidHome = "MultilaserPRO_ZTE_2.4G_drtPEm";
const char* passwordHome = "w5GM7Atu";
const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";

const bool testingHome = true;

void startCameraServer();
void setupLedFlash(int pin);

String requestBody;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println();
  
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH); // turn the LED on
  delay(500);             // wait for 500 milliseconds
  digitalWrite(4, LOW);  // turn the LED off
  delay(500);             // wait for 500 milliseconds

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
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    // s->set_framesize(s, FRAMESIZE_QVGA);
    s->set_framesize(s, FRAMESIZE_SVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  if(testingHome) {
    WiFi.begin(ssidHome, passwordHome);
    WiFi.setSleep(false);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");


    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
  } else {
    // Connect to Wi-Fi network with SSID and password
    Serial.print("Setting AP (Access Point)…");
    // Remove the password parameter, if you want the AP (Access Point) to be open
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    server.begin();
  }

  startCameraServer();


  Serial.println("Iniciando infrared transmitter...");
  IrSender.begin();
  Serial.println("Concluído infrared transmitter...");

//  if defined(IR_SEND_PIN)
//    IrSender.begin(); // Start with IR_SEND_PIN as send pin and enable feedback LED at default feedback LED pin
//#else
//    IrSender.begin(3, ENABLE_LED_FEEDBACK); // Specify send pin and enable feedback LED at default feedback LED pin
//#endif

//#if defined(IR_SEND_PIN)
//Serial.println(F("Ready to send IR signals at pin " STR(IR_SEND_PIN)));
//#else
//    Serial.println(F("Ready to send IR signals at pin 3"));
//#endif
}

// Storage for the recorded code
// int codeType = -1; // The type of code
// unsigned long codeValue; // The code value if not raw
// unsigned int rawCodes[RAWBUF]; // The durations if raw
// int codeLen; // The length of the code
// int toggle = 0; // The RC5/6 toggle state

// int lastButtonState;

// void sendCode(int repeat) {
//   if (codeType == NEC) {
//     if (repeat) {
//       irsend.sendNEC(REPEAT, codeLen);
//       Serial.println("Sent NEC repeat");
//     } 
//     else {
//       irsend.sendNEC(codeValue, codeLen);
//       Serial.print("Sent NEC ");
//       Serial.println(codeValue, HEX);
//     }
//   } 
//   else if (codeType == SONY) {
//     irsend.sendSony(codeValue, codeLen);
//     Serial.print("Sent Sony ");
//     Serial.println(codeValue, HEX);
//   } 
//   else if (codeType == RC5 || codeType == RC6) {
//     if (!repeat) {
//       // Flip the toggle bit for a new button press
//       toggle = 1 - toggle;
//     }
//     // Put the toggle bit into the code to send
//     codeValue = codeValue & ~(1 << (codeLen - 1));
//     codeValue = codeValue | (toggle << (codeLen - 1));
//     if (codeType == RC5) {
//       Serial.print("Sent RC5 ");
//       Serial.println(codeValue, HEX);
//       irsend.sendRC5(codeValue, codeLen);
//     } 
//     else {
//       irsend.sendRC6(codeValue, codeLen);
//       Serial.print("Sent RC6 ");
//       Serial.println(codeValue, HEX);
//     }
//   } 
//   else if (codeType == UNKNOWN /* i.e. raw */) {
//     // Assume 38 KHz
//     irsend.sendRaw(rawCodes, codeLen, 38);
//     Serial.println("Sent raw");
//   }
// }



// IRsend irsend;

// #define IR_LED_PIN 3 // Define the pin number for the IR LED
// IRsend irsend(IR_LED_PIN); // Initialize IRsend object with the specified pin

//ATUAL
uint16_t sAddress_Array[]= {0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF};
uint8_t sCommand_Array[]= {0x68,0x98,0xB0,0x30,0x7A,0x10,0x38,0x52};
uint8_t sRepeats = 3;

void loop() {
  delay(1000);

  if(requestBody == "UP") {
    IrSender.sendNEC(sAddress_Array[1] , sCommand_Array[1], sRepeats);
  } else if(requestBody == "DOWN") {
    IrSender.sendNEC(sAddress_Array[0] , sCommand_Array[0], sRepeats);
  } else if(requestBody == "LEFT") {
    IrSender.sendNEC(sAddress_Array[5] , sCommand_Array[5], sRepeats);
  } else if(requestBody == "RIGHT") {
    IrSender.sendNEC(sAddress_Array[4] , sCommand_Array[4], sRepeats);
  }

  if(requestBody != "") {
    // Print request body when available
    Serial.println("Request Body:");
    Serial.println(requestBody);
    requestBody = "";
  }
}

