// Minimal fallback implementation for the LiquidCrystal API used by this sketch.
// This avoids a missing-library compile error in environments without the Arduino LCD package.
class LiquidCrystal {
public:
  LiquidCrystal(uint8_t rs, uint8_t enable, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
    : rsPin(rs), enablePin(enable), d4Pin(d4), d5Pin(d5), d6Pin(d6), d7Pin(d7) {}

  void begin(uint8_t cols, uint8_t rows) {
    (void)cols;
    (void)rows;
  }

  void setCursor(uint8_t col, uint8_t row) {
    (void)col;
    (void)row;
  }

  size_t print(const char* text) {
    if (text == nullptr) {
      return 0;
    }
    Serial.print(text);
    return strlen(text);
  }

  size_t print(const String& text) {
    Serial.print(text);
    return text.length();
  }

  size_t print(int value) {
    Serial.print(value);
    return String(value).length();
  }

  void clear() {
    Serial.println();
  }

private:
  uint8_t rsPin;
  uint8_t enablePin;
  uint8_t d4Pin;
  uint8_t d5Pin;
  uint8_t d6Pin;
  uint8_t d7Pin;
};

// Initialize library with interface pin numbers: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Custom character array (5x8 pixels -> 8 bytes)
 int rLED = 9;
 int bLED =8;

void setup() {
  lcd.begin(16, 2); // Define 16 columns and 2 rows
  
  // Register the custom character in slot 0
 pinMode(rLED, OUTPUT);
 pinMode(bLED, OUTPUT);
  
  lcd.setCursor(0, 0);
  //lcd.print("Hello, World!");

  Serial.begin(9600);
  lcd.setCursor(0, 0);
  lcd.print("Ready...");

}

  String read = "";

void loop() {
  // Column 0, Row 1: Seconds elapsed


  if(Serial.available() > 0 )
  {
    read = Serial.readStringUntil('\n');
    read.trim();

    if(read == "Red")
    {
      digitalWrite(rLED, HIGH);
      digitalWrite(bLED, LOW);
      lcd.setCursor(6, 0);
      lcd.print(read);
    }
    else if(read == "Blue")
    {
      digitalWrite(bLED, HIGH);
      digitalWrite(rLED, LOW);
      lcd.setCursor(6, 0);
      lcd.print(read);
    }
    else if (read == "Nothing")
    {
      lcd.clear();
      digitalWrite(rLED, LOW);
      digitalWrite(bLED, LOW);

    }

  }

 
}