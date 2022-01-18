// Super-basic UNTZtrument MIDI example.  Maps buttons to MIDI note
// on/off events; this is NOT a sequencer or anything fancy.
// Requires an Arduino Leonardo w/TeeOnArdu config (or a PJRC Teensy),
// software on host computer for synth or to route to other devices.

// updated by vongomben to work with the mini oontz controller 
// https://learn.adafruit.com/mini-untztrument-3d-printed-midi-controller
// using the Arduino Midi Usb Library https://docs.arduino.cc/tutorials/generic/midi-device




#include <Wire.h>
#include <Adafruit_Trellis.h>
#include <Adafruit_UNTZtrument.h>
#include "MIDIUSB.h"

#define LED     13 // Pin for heartbeat LED (shows code is working)
#define CHANNEL 1  // MIDI channel number

#ifndef HELLA
// A standard UNTZtrument has four Trellises in a 2x2 arrangement
// (8x8 buttons total).  addr[] is the I2C address of the upper left,
// upper right, lower left and lower right matrices, respectively,
// assuming an upright orientation, i.e. labels on board are in the
// normal reading direction.
Adafruit_Trellis     T[4];
Adafruit_UNTZtrument untztrument(&T[0], &T[1], &T[2], &T[3]);
const uint8_t        addr[] = { 0x70, 0x71,
                                0x72, 0x73
                              };
#else
// A HELLA UNTZtrument has eight Trellis boards...
Adafruit_Trellis     T[8];
Adafruit_UNTZtrument untztrument(&T[0], &T[1], &T[2], &T[3],
                                 &T[4], &T[5], &T[6], &T[7]);
const uint8_t        addr[] = { 0x70, 0x71, 0x72, 0x73,
                                0x74, 0x75, 0x76, 0x77
                              };
#endif // HELLA

// For this example, MIDI note numbers are simply centered based on
// the number of Trellis buttons; each row doesn't necessarily
// correspond to an octave or anything.
#define WIDTH     ((sizeof(T) / sizeof(T[0])) * 2)
#define N_BUTTONS ((sizeof(T) / sizeof(T[0])) * 16)
#define LOWNOTE   ((128 - N_BUTTONS) / 2)

uint8_t       heart        = 0;  // Heartbeat LED counter
unsigned long prevReadTime = 0L; // Keypad polling timer

// potentiometer variable

uint8_t mod;
uint8_t vel;
uint8_t fxc;
uint8_t rate;

void setup() {
  pinMode(LED, OUTPUT);
#ifndef HELLA
  untztrument.begin(addr[0], addr[1], addr[2], addr[3]);
#else
  untztrument.begin(addr[0], addr[1], addr[2], addr[3],
                    addr[4], addr[5], addr[6], addr[7]);
#endif // HELLA
  // Default Arduino I2C speed is 100 KHz, but the HT16K33 supports
  // 400 KHz.  We can force this for faster read & refresh, but may
  // break compatibility with other I2C devices...so be prepared to
  // comment this out, or save & restore value as needed.
#ifdef ARDUINO_ARCH_SAMD
  Wire.setClock(400000L);
#endif
#ifdef __AVR__
  TWBR = 12; // 400 KHz I2C on 16 MHz AVR
#endif
  untztrument.clear();
  untztrument.writeDisplay();

// analog reading. note that I used A2 for the trellis, so I'm skipping A2. 

  mod = map(analogRead(0), 0, 1023, 0, 127);
  vel = map(analogRead(1), 0, 1023, 0, 127);
  fxc = map(analogRead(3), 0, 1023, 0, 127);
  rate = map(analogRead(4), 0, 1023, 0, 127);


}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}

void loop() {
  unsigned long t = millis();
  if ((t - prevReadTime) >= 20L) { // 20ms = min Trellis poll time

    // addedd pots

    uint8_t newModulation = map(analogRead(0), 0, 1023, 0, 127);
    if (mod != newModulation) {
      mod = newModulation;
      // MidiUSB.sendMIDI(1, mod, 64);
      midiEventPacket_t eventMod = {0x0B, 0xB0 | 1, 10, mod};
      MidiUSB.sendMIDI(eventMod);
      MidiUSB.flush();
    }
    uint8_t newVelocity = map(analogRead(1), 0, 1023, 0, 127);
    if (vel != newVelocity) {
      vel = newVelocity;
      // MidiUSB.sendMIDI(11, vel, 64); {0x0B, 0xB0 | channel, control, value};

      midiEventPacket_t eventVel = {0x0B, 0xB0 | 11, 11, vel};
      MidiUSB.sendMIDI(eventVel);

      MidiUSB.flush();
    }
    uint8_t newEffect = map(analogRead(3), 0, 1023, 0, 127);
    if (fxc != newEffect) {
      fxc = newEffect;
      midiEventPacket_t eventfxc = {0x0B, 0xB0 | 12, 12, fxc};
      MidiUSB.sendMIDI(eventfxc);
      MidiUSB.flush();
    }
    uint8_t newRate = map(analogRead(4), 0, 1023, 0, 127);
    if (rate != newRate) {
      rate = newRate;
      midiEventPacket_t eventfxc = {0x0B, 0xB0 | 13, 13, rate};
      MidiUSB.sendMIDI(eventfxc);
      MidiUSB.flush();
    }
   


    if (untztrument.readSwitches()) { // Button state change?

      for (uint8_t i = 0; i < N_BUTTONS; i++) { // For each button...
        // Get column/row for button, convert to MIDI note number
        uint8_t x, y, note;
        untztrument.i2xy(i, &x, &y);
        note = LOWNOTE + y * WIDTH + x;
        if (untztrument.justPressed(i)) {
          noteOn(CHANNEL, note, 127);
          untztrument.setLED(i);
        } else if (untztrument.justReleased(i)) {
          noteOff(CHANNEL, note, 0);
          untztrument.clrLED(i);
        }
      }
      untztrument.writeDisplay();
    }
    prevReadTime = t;
    digitalWrite(LED, ++heart & 32); // Blink = alive
  }
}
