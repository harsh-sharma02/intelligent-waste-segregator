#include <Servo.h>

const int MOTOR_IN1 = 22;
const int MOTOR_IN2 = 23;
const int MOTOR_ENA = 5;
const int CONVEYOR_SPEED = 150;

const int SERVO_PLASTIC_PIN = 9;
const int SERVO_METAL_PIN = 10;
const int SERVO_PAPER_PIN = 11;

const int GATE_CLOSED_ANGLE = 0;
const int GATE_OPEN_ANGLE = 90;
const int GATE_OPEN_DURATION_MS = 1200;

Servo servoPlastic;
Servo servoMetal;
Servo servoPaper;

String incomingLabel = "";

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);

  servoPlastic.attach(SERVO_PLASTIC_PIN);
  servoMetal.attach(SERVO_METAL_PIN);
  servoPaper.attach(SERVO_PAPER_PIN);

  closeAllGates();
  startConveyor();

  Serial.println("[MEGA] Ready. Waiting for classification labels...");
}

void loop() {
  if (Serial.available()) {
    incomingLabel = Serial.readStringUntil('\n');
    incomingLabel.trim();

    if (incomingLabel.length() > 0) {
      handleClassification(incomingLabel);
    }
  }
}

void startConveyor() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  analogWrite(MOTOR_ENA, CONVEYOR_SPEED);
}

void stopConveyor() {
  analogWrite(MOTOR_ENA, 0);
}

void closeAllGates() {
  servoPlastic.write(GATE_CLOSED_ANGLE);
  servoMetal.write(GATE_CLOSED_ANGLE);
  servoPaper.write(GATE_CLOSED_ANGLE);
}

void actuateGate(Servo &servo, const char *label) {
  stopConveyor();
  servo.write(GATE_OPEN_ANGLE);
  delay(GATE_OPEN_DURATION_MS);
  servo.write(GATE_CLOSED_ANGLE);
  startConveyor();

  Serial1.print("SORTED:");
  Serial1.println(label);
}

void handleClassification(String label) {
  Serial.print("[MEGA] Received label: ");
  Serial.println(label);

  if (label == "PLASTIC") {
    actuateGate(servoPlastic, "PLASTIC");
  } else if (label == "METAL") {
    actuateGate(servoMetal, "METAL");
  } else if (label == "PAPER") {
    actuateGate(servoPaper, "PAPER");
  } else {
    Serial.println("[MEGA] Unknown label, item passed through without sorting.");
    Serial1.println("SORTED:UNKNOWN");
  }
}
