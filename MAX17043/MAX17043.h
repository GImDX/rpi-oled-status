// MAX17043/44 library for Arduino
//
// Luca Dentella (http://www.lucadentella.it)

// #include "Arduino.h" //已修改！
#include <stdint.h> //已修改！
#include <wiringPiI2C.h> //已修改！

#ifndef _MAX17043_H
#define _MAX17043_H

#define MAX17043_ADDRESS	0x36

#define VCELL_REGISTER		0x02
#define SOC_REGISTER		0x04
#define MODE_REGISTER		0x06
#define VERSION_REGISTER	0x08
#define CONFIG_REGISTER		0x0C
#define COMMAND_REGISTER	0xFE


class MAX17043 {

	public:
	
		float getVCell();
		float getSoC();
		int getVersion();
		uint8_t getCompensateValue();
		uint8_t getAlertThreshold();
		void setAlertThreshold(uint8_t threshold);
		bool inAlert(); //已修改！
		void clearAlert();
		
		void reset();
		void quickStart();
		void init(uint16_t pAddress); //已修改！
	
	private:

		int i2c_handle; //已修改！
		bool initialize = false; //已修改！
		void readConfigRegister(uint8_t &MSB, uint8_t &LSB);
		void readRegister(uint8_t startAddress, uint8_t &MSB, uint8_t &LSB);
		void writeRegister(uint8_t address, uint8_t MSB, uint8_t LSB);
};

#endif