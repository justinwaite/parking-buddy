#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define LED_RED 12
#define LED_YELLOW 11
#define LED_GREEN 10
#define PIN_TRIG 5
#define PIN_ECHO 6
#define ENCODER_CLK 2
#define ENCODER_DT 3
#define ENCODER_BTN 4
#define SCREEN_CURR_DISTANCE 0
#define SCREEN_SET_DISTANCE 1

int distanceSetting = 27;
int distanceDeltaYellow = 72;
int distanceDeltaGreen = 144;
int ECHO_TIMEOUT = 3000;
int ECHO_MAX = 24864;
int btnDownTimestamp = 0;
int lastDistanceReadTimestamp = 0;
int currentDistance = 0;
int currentScreen = 1;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup()
{
    Serial.begin(115200);

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c))
    { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    display.display();
    delay(2000); // Pause for 2 seconds
    display.clearDisplay();

    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    pinMode(ENCODER_CLK, INPUT);
    pinMode(ENCODER_DT, INPUT);
    pinMode(ENCODER_BTN, INPUT_PULLUP);

    distanceSetting = EEPROM.read(0);

    if (distanceSetting == 255)
    {
        distanceSetting = 27;
    }

    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), readEncoder, FALLING);

    Serial.println("Beginning program.");
}

int getDistanceSetting()
{
    int value;
    noInterrupts();
    value = distanceSetting;
    interrupts();
    return value;
}

void setDistanceSetting(int value)
{
    noInterrupts();
    distanceSetting = value;
    EEPROM.write(0, value);
    interrupts();
}

void setLed()
{
    if (currentDistance <= distanceSetting)
    {
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_YELLOW, LOW);
        digitalWrite(LED_GREEN, LOW);
    }
    else if (currentDistance - distanceSetting <= distanceDeltaYellow)
    {
        digitalWrite(LED_YELLOW, HIGH);
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GREEN, LOW);
    }
    else if (currentDistance - distanceSetting <= distanceDeltaGreen)
    {
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_YELLOW, LOW);
        digitalWrite(LED_RED, LOW);
    }
    else
    {
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_YELLOW, LOW);
        digitalWrite(LED_RED, LOW);
    }
}

int getDistanceMeasurement()
{
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    int duration = pulseIn(PIN_ECHO, HIGH, ECHO_MAX);

    if (duration == 0)
    {
        duration = ECHO_MAX;
    }

    int currentDistance = duration / 148;

    return currentDistance;
}

void displayCurrentDistance()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.print(F("Distance: "));
    display.print(currentDistance);
    display.print(" in.");
    display.display();
}

void displaySetDistance()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.print(F("Set Distance: "));
    display.print(distanceSetting);
    display.print(" in.");
    display.display();
}

void displayDistanceSetMessage()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.println(F("Distance set!"));
    display.display();
    delay(2000);
}

void displayScreen()
{
    if (currentScreen == SCREEN_CURR_DISTANCE)
    {
        displayCurrentDistance();
    }
    else if (currentScreen == SCREEN_SET_DISTANCE)
    {
        displaySetDistance();
    }
}

void handleButtonPress()
{
    noInterrupts();
    if (digitalRead(ENCODER_BTN) == LOW && btnDownTimestamp == 0)
    {
        btnDownTimestamp = millis();
        Serial.println("Button Pressed");
    }
    else if (digitalRead(ENCODER_BTN) == LOW && millis() - btnDownTimestamp > 2000)
    {
        int currentDistance = getDistanceMeasurement();
        setDistanceSetting(currentDistance);
        btnDownTimestamp = 0;
        Serial.print("Set distance to: ");
        Serial.println(currentDistance);
        displayDistanceSetMessage();
    }
    else if (digitalRead(ENCODER_BTN) == HIGH && btnDownTimestamp != 0)
    {
        btnDownTimestamp = 0;
        Serial.println("Button Released");
    }
    interrupts();
}

void readEncoder()
{
    int dtValue = digitalRead(ENCODER_DT);
    int nextScreen = currentScreen;

    if (dtValue == HIGH)
    {
        nextScreen += 1;
    }
    if (dtValue == LOW)
    {
        nextScreen -= 1;
    }
    if (nextScreen < 0)
    {
        nextScreen = 1;
    }
    if (nextScreen > 1)
    {
        nextScreen = 0;
    }

    currentScreen = nextScreen;
}

void loop()
{
    if (millis() - lastDistanceReadTimestamp > 100)
    {
        currentDistance = getDistanceMeasurement();
    }

    setLed();

    displayScreen();

    handleButtonPress();
}