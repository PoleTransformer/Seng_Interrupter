#include <TimerOne.h>
#include <MIDI.h>
#include "pitches.h"

#define CLR(x,y) (x&=(~_BV(y)))
#define SET(x,y) (x|=(_BV(y)))
#define minVoice 1
#define maxVoice 3
#define vibratoSpeed 80000

byte currentNote[maxVoice + 1]; //current midi note on voice
byte currentChannel[maxVoice + 1]; //current midi channel on voice
float pitch[maxVoice + 1]; //current pitchval on voice
float onTime[maxVoice + 1]; //current ontime on voice
float offTime[maxVoice + 1]; //current offtime on voice
float dutyCycle[maxVoice + 1]; //current dutycycle on voice
unsigned long previousOff[maxVoice + 1]; //previous offtime on voice
unsigned long previousTime[maxVoice + 1]; //previous time on voice

float bendFactor[17]; //bendfactor midi channel
float originalBend[17]; //originalbend midi channel
float maxVibrato[17];
bool enableVibrato[17]; //enable vibrato midi channel
bool vibratoState[17]; //vibrato state midi channel
byte bendRange[17]; //bend range midi channel
byte controlVal1 = -1;
byte controlVal2 = -1;

MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
  for (int i = 1; i < 17; i++) {
    bendFactor[i] = 1;
    originalBend[i] = 1;
    bendRange[i] = 2;
    enableVibrato[i] = 0;
    vibratoState[i] = 0;
  }
  Timer1.initialize(vibratoSpeed);
  Timer1.attachInterrupt(vibrato);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.setHandleControlChange(handleControlChange);
  Serial.begin(115200);
}

void loop() {
  MIDI.read();
  unsigned long currentTime = micros();
  for (int i = minVoice; i <= maxVoice; i++) {
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
    for (int i = minVoice; i <= maxVoice; i++) {
      if (currentNote[i] == 0) {
        currentNote[i] = note;
        pitch[i] = pitchVals[note];
        currentChannel[i] = channel;
        dutyCycle[i] = mapF(velocity, 0.0, 127.0, 0, 0.015);
        reTrigger(channel);
        break;
      }
    }
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  for (int i = minVoice; i <= maxVoice; i++) {
    if (currentNote[i] == note && currentChannel[i] == channel) {
      currentNote[i] = 0;
      currentChannel[i] = 0;
      CLR(PORTD, i + 1);
      break;
    }
  }
}

void handlePitchBend(byte channel, int bend) {
  int mapped = map(bend * -1, -8192, 8191, 0, 64);
  if (bendRange[channel] == 12) {
    bendFactor[channel] = bendVal12[mapped];
  }
  else {
    bendFactor[channel] = bendVal2[mapped];
  }
  originalBend[channel] = bendFactor[channel];
  reTrigger(channel);
}

void handleControlChange(byte channel, byte firstByte, byte secondByte) {
  if (firstByte == 1) {
    if (secondByte > 0) {
      enableVibrato[channel] = true;
      maxVibrato[channel] = mapF(secondByte, 0.0, 127.0, 0.0, 0.05);
    }
    else {
      enableVibrato[channel] = false;
    }
  }
  if (firstByte == 100) {
    controlVal1 = secondByte;
  }
  if (firstByte == 101) {
    controlVal2 = secondByte;
  }
  if (firstByte == 6 && controlVal1 == 0 && controlVal2 == 0) {
    bendRange[channel] = secondByte;
    controlVal1 = -1;
    controlVal2 = -1;
  }
}

void vibrato() {
  for (int i = 1; i < 17; i++) {
    if (enableVibrato[i]) {
      if (vibratoState[i]) {
        bendFactor[i] = originalBend[i] + maxVibrato[i];
      }
      else {
        bendFactor[i] = originalBend[i] - maxVibrato[i];
      }
      vibratoState[i] = !vibratoState[i];
      reTrigger(i);
    }
  }
}

void reTrigger(byte channel) {
  for (int i = minVoice; i <= maxVoice; i++) {
    if (currentChannel[i] == channel) {
      onTime[i] = dutyCycle[i] * (pitch[i] * bendFactor[channel]);
      offTime[i] = (pitch[i] * bendFactor[channel]) - onTime[i];
    }
  }
}

float mapF(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
