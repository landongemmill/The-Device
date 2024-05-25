#include <EEPROM.h>
#include <SD.h>
#include "TMRpcm.h"
#include "SPI.h"

#define SD_ChipSelectPin 10
#define Button_Pin_1 7
#define Button_Pin_2 6

#define EEPROM_Address 0
#define EEPROM_Address_Roam 1
#define EEPROM_Address_2 3

TMRpcm tmrpcm;

unsigned long roamingTime = 300000; // 5 minutes
unsigned long lastRoam = 0;

int lastState1 = HIGH;
int currentState1;

int lastState2 = HIGH;
int currentState2;

int currentFile = 1;
int currentRoamFile = 1;

// how are you reading this? anyway if you are, landon gemmill was here (3/27/2024)

void setup() {
  Serial.begin(9600);

  if(!SD.begin(SD_ChipSelectPin)) {
    Serial.println("SD fail, oopsies.");
    return;
  }

  currentRoamFile = EEPROM.read(EEPROM_Address_Roam);
  currentFile = EEPROM.read(EEPROM_Address) + (EEPROM.read(EEPROM_Address_2) * 255);

  Serial.println(currentFile);

  tmrpcm.speakerPin = 9;
  tmrpcm.quality(1);
  tmrpcm.setVolume(5);

  pinMode(Button_Pin_1, INPUT_PULLUP);
  pinMode(Button_Pin_2, INPUT_PULLUP);
}

void playOutput(bool isRoam, bool increment) {
    char filePath[50];
    File dir;
    
    if(isRoam) {
      dir = SD.open("/roaming");
      sprintf(filePath, "roaming/%d.wav", currentRoamFile);
      int fileCount = getFileCount(dir);

      if(currentFile < fileCount) {
        currentRoamFile += 1;
      } else {
        currentRoamFile = 0;
      }
    } else {
      dir = SD.open("/");
      int fileCount = getFileCount(dir);
      if(increment) {
        if(currentFile < fileCount) {
          currentFile += 1;
        } else {
          currentFile = 0;
        }
      }
      sprintf(filePath, "/%d.wav", currentFile);
    }

    if(currentFile > 255) {
      EEPROM.update(EEPROM_Address_2, 1);
      EEPROM.update(EEPROM_Address, currentFile - 255);
    } else {
      EEPROM.update(EEPROM_Address_2, 0);
      EEPROM.update(EEPROM_Address, currentFile);
    }

    EEPROM.update(EEPROM_Address_Roam, currentRoamFile);

    Serial.print("Now playing: ");
    Serial.println(filePath);

    tmrpcm.play(filePath);
    while (tmrpcm.isPlaying()) {}

    lastRoam = millis();
}

void loop() {
  currentState1 = digitalRead(Button_Pin_1);
  currentState2 = digitalRead(Button_Pin_2);

  if(lastState1 == LOW && currentState1 == HIGH) {
    playOutput(false, true);
  }

  if(lastState2 == LOW && currentState2 == HIGH) {
    playOutput(false, false);
  }

  lastState1 = currentState1;
  lastState2 = currentState2;

  if(millis() - lastRoam >= roamingTime) {
    playOutput(true, true);
  }
}

int getFileCount(File dir) {
  int fileCountOnSD = 0; // for counting files

  while (true) {
    File entry = dir.openNextFile();

    if (!entry) {
      break;
    }

    if(!entry.isDirectory()) {
      fileCountOnSD++;
    }

    entry.close();
  }

  dir.close();

  return fileCountOnSD;
}
