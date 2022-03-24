/**
 * @file SerialServer.h
 * @author Phil Schatzmann
 * @brief Displays the commands sent by SerialClient
 * @version 0.1
 * @date 2022-03-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// include the library code:
#include <LCD.h>

LCDClient client(Serial);

void setup() {
  // Setup Serial 
  Serial.begin(115200);
}

void loop() {
    client.process();
}