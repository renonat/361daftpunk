/*
  Systems Design Engineering 361
  Engineering Design
  MIDI Instrument Teensy 3.5 Code
  Written by, R. Natalizio, L. Choi, A. LePage
 */

#include "Queue.h"
#include <Bounce.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

/* ---- Timers ---- */
elapsedMillis sensorFramePeriod = 0;      // Only update sensor values after a certain period of time
elapsedMillis capacitiveFramePeriod = 0;  // Update the capacitive sensors (the 12 notes)
elapsedMillis rollFramePeriod = 0;        // Preform rolling/tremolo of notes

/* ---- CONSTANTS ---- */
const float MAX_ROLL_FRAME = 500.0;  // The maximum time in between repeated notes
const byte CHANNEL = 1;              // The MIDI channel
const int NOTES_PER_OCTAVE = 12;
const int BASE_NOTE = 24;
const int BASE_NOTE_RIGHT = 30;            // The base note id for our instrument.
const int BASE_NOTE_LEFT = 18;
const int MAX_NOTES = 10;            // The max number of notes that can be pressed at once.

/* ---- PIN NUMBERS ---- */
const int OCTAVE_UP = 21;   
const int OCTAVE_DOWN = 20;
const int ROLL_SPEED = 32;
const int AMPLITUDE = 31;

/* ---- Buttons ---- */
Bounce butOctaveUp = Bounce(OCTAVE_UP, 10);
Bounce butOctaveDown = Bounce(OCTAVE_DOWN, 10);

/* ---- CAPACITIVE KEY INPUTS ---- */
Adafruit_MPR121 capA = Adafruit_MPR121();
// Current and previous state of the capacitive touch sensors
uint16_t currstateA = 0;
uint16_t prevstateA = 0;

/* ---- INSTRUMENT CONTROL VARIABLES ---- */
int octaveOffset = 0;
bool rollOn = true;
float rollVelocity = 0.0;
float rollSpeedScaler = 0;
float rollSpeed = 0.7;          // Relative speed of roll/tremolo
float noteAmplitudeChng = 0;
float noteAmplitudeScaler = 0;
float noteAmplitude = 60.0;     // Range(0, 127)

/* ---- INSTRUMENT STATE VARIABLES ---- */
Queue<int> queueNotes(MAX_NOTES);      // Queue of notes to play next in roll
Queue<int> queueNoteOff(MAX_NOTES);    // Queue of notes to send off signals for

// Base Arduino setup
void setup() {
  pinMode(OCTAVE_UP, INPUT);
  pinMode(OCTAVE_DOWN, INPUT);
  pinMode(ROLL_SPEED, INPUT);
  pinMode(AMPLITUDE, INPUT);
  Serial.begin(9600);

  // start the "A" cap-touch board - 0x5A is the I2C address
  capA.begin(0x5A);
  if (!capA.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
}

// Send all note off messages in the note off queue
void sendAllOffMessages() {
    while(queueNoteOff.count() > 0) {
      int note = queueNoteOff.pop();
      usbMIDI.sendNoteOff(note, noteAmplitude, CHANNEL);
    }
}

// Read in instrument controls and update control variables.
void readInputSensors() {
  // Up and down button controls for Octaves
  butOctaveUp.update();
  butOctaveDown.update();
  
  if (butOctaveUp.risingEdge()) {
    octaveOffset += 1;
    Serial.println("Octave up");
  }
  if (butOctaveDown.risingEdge()) {
    octaveOffset -= 1;
    Serial.println("Octave down");
  }
  
  // Potentiometer input for roll speed
  rollVelocity = ((analogRead(ROLL_SPEED)-1.00)*0.85/926.00); // Potentiometer value changes from 11 to 1023
  rollSpeedScaler = (abs(rollVelocity - 0.40)/5.0);
  if (rollVelocity < 0.39  && rollSpeed < 0.85) {
    if(rollSpeed + rollSpeedScaler < 0.85) {
      rollSpeed += rollSpeedScaler;
    } else {
      rollSpeed = 0.85;
    } 
  } else if(rollVelocity > 0.46 && rollSpeed > 0.00) {
    if (rollSpeed - rollSpeedScaler > 0.0) {
      rollSpeed -= rollSpeedScaler;
    } else {
      rollSpeed = 0.0;
    }
  }
  // Function call also updates the boolean roll state
  updateRollToggle(rollSpeed);
  
  // Potentiometer input for note amplitude
  noteAmplitudeChng = (analogRead(AMPLITUDE)-1.00)*127.00/919.00; // Potentiometer value changes from 9 to 1023
  noteAmplitudeScaler = (abs(noteAmplitudeChng - 62.33)/5.0);
  if(noteAmplitudeChng > 64.00 && noteAmplitude < 127.0) {
    if ((noteAmplitude + noteAmplitudeScaler) < 127.0) {
      noteAmplitude += noteAmplitudeScaler;
    } else {
      noteAmplitude = 127.00;
    }
  } else if(noteAmplitudeChng < 61.00 && noteAmplitude > 0) {
    if ((noteAmplitude - noteAmplitudeScaler) > 0) {
      noteAmplitude -= noteAmplitudeScaler;
    } else {
      noteAmplitude = 0.00;
    }
  }
}

// Change roll state based on the new roll speed
void updateRollToggle(float new_rollspeed) {
  bool new_rollon = true;
  if (new_rollspeed <= 0.01) {
    new_rollon = false;
  }
  if (new_rollon != rollOn) {
    // When roll state changes, turn off all previous notes.
    // Don't clear the note queue, because roll should resume when state becomes true.
    sendAllOffMessages();
  }
  rollOn = new_rollon;
}

// Send note off message for previously pressed note with the same key.
//
// Since octave can change during a note press, and note off messages must
// correspond to the original note id, find the note in the off queue with 
// the same key and send that message.
void sendNoteOff(int note) {
  // Search through the queue, find note with same key (note % NOTES_PER_OCTAVE).
  // Remove only that note from the queue, send that note off message
  for (int n = 0; n < queueNoteOff.count(); n++) {
    int comp = queueNoteOff.pop();
    if (comp % NOTES_PER_OCTAVE == note % NOTES_PER_OCTAVE) {
      usbMIDI.sendNoteOff(comp, noteAmplitude, CHANNEL);
    } else {
      queueNoteOff.push(comp);
    }
  }
}

// Read all touched capacitive touch notes and overwrite the note queue.
void readRollingCapacitiveNotes() {
  // Only overwrite the queue if the note state changed.
  currstateA = capA.touched();
  if (currstateA != prevstateA) {
    sendAllOffMessages();
    queueNotes.clear();
    for(int pin = 0; pin < 12; pin++) {
      if (bitRead(currstateA,pin) == 1) {
        int note = 0;
        if(BASE_NOTE + pin >= 30) {
          note = BASE_NOTE_LEFT + pin;
        } else {
          note = BASE_NOTE_RIGHT + pin;
        }
        queueNotes.push(note);
      }
    }
    // Play the notes as soon as the state updates for instantaneous feedback.
    playRollingNotes();
    // Reset the roll timer so that all notes have consistent spacing.
    rollFramePeriod = 0.0;
  }
  
  prevstateA = currstateA;
}

// Play the queued notes in a tremolo/roll. Follows a circular queue pattern.
void playRollingNotes() {
  if (queueNotes.count() > 0) {
    // Send off messages here so that each note has an definite ending when playing rolls/tremolo
    sendAllOffMessages();
    // Play the first note in the queue, then push to the back of the queue
    int note = queueNotes.pop();
    // Use the current octave info to adjust the note
    int offsetNote = note + NOTES_PER_OCTAVE*octaveOffset;
    usbMIDI.sendNoteOn(offsetNote, noteAmplitude, CHANNEL);
    queueNoteOff.push(offsetNote);
    queueNotes.push(note);
  }
}

// Play (non-rolling) notes directly from the sensed capacitive keys.
void playCapacitiveNotes() {
  // Get the currently touched pads
  currstateA = capA.touched();

  for(int pin = 0; pin < 12; pin++) {
    if (bitRead(currstateA,pin) != bitRead(prevstateA,pin)) {
      // Offset the octave of the note immediately
      int note = BASE_NOTE + pin + NOTES_PER_OCTAVE*octaveOffset;
      
      // Note has been pressed
      if (bitRead(currstateA,pin) == 1) {
        usbMIDI.sendNoteOn(note, noteAmplitude, CHANNEL);
        queueNoteOff.push(note);
      // Otherwise note has been released
      } else {
        sendNoteOff(note);        
      }
    }
  }

  prevstateA = currstateA;
}

// Main program loop
void loop() {
  if (sensorFramePeriod > 100.0) {
    readInputSensors();
    sensorFramePeriod = 0;   
  }

  if (capacitiveFramePeriod > 10.0) {
    if (rollOn) {
        readRollingCapacitiveNotes();
    } else {
        playCapacitiveNotes();
    }
    capacitiveFramePeriod = 0;
  }

  if (rollOn) {
    if (rollFramePeriod>(MAX_ROLL_FRAME - rollSpeed*MAX_ROLL_FRAME)) {
      playRollingNotes();
      rollFramePeriod = 0;
    }
  }
  
  // MIDI Controllers should discard incoming MIDI messages.
  while (usbMIDI.read()) {
  }
}
