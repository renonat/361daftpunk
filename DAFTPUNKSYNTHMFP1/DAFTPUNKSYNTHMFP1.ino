#include "Queue.h"
#include <Bounce.h>
elapsedMillis sensorFramePeriod = 0; // a timer used to only update sensor values after a certain period of time
elapsedMillis rollFramePeriod = 0; // a timer used to roll the notes

class Note
{
  public:
    byte noteInt;
    byte velocity;
};

// Constants
const int SENSOR_FRAME = 1.0;
const float MAX_ROLL_FRAME = 500.0;
const byte CHANNEL = 1;
const int NOTES_PER_OCTAVE = 12;
const int MAX_NOTES = 10;   // The ma number of notes that can be pressed at once.
const int OCTAVE_UP = 23;
const int OCTAVE_DOWN = 22;

// Button inputs
Bounce butOctaveUp = Bounce(OCTAVE_UP, 10);
Bounce butOctaveDown = Bounce(OCTAVE_DOWN, 10);

// Note variables caclulated from Sensor Data
int octaveOffset = 0;
int timbreProfile = 0;
Queue<Note> depressedNotes(MAX_NOTES);  // Currently depressed notes
Queue<Note> queueNotes(MAX_NOTES);      // Queue of notes to play next in roll
Queue<Note> queueNoteOff(MAX_NOTES);    // Queue of notes to send off signals for
float rollSpeed = 0.0;    // Relative speed of roll/tremolo
float pitchBend = 0.0;    //TODO: Scale seems wrong

void setup() {
  pinMode(0, INPUT);    // sets the digital pin 0 as input
  pinMode(1, INPUT);    // sets the digital pin 1 as input
  pinMode(2, INPUT);    // sets the digital pin 2 as input
  pinMode(3, INPUT);    // sets the digital pin 3 as input
  pinMode(4, INPUT);    // sets the digital pin 4 as input
  pinMode(5, INPUT);    // sets the digital pin 5 as input
  pinMode(OCTAVE_UP, INPUT);
  pinMode(OCTAVE_DOWN, INPUT);
  Serial.begin(9600);
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

void readSensors() { //TODO
  octaveOffset; //TODO
  butOctaveUp.update();
  butOctaveDown.update();
  if (butOctaveUp.fallingEdge()) {
    Serial.println("Up");
  }
  if (butOctaveDown.fallingEdge()) {
    Serial.println("Down");
  }
  timbreProfile; //TODO
  rollSpeed; //TODO
  pitchBend; //TODO
  Queue<Note> currentNotes(MAX_NOTES);

  for (int pin = 0; pin <= 5; pin++) {
    if (digitalRead(pin) == 1) {
      Note note = Note();
      note.noteInt = 24 + pin; // Multiplied by 10 for testing
      note.velocity = 127;
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
    // Use the current octave info to adjust the note
    note.noteInt = note.noteInt + NOTES_PER_OCTAVE*octaveOffset;
//    Serial.println(note.noteInt);
    usbMIDI.sendNoteOn(note.noteInt, note.velocity, CHANNEL);
    queueNoteOff.push(note);
    queueNotes.push(note);
  }
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
  if(rollSpeed > 0.0){
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
