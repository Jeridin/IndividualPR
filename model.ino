#include <string.h>

const byte BUZZER = 12;
const unsigned long TIMEOUT_MS = 2000;

// Note frequencies.
const unsigned int C = 262;
const unsigned int D = 294;
const unsigned int E = 330;
const unsigned int F = 349;
const unsigned int G = 392;
const unsigned int A = 440;

// Jingle Bells chorus.
// Each entry contains {frequency, number of beats}.
// A frequency of 0 means silence.
const unsigned int jingle[][2] = {
  {E, 1}, {E, 1}, {E, 2},
  {E, 1}, {E, 1}, {E, 2},
  {E, 1}, {G, 1}, {C, 1}, {D, 1}, {E, 4},
  {F, 1}, {F, 1}, {F, 1}, {F, 1},
  {F, 1}, {E, 1}, {E, 1}, {E, 1},
  {E, 1}, {D, 1}, {D, 1}, {E, 1}, {D, 2}, {G, 2},

  {E, 1}, {E, 1}, {E, 2},
  {E, 1}, {E, 1}, {E, 2},
  {E, 1}, {G, 1}, {C, 1}, {D, 1}, {E, 4},
  {F, 1}, {F, 1}, {F, 1}, {F, 1},
  {F, 1}, {E, 1}, {E, 1}, {E, 1},
  {G, 1}, {G, 1}, {F, 1}, {D, 1}, {C, 4},
  {0, 4}
};

// Twinkle, Twinkle, Little Star.
const unsigned int twinkle[][2] = {
  {C, 1}, {C, 1}, {G, 1}, {G, 1},
  {A, 1}, {A, 1}, {G, 2},
  {F, 1}, {F, 1}, {E, 1}, {E, 1},
  {D, 1}, {D, 1}, {C, 2},

  {G, 1}, {G, 1}, {F, 1}, {F, 1},
  {E, 1}, {E, 1}, {D, 2},
  {G, 1}, {G, 1}, {F, 1}, {F, 1},
  {E, 1}, {E, 1}, {D, 2},

  {C, 1}, {C, 1}, {G, 1}, {G, 1},
  {A, 1}, {A, 1}, {G, 2},
  {F, 1}, {F, 1}, {E, 1}, {E, 1},
  {D, 1}, {D, 1}, {C, 2},
  {0, 4}
};

// 0 = silent, 1 = happy, 2 = sad
byte currentSong = 0;
unsigned int noteIndex = 0;

unsigned long lastCommandTime = 0;
unsigned long noteStarted = 0;
unsigned long noteInterval = 0;

char command[16];
byte position = 0;
bool overflow = false;

void selectSong(byte requestedSong) {
  // Repeated commands keep playing the current song.
  if (requestedSong == currentSong) {
    return;
  }

  noTone(BUZZER);
  currentSong = requestedSong;
  noteIndex = 0;
  noteInterval = 0;
}

void setup() {
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  Serial.begin(9600);
}

void loop() {
  // Read commands from Python.
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n') {
      command[position] = '\0';

      if (!overflow) {
        if (strcmp(command, "happy") == 0) {
          lastCommandTime = millis();
          selectSong(1);
        } else if (strcmp(command, "sad") == 0) {
          lastCommandTime = millis();
          selectSong(2);
        } else if (strcmp(command, "off") == 0) {
          lastCommandTime = millis();
          selectSong(0);
        }
      }

      position = 0;
      overflow = false;

    } else if (c != '\r') {
      if (position < sizeof(command) - 1) {
        command[position++] = c;
      } else {
        overflow = true;
      }
    }
  }

  unsigned long now = millis();

  // Stop if Python stops communicating.
  if (now - lastCommandTime >= TIMEOUT_MS) {
    selectSong(0);
  }

  if (currentSong == 0) {
    return;
  }

  if (now - noteStarted < noteInterval) {
    return;
  }

  const unsigned int (*melody)[2];
  unsigned int length;
  unsigned int beatMs;

  if (currentSong == 1) {
    melody = twinkle;
    length = sizeof(twinkle) / sizeof(twinkle[0]);
    beatMs = 350;
  } else {
    melody = jingle;
    length = sizeof(jingle) / sizeof(jingle[0]);
    beatMs = 250;
  }

  unsigned int frequency = melody[noteIndex][0];
  unsigned long duration = melody[noteIndex][1] * beatMs;

  noTone(BUZZER);

  if (frequency > 0) {
    // Leave a short gap between notes.
    tone(BUZZER, frequency, duration * 9 / 10);
  }

  noteStarted = now;
  noteInterval = duration;
  noteIndex = (noteIndex + 1) % length;
}