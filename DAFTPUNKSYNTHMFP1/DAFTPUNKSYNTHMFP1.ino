#include "Queue.h"
#include <Bounce.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

elapsedMillis sensorFramePeriod = 0; // a timer used to only update sensor values after a certain period of time
elapsedMillis rollFramePeriod = 0; // a timer used to roll the notes

class Note
{
  public:
    byte noteInt;
    byte velocity;
};

// Constants
const int SENSOR_FRAME = 10.0;
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
Queue<Note> depressedNotes(MAX_NOTES);  // Currently depressed notes
Queue<Note> queueNotes(MAX_NOTES);      // Queue of notes to play next in roll
Queue<Note> queueNoteOff(MAX_NOTES);    // Queue of notes to send off signals for
float rollSpeed = 0.7;    // Relative speed of roll/tremolo
float pitchBend = 0.0;    // Range(0.0,1.0)

void setup() {
  pinMode(0, INPUT);    // sets the digital pin 0 as input
  pinMode(1, INPUT);    // sets the digital pin 1 as input
  pinMode(2, INPUT);    // sets the digital pin 2 as input
  pinMode(3, INPUT);    // sets the digital pin 3 as input
  pinMode(4, INPUT);    // sets the digital pin 4 as input
  pinMode(5, INPUT);    // sets the digital pin 5 as input
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

void sendOffMessages() {
    // Send note off messages for all previously pressed notes when the number of notes changes
    while(queueNoteOff.count() > 0) {
      Note note = queueNoteOff.pop();
      usbMIDI.sendNoteOff(note.noteInt, note.velocity, CHANNEL);
    }
}

void updateNotes(Queue<Note> *currentNotes) {
  // If the currently pressed notes have changed, then refresh our depressed notes tracker
  // Also reset the queue with the new set of notes
  if (currentNotes->count() != depressedNotes.count()) {
    // Send off messages here so that they are properly sent when the state changes
    sendOffMessages();
    queueNotes.clear();
    depressedNotes.clear();
    while (currentNotes->count() > 0) {
      Note note = currentNotes->pop();
      queueNotes.push(note);
      depressedNotes.push(note);
    }
  }
}

void readSensors() {
  currstateA = capA.touched();
  Serial.println(currstateA);
  
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
  }
  
  timbreProfile; //TODO
  rollSpeed; //TODO
  pitchBend; //TODO
  Queue<Note> currentNotes(MAX_NOTES);

  for (int pin = 0; pin < 6; pin++) {
    if (bitRead(currstateA,pin) == 1) {
      Note note = Note();
      note.noteInt = 24 + (pin - 1); // Base note is 24 (C1)
      note.velocity = 127;     // Placeholder value for the maximum velocity
      currentNotes.push(note);
    }
  }
  updateNotes(&currentNotes);
}


void transmitMessages() {
  if (queueNotes.count() > 0) {
    // Send off messages here so that each note has an definite ending when playing rolls/tremolo
    sendOffMessages();
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
//  usbMIDI.sendPitchBend(pitchBend * 16384, CHANNEL);
  //TODO: Also send messages related to other values
}

void loop() {
  // Update the instrument with parameters from the sensors
  // Only do this after a certain amount of time (so we're not always updating it on every loop)
  if(sensorFramePeriod>SENSOR_FRAME){ //TODO: Adjust this timing parameter
    readSensors();
    sensorFramePeriod = 0;   
  }
  // Preform this after the above, that way everything occurs in a single thread
  if(rollOn){
    if(rollFramePeriod>(MAX_ROLL_FRAME - rollSpeed*MAX_ROLL_FRAME)){
      transmitMessages();
      rollFramePeriod = 0;
    }
  } else {
    // No roll, so instantly play all the notes in the queue
    // The queue will not get updated until the current set of notes changes
    while (queueNotes.count() > 0) {
      Note note = queueNotes.pop();
      // Use the current octave info to adjust the note
      note.noteInt = note.noteInt + NOTES_PER_OCTAVE*octaveOffset;
      usbMIDI.sendNoteOn(note.noteInt, note.velocity, CHANNEL);
      queueNoteOff.push(note);
    }
  }

//   MIDI Controllers should discard incoming MIDI messages.
  while (usbMIDI.read()) {
  }
}
