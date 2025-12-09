/**
 * @file DataLogger_extended.ino
 * @brief INA219-based datalogger with SSD1306 OLED and MicroSD logging.
 *        Logs every 3 minutes and timestamps entries as seconds since boot, writes to SD.
 * @date 10/06/2025
 */
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SSD1306AsciiAvrI2c.h>
#include <SdFat.h>

#define MS_PER_HOUR     3600000.0F  ///< Millis per hour
#define LOG_INTERVAL_MS 180000      ///< 3 minutes in milliseconds

volatile boolean timerOccurred = false;

#define OLED_RESET 4
SSD1306AsciiAvrI2c display;

Adafruit_INA219 ina219;
float current_mA    = 0.0F;
float loadvoltage_V = 0.0F;
float power_mW      = 0.0F;
float energy_mWh    = 0.0F;
unsigned long msSinceBoot = 0;

#define CHIPSELECT 10
SdFat32 sd;
File32 outputFile;

static void displayline(const float value, const uint8_t line, const char unit[]);
static void ina219values();
static void writeFile();

void setup() {
    // Disable ADC & comparator for power savings
    ADCSRA = 0;
    ACSR   = 0x80;

    // I2C & sensor init
    Wire.begin();
    ina219.begin();

    // SD card init
    sd.begin(CHIPSELECT);
    outputFile.open("DataLog.csv", O_WRITE | O_CREAT | O_TRUNC);
    // Update header to 'Seconds'
    outputFile.print("Seconds,Voltage,Current,Power,Energy\n");
    outputFile.sync();

    // OLED init
    display.begin(&Adafruit128x64, 0x3C, OLED_RESET);
    display.setFont(System5x7);
    display.clear();

    // Timer1 setup (~50ms intervals)
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;
    OCR1A  = 12499;
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (0 << CS12) | (1 << CS11) | (1 << CS10);
    TIMSK1 |= (1 << OCIE1A);
    sei();
}

void loop() {
    static float prevI = 0, prevV = 0, prevP = 0, prevE = 0;
    static unsigned long lastLogMs = 0;

    if (timerOccurred) {
        timerOccurred = false;
        ina219values();
        if (loadvoltage_V != prevV) { displayline(loadvoltage_V, 0, PSTR("V")); prevV = loadvoltage_V; }
        if (current_mA     != prevI) { displayline(current_mA,   2, PSTR("mA")); prevI = current_mA; }
        if (power_mW       != prevP) { displayline(power_mW,     4, PSTR("mW")); prevP = power_mW; }
        if (energy_mWh     != prevE) { displayline(energy_mWh,   6, PSTR("mWh")); prevE = energy_mWh; }
    }

    // Every LOG_INTERVAL_MS, write to SD
    if ((msSinceBoot - lastLogMs) >= LOG_INTERVAL_MS) {
        writeFile();
        lastLogMs = msSinceBoot;
    }
}

ISR(TIMER1_COMPA_vect) {
    timerOccurred = true;
}

static void displayline(const float value, const uint8_t line, const char unit[]) {
    char buf[16] = {0};
    dtostrf(value, 10, 3, buf);
    strcat_P(buf, PSTR(" "));
    strcat_P(buf, unit);
    display.setCursor(0, line);
    display.print(buf);
}

static void ina219values() {
    static unsigned long prev_ms = 0;
    ina219.powerSave(false);
    float shunt_mV   = ina219.getShuntVoltage_mV();
    float bus_V      = ina219.getBusVoltage_V();
    current_mA       = ina219.getCurrent_mA();
    msSinceBoot      = millis();
    ina219.powerSave(true);

    loadvoltage_V = bus_V + (shunt_mV / 1000.0F);
    power_mW      = loadvoltage_V * current_mA;
    energy_mWh   += (power_mW * (msSinceBoot - prev_ms)) / MS_PER_HOUR;
    prev_ms       = msSinceBoot;
}

static void writeFile() {
    // Convert time to seconds since boot
    unsigned long totalSec = msSinceBoot / 1000;

    char line[64], vStr[16] = {0}, iStr[16] = {0}, pStr[16] = {0}, eStr[16] = {0};
    dtostrf(loadvoltage_V, 10, 3, vStr);
    dtostrf(current_mA,    10, 3, iStr);
    dtostrf(power_mW,      10, 3, pStr);
    dtostrf(energy_mWh,    10, 3, eStr);
    // Write seconds instead of HH:MM:SS
    sprintf_P(line, PSTR("%lu,%s,%s,%s,%s\n"), totalSec, vStr, iStr, pStr, eStr);

    // Write to SD
    outputFile.write(line);
    outputFile.sync();
}
