// === Smart 4-Way Traffic System with Speed Detection, LED Control + Serial Logging ===

#define BUZZER 52
const float SPEED_SENSOR_DISTANCE = 0.1;  // 10 cm
const float SPEED_LIMIT = 100.0;          // km/h

const unsigned long MIN_GREEN_TIME = 10000;
const unsigned long MAX_CYCLE_TIME = 30000;
const unsigned long FIXED_GREEN_TIME = 10000;
const unsigned long YELLOW_TIME = 2000;
const int TRAFFIC_THRESHOLD = 6;

struct RoadConfig {
  char label;
  int trigCount, echoCount;
  int trigSpeed1, echoSpeed1;
  int trigSpeed2, echoSpeed2;
  int redLED, yellowLED, greenLED;
};

RoadConfig roads[4] = {
  {'A', 6, 7,   40, 41, 42, 43, 28, 29, 30},
  {'B', 8, 9,   44, 45, 46, 47, 31, 32, 33},
  {'C', 10, 11, 48, 49, 50, 51, 34, 35, 36},
  {'D', 26, 27, 22, 23, 24, 25, 37, 38, 39}
};

int vehicleCount[4] = {0, 0, 0, 0};
bool detected[4] = {false, false, false, false};
unsigned long startTime[4] = {0, 0, 0, 0};

enum LightState { PRE_GREEN_YELLOW, GREEN, PRE_RED_YELLOW };
LightState state = PRE_GREEN_YELLOW;
int currentRoadIndex = 0;
unsigned long stateStartTime = 0;
unsigned long greenDuration = 0;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 4; i++) {
    pinMode(roads[i].trigCount, OUTPUT); pinMode(roads[i].echoCount, INPUT);
    pinMode(roads[i].trigSpeed1, OUTPUT); pinMode(roads[i].echoSpeed1, INPUT);
    pinMode(roads[i].trigSpeed2, OUTPUT); pinMode(roads[i].echoSpeed2, INPUT);
    pinMode(roads[i].redLED, OUTPUT); pinMode(roads[i].yellowLED, OUTPUT); pinMode(roads[i].greenLED, OUTPUT);
  }
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  Serial.println("\nSmart Traffic System Started\n");
}

void loop() {
  readVehicleCounts();
  readVehicleSpeeds();

  unsigned long now = millis();
  switch (state) {
    case PRE_GREEN_YELLOW:
      showYellow(currentRoadIndex);
      if (now - stateStartTime >= YELLOW_TIME) {
        state = GREEN;
        computeGreenTime(currentRoadIndex);
        Serial.print("Road "); Serial.print(roads[currentRoadIndex].label);
        Serial.print(" GREEN for "); Serial.print(greenDuration / 1000); Serial.println(" seconds");
        showGreen(currentRoadIndex);
        stateStartTime = now;
      }
      break;

    case GREEN:
      if (now - stateStartTime >= greenDuration) {
        state = PRE_RED_YELLOW;
        showYellow(currentRoadIndex);
        stateStartTime = now;
      }
      break;

    case PRE_RED_YELLOW:
      if (now - stateStartTime >= YELLOW_TIME) {
        vehicleCount[currentRoadIndex] = 0;
        currentRoadIndex = (currentRoadIndex + 1) % 4;
        Serial.print("\nSwitching to Road "); Serial.println(roads[currentRoadIndex].label);

        if (currentRoadIndex == 0) printSummary();

        state = PRE_GREEN_YELLOW;
        stateStartTime = now;
      }
      break;
  }
}

void readVehicleCounts() {
  for (int i = 0; i < 4; i++) {
    float d = getDistance(roads[i].trigCount, roads[i].echoCount);
    if (d > 0 && d < 15 && !detected[i]) {
      vehicleCount[i]++;
      detected[i] = true;

      // Log to Serial for Database
      Serial.print("LOG,");
      Serial.print(millis()); Serial.print(",");
      Serial.print(roads[i].label); Serial.print(",");
      Serial.print("COUNT,");
      Serial.println(vehicleCount[i]);

      Serial.print("Vehicle detected on Road "); Serial.print(roads[i].label);
      Serial.print(" | Count: "); Serial.println(vehicleCount[i]);
    } else if (d >= 15) {
      detected[i] = false;
    }
  }
}

void readVehicleSpeeds() {
  for (int i = 0; i < 4; i++) {
    float d1 = getDistance(roads[i].trigSpeed1, roads[i].echoSpeed1);
    float d2 = getDistance(roads[i].trigSpeed2, roads[i].echoSpeed2);

    if (d1 > 0 && d1 < 15 && startTime[i] == 0) {
      startTime[i] = micros();
    }

    if (d2 > 0 && d2 < 15 && startTime[i] != 0) {
      unsigned long endTime = micros();
      float elapsed = (endTime - startTime[i]) / 1000000.0;
      float scaledDistance = SPEED_SENSOR_DISTANCE * 100.0;
      float speed = (scaledDistance / elapsed) * 3.6;

      if (speed < 500) {
        // Log to Serial for Database
        Serial.print("LOG,");
        Serial.print(millis()); Serial.print(",");
        Serial.print(roads[i].label); Serial.print(",");
        Serial.print("SPEED,");
        Serial.print(speed); Serial.print(",");
        Serial.println((speed > SPEED_LIMIT) ? "OVER" : "OK");

        Serial.print("Road "); Serial.print(roads[i].label);
        Serial.print(" | Speed: "); Serial.print(speed); Serial.println(" km/h");

        if (speed > SPEED_LIMIT) {
          Serial.println("Speeding detected! Buzzer ON.");
          digitalWrite(BUZZER, HIGH);
          delay(1000);
          digitalWrite(BUZZER, LOW);
        }
      }
      startTime[i] = 0;
    }
  }
}

void showGreen(int index) {
  allLightsOff();
  setRedOthers(index);
  digitalWrite(roads[index].greenLED, HIGH);
}

void showYellow(int index) {
  allLightsOff();
  setRedOthers(index);
  digitalWrite(roads[index].yellowLED, HIGH);
}

void allLightsOff() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(roads[i].redLED, LOW);
    digitalWrite(roads[i].yellowLED, LOW);
    digitalWrite(roads[i].greenLED, LOW);
  }
}

void setRedOthers(int activeIndex) {
  for (int i = 0; i < 4; i++) {
    if (i != activeIndex) digitalWrite(roads[i].redLED, HIGH);
  }
}

void computeGreenTime(int index) {
  int total = vehicleCount[0] + vehicleCount[1] + vehicleCount[2] + vehicleCount[3];
  if (total < TRAFFIC_THRESHOLD) {
    greenDuration = FIXED_GREEN_TIME;
    return;
  }
  float share = (float)vehicleCount[index] / total;
  greenDuration = share * MAX_CYCLE_TIME;
  if (greenDuration < MIN_GREEN_TIME) greenDuration = MIN_GREEN_TIME;
}

float getDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 20000);
  return duration * 0.034 / 2.0;
}

void printSummary() {
  Serial.println("\nVehicle Count Summary:");
  Serial.print("Road A: "); Serial.print(vehicleCount[0]); Serial.print(" | ");
  Serial.print("Road B: "); Serial.print(vehicleCount[1]); Serial.print(" | ");
  Serial.print("Road C: "); Serial.print(vehicleCount[2]); Serial.print(" | ");
  Serial.print("Road D: "); Serial.print(vehicleCount[3]); Serial.println(" | ");
}
