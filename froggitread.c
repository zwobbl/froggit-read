#include "froggitread.h"

static const int MAX_NUM_SENSORS   =    8;
static const int DEFAULT_VALUE     = 9999;
static const int RX_PIN            =    2;
static const int SHORT_DELAY       =  242;
static const int LONG_DELAY        =  484;
static const int NUM_HEADER_BITS   =   10;
static const int MAX_BYTES         =   7;

char      temp_bit;
bool      first_zero;
char      header_hits;
char      data_byte;
char      num_bits;
char      num_bytes;
char      manchester[7];
sigset_t  myset;

int main() 
{
    init();
    wiringPiISR(RX_PIN, INT_EDGE_BOTH, &read_signal);
    sigsuspend(&myset);
}

int init() 
{
    init_globals();
    for (int i = 0; i < 4; i++) {
        manchester[i] = i;
    }
    wiringPiSetup();
    pinMode(RX_PIN, INPUT);
}

void init_globals() 
{
    temp_bit = 1;
    first_zero = false;
    header_hits = 0;
    num_bits = 6;
    num_bytes = 0;
}

void read_signal()
{
    if(digitalRead(RX_PIN) != temp_bit) {
        return;
    }
    delayMicroseconds(SHORT_DELAY);
    if (digitalRead(RX_PIN) != temp_bit) {
        // Halfway through the bit pattern the RX_PIN should
        // be equal to tempBit, if not then error, restart!
        init_globals();
        return;
    }
    delayMicroseconds(LONG_DELAY);
    if(digitalRead(RX_PIN) == temp_bit) {
        temp_bit = temp_bit ^ 1;
    }
    char bit = temp_bit ^ 1;
    if (bit == 1) {
        if (!first_zero) {
            header_hits++;
        } else {
            add_bit(bit);
        }
    } else {
        if(header_hits < NUM_HEADER_BITS) {
            // Something went wrong, we should not be in the
            // header here, so restart!
            init_globals();
            return;
        }
        if (!first_zero) {
            first_zero = true;
        }
        add_bit(bit);
    }
    if (num_bytes == MAX_BYTES) {
        display_sensor_data(); 
        init_globals();
    }
}

void add_bit(char bit) 
{
    data_byte = (data_byte << 1) | bit;
    if (++num_bits == 8) {
        num_bits=0;
        manchester[num_bytes++] = data_byte;
    }
}

// checksum code from BaronVonSchnowzer at
// http://forum.arduino.cc/index.php?topic=214436.15
char checksum(int length, char *buff)
{
    char mask = 0x7C;
    char checksum = 0x64;
    char data;
    int byteCnt;

    for (byteCnt=0; byteCnt < length; byteCnt++) {
        int bitCnt;
        data = buff[byteCnt];
        for (bitCnt= 7; bitCnt >= 0 ; bitCnt--) {
            char bit;
            // Rotate mask right
            bit = mask & 1;
            mask = (mask >> 1) | (mask << 7);
            if (bit) {
              mask ^= 0x18;
            }

            // XOR mask into checksum if data bit is 1
            if(data & 0x80) {
              checksum ^= mask;
            }
            data <<= 1;
        }
    }
    return checksum;
}

void display_sensor_data()
{
    int sensor_type = manchester[1];
    int ch = ((manchester[3] & 0x70) / 16) + 1;
    int temp_raw = (manchester[3] & 0x7) * 256 + manchester[4];
    float temp_fahrenheit = (temp_raw - 400) / 10;
    float temp_celsius = (temp_raw - 720) * 0.0556;
    int humidity = manchester[5]; 
    int low_bat = manchester[3] & 0x80 / 128;
    char check_byte = manchester[MAX_BYTES-1];
    char check = checksum(MAX_BYTES-2, manchester+1);

    printf("received message: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", 
           manchester[0], manchester[1], manchester[2], manchester[3], manchester[4], manchester[5], manchester[6]);

    if (!(sensor_type == 0x45 || sensor_type == 0x46))
        printf("WARN: unknown sensor type 0x%02x received\n", sensor_type);
 
    if (ch < 1 || ch > MAX_NUM_SENSORS)
        printf("WARN: illegal channel id %d received\n", ch);
  
    if (humidity > 100) 
        printf("WARN: invalid humidity value received\n", humidity);
    
    if (check != check_byte) {
        printf("checksum error: got %02x but expected %02x\n\n", check_byte, check);
        return;
    }

    printf("Sensor type: 0x%02x, Channel: %d, Temperature: %.1f°C / %.1f°F, Humidity: %d%, Low battery: %d\n\n", 
           sensor_type, ch, temp_celsius, temp_fahrenheit, humidity, low_bat);
}
