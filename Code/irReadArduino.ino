#define IR_PIN 4

#define OUT1_PIN 8   // CODE1 -> HIGH
#define OUT2_PIN 9   // CODE2 -> HIGH

// ===== PASTE LEARNED CODES HERE =====
const uint32_t CODE1 = 0xED12FB04;  // <-- replace
const uint32_t CODE2 = 0xEA15FB04;  // <-- replace

// NEC timing windows (microseconds)
const unsigned int LEAD_MARK_MIN  = 8500, LEAD_MARK_MAX  = 9500;
const unsigned int LEAD_SPACE_MIN = 4000, LEAD_SPACE_MAX = 5000;
const unsigned int REP_SPACE_MIN  = 2000, REP_SPACE_MAX  = 2600;

const unsigned int BIT_MARK_MIN   = 350,  BIT_MARK_MAX   = 800;
const unsigned int ZERO_SPACE_MIN = 350,  ZERO_SPACE_MAX = 800;
const unsigned int ONE_SPACE_MIN  = 1200, ONE_SPACE_MAX  = 2000;

const unsigned long TIMEOUT_US    = 30000;

static inline bool inRange(unsigned long v, unsigned long lo, unsigned long hi) {
  return (v >= lo && v <= hi);
}

static inline unsigned long measureLevel(uint8_t level, unsigned long timeoutUs) {
  unsigned long start = micros();
  while (digitalRead(IR_PIN) == level) {
    if (micros() - start >= timeoutUs) return 0;
  }
  return micros() - start;
}

bool readNEC(uint32_t &code, bool &isRepeat) {
  code = 0;
  isRepeat = false;

  unsigned long start = micros();
  while (digitalRead(IR_PIN) == HIGH) {
    if (micros() - start >= TIMEOUT_US) return false;
  }

  unsigned long leadMark = measureLevel(LOW, TIMEOUT_US);
  if (!inRange(leadMark, LEAD_MARK_MIN, LEAD_MARK_MAX)) return false;

  unsigned long leadSpace = measureLevel(HIGH, TIMEOUT_US);

  // Repeat frame
  if (inRange(leadSpace, REP_SPACE_MIN, REP_SPACE_MAX)) {
    unsigned long repMark = measureLevel(LOW, TIMEOUT_US);
    if (inRange(repMark, BIT_MARK_MIN, BIT_MARK_MAX)) {
      isRepeat = true;
      return true;
    }
    return false;
  }

  if (!inRange(leadSpace, LEAD_SPACE_MIN, LEAD_SPACE_MAX)) return false;

  for (uint8_t i = 0; i < 32; i++) {
    unsigned long mark = measureLevel(LOW, TIMEOUT_US);
    if (!inRange(mark, BIT_MARK_MIN, BIT_MARK_MAX)) return false;

    unsigned long space = measureLevel(HIGH, TIMEOUT_US);
    if (inRange(space, ONE_SPACE_MIN, ONE_SPACE_MAX)) {
      code |= (1UL << i);
    } else if (!inRange(space, ZERO_SPACE_MIN, ZERO_SPACE_MAX)) {
      return false;
    }
  }

  return true;
}

void setup() {
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);
  pinMode(OUT1_PIN, OUTPUT);
  pinMode(OUT2_PIN, OUTPUT);

  // start with both LOW
  digitalWrite(OUT1_PIN, LOW);
  digitalWrite(OUT2_PIN, LOW);

  Serial.println(F("UNO IR test: mutual-exclusive outputs"));
}

void loop() {
  uint32_t code;
  bool repeat;

  if (readNEC(code, repeat)) {
    if (repeat) return; // ignore hold-down repeats

    if (code == CODE1) {
      digitalWrite(OUT1_PIN, HIGH);
      digitalWrite(OUT2_PIN, LOW);
      Serial.println(F("CODE1 -> PIN 8 HIGH, PIN 9 LOW"));
    }
    else if (code == CODE2) {
      digitalWrite(OUT2_PIN, HIGH);
      digitalWrite(OUT1_PIN, LOW);
      Serial.println(F("CODE2 -> PIN 9 HIGH, PIN 8 LOW"));
    }
    else {
      Serial.print(F("Other code: 0x"));
      Serial.println(code, HEX);
    }

    delay(100);
  }
}

