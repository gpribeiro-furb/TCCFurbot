#include "esp_camera.h"
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <base64.h>
#include <EEPROM.h>
#include <Arduino.h>
#include "PinDefinitionsAndMore.h" //Define macros for input and output pin etc.
#include <IRremote.hpp>

#define DELAY_AFTER_SEND 2000
#define DELAY_AFTER_LOOP 5000

//https://simple-circuit.com/arduino-nec-remote-control-decoder/

//1 00FF6897
//2 00FF9867
//3 00FFB04F
//4 00FF30CF
//5 RESERVADO FURBOT
//6 00FF7A85
//7 00FF10EF
//8 00FF38C7
//9 00FF5AA5
//0 00FF4AB5
//# 00FF52AD

#define LED_FLASH 4
#define CARD_DETECT 2

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
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD

#include "camera_pins.h"

// ===========================
// Enter your WiFi credentials
// ===========================
//const char* ssid = "DaltonReis";
//const char* password = "!anadal!";

const char* ssid = "Andreia Oi Miguel 2.4G";
const char* password = "mcs80251";

void startCameraServer();

// IOT ENDPOINT (created on AWS IOT Core) here... like it: xxxxxxxxxxx-ats.iot.us-east-1.amazonws.com
const char AWS_IOT_ENDPOINT[] = "a1dl08fmj981lr-ats.iot.us-east-1.amazonaws.com";

// THINGNAME (created on AWS IoT Core) here... It will be the CLIENTID
#define THINGNAME "lamp"


// Amazon Root CA 1 - https://www.amazontrust.com/repository/AmazonRootCA1.pem
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

// Device Certificate - Present in kit - name like this: THINGNAME.cert.pem
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDWjCCAkKgAwIBAgIVAOqviqfRCWkOjPQ6f8KJE1MXQXUCMA0GCSqGSIb3DQEB
CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t
IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yMTA3MzAxMTU1
MjNaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh
dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC6PPAhJ/FByBdHFwls
Wj458/HaIMwYsaafTuMJjUCo+GEQaIIYYKbwnuy5efD/zhvLoPmiRQM+eBV3OmxL
Hm96NobTqtpMpSpin8fSfC6xZa65nsMkiSEIFhwY4ousglWDXpLXMvd5PanmuBj2
JM0aWL7lOK1UVCf+xcJfM14nKnAJMg6/kc5FyQe96NFrLu5YWUJp0YzyvJSs/n/X
GOFYq0CD4EBjNZ8AJ6NrFtVzg5hgVFjUEZQf30nvFjKv94Wk6KNGLLgaPZGh3E5Y
/g0vXPIObY7ry+i3ZFfsyLXD8uM21nHqCE8TO/0dKT2BHhmy/xleLLh577U12sQe
9UfJAgMBAAGjYDBeMB8GA1UdIwQYMBaAFIsf5so9vvcPyvZx9oMOuKpzusQeMB0G
A1UdDgQWBBSwyOjirj8HlluOcxmocRBxeGUwEzAMBgNVHRMBAf8EAjAAMA4GA1Ud
DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAQlkyQ/AqAWDG1Cfal0ZZy6sZ
aWJOrEZA5y6ehMEcSb8yZp4sugvWGWPLaxpjjcuzn32RszSY4qVgDYQubA37C0dn
FWGJOsxf6wEtXeCvky02N8EzHl4/OAYsn12/zbWFo9Qdk0/MhFeaC2m4UMFEImBc
SMJiZPI9UcQikRmCpyzwbBrXlDviYkY61KCVla2TaXGnfcR+yXUrcUjWvb+WZRyw
oDCZnQEPln9fWkEtE6y2suBLRqE4DFdg62+ffE8yRsq2Iay4Ip/r0Eq2DC3P1GRm
DoFH7GdNbMPFhzkxe1KFbr/v1AqKPnf0hzjw7sCmYnXGMCD3GQL0yuqc6RTygg==
-----END CERTIFICATE-----
)KEY";

// Device Private Key - Present in kit - name like this: THINGNAME.private.key
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEAujzwISfxQcgXRxcJbFo+OfPx2iDMGLGmn07jCY1AqPhhEGiC
GGCm8J7suXnw/84by6D5okUDPngVdzpsSx5vejaG06raTKUqYp/H0nwusWWuuZ7D
JIkhCBYcGOKLrIJVg16S1zL3eT2p5rgY9iTNGli+5TitVFQn/sXCXzNeJypwCTIO
v5HORckHvejRay7uWFlCadGM8ryUrP5/1xjhWKtAg+BAYzWfACejaxbVc4OYYFRY
1BGUH99J7xYyr/eFpOijRiy4Gj2RodxOWP4NL1zyDm2O68vot2RX7Mi1w/LjNtZx
6ghPEzv9HSk9gR4Zsv8ZXiy4ee+1NdrEHvVHyQIDAQABAoIBABiAV98k0BqlZlqV
70lv/evF6jWWRwOgq5kd2cSx7qXMN4jfFu56GLxjTCWCo3Aj/4gkn3L3vVuElN5g
s0qWsHfVoJtMSPf4asEa4EES0VnJVNlax7WQzOWxcvTbKSEC7Jkh4hOpwuESeb2+
zpnjcZBbyJGDiVok0DY2ckHwEhz4wNcaVueKV39BIoVyRct4MbDEBdzz1qm0KjlN
VXR5OCIl/oIaf41D5Jh/R4J0Kdk5Yvj6DolC7rj7ZLQhB5I56bmtxk8plVQWJBoc
k/Fbl63NX2YQ/92sR9X+8xiA1cjOIdV3Sj/miQtOYAKiwt4hYGqX1lVlCUqlCSHs
HPyqibkCgYEA4GshHaMIjmez9hGknRsFJZqMNnx0mJRyKEfzqvgJcS4EMBzNTB/T
VK1MBKvuJ3JXGWwTjpRDc4P/6D5nbd54VPE5rwKoYOz7PEdr/CXyIhTxOqVsCbn+
DJcHnicEg/jdFxz1o6UPp3RAwKYZ5UibHAXYLX/4oAL+t8hBt8TjKAMCgYEA1HJT
RbxbrAQRknUTeTR9t5xRRhfzFg1Tv6pa+d9lhoNtJWvaq1kTQXbJzhU27DJdmYwB
YivPE/Z80LDtmttPcqENUGX2V11gBPD0U+xlBXy6TcjWlqIXzmaWxRJQJtFxDMf0
pem8Wl5UoLuf1o+tuv19Wv8PnTHtuwJvGMyTRUMCgYEAlTCi40Du/zyRtpQkCw1b
bpQ/gjlaidSlz4GFy7L5E/SzdAixslm19CUA1Fo4CKFKpevjVHXcIX8vMQXLXI75
D/8NifsKVOU2X+xbLVVMkru5YnN2T0EGUvhUaB8oMVPQJv1pj1C8B2aRZYEG1hZe
h1KbVCQu9LEJBMRsa+7H1GECgYEAp+MdGZwL2flxJYtt8zffLc3TkI/2o3OuvmXS
9WpsoguUoqEEMqr3A6HrQvgS6j+3JlO0+C7ku8LrykQ30UwFEfJiQioBrfL1oZIF
k5jaEKcL19KienHoXE1IUr7xJmwXaeFO3hrDpaqsOEVtwDLVOVN/z/yHC1Pcqg/3
WeK5c18CgYEAk1/fXHFMexgRhSD0qRwwVxWRSoygrcQ45YIxMwwBZZgNfLtgWzuo
2bG9uM7vIyeIoHXe4pd1SC/zcAFiA5qZ62l5SHdL2p7WFFzsz4pW9NmrCPlX2Dl9
kbMLKV3fuBvdm6FGgbfWpADEPQh+98UsndV5G7vFf8o72k8yoV7dUeE=
-----END RSA PRIVATE KEY-----
)KEY";

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "$aws/things/lamp/shadow/update"
#define AWS_IOT_SUBSCRIBE_TOPIC "$aws/things/lamp/shadow/update/accepted"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(15000);

void Wifi_Connected(void)
{
  digitalWrite(LED_FLASH, HIGH);
  delay(400);
  digitalWrite(LED_FLASH, LOW);
  delay(400);  
}

void AWS_Connected(void)
{
  digitalWrite(LED_FLASH, HIGH);
  delay(400);
  digitalWrite(LED_FLASH, LOW);
  delay(400);
  digitalWrite(LED_FLASH, HIGH);
  delay(400);
  digitalWrite(LED_FLASH, LOW);
  delay(400);
}

void AWS_PUBLISHED(void)
{
  digitalWrite(LED_FLASH, HIGH);
  delay(100);
  digitalWrite(LED_FLASH, LOW);
  delay(100);
  digitalWrite(LED_FLASH, HIGH);
  delay(100);
  digitalWrite(LED_FLASH, LOW);
  delay(100);
  digitalWrite(LED_FLASH, HIGH);
  delay(100);
  digitalWrite(LED_FLASH, LOW);
  delay(100);
  digitalWrite(LED_FLASH, HIGH);
  delay(100);
  digitalWrite(LED_FLASH, LOW);
  delay(100);
  digitalWrite(LED_FLASH, HIGH);
  delay(100);
  digitalWrite(LED_FLASH, LOW);
  delay(100);
}

void LAMP_ON(void)
{
  digitalWrite(LED_FLASH, HIGH);
  
}

void LAMP_OFF(void)
{
  digitalWrite(LED_FLASH,LOW);
}

void connectAWS()
{
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected");
  AWS_Connected();
}

boolean Got_Message = false;
String Message;

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  Got_Message = true;
  Message = payload;
          
//  StaticJsonDocument<200> doc;
//  deserializeJson(doc, payload);
//  const char* message = doc["message"];
}

void takePictureAndSubmit(bool Destroy) {
  
  // capture camera frame
  //LAMP_ON();
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb) {
      Serial.println("Camera capture failed");
      return;
  } else {
      if(!Destroy)
      {
        //delay(1000);
        //LAMP_OFF();
        Serial.println("Camera capture successful!");
      }
  }
  
  const char *data = (const char *)fb->buf;
  // Image metadata.  Yes it should be cleaned up to use printf if the function is available
  if(!Destroy)
  {
    Serial.print("Size of image:");
    Serial.println(fb->len);
    Serial.print("Shape->width:");
    Serial.print(fb->width);
    Serial.print("height:");
    Serial.println(fb->height);
  }

  String encoded = base64::encode(fb->buf, fb->len);

  // formating json
  DynamicJsonDocument doc(15000);
  doc["picture"] = encoded;
  String jsonBuffer;
  
  serializeJson(doc, jsonBuffer);

  if(!Destroy)
  {
    // publishing topic 
    if (!client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer)) {
      lwMQTTErr(client.lastError());
    }
    else
      {
      AWS_PUBLISHED();  
      Serial.println("Published");
      }
  }
    // Killing cam resource
  esp_camera_fb_return(fb);
 
}

void lwMQTTErr(lwmqtt_err_t reason)
{
  if (reason == lwmqtt_err_t::LWMQTT_SUCCESS)
    Serial.print("Success");
  else if (reason == lwmqtt_err_t::LWMQTT_BUFFER_TOO_SHORT)
    Serial.print("Buffer too short");
  else if (reason == lwmqtt_err_t::LWMQTT_VARNUM_OVERFLOW)
    Serial.print("Varnum overflow");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_CONNECT)
    Serial.print("Network failed connect");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_TIMEOUT)
    Serial.print("Network timeout");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_READ)
    Serial.print("Network failed read");
  else if (reason == lwmqtt_err_t::LWMQTT_NETWORK_FAILED_WRITE)
    Serial.print("Network failed write");
  else if (reason == lwmqtt_err_t::LWMQTT_REMAINING_LENGTH_OVERFLOW)
    Serial.print("Remaining length overflow");
  else if (reason == lwmqtt_err_t::LWMQTT_REMAINING_LENGTH_MISMATCH)
    Serial.print("Remaining length mismatch");
  else if (reason == lwmqtt_err_t::LWMQTT_MISSING_OR_WRONG_PACKET)
    Serial.print("Missing or wrong packet");
  else if (reason == lwmqtt_err_t::LWMQTT_CONNECTION_DENIED)
    Serial.print("Connection denied");
  else if (reason == lwmqtt_err_t::LWMQTT_FAILED_SUBSCRIPTION)
    Serial.print("Failed subscription");
  else if (reason == lwmqtt_err_t::LWMQTT_SUBACK_ARRAY_OVERFLOW)
    Serial.print("Suback array overflow");
  else if (reason == lwmqtt_err_t::LWMQTT_PONG_TIMEOUT)
    Serial.print("Pong timeout");
}

void lwMQTTErrConnection(lwmqtt_return_code_t reason)
{
  if (reason == lwmqtt_return_code_t::LWMQTT_CONNECTION_ACCEPTED)
    Serial.print("Connection Accepted");
  else if (reason == lwmqtt_return_code_t::LWMQTT_UNACCEPTABLE_PROTOCOL)
    Serial.print("Unacceptable Protocol");
  else if (reason == lwmqtt_return_code_t::LWMQTT_IDENTIFIER_REJECTED)
    Serial.print("Identifier Rejected");
  else if (reason == lwmqtt_return_code_t::LWMQTT_SERVER_UNAVAILABLE)
    Serial.print("Server Unavailable");
  else if (reason == lwmqtt_return_code_t::LWMQTT_BAD_USERNAME_OR_PASSWORD)
    Serial.print("Bad UserName/Password");
  else if (reason == lwmqtt_return_code_t::LWMQTT_NOT_AUTHORIZED)
    Serial.print("Not Authorized");
  else if (reason == lwmqtt_return_code_t::LWMQTT_UNKNOWN_RETURN_CODE)
    Serial.print("Unknown Return Code");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_FLASH, OUTPUT); //LED BLINK
  pinMode(CARD_DETECT, INPUT_PULLUP); //INFRARED SENSOR (APROXIMACAO)

  Serial.setDebugOutput(true);
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
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Wifi_Connected();

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  Serial.println("Conecta AWS");  
  connectAWS();

  #if defined(IR_SEND_PIN)
    IrSender.begin(); // Start with IR_SEND_PIN as send pin and enable feedback LED at default feedback LED pin
#else
    IrSender.begin(3, ENABLE_LED_FEEDBACK); // Specify send pin and enable feedback LED at default feedback LED pin
#endif

#if defined(IR_SEND_PIN)
    Serial.println(F("Ready to send IR signals at pin " STR(IR_SEND_PIN)));
#else
    Serial.println(F("Ready to send IR signals at pin 3"));
#endif

}

/*
 * Set up the data to be sent.
 * For most protocols, the data is build up with a constant 8 (or 16 byte) address
 * and a variable 8 bit command.
 * There are exceptions like Sony and Denon, which have 5 bit address.
 */

//1 00FF6897
//2 00FF9867
//3 00FFB04F
//4 00FF30CF
//5 RESERVADO FURBOT
//6 00FF7A85
//7 00FF10EF
//8 00FF38C7
//9 00FF5AA5
//0 00FF4AB5
//# 00FF52AD

//ATUAL
uint16_t sAddress_Array[]= {0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF};
uint8_t sCommand_Array[]= {0x68,0x98,0xB0,0x30,0x7A,0x10,0x38,0x52};

//TESTAR
//uint16_t sAddress_Array[]= {0xFF68,0xFF98,0xFFB0,0xFF30,0xFF7A,0xFF10,0xFF38};
//uint8_t sCommand_Array[]= {0x97,0x67,0x4F,0xCF,0x85,0xEF,0xC7};

uint16_t sAddress;
uint8_t sCommand;
uint8_t sRepeats = 3;

void loop() {
/*
while(true)
{  
    for(uint8_t i=0; i<=6; i++)
    {
      Serial.println();
      Serial.print(F("address=0x"));
      Serial.print(sAddress_Array[i], HEX);
      Serial.print(F(" command=0x"));
      Serial.print(sCommand_Array[i], HEX);
      Serial.println();
      Serial.println();
      Serial.flush();

      Serial.println(F("Send NEC with 8 bit address"));
      Serial.flush();
      IrSender.sendNEC(sAddress_Array[i], sCommand_Array[i], sRepeats);
      delay(DELAY_AFTER_SEND); // delay must be greater than 5 ms (RECORD_GAP_MICROS), otherwise the receiver sees it as one long signal

      client.loop(); 
    }  
}
  */
  
  if(digitalRead(CARD_DETECT) == LOW)
  {
      //delay(400); //Não é ruído
      if(digitalRead(CARD_DETECT) == LOW)
      {  
        while(digitalRead(CARD_DETECT) == LOW);
        delay(1500);
        Serial.println("Mandando foto...");
        takePictureAndSubmit(true);
        for(int i = 0; i<=10; i++)
        {
          delay(10);
          client.loop();
        }
        Serial.println("+++++++++++Detecting+++++++++++");
        takePictureAndSubmit(false);  
        for(int i = 0; i<=10; i++)
        {
          delay(10);
          client.loop();
        }
     }
  }

//1 00FF6897
//2 00FF9867
//3 00FFB04F
//4 00FF30CF
//5 RESERVADO FURBOT
//6 00FF7A85
//7 00FF10EF
//8 00FF38C7
//9 00FF5AA5
//0 00FF4AB5
//# 00FF52AD
 
  if(Got_Message==true)
    {
      Serial.println(Message);
      //1
      if(Message.indexOf("ANDARSUL")>=0)
          IrSender.sendNEC(sAddress_Array[0] , sCommand_Array[0], sRepeats);
      else
      //2
      if(Message.indexOf("ANDARNORTE")>=0)
            IrSender.sendNEC(sAddress_Array[1] , sCommand_Array[1], sRepeats);
      else
      //3
      if(Message.indexOf("VIRARESQUERDA")>=0)
          IrSender.sendNEC(sAddress_Array[2] , sCommand_Array[2], sRepeats);
      else
      //4
      if(Message.indexOf("VIRARDIREITA")>=0)
          IrSender.sendNEC(sAddress_Array[3] , sCommand_Array[3], sRepeats);
      else
      //6
      if(Message.indexOf("ANDARLESTE")>=0)
          IrSender.sendNEC(sAddress_Array[4] , sCommand_Array[4], sRepeats);
      else
      //7
      if(Message.indexOf("ANDAROESTE")>=0)
          IrSender.sendNEC(sAddress_Array[5] , sCommand_Array[5], sRepeats);
      else
      //8
      if(Message.indexOf("none")>=0)
          IrSender.sendNEC(sAddress_Array[6] , sCommand_Array[6], sRepeats);
      else
          IrSender.sendNEC(sAddress_Array[7] , sCommand_Array[7], sRepeats);          
      Got_Message = false;
    }  
    //Para liberar recurso para o WebServer, até algum evento ocorrer  
    for(uint8_t i=0; i<=79; i++)
    {
      client.loop();
      delay(250);
      if((digitalRead(CARD_DETECT) == LOW) || (Got_Message==true)) return;
    }
    //takePictureAndSubmit(false);      
}
