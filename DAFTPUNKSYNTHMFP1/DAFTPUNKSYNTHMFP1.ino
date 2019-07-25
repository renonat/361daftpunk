#include "Queue.h"
#include <Bounce.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

elapsedMillis sensorFramePeriod = 0; // a timer used to only update sensor values after a certain period of time
elapsedMillis capacitiveFramePeriod = 0; // a timer used to update the cap sensors
elapsedMillis rollFramePeriod = 0; // a timer used to roll the notes

// Constants
const float MAX_ROLL_FRAME = 500.0;
const byte CHANNEL = 1;
const int NOTES_PER_OCTAVE = 12;
const int MAX_NOTES = 10;   // The max number of notes that can be pressed at once.
const int OCTAVE_UP = 21;
const int OCTAVE_DOWN = 20;
const int ROLL_SPEED = 32;
const int AMPLITUDE = 31;

// Button inputs
Bounce butOctaveUp = Bounce(OCTAVE_UP, 10);
Bounce butOctaveDown = Bounce(OCTAVE_DOWN, 10);

//Capacitive Inputs
Adafruit_MPR121 capA = Adafruit_MPR121();
// Current and previous state of the capacitive touch sensors
uint16_t currstateA = 0;
uint16_t prevstateA = 0;

// Note variables caclulated from Sensor Data
int octaveOffset = 0;
bool rollOn = true;
Queue<int> queueNotes(MAX_NOTES);      // Queue of notes to play next in roll
Queue<int> queueNoteOff(MAX_NOTES);    // Queue of notes to send off signals for
float rollVelocity = 0.0;
float rollSpeedScaler = 0;
float rollSpeed = 0.7;    // Relative speed of roll/tremolo
float pitchBend = 0.0;    // Range(0.0,1.0)
float noteAmplitudeChng = 0;
float noteAmplitudeScaler = 0;
float noteAmplitude = 60.0;

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

void sendAllOffMessages() {
    // Send note off messages for all previously pressed notes when the number of notes changes
    while(queueNoteOff.count() > 0) {
      int note = queueNoteOff.pop();
      usbMIDI.sendNoteOff(note, noteAmplitude, CHANNEL);
    }
}

void readInputSensors() {
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
  

  rollVelocity = ((analogRead(ROLL_SPEED)-1.00)*0.85/926.00); // Potentiometer value changes from 11 to 1023
  //Serial.println(analogRead(ROLL_SPEED));
  //Serial.println(rollVelocity);
  rollSpeedScaler = (abs(rollVelocity - 0.40)/5.0);
  //Serial.println(rollSpeedScaler);
  if (rollVelocity < 0.39  && rollSpeed < 0.85){
    if(rollSpeed + rollSpeedScaler < 0.85) {
      rollSpeed += rollSpeedScaler;
    } else {
      rollSpeed = 0.85;
    }
    
  }else if(rollVelocity > 0.46 && rollSpeed > 0.00) {
    if(rollSpeed - rollSpeedScaler > 0.0){
      rollSpeed -= rollSpeedScaler;
    } else {
      rollSpeed = 0.0;
    }
  }
  updateRollToggle(rollSpeed);
  //Serial.print("rollspeed ");
  //Serial.println(rollSpeed);

  
  noteAmplitudeChng = (analogRead(AMPLITUDE)-1.00)*127.00/919.00; // Potentiometer value changes from 9 to 1023
  //Serial.println(noteAmplitudeChng);
  noteAmplitudeScaler = (abs(noteAmplitudeChng - 62.33)/5.0);
  if(noteAmplitudeChng > 64.00 && noteAmplitude < 127.0) {
     if ((noteAmplitude + noteAmplitudeScaler) < 127.0) {
      noteAmplitude += noteAmplitudeScaler;
     }else{
      noteAmplitude = 127.00;
     }
  } else if(noteAmplitudeChng < 61.00 && noteAmplitude > 0) {
    if ((noteAmplitude - noteAmplitudeScaler) > 0){
      noteAmplitude -= noteAmplitudeScaler;
     }else{
      noteAmplitude = 0.00;
     }
  }
  
  //Serial.print("amplitude ");
 // Serial.println(noteAmplitude);
}

void updateRollToggle(float new_rollspeed) {
  bool new_rollon = true;
  if (new_rollspeed <= 0.01) {
    new_rollon = false;
  }
  if (new_rollon != rollOn) {
    sendAllOffMessages();
  }
  rollOn = new_rollon;
}

void sendNoteOff(int note) {
  // Find the note in the OFF queue that has the same note (minus octave)
  // And send the off message.
  // The rest of the queue shall remain as is
  for (int n = 0; n < queueNoteOff.count(); n++) {
    int comp = queueNoteOff.pop();
    if (comp % 12 == note % 12) {
      usbMIDI.sendNoteOff(comp, noteAmplitude, CHANNEL);
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
    for(int pin = 0; pin < 12; pin++) {
      if (bitRead(currstateA,pin) == 1) {
        int note = 24 + pin;
        queueNotes.push(note);
      }
    }
    playRollingNotes();
    rollFramePeriod = 0.0;
  }
  
  // Update our state
  prevstateA = currstateA;
}

void playCapacitiveNotes() {
  // Get the currently touched pads
  currstateA = capA.touched();

  for(int pin = 0; pin < 12; pin++) {
    if (bitRead(currstateA,pin) != bitRead(prevstateA,pin)) {
      // Offset the octave of the note immediately
      int note = 24 + pin + NOTES_PER_OCTAVE*octaveOffset;
      
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

  // Update our state
  prevstateA = currstateA;
}

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
