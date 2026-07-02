#include <SPI.h> // Keep for compilation, but we bit-bang here

#define LATCH_PIN 10          // STCP pin on 74HC595
#define SHIFT_CLK_PIN 9       // SHCP pin on 74HC595
#define SCL_PIN A5            // Manual I2C Clock Pin
#define SDA_PIN A4            // Manual I2C Data Pin

#define EEPROM_WRITE_ADDR 0xA0 // 0x50 shifted left by 1 (Write)
#define EEPROM_READ_ADDR  0xA1 // 0x50 shifted left by 1 + 1 (Read)

const uint8_t CC_WITHOUT_DP_OFFSET = 0;

void setup() {
  // Configure I2C pins as open-drain style (Input with pullups)
  pinMode(SCL_PIN, INPUT_PULLUP);
  pinMode(SDA_PIN, INPUT_PULLUP);
  
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(SHIFT_CLK_PIN, OUTPUT);
  
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(SHIFT_CLK_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("--- Bit-Banged I2C Spy Running ---");
}

void loop() {
  for (int digit = 0; digit < 16; digit++) {
    uint8_t memAddr = CC_WITHOUT_DP_OFFSET + digit;
    
    // --- I2C TRANSACTION START ---
    i2c_start();
    
    // 1. Send Device Write Address (0xA0) to set memory pointer
    i2c_write_byte(EEPROM_WRITE_ADDR, false); 
    
    // 2. Send the Memory Address we want to read
    i2c_write_byte(memAddr, false);
    
    // 3. Send Repeated Start to switch to Read Mode
    i2c_start();
    i2c_write_byte(EEPROM_READ_ADDR, false);
    
    // 4. READ THE DATA BYTE AND SPY!
    // We pass 'true' to signal the 74HC595 clock to duplicate the pulses
    i2c_read_byte_and_spy(true); 
    
    i2c_stop();
    // --- I2C TRANSACTION END ---
    
    delay(1000); // Hold each digit for 1 second
  }
}

// --- LOW LEVEL BIT-BANG I2C FUNCTIONS ---

void i2c_start() {
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);
  delayMicroseconds(4);
  pinMode(SDA_PIN, OUTPUT);
  digitalWrite(SDA_PIN, LOW); // SDA goes low while SCL is high
  delayMicroseconds(4);
  pinMode(SCL_PIN, OUTPUT);
  digitalWrite(SCL_PIN, LOW);
}

void i2c_stop() {
  pinMode(SDA_PIN, OUTPUT);
  digitalWrite(SDA_PIN, LOW);
  delayMicroseconds(4);
  pinMode(SCL_PIN, INPUT_PULLUP); // SCL goes high
  delayMicroseconds(4);
  pinMode(SDA_PIN, INPUT_PULLUP); // SDA goes high while SCL is high
  delayMicroseconds(4);
}

void i2c_write_byte(uint8_t data, bool spyOnRegister) {
  // Transmit 8 bits MSB first
  for (uint8_t mask = 0x80; mask; mask >>= 1) {
    if (data & mask) {
      pinMode(SDA_PIN, INPUT_PULLUP);
    } else {
      pinMode(SDA_PIN, OUTPUT);
      digitalWrite(SDA_PIN, LOW);
    }
    delayMicroseconds(4);
    
    pinMode(SCL_PIN, INPUT_PULLUP); // Clock High
    delayMicroseconds(4);
    pinMode(SCL_PIN, OUTPUT);       // Clock Low
    digitalWrite(SCL_PIN, LOW);
  }
  
  // Read ACK bit (ignored for simplification, just pulse clock)
  pinMode(SDA_PIN, INPUT_PULLUP);
  delayMicroseconds(4);
  pinMode(SCL_PIN, INPUT_PULLUP);
  delayMicroseconds(4);
  pinMode(SCL_PIN, OUTPUT);
  digitalWrite(SCL_PIN, LOW);
}

void i2c_read_byte_and_spy(bool spyOnRegister) {
  pinMode(SDA_PIN, INPUT_PULLUP); // Release SDA for EEPROM to drive
  
  if (spyOnRegister) {
    digitalWrite(LATCH_PIN, LOW); // Open 74HC595 storage layer
  }

  for (int i = 0; i < 8; i++) {
    delayMicroseconds(4);
    pinMode(SCL_PIN, INPUT_PULLUP); // SCL goes HIGH, data is now valid on SDA
    delayMicroseconds(2);
    
    if (spyOnRegister) {
      // Pulse the 74HC595 clock while I2C data is completely stable!
      digitalWrite(SHIFT_CLK_PIN, HIGH);
      delayMicroseconds(2);
      digitalWrite(SHIFT_CLK_PIN, LOW);
    } else {
      delayMicroseconds(2);
    }
    
    pinMode(SCL_PIN, OUTPUT); // SCL goes LOW
    digitalWrite(SCL_PIN, LOW);
  }
  
  if (spyOnRegister) {
    digitalWrite(LATCH_PIN, HIGH); // Latch the spied data out to the display pins!
  }
  
  // Send NACK (Master ends read session)
  pinMode(SDA_PIN, INPUT_PULLUP); // Keep SDA High for NACK
  delayMicroseconds(4);
  pinMode(SCL_PIN, INPUT_PULLUP);
  delayMicroseconds(4);
  pinMode(SCL_PIN, OUTPUT);
  digitalWrite(SCL_PIN, LOW);
}
