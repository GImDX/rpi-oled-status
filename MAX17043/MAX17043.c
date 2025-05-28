// MAX17043/44 library for Arduino
//
// Luca Dentella (http://www.lucadentella.it)
// ImDX 20211011 Modified for Raspberry PI

#include "MAX17043.h" //已修改！

float MAX17043::getVCell() {

	uint8_t MSB = 0;
	uint8_t LSB = 0;
	
	readRegister(VCELL_REGISTER, MSB, LSB);
	int value = ((uint16_t)MSB << 4) | (LSB >> 4);
	// return map(value, 0x000, 0xFFF, 0, 50000) / 10000.0;
	return value * 0.00125;
}

float MAX17043::getSoC() {
	
	uint8_t MSB = 0;
	uint8_t LSB = 0;
	
	readRegister(SOC_REGISTER, MSB, LSB);
	float decimal = LSB / 256.0;
	return MSB + decimal;	
}

int MAX17043::getVersion() {

	uint8_t MSB = 0;
	uint8_t LSB = 0;
	
	readRegister(VERSION_REGISTER, MSB, LSB);
	return (MSB << 8) | LSB;
}

uint8_t MAX17043::getCompensateValue() {

	uint8_t MSB = 0;
	uint8_t LSB = 0;
	
	readConfigRegister(MSB, LSB);
	return MSB;
}

uint8_t MAX17043::getAlertThreshold() {

	uint8_t MSB = 0;
	uint8_t LSB = 0;
	
	readConfigRegister(MSB, LSB);	
	return 32 - (LSB & 0x1F);
}

void MAX17043::setAlertThreshold(uint8_t threshold) {

	uint8_t MSB = 0;
	uint8_t LSB = 0;
	
	readConfigRegister(MSB, LSB);	
	if(threshold > 32) threshold = 32;
	threshold = 32 - threshold;
	
	writeRegister(CONFIG_REGISTER, MSB, (LSB & 0xE0) | threshold);
}

bool MAX17043::inAlert() {

	uint8_t MSB = 0;
	uint8_t LSB = 0;
	
	readConfigRegister(MSB, LSB);	
	return LSB & 0x20;
}

void MAX17043::clearAlert() {

	uint8_t MSB = 0;
	uint8_t LSB = 0;
	
	readConfigRegister(MSB, LSB);	
}

void MAX17043::reset() {
	
	writeRegister(COMMAND_REGISTER, 0x00, 0x54);
}

void MAX17043::quickStart() {
	
	writeRegister(MODE_REGISTER, 0x40, 0x00);
}


void MAX17043::readConfigRegister(uint8_t &MSB, uint8_t &LSB) {

	readRegister(CONFIG_REGISTER, MSB, LSB);
}

void MAX17043::readRegister(uint8_t startAddress, uint8_t &MSB, uint8_t &LSB) {

	// Wire.beginTransmission(MAX17043_ADDRESS);
	// Wire.write(startAddress);
	// Wire.endTransmission();
	
	// Wire.requestFrom(MAX17043_ADDRESS, 2);
	// MSB = Wire.read();
	// LSB = Wire.read();
	MSB = wiringPiI2CReadReg8(i2c_handle, startAddress); //已修改！
	LSB = wiringPiI2CReadReg8(i2c_handle, startAddress + 1);
}

void MAX17043::writeRegister(uint8_t address, uint8_t MSB, uint8_t LSB) {

	// Wire.beginTransmission(MAX17043_ADDRESS);
	// Wire.write(address);
	// Wire.write(MSB);
	// Wire.write(LSB);
	// Wire.endTransmission();
	wiringPiI2CWriteReg8(i2c_handle, address, MSB);
	wiringPiI2CWriteReg8(i2c_handle, address + 1, MSB);
}

void MAX17043::init(uint16_t pAddress) {
	i2c_handle = wiringPiI2CSetup(pAddress);
	initialize = true;
	if(i2c_handle == -1) {
		initialize = false;
	}
}