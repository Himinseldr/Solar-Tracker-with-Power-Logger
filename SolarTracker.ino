#include <Servo.h>

// Servo objects
Servo servoX;  // Pan (left/right)
Servo servoY;  // Tilt (up/down)

// LDR pin definitions
const int LDR_tl = A0;
const int LDR_tr = A1;
const int LDR_bl = A2;
const int LDR_br = A3;

// ADC reference voltage (sesuaikan jika pakai 3.3V board)
const float ADC_REF = 5.0;

// Tuning
const int THRESH        = 20;           // Minimum light difference to move
const int STEP          = 1;            // Servo angle step
const int WAIT_MS       = 10;           // Delay after each move
const unsigned long STARTUP_MS = 1000;  // Don’t move servos for 1s

// Track last known angles to restore on attach
int angleX = 90;  // initial pan
int angleY = 90;  // initial tilt

bool servosAttached = false;
unsigned long startupTime;

void setup() {
  // Mulai serial
  Serial.begin(9600);
  Serial.println(">> Solar Tracker Starting <<");

  // Record when we powered up
  startupTime = millis();
  // Catatan: attach servo setelah STARTUP_MS
}

void loop() {
  unsigned long now = millis();

  // Baca LDR
  int tl = analogRead(LDR_tl);
  int tr = analogRead(LDR_tr);
  int bl = analogRead(LDR_bl);
  int br = analogRead(LDR_br);

  // Hitung tegangan
  float v_tl = tl * (ADC_REF / 1023.0);
  float v_tr = tr * (ADC_REF / 1023.0);
  float v_bl = bl * (ADC_REF / 1023.0);
  float v_br = br * (ADC_REF / 1023.0);

  // Print ke Serial: ADC & Voltase
  Serial.print("ADC [TL,TR,BL,BR]: ");
  Serial.print(bl); Serial.print(",");
  Serial.print(br); Serial.print(",");
  Serial.print(tl); Serial.print(",");
  Serial.print(tr);
  Serial.print("  |  V [TL,TR,BL,BR]: ");
  Serial.print(v_bl, 3); Serial.print("V, ");
  Serial.print(v_br, 3); Serial.print("V, ");
  Serial.print(v_tl, 3); Serial.print("V, ");
  Serial.print(v_tr, 3); Serial.println("V");

  // Compute differences
  int diffX = ((tl + bl) / 2) - ((tr + br) / 2);
  int diffY = ((tl + tr) / 2) - ((bl + br) / 2);

  // Setelah delay startup, attach dan restore posisi
  if (!servosAttached && now - startupTime >= STARTUP_MS) {
    servoX.attach(9);
    servoY.attach(10);
    servoX.write(angleX);
    servoY.write(angleY);
    delay(500);
    servosAttached = true;
  }
  if (!servosAttached) return;

  // Pan (X-axis)
  if (abs(diffX) > THRESH) {
    int newX = servoX.read() + (diffX > 0 ? STEP : -STEP);
    newX = constrain(newX, 0, 180);
    servoX.write(newX);
    angleX = newX;
    delay(WAIT_MS);
  }

  // Tilt (Y-axis)
  if (abs(diffY) > THRESH) {
    int newY = servoY.read() + (diffY > 0 ? -STEP : STEP);
    newY = constrain(newY, 0, 180);
    servoY.write(newY);
    angleY = newY;
    delay(WAIT_MS);
  }
}
