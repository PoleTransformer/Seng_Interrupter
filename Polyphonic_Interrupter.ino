#include <TimerOne.h>
#include <MIDI.h>
#include "pitches.h"

#define CLR(x,y) (x&=(~_BV(y)))
#define SET(x,y) (x|=(_BV(y)))
#define startChannel 1
#define maxChannel 5

byte currentNote[maxChannel + 1];
byte currentChannel[maxChannel + 1];
float bendFactor[maxChannel + 1];
float pitch[maxChannel + 1];
float onTime[maxChannel + 1];
float offTime[maxChannel + 1];
float dutyCycle[maxChannel + 1];

unsigned long previousOff[maxChannel + 1];
unsigned long previousTime[maxChannel + 1];

MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
  for (int i = startChannel; i <= maxChannel; i++) {
    bendFactor[i] = 1;
  }
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandlePitchBend(handlePitchBend);
  Serial.begin(115200);
}

void loop() {
  MIDI.read();
  unsigned long currentTime = micros();
  for (int i = startChannel; i <= maxChannel; i++) {
    if (currentNote[i] != 0 && currentTime - previousTime[i] >= onTime[i]) {
      if (currentTime - previousOff[i] >= offTime[i]) {
        CLR(PORTD, i + 1);
        previousOff[i] = currentTime;
      }
      else {
        SET(PORTD, i + 1);
      }
      previousTime[i] = currentTime;
    }
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (channel != 10) {
    for (int i = startChannel; i <= maxChannel; i++) {
      if (currentNote[i] == 0) {
        currentNote[i] = note;
        pitch[i] = pitchVals[note];
        dutyCycle[i] = mapF(velocity, 0.0, 127.0, 0, 0.015);
        currentChannel[i] = channel;
        reTrigger(i);
        break;
      }
    }
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  for (int i = startChannel; i <= maxChannel; i++) {
    if (currentNote[i] == note) {
      currentNote[i] = 0;
      currentChannel[i] = 0;
      CLR(PORTD, i + 1);
      break;
    }
  }
}

void handlePitchBend(byte channel, int bend) {
  float bendF = bend * -1;
  bendF /= 8192;
  bendF *= 2;
  bendF /= 12;
  bendFactor[channel] = pow(2, bendF);
  for (int i = startChannel; i <= maxChannel; i++) {
    if (currentChannel[i] == channel) {
      reTrigger(i);
    }
  }
}

void reTrigger(byte channel) {
  onTime[channel] = dutyCycle[channel] * (pitch[channel] * bendFactor[channel]);
  offTime[channel] = (pitch[channel] * bendFactor[channel] ) - onTime[channel];
}

float mapF(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
