#include <IRremote.h>
#include <ST7735_AD_ClockDisplay.h>

#define TFT_SCLK 13
#define TFT_MOSI 11
#define TFT_CS 10
#define TFT_RST 8
#define TFT_DC 9

#define SPEAKER_PIN 5
#define IR_PIN 4

ST7335_AD_ClockDisplay display = ST7335_AD_ClockDisplay(TFT_CS,  TFT_DC, TFT_RST);
IRrecv irrecv(IR_PIN);
decode_results results;

bool alarm,
    alarmActivated = false;

// Timer variables to check if we need
// to redraw a value
int alarmDay = 0,
    alarmMonth = 0,
    alarmHour = 0,
    alarmMinute = 0;

void setup(void) {
    Serial.begin(9600);

    pinMode(SPEAKER_PIN, OUTPUT);
    pinMode(IR_PIN, INPUT);
    irrecv.enableIRIn(); // Start the IR Reciever


    // We rotate the display so the bottom faces away from the arduino
    display.init();
    display.getScreen().setRotation(90);
    display.setAnalog();
    /* display.setTime(20, 30, 0, 16, 4, 2017); */

    // Set the alarm
    /* alarm = true; */
    /* setAlarm(11, 4, 23, 59); */
}

void loop() {
    // Update the timer
    display.drawClock();
    
    // Handle alarm
    if(checkAlarm()){
        alarmActivated = true;
    }
    // Handle remote control
    if(irrecv.decode(&results)){
        translateIR();
        irrecv.resume(); // Recieve the next value
    }
}

void soundAlarm(){
    if((display.getCurrentSecond() % 2) == 0){
        noTone(SPEAKER_PIN);
        tone(SPEAKER_PIN, 262, 1000);
    } else{
        noTone(SPEAKER_PIN);
        tone(SPEAKER_PIN, 330, 1000);
    }

    // After 45 seconds we we snooze the alarm for 5 minutes
    if(display.getCurrentSecond() > 30){
        snoozeAlarm(5);
    }
}

void setAlarm(int day, int month, int hour, int minute){
    alarmDay = day;
    alarmMonth = month;
    alarmHour = hour;
    alarmMinute = minute;
}

void snoozeAlarm(int timeMinutes){
    alarmActivated = false;
    alarmMinute += timeMinutes;
    if(alarmMinute > 59){
        alarmHour++;
        alarmMinute -= 60;
        if(alarmHour > 23){
            alarmHour -= 24;
            alarmDay++;
        }
    }
}

bool checkAlarm(){
    return alarm 
    && (alarmDay == day(display.getTimer()) 
    && alarmMonth == month(display.getTimer()) 
    && alarmHour == hour(display.getTimer()) 
    && alarmMinute == minute(display.getTimer()) 
    && 0 == second(display.getTimer()));
}

void translateIR() // takes action based on IR code received describing Car MP3 IR codes
{

    switch(results.value){
    case 0xFFA25D:
        Serial.println(" CH-            ");
        break;
    case 0xFF629D:
        Serial.println(" CH             ");
        display.incrementFgColor();
        break;
    case 0xFFE21D:
        Serial.println(" CH+            ");
        display.incrementBgColor();
        break;
    case 0xFF22DD:
        Serial.println(" |<<          ");
        display.hideDate();
        break;
    case 0xFF02FD:
        Serial.println(" >>|        ");
        display.showDate();
        /* if(alarmActivated){ */
            /* snoozeAlarm(1); */
            /* display.redraw(); */
        /* } */
        break;
    case 0xFFC23D:
        Serial.println(" PLAY/PAUSE     ");
        Serial.print("Alarm is now: ");
        break;
    case 0xFFE01F:
        Serial.println(" VOL-           ");
        display.hideAlarm();
        break;
    case 0xFFA857:
        Serial.println(" VOL+           ");
        display.showAlarm();
        break;
    case 0xFF906F:
        Serial.println(" EQ             ");
        if(display.isAnalog()){
            display.setDigital();
        } else {
            display.setAnalog();
        }
        display.redraw();
        break;
    case 0xFF6897:
        Serial.println(" 0              ");
        break;
    case 0xFF9867:
        Serial.println(" 100+           ");
        break;
    case 0xFFB04F:
        Serial.println(" 200+           ");
        break;
    case 0xFF30CF:
        Serial.println(" 1              ");
        break;
    case 0xFF18E7:
        Serial.println(" 2              ");
        break;
    case 0xFF7A85:
        Serial.println(" 3              ");
        break;
    case 0xFF10EF:
        Serial.println(" 4              ");
        break;
    case 0xFF38C7:
        Serial.println(" 5              ");
        break;
    case 0xFF5AA5:
        Serial.println(" 6              ");
        break;
    case 0xFF42BD:
        Serial.println(" 7              ");
        break;
    case 0xFF4AB5:
        Serial.println(" 8              ");
        break;
    case 0xFF52AD:
        Serial.println(" 9              ");
        break;
    default:
        Serial.print(" unknown button   ");
        Serial.println(results.value, HEX);
    }
    delay(500);
}
