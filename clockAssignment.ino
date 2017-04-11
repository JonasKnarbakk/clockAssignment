#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Time.h>

#define TFT_SCLK 13
#define TFT_MOSI 11
#define TFT_CS 10
#define TFT_RST 8
#define TFT_DC 9

Adafruit_ST7735 screen = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

bool analog = true,
    alarm = true,
    alarmActivated = false,
    alarmSnooze = false,
    screenCleared = false;

time_t timer;

// Timer variables to check if we need
// to redraw a value
int lastSecond = 0,
    currentSecond = 0,
    lastHour = 0,
    lastMinute = 0,
    lastDay = 0,
    lastWeekday = 0,
    alarmDay = 0,
    alarmMonth = 0,
    alarmHour = 0,
    alarmMinute = 0;

// To keep track of analog watch hands
// so we can erase them on update
struct drawnLine {
    int x;
    int y;
    int xDestination;
    int yDestination;
    float angle;
};

drawnLine drawnLines[3];

void setup(void) {
    Serial.begin(9600);

    // Use this initializer if you're using a 1.8" TFT
    screen.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab

    screen.fillScreen(ST7735_BLACK);

    // Initialize timer variables
    setTime(22, 4, 55, 11, 4, 2017);
    timer = now();
    lastSecond = second(timer);
    lastMinute = minute(timer);
    lastHour = hour(timer);
    lastDay = day(timer);
    lastWeekday = weekday(timer);

    // Make sure the screen background i black
    screen.fillScreen(ST7735_BLACK);

    // Set the alarm
    alarm = true;
    setAlarm(11, 4, 22, 5);

    Serial.print("Hour value: ");
    Serial.println(hour(timer));
}

void loop() {
    // Only redraw the screen each second
    timer = now();
    currentSecond = second(timer);
    if(currentSecond != lastSecond){
        if(alarmActivated){
            drawActivatedAlarm();
        } else{
            if(analog){
                drawAnalogClock(timer);
            } else {
                drawDigitalClock(timer);
            }
        }
        lastSecond = currentSecond;
    }
    if(checkAlarm() && !alarmSnooze){
        alarmActivated = true;
    }
}


void drawDigitalClock(time_t &timer){

    // Draw a black rect over the seconds value
    screen.fillRect(103, screen.height()/2-5, 12, 7, ST7735_BLACK);

    // Draw a black rect over the minutes value
    if(lastMinute != minute(timer)){
        screen.fillRect(67, screen.height()/2-5, 34, 21, ST7735_BLACK);
        lastMinute = minute(timer);
    }
    // Draw a black rect over the hour value
    if(lastHour != hour(timer)){
        screen.fillRect(14, screen.height()/2-5, 34, 21, ST7735_BLACK);
        lastHour = hour(timer);
    }

    // Get the time value
    char * hh = calloc(3, sizeof(char));
    itoa(hour(timer), hh, 10);
    char * mm = calloc(3, sizeof(char));
    itoa(minute(timer), mm, 10);
    char * ss = calloc(3, sizeof(char));
    itoa(second(timer), ss, 10);

    // Set cursor position and text size for HH:MM
    screen.setCursor(screen.width()/2-50, screen.height()/2-5);
    screen.setTextColor(ST7735_WHITE);
    screen.setTextSize(3);

    // Draw hours
    if(hour(timer) < 10){
        screen.print("0");
    }
    screen.print(hh);
    screen.print(":");

    // Draw minutes
    if(minute(timer) < 10){
        screen.print("0");
    }
    screen.print(mm);

    // Adjust size for seconds
    screen.setTextSize(1);
    
    // Draw seconds
    if(second(timer) < 10){
        screen.print("0");
    }
    screen.print(ss);

    // Free allocated memory
    free(hh);
    free(mm);
    free(ss);

    drawDate(screen.width()/2, 115);

    if(alarm){
        drawAlarm(screen.width()/2, 55);
    }
}

void drawAnalogClock(time_t &timer){
    drawDate(screen.width()/2, 100);
    if(alarm){
        drawAlarm(screen.width()/2, 55);
    }
    drawWatchIndicators(screen.width(), screen.height(), 62, hour(timer), minute(timer), second(timer));
    drawWatchFace(screen.width(), screen.height(), 62);
}

void drawWatchFace(int width, int height, int radius){
    // Outer circle
    screen.drawCircle(width/2, height/2, radius, ST7735_WHITE);
    
    // Hour ticks
    for(int i = 0; i < 360; i+=30){
        float angle = i;

        angle = (angle * 3.14159265359 / 180); // Convert degrees to radians
        int x = (width/2+(sin(angle)*radius));
        int y = (height/2-(cos(angle)*radius));
        int x2 = (width/2+(sin(angle)*(radius-10)));
        int y2 = (height/2-(cos(angle)*(radius-10)));
        
        // Hack for the misbehaving hour ticks
        if(i == 90){
            x -= 10;
            x2 -= 10;
        }       
        if(i == 180){
            y -= 10;
            y2 -= 10;
        }

        screen.drawLine(x, y, x2, y2, ST7735_WHITE);
    }
}

void drawWatchIndicators(int width, int height, int radius, int hours, int minutes, int seconds){

    // Cover the previes indicator lines with black
    cleanUpLines(hours, minutes);

    // Draw the hand that indicates seconds
    drawHand(width/2, height/2, (seconds*6), 50, 0);
    
    // Draw the hand that indicates minutes
    drawHand(width/2, height/2, (minutes*6), 40, 1);

    // Draw the hand that indicates hours
    drawHand(width/2, height/2, (hours*30), 30, 2);

    lastMinute = minutes;
    lastHour = hours;
}

void drawHand(int x, int y, float angle, int lenght, int index){
    angle = (angle * 3.14159265359 / 180); // Convert degrees to radians
    int xDestination = (x+(sin(angle)*lenght));
    int yDestination = (y-(cos(angle)*lenght));
    if(angle == 0 || (angle > 4.71f && angle < 4.72f)){
        screen.drawLine(xDestination, yDestination, x, y, ST7735_WHITE);
    }else {
        screen.drawLine(x, y, xDestination, yDestination, ST7735_WHITE);
    }

    drawnLines[index].x = x;
    drawnLines[index].y = y;
    drawnLines[index].xDestination = xDestination;
    drawnLines[index].yDestination = yDestination;
    drawnLines[index].angle = angle;
}

void cleanUpLines(int hours, int minutes){
    // Clear the seconds hand
    if((drawnLines[0].angle < 0.01f) || (drawnLines[0].angle > 4.71f && drawnLines[0].angle < 4.72f)){
        screen.drawLine(drawnLines[0].xDestination, drawnLines[0].yDestination, drawnLines[0].x, drawnLines[0].y, ST7735_BLACK);
    } else {
        screen.drawLine(drawnLines[0].x, drawnLines[0].y, drawnLines[0].xDestination, drawnLines[0].yDestination, ST7735_BLACK);
    }

    // Clear the minute hand
    if(lastMinute != minutes){
        if(drawnLines[1].angle == 0 || (drawnLines[1].angle > 4.71f && drawnLines[1].angle < 4.72f)){
            screen.drawLine(drawnLines[1].xDestination, drawnLines[1].yDestination, drawnLines[1].x, drawnLines[1].y, ST7735_BLACK);
        } else {
            screen.drawLine(drawnLines[1].x, drawnLines[1].y, drawnLines[1].xDestination, drawnLines[1].yDestination, ST7735_BLACK);
        }
    }

    // Clear the hour hand
    if(lastHour != hours){
        if(drawnLines[2].angle == 0 || (drawnLines[2].angle > 4.71f && drawnLines[2].angle < 4.72f)){
            screen.drawLine(drawnLines[2].xDestination, drawnLines[2].yDestination, drawnLines[2].x, drawnLines[2].y, ST7735_BLACK);
        } else {
            screen.drawLine(drawnLines[2].x, drawnLines[2].y, drawnLines[2].xDestination, drawnLines[2].yDestination, ST7735_BLACK);
        }
    }
}

void drawDate(int x, int y){
    int currentDay = weekday(timer);

    if(lastWeekday != currentDay){
        screen.fillRect(x-25, y-5, 50, 10, ST7735_BLACK);
        lastWeekday = currentDay;
    }
    if(lastDay != day(timer)){
        screen.fillRect(x-25, y+5, 50, 10, ST7735_BLACK);
        lastDay = day(timer);
    }

    switch(currentDay){
        case 1: 
            drawTextCentered(x, y, "Sunday", 1, ST7735_WHITE);
            break;
        case 2:
            drawTextCentered(x, y, "Monday", 1, ST7735_WHITE);
            break;
        case 3:
            drawTextCentered(x, y, "Tuesday", 1, ST7735_WHITE);
            break;
        case 4:
            drawTextCentered(x, y, "Wednesday", 1, ST7735_WHITE);
            break;
        case 5:
            drawTextCentered(x, y, "Thursday", 1, ST7735_WHITE);
            break;
        case 6:
            drawTextCentered(x, y, "Friday", 1, ST7735_WHITE);
            break;
        case 7:
            drawTextCentered(x, y, "Saturday", 1, ST7735_WHITE);
            break;
    }
    drawTextCentered(x, y+10, "11/04/17", 1, ST7735_WHITE);
}

void drawAlarm(int x, int y){
    char *infoText = "Alarm: ";
    char *hourText = calloc(3, sizeof(char));
    itoa(alarmHour, hourText, 10);
    char *seperator = ":";
    char *minuteText = calloc(3, sizeof(char));
    itoa(alarmMinute, minuteText, 10);
    char * alarmText = calloc(strlen(infoText)
                                + strlen(hourText)
                                + strlen(seperator)
                                + strlen(minuteText) + 3, sizeof(char));
    strcat(alarmText, infoText);
    if(alarmHour < 10){
        strcat(alarmText, "0");
    }
    strcat(alarmText, hourText);
    strcat(alarmText, seperator);
    if(alarmMinute < 10){
        strcat(alarmText, "0");
    }
    strcat(alarmText, minuteText);
    drawTextCentered(x, y, alarmText, 1, ST7735_RED);

    // Free allocated memory
    free(hourText);
    free(minuteText);
    free(alarmText);
}

void drawActivatedAlarm(){
    if(!screenCleared){
        screen.fillScreen(ST7735_BLACK);
        screenCleared = true;
    }
    if((currentSecond % 2) == 0){
        drawTextCentered(screen.width()/2, screen.height()/2, "ALARM!", 3, ST7735_YELLOW);
    } else{
        drawTextCentered(screen.width()/2, screen.height()/2, "ALARM!", 3, ST7735_RED);
    }

    // TODO: remove when deactivate button is installed
    if(currentSecond > 5){
        alarmSnooze = true;
        alarmActivated = false;
        screen.fillScreen(ST7735_BLACK);
    }
}

void setAlarm(int day, int month, int hour, int minute){
    alarmDay = day;
    alarmMonth = month;
    alarmHour = hour;
    alarmMinute = minute;
}

bool checkAlarm(){
    return alarmDay == day(timer) && alarmMonth == month(timer) && alarmHour == hour(timer) && alarmMinute == minute(timer);
}

void drawTextCentered(int16_t x, int16_t y, const char *string, int8_t size,uint16_t color){
    if(string != NULL){
        screen.setTextColor(color);
        screen.setTextSize(size);
        screen.setCursor(x - (strlen(string)*3*size), y-(4*size));
        screen.print(string);
    }
}
