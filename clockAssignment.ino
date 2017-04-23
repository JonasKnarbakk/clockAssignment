#include <IRremote.h>
#include <ST7735_AD_ClockDisplay.h>
#include <Wire.h>
#include <RTClib.h>

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
   #define Serial SerialUSB
#endif

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

bool alarm = false,
    alarmActivated = false,
    setAlarmMode = false,
    interval = false;

int alarmDay = 0,
    alarmMonth = 0,
    alarmHour = 0,
    alarmMinute = 0,
    alarmSetState = 0,
    alarmSetOne = 0,
    alarmSetTwo = 0,
    alarmSetThree = 0,
    alarmSetFour = 0;

RTC_DS1307 rtc;
DateTime dateTime;

void setup(void) {
    Serial.begin(57600);

    pinMode(SPEAKER_PIN, OUTPUT);
    pinMode(IR_PIN, INPUT);
    irrecv.enableIRIn(); // Start the IR Reciever


    // We rotate the display so the bottom faces away from the arduino
    display.init();
    display.getScreen().setRotation(90);
    display.setDigital();
    /* display.setAnalog(); */

    Wire.begin();
    
    if(!rtc.begin()){
        Serial.println("Could not find RTC!");
    } else {
        if (!rtc.isrunning()) {
            Serial.println("RTC is NOT running!");
        }
        // following line sets the RTC to the date & time this sketch was compiled
        // uncomment it & upload to set the time, date and start run the RTC!
        rtc.adjust(DateTime(__DATE__, __TIME__));
        dateTime = rtc.now();
        display.setTime(dateTime.hour(), dateTime.minute(), dateTime.second(), dateTime.day(), dateTime.month(), dateTime.year());
    }

    // Set the alarm
    alarm = true;
    setAlarm(23, 4, 17, 33);

    /* setAlarmMode = true; */
}

void loop() {
    // Update time
    dateTime = rtc.now();
    display.setTime(dateTime.hour(), dateTime.minute(), dateTime.second(), dateTime.day(), dateTime.month(), dateTime.year());

    // Handle remote control
    if(irrecv.decode(&results)){
        translateIR();
        irrecv.resume(); // Recieve the next value
    }

    if(setAlarmMode){
        if((display.getCurrentSecond() % 2) == 0){
            if(!interval){
                display.drawSetAlarm(alarmSetOne, alarmSetTwo, alarmSetThree, alarmSetFour);
                interval = true;
            }
        } else{
            if(interval){
                switch(alarmSetState){
                    case 0:
                        display.getScreen().fillRect(33, 70, 13, 20, ST7735_BLACK);
                        break;
                    case 1:
                        display.getScreen().fillRect(44, 70, 13, 20, ST7735_BLACK);
                        break;
                    case 2:
                        display.getScreen().fillRect(68, 70, 13, 20, ST7735_BLACK);
                        break;
                    case 3:
                        display.getScreen().fillRect(81, 70, 13, 20, ST7735_BLACK);
                        break;
                }
                interval = false;
            }
        }
    } else {
        // Redraw the clock
        if(alarmActivated){
            display.drawActivatedAlarm();
            soundAlarm();
        } else {
            display.drawClock();
        }
        
        // Handle alarm
        if(checkAlarm()){
            alarmActivated = true;
        }
    }
}


void soundAlarm(){
    if((dateTime.second() % 2) == 0){
        if(interval){
            noTone(SPEAKER_PIN);
            tone(SPEAKER_PIN, 262, 1000);
            interval = false;
        }
    } else{
        if(!interval){
            noTone(SPEAKER_PIN);
            tone(SPEAKER_PIN, 330, 1000);
            interval = true;
        }
    }

    // After 45 seconds we we snooze the alarm for 5 minutes
    if(display.getCurrentSecond() > 30){
        snoozeAlarm(2);
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
    display.setAlarm(alarmHour, alarmMinute);
    display.getScreen().fillScreen(ST7735_BLACK);
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
            display.hideAlarm();
            alarm = false;
            break;
        case 0xFF02FD:
            Serial.println(" >>|        ");
            display.showAlarm();
            alarm = true;
            /* if(alarmActivated){ */
                /* snoozeAlarm(1); */
                /* display.redraw(); */
            /* } */
            break;
        case 0xFFC23D:
            Serial.println(" PLAY/PAUSE     ");
            snoozeAlarm(5);
            break;
        case 0xFFE01F:
            Serial.println(" VOL-           ");
            display.hideDate();
            break;
        case 0xFFA857:
            Serial.println(" VOL+           ");
            display.showDate();
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
            setAlarmInput(0);
            break;
        case 0xFF9867:
            Serial.println(" 100+           ");
            setAlarmMode = true;
            display.getScreen().fillScreen(ST7735_BLACK);
            break;
        case 0xFFB04F:
            Serial.println(" 200+           ");
            break;
        case 0xFF30CF:
            Serial.println(" 1              ");
            setAlarmInput(1);
            break;
        case 0xFF18E7:
            Serial.println(" 2              ");
            setAlarmInput(2);
            break;
        case 0xFF7A85:
            Serial.println(" 3              ");
            setAlarmInput(3);
            break;
        case 0xFF10EF:
            Serial.println(" 4              ");
            setAlarmInput(4);
            break;
        case 0xFF38C7:
            Serial.println(" 5              ");
            setAlarmInput(5);
            break;
        case 0xFF5AA5:
            Serial.println(" 6              ");
            setAlarmInput(6);
            break;
        case 0xFF42BD:
            Serial.println(" 7              ");
            setAlarmInput(7);
            break;
        case 0xFF4AB5:
            Serial.println(" 8              ");
            setAlarmInput(8);
            break;
        case 0xFF52AD:
            Serial.println(" 9              ");
            setAlarmInput(9);
            break;
        default:
            Serial.print(" unknown button   ");
            Serial.println(results.value, HEX);
            break;
    }
    delay(500);
}

void setAlarmInput(int value){
    if(setAlarmMode){
        switch(alarmSetState){
            case 0:
                display.getScreen().fillRect(33, 70, 13, 20, ST7735_BLACK);
                alarmSetOne = value;
                alarmSetState++;
                break;
            case 1:
                display.getScreen().fillRect(44, 70, 13, 20, ST7735_BLACK);
                alarmSetTwo = value;
                alarmSetState++;
                break;
            case 2:
                display.getScreen().fillRect(68, 70, 13, 20, ST7735_BLACK);
                alarmSetThree = value;
                alarmSetState++;
                break;
            case 3:
                alarmSetFour = value;
                Serial.print("Setting fourth value as");
                Serial.println(alarmSetFour);
                alarmSetState = 0;
                setAlarmMode = false;
                setAlarm(dateTime.day(), dateTime.month(), (alarmSetOne*10) + alarmSetTwo, (alarmSetThree*10) + alarmSetFour);
                display.setAlarm((alarmSetOne*10) + alarmSetTwo, (alarmSetThree*10) + alarmSetFour);
                display.getScreen().fillScreen(ST7735_BLACK);
                break;
        }
    }
}
