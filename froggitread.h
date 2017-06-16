#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include <wiringPi.h>

int init();
void init_globals();

void read_signal();
void add_bit(char bit);
void display_sensor_data();
