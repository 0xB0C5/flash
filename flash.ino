/*
 * SST39SF010A flash chip programmer
 * 
 * So this is a bit of a janky mess.
 * It uses all but one I/O pin of the Raspberry Pi Pico, which means that garbage serial data will be sent while flash stuff is happening.
 * Once it's done flash stuff, it responds with a single byte indicating the type of response (e.g. R for read, W for write)
 * But this really isn't enough to ensure it's actually done, that single byte could potentially be generated from the flash stuff.
 * But I haven't seen that happen yet so I haven't done anything about it.
 */

const int PIN_FLASH_OUTPUT_ENABLE = 26;
const int PIN_FLASH_WRITE_ENABLE  = 27;

const int PIN_FLASH_DATA0 = 0;
const int PIN_FLASH_ADDR0 = 8;

const int ADDR_PIN_COUNT = 15;

const int FLASH_MAX_ADDRESS = 0x7fff;

const int SERIAL_RATE = 9600;

uint8_t buffer[256];

void setup()
{
  // Set output & write enable pins to output high (disabled)
  pinMode(PIN_FLASH_OUTPUT_ENABLE, OUTPUT);
  pinMode(PIN_FLASH_WRITE_ENABLE, OUTPUT);
  digitalWrite(PIN_FLASH_OUTPUT_ENABLE, HIGH);
  digitalWrite(PIN_FLASH_WRITE_ENABLE, HIGH); 

  // Set flash address pins to output.
  for (int i = 0; i < ADDR_PIN_COUNT; i++) {
    pinMode(PIN_FLASH_ADDR0 + i, OUTPUT);
  }

  // Set flash data pins to input.
  set_flash_data_pin_mode(INPUT);

  Serial.begin(SERIAL_RATE);
}

void set_flash_data_pin_mode(int mode) {
  for (int i = 0; i < 8; i++) {
    pinMode(PIN_FLASH_DATA0+i, mode);
  }
}

void set_flash_address(uint16_t address) {
  if (address > FLASH_MAX_ADDRESS) {
    // Serial.print("INVALID ADDRESS ");
    // Serial.println(address);
    while (true) {}
  }

  // Update flash pins
  for (int i = 0; i < ADDR_PIN_COUNT; i++) {
    digitalWrite(PIN_FLASH_ADDR0+i, (address >> i) & 1);
  }
}

uint8_t read_flash(uint16_t address) {
  set_flash_data_pin_mode(INPUT);
  set_flash_address(address);
  digitalWrite(PIN_FLASH_OUTPUT_ENABLE, LOW);
  delayMicroseconds(1);
  uint8_t data = 0;
  for (int i = 0; i < 8; i++) {
    data |= digitalRead(PIN_FLASH_DATA0+i) << i;
  }

  digitalWrite(PIN_FLASH_OUTPUT_ENABLE, HIGH);

  return data;
}

void command_flash(uint16_t address, uint8_t data) {
  set_flash_address(address);

  set_flash_data_pin_mode(OUTPUT);

  for (int i = 0; i < 8; i++) {
    digitalWrite(PIN_FLASH_DATA0+i, (data >> i) & 1);
  }
  delayMicroseconds(1);

  // Apply the command.
  digitalWrite(PIN_FLASH_WRITE_ENABLE, LOW);
  delayMicroseconds(1);
  digitalWrite(PIN_FLASH_WRITE_ENABLE, HIGH);
  delayMicroseconds(1);
}

void clear_flash() {
  command_flash(0x5555, 0xAA);
  command_flash(0x2AAA, 0x55);
  command_flash(0x5555, 0x80);
  command_flash(0x5555, 0xAA);
  command_flash(0x2AAA, 0x55);
  command_flash(0x5555, 0x10);
  delay(100);
}

void write_flash(uint16_t addr, uint8_t data) {
  command_flash(0x5555, 0xAA);
  command_flash(0x2AAA, 0x55);
  command_flash(0x5555, 0xA0);
  command_flash(addr, data);
  delayMicroseconds(10);
}

uint8_t serial_read_byte() {
  int serial_data;
  do {
    serial_data = Serial.read();
  } while (serial_data == -1);
  return (uint8_t) serial_data;
}

void serial_command_read() {
    uint8_t address_high = serial_read_byte();

    uint16_t address_start = ((uint16_t)address_high) << 8;

    if (address_start > FLASH_MAX_ADDRESS) {
      Serial.write("!ADDRESS OUT OF RANGE\n");
      return;
    }

    // Serial must be disabled during flash stuff.
    Serial.end();

    for (uint16_t i = 0; i < 256; i++) {
      buffer[i] = read_flash(address_start | i);
    }

    Serial.begin(SERIAL_RATE);
    delay(100);

    Serial.write("R"); // read result
    Serial.write(buffer, 256);
}

void serial_command_write() {
    uint8_t address_high = serial_read_byte();

    uint16_t address_start = ((uint16_t)address_high) << 8;

    if (address_start > FLASH_MAX_ADDRESS) {
      Serial.write("!ADDRESS OUT OF RANGE\n");
      return;
    }

    for (uint16_t i = 0; i < 256; i++) {
      buffer[i] = serial_read_byte();
    }

    // Serial must be disabled during flash stuff.
    Serial.end();

    for (uint16_t i = 0; i < 256; i++) {
      write_flash(address_start | i, buffer[i]);
    }

    Serial.begin(SERIAL_RATE);
    delay(100);

    Serial.write('W'); // write done.
}

void serial_command_clear() {
  Serial.end();
  clear_flash();
  Serial.begin(SERIAL_RATE);
  delay(100);

  Serial.write('C'); // clear done.
}

void serial_command_echo() {
  int count = Serial.readBytes((char*)buffer, 4);
  if (count != 4) {
    Serial.write("!ECHO DIDN'T RECEIVE ENOUGH BYTES\n");
    return;
  }
  Serial.write('E');
  Serial.write(buffer, 4);
  Serial.flush();
}

void loop()
{
  uint8_t serial_data = serial_read_byte();

  switch (serial_data) {
    case 'R': // read page
      serial_command_read();
      break;

    case 'W': // write page
      serial_command_write();
      break;

    case 'C': // clear all
      serial_command_clear();
      break;

    case 'E': // echo command
      serial_command_echo();
      break;
  }
}
