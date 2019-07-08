#include "Queue.h"

elapsedMillis sensorFramePeriod = 0; // a timer used to only update sensor values after a certain period of time
elapsedMillis rollFramePeriod = 0; // a timer used to roll the notes

class Note
{
  public:
    byte noteInt;
    byte velocity;
};

// Constants
const int SENSOR_FRAME = 100;
const float MAX_ROLL_FRAME = 500.0;
const byte CHANNEL = 1;
const int NOTES_PER_OCTAVE = 12;
const int MAX_NOTES = 10;   // The ma number of notes that can be pressed at once.

// Note variables caclulated from Sensor Data
int octaveOffset = 0;
int timbreProfile = 0;
Queue<Note> depressedNotes(MAX_NOTES);  // Currently depressed notes
Queue<Note> queueNotes(MAX_NOTES);      // Queue of notes to play next in roll
Queue<Note> queueNoteOff(MAX_NOTES);    // Queue of notes to send off signals for
float rollSpeed = 0.5;    // Relative speed of roll/tremolo
float pitchBend = 0.0;    //TODO: Scale seems wrong

void setup() {
  pinMode(0, INPUT);    // sets the digital pin 0 as input
  pinMode(1, INPUT);    // sets the digital pin 1 as input
  Serial.begin(9600);
}

//void loop() {
//  // Update the instrument with parameters from the sensors
//  // Only do this after a certain amount of time (so we're not always updating it on every loop)
//  if(sensorFramePeriod>SENSOR_FRAME){ //TODO: Adjust this timing parameter
//    readSensors();
//    sensorFramePeriod = 0;   
//  }
//  // Preform this after the above, that way everything occurs in a single thread
//  if(rollSpeed > 0.0){
//    if(rollFramePeriod>(MAX_ROLL_FRAME - rollSpeed*MAX_ROLL_FRAME)){
//      transmitMessages();
//      rollFramePeriod = 0;
//    }
//  } else {
//    // No roll, so instantly play all the notes in the queue
//    // The queue will not get updated until the current set of notes changes
//    while (queueNotes.count() > 0) {
//      Note note = queueNotes.pop();
//      // Use the current octave info to adjust the note
//      note.noteInt = note.noteInt + NOTES_PER_OCTAVE*octaveOffset;
//      Serial.println(note.noteInt);
////      usbMIDI.sendNoteOn(note.noteInt, note.velocity, CHANNEL);
//      queueNoteOff.push(note);
//    }
//  }

  // MIDI Controllers should discard incoming MIDI messages.
//  while (usbMIDI.read()) {
//  }
//}

void updateNotes(Queue<Note> currentNotes) {
  // If the currently pressed notes have changed, then refresh our depressed notes tracker
  // Also reset the queue with the new set of notes
//  if (currentNotes.count() != depressedNotes.count()) {
    // Send note off messages for all previously pressed notes when the number of notes changes
//    while(queueNoteOff.count() > 0) {
//      Note note = queueNoteOff.pop();
////    usbMIDI.sendNoteOff(note.noteInt, note.velocity, CHANNEL);
//    }
//    queueNotes.clear();
//    depressedNotes.clear();
//    while (currentNotes.count() > 0) {
//      Note note = currentNotes.pop();
//      queueNotes.push(note);
//      depressedNotes.push(note);
//    }
//  }
}

void readSensors() { //TODO
  octaveOffset; //TODO
  timbreProfile; //TODO
  rollSpeed; //TODO
  pitchBend; //TODO
  Queue<Note> currentNotes(MAX_NOTES);

  for (int pin = 0; pin <= 1; pin++) {
    if (digitalRead(pin) == 1) {
      Note note = Note();
      note.noteInt = pin;
      note.velocity = 1.0;
      currentNotes.push(note);
      Serial.print("Pushed note ");
      Serial.println(pin);
    }
  }
  Serial.print("Reading!");
  Serial.println(currentNotes.count());
//  updateNotes(currentNotes);
}


void transmitMessages() {
  if (queueNotes.count() > 0) {
    // Play the first note in the queue, then push to the back of the queue
    Note note = queueNotes.pop();
    // Use the current octave info to adjust the note
    note.noteInt = note.noteInt + NOTES_PER_OCTAVE*octaveOffset;
    Serial.println(note.noteInt);
//    usbMIDI.sendNoteOn(note.noteInt, note.velocity, CHANNEL);
    queueNoteOff.push(note);
    queueNotes.push(note);
  }
  //TODO: Also send messages related to other values
}

void loop() {
  if(sensorFramePeriod>SENSOR_FRAME){ //TODO: Adjust this timing parameter
    readSensors();
    sensorFramePeriod = 0;   
  }
}
