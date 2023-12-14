#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <InterpolationLib.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket. Legit har ingen aning va en trinket är men den fungerar inte utan denna.
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define CLK 7
#define DT 8
#define SW 9
#define LED_PIN 6
#define LED_COUNT 24 

//Initialize
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

//Roulette variables
const int BUZZER_PIN = 3;
bool cycleComplete = false;
    // interpolation values: Display roulette
const int numValues1 = 3;
double xValues1[3] = { 0.0, 60.0, 120.0 }; // de här två gör up en funktion för lerp. Kunde gjort detta i for-loopen men jag ville testa.
double yValues1[3] = { 0.0, 60.0, 700.0 };
    // colors
uint32_t rouletteColors[24] = { 30208, 7733248, 7763574, 7733248, 7763574, 7733248, 7763574, 7733248, 7763574, 7733248, 7763574, 7733248, 30208, 7733248, 7763574, 7733248, 7763574, 7733248, 7763574, 7733248, 7763574, 7733248, 7763574, 7733248 };
uint32_t red = 7733248; // Color(127, 0, 0);
uint32_t white = 7763574; // Color(127, 127, 127);
uint32_t green = 30208; // Color(0, 127, 0);
uint32_t blue = 118; // Color(0, 0, 127);
uint32_t purple = strip.Color(255, 0, 255);


//Oled variables
const char* mainMenuElements[2] = { "PLAY", "DEBUG MODE" }; // text element 
const char* betMenuElements[5] = { "GREEN", "ODD", "EVEN", "WHITE" };
int counter = 0;
int currentStateCLK;
int lastStateCLK;
int btnState;
int selectedElement=0;
unsigned long lastButtonPress = 0;
bool buttonClicked = false;
bool frameUpdate;
uint8_t testScroll = 0;
    //Menu state
enum MenuState {
    MainMenu,
    BetMenu
};
MenuState currentMenu = MainMenu;
MenuState previousMenu = BetMenu;
    //Scroll symbol bitmap
#define SELECTSYMBOL_WIDTH 6
#define SELECTSYMBOL_HEIGHT 6
static const unsigned char PROGMEM selectSymbolBMP[] = {
    0b111111,
    0b111111,
    0b111111,
    0b111111,
    0b111111,
    0b111111
};

void setup(){
    //Pinmode för allt
    pinMode(CLK, INPUT);
    pinMode(DT, INPUT);
    pinMode(SW, INPUT_PULLUP);
    lastStateCLK = digitalRead(CLK);

    Serial.begin(9600);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ;
    }
    //Starta neopixel samt initialisera brightness, oled, starta upp roulette hjulet och ge en randomSeed till random funktionen
    strip.begin();
    strip.show();
    strip.setBrightness(255);
    display.display();
    delay(1000);
    display.clearDisplay();
    displayRoulette(red, white, green, 50);
    randomSeed(analogRead(A0));
}

void loop()
{
    //Hanterar meny kontroller och skrivandet till skärmen
    handleMenuControls();
    menuHandler();
}


//OLED--------------------------------------------------------------------------------------------------------------------------
void menuHandler() // En state manager som har olika funktioner beroende på enum variabeln currentMenu
{
    //Uppdaterar bara om det behövs. Sätter denna till true i funktionen som hanterar rotary encoder
    if (!frameUpdate) {
        return;
    }
    previousMenu = currentMenu;
    switch (currentMenu) {
    case MainMenu:
        drawElements(mainMenuElements, 2, 2, 2, 5, 10, false);
        if (buttonClicked)
        {
            switch (selectedElement)
            {
            case 0:
                currentMenu = BetMenu;
                break;
            case 1:
                debug();
                break;
            }
            buttonClicked = false;
        }
        

        break;
    case BetMenu: //menyn där man väljer vad man ska satsa på
        drawElements(betMenuElements, 4, 1, 4, 5, 5, true);
        if (buttonClicked)
        {
            launchBall(blue, purple, semiRandomGoal(3));
            buttonClicked = false;
        }
        switch (selectedElement)
        {
        case 0:
            betGreen(purple);
            break;
        case 1:
            betOdd(purple);
            break;
        case 2:
            betEven(purple);
            break;
        case 3:
            betWhite(purple);
            break;
        }
        break;
    }

    frameUpdate = false;
}

//Olika variabler för positioner av text element. Används för att skapa arrays av arraysd för att få rätt positioner. 
#define XPOS 0
#define YPOS 1
#define WIDTH 2
#define HEIGHT 3

//ritar element på skärmen. Fungerar så där. 
void drawElements(const char* textElements[], uint8_t arraySize, uint8_t textSize, uint8_t elementsOnScreen, uint8_t yMargin, uint8_t textPadding, bool scrollable)
{
    display.clearDisplay();
    uint8_t i, wordSize, elementsPos[arraySize][4];
    display.setTextSize(textSize);
    display.setTextColor(WHITE);
    int16_t x1, y1;
    uint16_t w, h;
    uint16_t y;
    //Hitta positioner för text element
    for (i = 0; i < arraySize; i++) {
        display.getTextBounds(textElements[i], 0, 0, &x1, &y1, &w, &h);
        y = i*((display.height()-yMargin*2)/elementsOnScreen);
        elementsPos[i][XPOS] = (display.width() / 2) - w / 2;
        elementsPos[i][YPOS] = y;  
        elementsPos[i][WIDTH] = w;
        elementsPos[i][HEIGHT] = h;
    }
    

    uint8_t cursorPosY;

    //Skriv ut textelement buffern
    for (i = 0; i < arraySize; i++) {
        if (i == abs(counter % arraySize)) {
            selectedElement = i;
            //Kollar om menyn ska kunna skrollas. T.ex main menyn behöver inte skrolla 
            if (scrollable)
            {
                selectElement(elementsPos[i][XPOS], elementsPos[i][YPOS]-elementsPos[i][YPOS]+yMargin, elementsPos[i][WIDTH], elementsPos[i][HEIGHT],
                selectSymbolBMP, SELECTSYMBOL_WIDTH, SELECTSYMBOL_HEIGHT);
            }else
            {
                selectElement(elementsPos[i][XPOS], elementsPos[i][YPOS]+yMargin, elementsPos[i][WIDTH], elementsPos[i][HEIGHT],
                selectSymbolBMP, SELECTSYMBOL_WIDTH, SELECTSYMBOL_HEIGHT);
            }          
        }
        if (scrollable)
        {
            //Sätter text cursor till positionerna
            display.setCursor(elementsPos[i][XPOS], elementsPos[i][YPOS]-elementsPos[selectedElement][YPOS]+yMargin);
        }else
        {
            display.setCursor(elementsPos[i][XPOS], elementsPos[i][YPOS]+yMargin);
        }
        
        display.print(textElements[i]);
    }
    if (scrollable)
    {
        drawScroll(yMargin, elementsPos[selectedElement][YPOS], elementsPos[arraySize-1][YPOS], selectSymbolBMP, SELECTSYMBOL_WIDTH, SELECTSYMBOL_HEIGHT);
    }
    //rita hela buffern
    display.display();
}

//ritar en låda samt en ikon bredvid selected element
void selectElement(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const unsigned char* BMP, uint8_t widthBMP, uint8_t heightBMP)
{
    display.fillRoundRect(
        x - 4,
        y - 4,
        w + 6,
        h + 6,
        4,
        BLACK);
    display.drawRoundRect(
        x - 4,
        y - 4,
        w + 6,
        h + 6,
        4,
        WHITE);
    display.drawBitmap(
        x + w + 3,
        y,
        BMP,
        widthBMP,
        heightBMP,
        WHITE);
}

//Ritar en scrollbar till menyn
void drawScroll(uint8_t yMargin, uint8_t selectedElementPos, uint8_t lastElementPos, const unsigned char *scrollSymbol, uint8_t symbolWidth, uint8_t symbolHeight){
    display.drawFastVLine(display.width()-5, yMargin, display.height()-yMargin*2, WHITE);
    display.drawFastHLine(display.width()-9, yMargin, 4, WHITE);
    display.drawBitmap(display.width()-11, map(selectedElementPos, yMargin, lastElementPos, 
    yMargin+1, display.height()-1-yMargin*2), 
    scrollSymbol, SELECTSYMBOL_WIDTH, SELECTSYMBOL_WIDTH, WHITE);
}

//Rotary encoder kontroller
void handleMenuControls()
{
    //lägger till om du vrider åt höger och vise versa
    currentStateCLK = digitalRead(CLK);
    if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
        if (digitalRead(DT) != currentStateCLK) {
            counter++;
        } else {
            counter--;
        }
        frameUpdate = true;
    }

    //Knapp på rotary encoder. Ganska lång check på 300ms för att motverka massa inputs.
    lastStateCLK = currentStateCLK;
    btnState = digitalRead(SW);
    if (btnState == LOW) {
        frameUpdate = true;
        if (millis() - lastButtonPress > 300){
            buttonClicked = true;
            lastButtonPress = millis();
        }
             delay(1);
    }
}

//Roulette--------------------------------------------------------------------------------------------------------------------------

//Vad rouletten kommer landa på, plus hur många gånger den ska snurra omkring
int semiRandomGoal(int revolutions)
{
    int randomNum = random(0, 23);
    return (revolutions * 24) + randomNum;
}

    //------------------------------------------------------------------------------------------------------------------------------

//Sätter på pixlarna på neopixel ringen en efter en. Ger en cool effect.
void displayRoulette(uint32_t red, uint32_t black, uint32_t green, int framerate)
{
    strip.clear();
    for (int i = 0; i < strip.numPixels(); i++) {
        if (i == 0 || i == 12) {
            strip.setPixelColor(i, green);
        } else if (i % 2 == 0) {
            strip.setPixelColor(i, black);
        } else {
            strip.setPixelColor(i, red);
        }
        strip.show();
        delay(framerate);
    }
}

    //------------------------------------------------------------------------------------------------------------------------------

//Skjuter bollen som hamnar på en random pixel
void launchBall(uint32_t ballColor, uint32_t betColor, int goal)
{

    int mappedPixel;
    int mappedPrevPixel;
    uint32_t prevColor = strip.getPixelColor(0);
    uint32_t lastColor;
    
    for (int i = 0; i < goal; i++) {
        //mappar alla index värden från 0-23, alltså en pixel på hjulet
        mappedPixel = i % 24;
        mappedPrevPixel = (i - 1) % 24;
        strip.setPixelColor(mappedPrevPixel, prevColor);
        prevColor = strip.getPixelColor(mappedPixel);
        strip.setPixelColor(mappedPixel, ballColor);
        //Spelar ett ljud varje gång den byter pixel
        tone(BUZZER_PIN, 262, 10);
        strip.show();
        //acceleration delay med interpolation.
        delay(Interpolation::SmoothStep(xValues1, yValues1, numValues1, i)); // interpolation enligt en graf. Behövs egentligen inte.
    }
    //Blinka pixeln bollen stanna på.
    flashPixel(mappedPixel, blue, 10, 70, 100);

    //Om bollen hamnar på en vinnande number så spelas några ljus effekter
    if (prevColor == betColor)
    {
        theaterChase(blue, 100);
        flash(blue, 5, 60, 100);
    }
    
}

    //------------------------------------------------------------------------------------------------------------------------------

//En funktion som hjälper om man vill ha en färg kod till en specific färg
uint32_t getColorCode(uint32_t color)
{
    strip.setPixelColor(1, color);
    return (strip.getPixelColor(1));
}

    //------------------------------------------------------------------------------------------------------------------------------

//flashar hela roulette hjulet
void flash(uint32_t flashColor, int flashes, float flashTime, float timeBetween)
{
    for (int i = 0; i < flashes; i++) {
        displayColor(flashColor);
        delay(flashTime);
        displayRouletteColors();
        delay(timeBetween);
    }
}

    //------------------------------------------------------------------------------------------------------------------------------

//Gör hela hjulet till en färg
void displayColor(uint32_t color)
{
    for (int i = 0; i < 24; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

    //------------------------------------------------------------------------------------------------------------------------------
//Gör hela hjulet till orginala roullete hjulet. Använder en array med alla färger i sig.
void displayRouletteColors()
{
    for (int i = 0; i < 24; i++) {
        strip.setPixelColor(i, rouletteColors[i]);
    }
    strip.show();
}

    //------------------------------------------------------------------------------------------------------------------------------

//En funktion som gör en snurrande och blinkande färg hjul typ. Använde en exempel kod som jag modifierma med displayRouletteColors();
void theaterChase(uint32_t color, int wait)
{
    for (int a = 0; a < 10; a++) { // Repeat 10 times...
        for (int b = 0; b < 3; b++) { //  'b' counts from 0 to 2...
            displayRouletteColors(); //Show original colours
            // 'c' counts up from 'b' to end of strip in steps of 3...
            for (int c = b; c < strip.numPixels(); c += 3) {
                strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
            }
            strip.show(); // Update strip with new contents
            delay(wait); // Pause for a moment
        }
    }
}

    //------------------------------------------------------------------------------------------------------------------------------

//flashar en pixel med en specifik färg.
void flashPixel(int pixel, uint32_t color, int flashAmount, float flashTime, float timeBetween)
{
    uint32_t prevColor = rouletteColors[pixel];

    for (int i = 0; i < flashAmount; i++) {
        strip.setPixelColor(pixel, color);
        strip.show();
        delay(flashTime);
        strip.setPixelColor(pixel, prevColor);
        strip.show();
        delay(timeBetween);
    }
}
    
    //------------------------------------------------------------------------------------------------------------------------------

//Funktioner som visar en specifik färg på vissa pixlar. Du kan satsa på grön, udda, jämnt och vitt. 

void betGreen(uint32_t color){
    displayRouletteColors();
    strip.setPixelColor(0, color);
    strip.setPixelColor(12, color);
    strip.show();
}

void betOdd(uint32_t color){
    displayRouletteColors();
    for (int i = 0; i < strip.numPixels(); i++)
    {
        if (i%2!=0)
        {
            strip.setPixelColor(i, color);
        }
    }
    strip.show();
}

void betEven(uint32_t color){
    displayRouletteColors();
    for (int i = 0; i < strip.numPixels(); i++)
    {
        if (i%2==0)
        {
            strip.setPixelColor(i, color);
        }
    }
    strip.show();
}

void betWhite(uint32_t color){
    displayRouletteColors();
    for (int i = 0; i < strip.numPixels(); i++)
    {
        if (i%2==0 && i!=0 && i!=12)
        {
            strip.setPixelColor(i, color);
        }
    }
    strip.show();
}

//En debug funktion som garanterar att man vinner. 
void debug(){
    betGreen(purple);
    launchBall(blue,purple, 13);
}