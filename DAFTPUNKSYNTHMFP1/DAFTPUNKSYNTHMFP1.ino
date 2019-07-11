#include "Queue.h"
#include <Bounce.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

elapsedMillis sensorFramePeriod = 0; // a timer used to only update sensor values after a certain period of time
elapsedMillis capacitiveFramePeriod = 0; // a timer used to update the cap sensors
elapsedMillis rollFramePeriod = 0; // a timer used to roll the notes

class Note
{
  public:
    byte noteInt;
    byte velocity;
};

// Constants
const float MAX_ROLL_FRAME = 500.0;
const byte CHANNEL = 1;
const int NOTES_PER_OCTAVE = 12;
const int MAX_NOTES = 10;   // The max number of notes that can be pressed at once.
const int OCTAVE_UP = 21;
const int OCTAVE_DOWN = 22;
const int ROLL_TOGGLE = 23;

// Button inputs
Bounce butOctaveUp = Bounce(OCTAVE_UP, 10);
Bounce butOctaveDown = Bounce(OCTAVE_DOWN, 10);
Bounce butRollToggle = Bounce(ROLL_TOGGLE, 10);

//Capacitive Inputs
Adafruit_MPR121 capA = Adafruit_MPR121();
// Current and previous state of the capacitive touch sensors
uint16_t currstateA = 0;
uint16_t prevstateA = 0;

// Note variables caclulated from Sensor Data
int octaveOffset = 0;
bool rollOn = true;
int timbreProfile = 0;
Queue<Note> queueNotes(MAX_NOTES);      // Queue of notes to play next in roll
Queue<Note> queueNoteOff(MAX_NOTES);    // Queue of notes to send off signals for
float rollSpeed = 0.7;    // Relative speed of roll/tremolo
float pitchBend = 0.0;    // Range(0.0,1.0)

void setup() {
  pinMode(OCTAVE_UP, INPUT);
  pinMode(OCTAVE_DOWN, INPUT);
  pinMode(ROLL_TOGGLE, INPUT);
  Serial.begin(9600);

  // start the "A" cap-touch board - 0x5A is the I2C address
  capA.begin(0x5A);
  if (!capA.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
}

void sendAllOffMessages() {
    // Send note off messages for all previously pressed notes when the number of notes changes
    while(queueNoteOff.count() > 0) {
      Note note = queueNoteOff.pop();
      usbMIDI.sendNoteOff(note.noteInt, note.velocity, CHANNEL);
    }
}

void readInputSensors() {
  butOctaveUp.update();
  butOctaveDown.update();
  butRollToggle.update();
  
  if (butOctaveUp.fallingEdge()) {
    octaveOffset += 1;
    Serial.println("Octave up");
  }
  if (butOctaveDown.fallingEdge()) {
    octaveOffset -= 1;
    Serial.println("Octave down");
  }
  if (butRollToggle.fallingEdge()) {
    rollOn = !rollOn;
    Serial.println("roll");
    // When roll control is changed, send all old off messages
    sendAllOffMessages();
    queueNotes.clear();
  }
  
  timbreProfile; //TODO
  rollSpeed; //TODO
  pitchBend; //TODO
}

void sendNoteOff(Note *note) {
  // Find the note in the OFF queue that has the same note (minus octave)
  // And send the off message.
  // The rest of the queue shall remain as is
  for (int n = 0; n < queueNoteOff.count(); n++) {
    Note comp = queueNoteOff.pop();
    if (comp.noteInt % 12 == note->noteInt % 12) {
      usbMIDI.sendNoteOff(comp.noteInt, comp.velocity, CHANNEL);
    } else {
      queueNoteOff.push(comp);
    }
  }
}

void readRollingCapacitiveNotes() {
  currstateA = capA.touched();
  if (currstateA != prevstateA) {
    sendAllOffMessages();
    queueNotes.clear();
    for(int pin = 0; pin < 6; pin++) {
      if (bitRead(currstateA,pin) == 1) {
        Note note = Note();
        note.noteInt = 24 + pin;
        note.velocity = 127;
        queueNotes.push(note);
      }
    }  
  }
  
  // Update our state
  prevstateA = currstateA;
}

void playCapacitiveNotes() {
  // Get the currently touched pads
  currstateA = capA.touched();

  for(int pin = 0; pin < 6; pin++) {
    if (bitRead(currstateA,pin) != bitRead(prevstateA,pin)) {
      Note note = Note();
      note.velocity = 127;
      // Offset the octave of the note immediately
      note.noteInt = 24 + pin + NOTES_PER_OCTAVE*octaveOffset;
      
      // Note has been pressed
      if (bitRead(currstateA,pin) == 1) {
        usbMIDI.sendNoteOn(note.noteInt, note.velocity, CHANNEL);
        queueNoteOff.push(note);
      // Otherwise note has been released
      } else {
        sendNoteOff(&note);        
      }
    }
  }

  // Update our state
  prevstateA = currstateA;
}

void playRollingNotes() {
  if (queueNotes.count() > 0) {
    // Send off messages here so that each note has an definite ending when playing rolls/tremolo
    sendAllOffMessages();
    // Play the first note in the queue, then push to the back of the queue
    Note note = queueNotes.pop();
    Note offsetNote = Note();
    // Use the current octave info to adjust the note
    offsetNote.velocity = note.velocity;
    offsetNote.noteInt = note.noteInt + NOTES_PER_OCTAVE*octaveOffset;
    usbMIDI.sendNoteOn(offsetNote.noteInt, offsetNote.velocity, CHANNEL);
    queueNoteOff.push(offsetNote);
    queueNotes.push(note);
  }
}

void loop() {
  if (sensorFramePeriod>100.0) {
    readInputSensors();
    sensorFramePeriod = 0;   
  }

  if (capacitiveFramePeriod>10.0) {
    if (rollOn) {
        readRollingCapacitiveNotes();
    } else {
        playCapacitiveNotes();
    }
    capacitiveFramePeriod = 0;
  }

  if(rollOn){
    if(rollFramePeriod>(MAX_ROLL_FRAME - rollSpeed*MAX_ROLL_FRAME)){
      playRollingNotes();
      rollFramePeriod = 0;
    }
  }
  
  // MIDI Controllers should discard incoming MIDI messages.
  while (usbMIDI.read()) {
  }
}
