#include <avr/io.h>
#include <avr/iotn84.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define ONOFFSWITCH PA0
#define DATAIN PA1
#define DATACLK PA2
#define LATCHCLK PA3
#define SCL PA4
#define SDA PA6
#define MFP PA7

#define MCP_ADDR 0x6F << 1
#define READ 1
#define WRITE 0

#define RTCSEC 0x00
#define RTCMIN 0x01
#define RTCHOUR 0x02
#define RTCWKDAY 0x03
#define RTCDATE 0x04
#define RTCMTH 0x05
#define RTCYEAR 0x06
#define CONTROL 0x07
#define OSCTRIM 0x08
#define ALM0SEC 0x0A
#define ALM0MIN 0x0B
#define ALM0HOUR 0x0C
#define ALM0WKDAY 0x0D
#define ALM0DATE 0x0E
#define ALM0MTH 0x0F
#define ALM1SEC 0x11
#define ALM1MIN 0x12
#define ALM1HOUR 0x13
#define ALM1WKDAY 0x14
#define ALM1DATE 0x15
#define ALM1MTH 0x16
#define PWRDNMIN 0x18
#define PWRDNHOUR 0x19
#define PWRDNDATE 0x1A
#define PWRDNMTH 0x1B
#define PWRUPMIN 0x1C
#define PWRUPHOUR 0x1D
#define PWRUPDATE 0x1E
#define PWRUPMTH 0x1F

#define ALM0IF 3
#define ALM1IF 3
#define ALM0EN 4
#define ALM1EN 5

#define OUTPUT 1
#define INPUT 0

#define sbi(reg, bit)   ((reg) |=  (1 << (bit)))
#define cbi(reg, bit)   ((reg) &= ~(1 << (bit)))

typedef uint8_t byte;




void digitalWrite(byte portbit, byte state) {
	if (state)
		sbi(PORTA, portbit);
	else
		cbi(PORTA, portbit);

}

byte digitalRead(byte portbit) {
	return PINA & (1<<portbit);
}

byte isButtonOn() {
	return !(PINA & (1 << ONOFFSWITCH));
}

void pinMode(byte portbit, byte mode) {
	if (mode)
		sbi(DDRA, portbit);
	else
		cbi(DDRA, portbit);
}

void pulse(byte portbit) {
	sbi(PORTA, portbit);
	cbi(PORTA, portbit);
}
void displayNumber(uint16_t number);

void i2c_release_sda() { cbi(DDRA, SDA); }
void i2c_release_scl() { cbi(DDRA, SCL); }
byte i2c_read_sda() { return PINA & SDA; }

void i2c_low_sda() { sbi(DDRA, SDA); }
void i2c_low_scl() { sbi(DDRA, SCL); }

void i2c_init() {

	PORTA &= ~((1<<SDA) | (1<<SCL));
	i2c_release_scl();
	i2c_release_sda();
}


void i2c_start() {
	i2c_release_sda();
	i2c_release_scl();
	i2c_low_sda();
	i2c_low_scl();
}

void i2c_stop() {
	i2c_low_sda();
	i2c_release_scl();
	i2c_release_sda();
}

void i2c_write(byte data) {

	for (byte i = 0; i < 8; i++) {
		if (data & 0x80)
			i2c_release_sda();
		else
			i2c_low_sda();
		data <<= 1;

		i2c_release_scl();
		i2c_low_scl();

	}
	i2c_release_sda();
	i2c_release_scl();

	byte ack = !i2c_read_sda(); 

	i2c_low_scl();

}

byte i2c_read() {
	i2c_release_sda();
	byte data = 0;
	for (int8_t i = 7; i >= 0; i--) {
		i2c_release_scl();
		data |= ((PINA >> SDA) & 0x01) << i;
		i2c_low_scl();
	}
	return data;
}

void mcp_write(byte reg, byte data) {
	i2c_start();
	i2c_write(MCP_ADDR | WRITE);
	i2c_write(reg);
	i2c_write(data);
	i2c_stop();
}

byte mcp_read(byte reg) {
	i2c_start();
	i2c_write(MCP_ADDR | WRITE);
	i2c_write(reg);
	i2c_start();
	i2c_write(MCP_ADDR | READ);
	byte data = i2c_read();
	i2c_stop();
	return data;
}

void delay(byte i);
uint16_t calculateDays() {
	byte wkday = mcp_read(RTCWKDAY) & 0x07;
	byte date = mcp_read(RTCDATE);
	date = ((date >> 4) * 10) + (date & 0x0f);
	byte month = mcp_read(RTCMTH);
	month = (((month >> 4) & 0x01) * 10) + (month & 0x0f);
	byte year = mcp_read(RTCYEAR);
	year = ((year >> 4) * 10) + (year & 0x0f);


	uint16_t days = 0;
						// sep oct nov dec jan feb mar apr may jun
	const byte monthlength[] = {30, 31, 30, 31 ,31, 28, 31, 30, 31, 30};


	if (month == 7 || month == 8 || year == 1)
		return -1;


	if (month == 6) {
		days = 21 - date;
		wkday = (wkday + days) % 7;
		days += 5 - wkday;
		if (wkday > 5)
		    days += 7;
		return days;
	}

	byte index = month;
	if (month >= 9) {
		index -= 9;
		year++; //for calculating leap year
	} else
		index += 3;

	if ((year % 4 == 0) && (year != 0) && (index <= 5))
		days++;		//29 feb

	days += monthlength[index++] - date;
	while (index != 9)
		days += monthlength[index++];

	days += 21;

	wkday = (wkday + days) % 7;

	days += 5 - wkday;
	if (wkday > 5)
        days += 7;

	return days;
}



void initMCP() {}


void delay(byte i) {

	while (i > 0) {
		for (byte j = 0; j < 255; j++) {
			asm volatile ("nop");
		}
		i--;
	}
}

void sendDigits595(byte digit1, byte digit2, byte digit3) {
	for (byte i = 0; i < 8; i++) {
		digitalWrite(DATAIN, digit1 & (1<<i));
		pulse(DATACLK);
	}
	for (byte i = 0; i < 8; i++) {
		digitalWrite(DATAIN, digit2 & (1<<i));
		pulse(DATACLK);
	}
	for (byte i = 0; i < 8; i++) {
		digitalWrite(DATAIN, digit3 & (1<<i));
		pulse(DATACLK);
	}
	pulse(LATCHCLK);
	delay(100);
}

byte intToSegments(byte integer) {
	switch (integer) {
	    case 0:	return 0b01111101; // 0 → ABCDEF
	    case 1:	return 0b00011000; // 1 → BC
	    case 2:	return 0b01101110; // 2 → ABDEG
	    case 3:	return 0b00111110; // 3 → ABCDG
	    case 4:	return 0b00011011; // 4 → BCFG
	    case 5:	return 0b00110111; // 5 → ACDFG
	    case 6:	return 0b01110111; // 6 → ACDEFG
	    case 7:	return 0b00011100; // 7 → ABC
	    case 8:	return 0b01111111; // 8 → ABCDEFG
	    case 9:	return 0b00111111; // 9 → ABCDFG
	    default:	return 0; // all off
	}
}


void displayNumber(uint16_t number) {

	if (number > 310) {
		sendDigits595(0b00000010, 0b00000010, 0b00000010);
		return;
	}

	byte digit1 = number / 100;		// hundreds
	byte digit2 = (number / 10) % 10;	// tens
	byte digit3 = number % 10;		// ones
	if (digit1 == 0) {
		digit1 = 255;
		if (digit2 == 0)
			digit2 = 255;
	}
	digit1 = intToSegments(digit1);
	digit2 = intToSegments(digit2);
	digit3 = intToSegments(digit3);
	sendDigits595(digit1, digit2, digit3);
}

void animateOn(uint16_t number) {
	//    -- A --
	//   |       |
	//   F       B
	//   |       |
	//    -- G --
	//   |       |
	//   E       C
	//   |       |
	//    -- D --
	//	segments = 0EDCBAGF
	//	animate  = 01233221
	const byte animateStage1 = 0b01000001;
	const byte animateStage2 = 0b00100110;
	const byte animateStage3 = 0b00011000;

	byte digit1 = number / 100;		// hundreds
	byte digit2 = (number / 10) % 10;	// tens
	byte digit3 = number % 10;		// ones

	if (number <= 310) {

		if (digit1 == 0) {
			digit1 = -1;
			if (digit2 == 0)
				digit2 = -1;
		}
		digit1 = intToSegments(digit1);
		digit2 = intToSegments(digit2);
		digit3 = intToSegments(digit3);
	} else {
		digit1 = 0b00000010;
		digit2 = 0b00000010;
		digit3 = 0b00000010;
	}

	
	byte digit1segments = 0;
	byte digit2segments = 0;
	byte digit3segments = 0;
	
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit1segments |= animateStage1;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit1segments |= animateStage2;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit1segments |= animateStage3;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit2segments |= animateStage1;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit1segments &= ~animateStage1 | digit1;
	digit2segments |= animateStage2;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit1segments &= ~animateStage2 | digit1;
	digit2segments |= animateStage3;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit1segments &= ~animateStage3 | digit1;
	digit3segments |= animateStage1;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit2segments &= ~animateStage1 | digit2;
	digit3segments |= animateStage2;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit2segments &= ~animateStage2 | digit2;
	digit3segments |= animateStage3;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit2segments &= ~animateStage3 | digit2;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit3segments &= ~animateStage1 | digit3;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit3segments &= ~animateStage2 | digit3;
	sendDigits595(digit1segments, digit2segments, digit3segments);

	digit3segments &= ~animateStage3 | digit3;
	sendDigits595(digit1segments, digit2segments, digit3segments);

}

void animateOff(uint16_t number) {
	const byte animateStage1 = 0b01000001;
	const byte animateStage2 = 0b00100110;
	const byte animateStage3 = 0b00011000;

	byte digit1 = number / 100;		// hundreds
	byte digit2 = (number / 10) % 10;	// tens
	byte digit3 = number % 10;		// ones

	if (number <= 310) {

		if (digit1 == 0) {
			digit1 = -1;
			if (digit2 == 0)
				digit2 = -1;
		}
		digit1 = intToSegments(digit1);
		digit2 = intToSegments(digit2);
		digit3 = intToSegments(digit3);
	} else {
		digit1 = 0b00000010;
		digit2 = 0b00000010;
		digit3 = 0b00000010;
	}
	
	sendDigits595(digit1, digit2, digit3);

	digit1 |= animateStage1;
	sendDigits595(digit1, digit2, digit3);

	digit1 |= animateStage2;
	sendDigits595(digit1, digit2, digit3);

	digit1 |= animateStage3;
	sendDigits595(digit1, digit2, digit3);

	digit2 |= animateStage1;
	sendDigits595(digit1, digit2, digit3);

	digit1 &= ~animateStage1;
	digit2 |= animateStage2;
	sendDigits595(digit1, digit2, digit3);

	digit1 &= ~animateStage2;
	digit2 |= animateStage3;
	sendDigits595(digit1, digit2, digit3);

	digit1 &= ~animateStage3;
	digit3 |= animateStage1;
	sendDigits595(digit1, digit2, digit3);

	digit2 &= ~animateStage1;
	digit3 |= animateStage2;
	sendDigits595(digit1, digit2, digit3);

	digit2 &= ~animateStage2;
	digit3 |= animateStage3;
	sendDigits595(digit1, digit2, digit3);

	digit2 &= ~animateStage3;
	sendDigits595(digit1, digit2, digit3);

	digit3 &= ~animateStage1;
	sendDigits595(digit1, digit2, digit3);

	digit3 &= ~animateStage2;
	sendDigits595(digit1, digit2, digit3);

	digit3 &= ~animateStage3;
	sendDigits595(digit1, digit2, digit3);

}

ISR(PCINT0_vect){
}

byte interrupt_hour = 0;

int main() {
	DDRA = (1<<DATAIN)|(1<<DATACLK)|(1<<LATCHCLK);
	PORTA = 1 << ONOFFSWITCH;

	GIMSK = 1 << PCIE0;
	PCMSK0 = (1<<PCINT7) | (1<<PCINT0);

	i2c_init();

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);

	if (isButtonOn())
		displayNumber(calculateDays());
	else
		sendDigits595(0b00000000, 0b00000000, 0b00000000);

	sei();

	while (1) {

		sleep_mode();

		if (!(PINA & (1<<MFP))) {		// MFP 0 - interrupt
			//handle mfp interrupt

			// since interrupts on mcp7940 will be fired till matching event stops, we cant just set one alarm to 1:00 everyday
			// calculateDays , and go back to sleep. we need to set up 2 alarms and when first is fired calculateDays, then disable
			// alarm, turn the other on and wait for it. after it interrupts (hour after) we disable second one and enable main.

			if (mcp_read(ALM0WKDAY) & (1 << ALM0IF)) {	// Alarm 0 Interrupt Flag
				mcp_write(CONTROL, (mcp_read(CONTROL) & 0xcf) | (1 << ALM1EN));	// turn off ALM0, turn on ALM1
				mcp_write(ALM0WKDAY, mcp_read(ALM0WKDAY) & ~(1 << ALM0IF));		// clear interrupt flag
			}

			if (mcp_read(ALM1WKDAY) & (1 << ALM1IF)) {	// Alarm 1 Interrupt Flag
				mcp_write(CONTROL, (mcp_read(CONTROL) & 0xcf) | (1 << ALM0EN));	// turn off ALM1, turn on ALM0
				mcp_write(ALM1WKDAY, mcp_read(ALM1WKDAY) & ~(1 << ALM1IF));		// clear interrupt flag
			}

			if (isButtonOn())
				displayNumber(calculateDays());
			
			continue;
		}
		
		if (isButtonOn())
			animateOn(calculateDays());
		else
			animateOff(calculateDays());

	}
}
