#define TRIG1_PIN 2
#define ECHO1_PIN 3
#define TRIG2_PIN 4
#define ECHO2_PIN 5

const float distanceBetweenSensors = 0.01; // meters
const int detectionThreshold = 10; // cm

volatile unsigned long timeSensor1 = 0;
volatile unsigned long timeSensor2 = 0;
volatile bool sensor1Triggered = false;
volatile bool sensor2Triggered = false;

float speed = 0;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG1_PIN, OUTPUT);
  pinMode(ECHO1_PIN, INPUT);
  pinMode(TRIG2_PIN, OUTPUT);
  pinMode(ECHO2_PIN, INPUT);

  Serial.println("Ultrasonic Speed Gun Ready");
}

void loop() {
  // Read distances
  int distance1 = getDistance(TRIG1_PIN, ECHO1_PIN);
  int distance2 = getDistance(TRIG2_PIN, ECHO2_PIN);

  // Trigger sensor 1
  if (!sensor1Triggered && distance1 > 0 && distance1 < detectionThreshold) {
    timeSensor1 = micros();
    sensor1Triggered = true;
    Serial.println("Sensor 1 triggered");
  }

  // Trigger sensor 2
  if (sensor1Triggered && !sensor2Triggered && distance2 > 0 && distance2 < detectionThreshold) {
    timeSensor2 = micros();
    sensor2Triggered = true;
    Serial.println("Sensor 2 triggered");
  }

  // Both triggered → calculate speed
  if (sensor1Triggered && sensor2Triggered) {
    sensor1Triggered = false;
    sensor2Triggered = false;

    unsigned long timeDifference = abs(timeSensor2 - timeSensor1); // microseconds
    if (timeDifference > 0) {
      speed = distanceBetweenSensors / (timeDifference / 3600000000.0); // m/s
      Serial.print("Speed: ");
      Serial.print(speed, 2);
      Serial.println(" km/h");
    } else {
      Serial.println("Invalid Reading");
    }

    delay(1000); // small delay before restarting next cycle
  }
}

// Get distance from ultrasonic sensor (in cm)
int getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // Timeout at 30ms (max ~5m)
  if (duration == 0) return -1; // No object detected
  int distance = duration * 0.034 / 2;
  return distance;
}
