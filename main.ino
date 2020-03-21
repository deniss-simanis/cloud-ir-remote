#include <IRLibAll.h>
#include <IRLibSendBase.h>
#include <IRLib_P01_NEC.h>    
#include <IRLibCombo.h> 
#include <ArduinoJson.h>
#include <aws_iot_mqtt.h>
#include <aws_iot_version.h>
#include "aws_iot_config.h"

#define MAX_TIME 150 // max ms between codes
#define LED_PIN 7
#define IR_RECEIVER_PIN 2
#define MAX_IR_CODES 10
#define MAX_DIGITS 3

int state = LOW;

IRsend mySender;

aws_iot_mqtt_client myClient; //  Object defines AWS IoT device
// char JSON_buf[100];           //  This is JSON formatted buffer used to read and update AWS Thing shadow
bool success_connect = false;

bool print_log(const char* src, int code) {
  bool ret = true;
  if(code == 0) {
    #ifdef AWS_IOT_DEBUG
      Serial.print(F("[LOG] command: "));
      Serial.print(src);
      Serial.println(F(" completed."));
    #endif
    ret = true;
  }
  else {
    #ifdef AWS_IOT_DEBUG
      Serial.print(F("[ERR] command: "));
      Serial.print(src);
      Serial.print(F(" code: "));
     Serial.println(code);
    #endif
    ret = false;
  }
  Serial.flush();
  return ret;
}

void msg_callback_delta(char* src, unsigned int len, Message_status_t flag) {
  StaticJsonBuffer<200> jsonBuffer;   //  JSON object for Arduino JSON library
  char JSON_buf[100];                 //  This is JSON formatted buffer used to read and update AWS Thing shadow
  // String payload;

int channelNumber = 0;
int channelDigits[MAX_DIGITS];
// unsigned long IRCodes_Samsung[MAX_IR_CODES] = {0xE0E08877, 0xE0E020DF, 0xE0E0A05F, 0xE0E0609F, 0xE0E010EF, 0xE0E0906F, 0xE0E050AF, 0xE0E030CF, 0xE0E0B04F, 0xE0E0708F};
unsigned long IRCodes_Panasonic_Old[MAX_IR_CODES] = {0x373119, 0x36113D, 0x37111D, 0x36912D, 0x37910D, 0x365135, 0x375115, 0x36D125, 0x37D105, 0x363139};
  
  if(flag == STATUS_NORMAL) {
    print_log("getDeltaKeyValue", myClient.getDeltaValueByKey(src, "", JSON_buf, 100));  // Get only  the whole delta section from AWS
    JsonObject& root = jsonBuffer.parseObject(JSON_buf);  // Use Arduino.JSON library to JSON object root
    channelNumber = root["channel"];
    
    Serial.print("We are going to switch to channel: ");
    Serial.println(channelNumber);

     if (channelNumber >= 1 && channelNumber <= 999) {
        channelDigits[2] = channelNumber % 10;
        channelDigits[1] = (channelNumber / 10) % 10;
        channelDigits[0] = (channelNumber / 100) % 10;

        /* Serial.print("Received Channel Digits: ");
        Serial.print(channelDigits[0]);
        Serial.print(channelDigits[1]);
        Serial.println(channelDigits[2]); */

        delay(500);

        Serial.println("");
        Serial.print("*** Switching to channel ");
        Serial.println(channelNumber);

        for (int i = 0; i < MAX_DIGITS; i++){
          int codeIndex = channelDigits[i];
          
          Serial.print("Sending digit: ");
          Serial.println(channelDigits[i]);
          
          // mySender.send(NECX, IRCodes_Samsung[codeIndex], 32);
          mySender.send(PANASONIC_OLD, IRCodes_Panasonic_Old[codeIndex], 22);
          delay(500);
        }
     }
    
    // mySender.send(NECX, IRCodes_Samsung[codeIndex], 32);
    // mySender.send(NECX, IRCodes_Samsung[channelNumber], 32);
    // mySender.send(PANASONIC_OLD, IRCodes_Panasonic_Old[codeIndex], 22);

    /* payload += "{\"state\":{\"reported\":{\"channel\":\"";
    payload += channelNumber;
    payload += "\"}}}";
    payload.toCharArray(JSON_buf, 100); */
    
    Serial.print("New Shadow: ");
    Serial.println(JSON_buf);
    // print_log("update thing shadow", myClient.shadow_update(AWS_IOT_MY_THING_NAME, JSON_buf, strlen(JSON_buf), NULL, 5));
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial);

  char curr_version[80];
  snprintf_P(curr_version, 80, PSTR("AWS IoT SDK Version(dev) %d.%d.%d-%s\n"), VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
  Serial.println(curr_version);

  if(print_log("setup", myClient.setup(AWS_IOT_CLIENT_ID))) {
    if(print_log("config", myClient.config(AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, AWS_IOT_ROOT_CA_PATH, AWS_IOT_PRIVATE_KEY_PATH, AWS_IOT_CERTIFICATE_PATH))) {
      if(print_log("connect", myClient.connect())) {
        success_connect = true;
        print_log("shadow init", myClient.shadow_init(AWS_IOT_MY_THING_NAME));
        print_log("register thing shadow delta function", myClient.shadow_register_delta_func(AWS_IOT_MY_THING_NAME, msg_callback_delta));
      }
    }
  }
}

void loop() {
  if(success_connect) {
    if(myClient.yield()) {
      Serial.println(F("Yield failed."));
    }
    delay(5000); // check for incoming delta per 100 ms
  }
}
