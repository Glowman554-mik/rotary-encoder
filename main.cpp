/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "hwlcd.hpp"


int segments[] = {
    0b00111111,
    0b00000110,
    0b01011011,
    0b01001111,
    0b01100110,
    0b01101101,
    0b01111101,
    0b00000111,
    0b01111111,
    0b01101111,
    0b01110111,
    0b01111100,
    0b00111001,
    0b01011110,
    0b01111001,
    0b01110001
};

PortOut output(PortC, 0xff);
DigitalOut seg1(PC_12);
DigitalOut seg2(PC_11);
bool curr = false;

int num = 0;


void timer() {
    curr = !curr;

    if (curr) {
        int seg2_num = num & 0b00001111;
        seg1 = false;
        seg2 = true;
        output = segments[seg2_num];
    } else {
        int seg1_num = (num & 0b11110000) >> 4;
        seg2 = false;
        seg1 = true;
        output = segments[seg1_num];
    }
}

int sm_state = 0;

void aRise() {
    switch (sm_state) {
        case 0:
            num++;
            sm_state = 1;
            break;
        case 3:
            num--;
            sm_state = 2;
            break;
    }
}

void bRise() {
    switch (sm_state) {
        case 0:
            num--;
            sm_state = 3;
            break;
        case 1:
            num++;
            sm_state = 2;
            break;
    }
}

void aFall() {
    switch (sm_state) {
        case 1:
            num--;
            sm_state = 0;
            break;
        case 2:
            num++;
            sm_state = 3;
            break;
    }
}

void bFall() {
    switch (sm_state) {
        case 2:
            num--;
            sm_state = 1;
            break;
        case 3:
            num++;
            sm_state = 0;
            break;
    }
}

I2C i2c(I2C_SDA, I2C_SCL);

int main() {
    Ticker ticker;
    ticker.attach(timer, 10ms);

    InterruptIn A(PB_0); //CLK
    InterruptIn B(PB_1); //DT
    InterruptIn reset(PB_2);
    reset.mode(PullUp);

    A.rise(aRise);
    A.fall(aFall);
    B.rise(bRise);
    B.fall(bFall);

    reset.fall([]() {
        num = 0;
    });

    // Make PA_11 and PA_12 floating that we won't short anything when connecting SDA->PA12 and SCL->PA11
    DigitalIn _(PA_11);
    DigitalIn __(PA_12);

    hwlcd lcd(&i2c, i2c_find_first_device(i2c));
    lcd.init(true, false);

    while (true) {
        lcd.cursorpos(0);
        char buffer[32] = { 0 };
        sprintf(buffer, "pos: %d                   ", num);
        lcd.puts(buffer);

        lcd.cursorpos(0x40);
        char buffer2[32] = { 0 };
        sprintf(buffer2, "state: %d                   ", sm_state);
        lcd.puts(buffer2);
    }
}
