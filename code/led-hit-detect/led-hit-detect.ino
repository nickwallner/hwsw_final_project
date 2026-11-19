#include <CircularBuffer.hpp>
// #define p_pin A0
// #define BUFFER_SIZE 100
// #define THRESHOLD 10

const int p_pin = A0;
const int BUFFER_SIZE = 100;
const int THRESHOLD = 10;
const float REARM_RATIO = 2.0;
const uint8_t FLASH_LENGTH = 200;
const uint8_t RPIN = 10, GPIN = 6, BPIN = 9;
const uint8_t LED_PIN[2] = {7,8};

struct RGB {uint8_t r, g, b; };
const RGB IDLE_COLOR = {0,0,0};
const RGB GOOD_COLOR = {0,255,0};   // correct hit green
const RGB OK_COLOR = {255,255,0};   // close hit yellow
const RGB MISS_COLOR = {255,0,0};   // miss red

RGB active_color[2] = {IDLE_COLOR, IDLE_COLOR};

int sta_len = 10;
int p_val = 0;
unsigned const long LOCKOUT_MS = 30;
unsigned long last_event = 0;
unsigned long now = 0;
unsigned long relative_event = 0;


long sum = 0;
float lta = 0;
float sta = 0;
float ratio = 0;

bool event_active = false;
bool flashing[2];

CircularBuffer<int, BUFFER_SIZE> buffer;
void flashled(RGB color) {
    analogWrite(RPIN, color.r);
    analogWrite(GPIN, color.g);
    analogWrite(BPIN, color.b);
}
void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN[0], OUTPUT);
    // pinMode(LED_PIN[1], OUTPUT);
    // digitalWrite(LED_PIN[1], LOW);
    digitalWrite(LED_PIN[0], HIGH);
    Serial.println("setup done");
}
void loop() {
    p_val = analogRead(p_pin);
    buffer.push(p_val);
    // STA: newest sta_len samples
    sum = 0;
    for (int i = 0; i < sta_len; i++) {
        sum += buffer[buffer.size() - 1 - i];
    }
    sta = (float)sum / sta_len;

    // LTA: only the OLDER samples, so the hit itself doesn't inflate the baseline
    long total = 0;
    for (int i = 0; i < buffer.size(); i++) {
        total += buffer[i];
    }
    int older = buffer.size() - sta_len;
    lta = (older > 0) ? (float)(total - sum) / older : 0;

    // floor the baseline at 1 instead of forcing ratio = 1
    ratio = sta / max(lta, 1.0f);

    now = millis();
    
    if(ratio > THRESHOLD && !event_active && now - last_event > LOCKOUT_MS) {
        event_active = true;
        last_event = now;
        relative_event = now >> 6;
        flashing[0] = true;
        switch (relative_event % 10) {
            case 0:
            case 1:
                active_color[0] = GOOD_COLOR;
                Serial.println("good");
                break;
            case 2:
            case 3:
            case 4:
                active_color[0] = OK_COLOR;
                Serial.println("ok");
                break;
            default:
                active_color[0] = MISS_COLOR;
                Serial.println("miss");
        }
        Serial.print("event detected: ");
        Serial.print(p_val);
        Serial.print(" , ");
        Serial.print(sta);
        Serial.print(" , ");
        Serial.print(lta);
        Serial.print(" , ");
        Serial.print(relative_event);
        Serial.print(" , ");
        Serial.println(now);
    }
    else if (event_active && ratio < REARM_RATIO && now - last_event > LOCKOUT_MS) {
        event_active = false;
        active_color[0] = IDLE_COLOR;
    }

    // if (last_event + FLASH_LENGTH == now) {
    //     flashing[0] = false;
    // }

    analogWrite(RPIN, active_color[0].r);
    analogWrite(GPIN, active_color[0].g);
    analogWrite(BPIN, active_color[0].b);
    
}