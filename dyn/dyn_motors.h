//
// Created by Rocho on 18/04/2020.
//

#ifndef JOYSTICK_DYN_MOTORS_H
#define JOYSTICK_DYN_MOTORS_H

#include <stdint.h>
#include <stdbool.h>
//Wheels
#define RIGHT_WHEEL 2
#define LEFT_WHEEL 3

//Instruccions
#define WRITE_DATA 0x03
#define READ_DATA 0x02

//Directions
#define CW_ANGLE_LIMIT_L 0x06
#define CW_ANGLE_LIMIT_H 0x07
#define CCW_ANGLE_LIMIT_L 0x08
#define CCW_ANGLE_LIMIT_H 0x09

//Speed
#define MOV_SPEED_L 0x20
#define MOV_SPEED_H 0x21
#define M_LED 0x19
#define MAX_SPEED 1024

typedef uint8_t byte;

void endlessTurn(byte idMotor);//to set wheels

//moveTo -> true Right; false Left
void moveWheel(byte ID, bool moveTo, unsigned int speed);

void stop(void);

void turnLeft(unsigned int speed);

void turnOnItselfLeft(unsigned int speed);

void turnRight(unsigned int speed);


void turnOnItselfRight(unsigned int speed);

void forward(unsigned int speed);

void backward(unsigned int speed);


#endif //JOYSTICK_DYN_MOTORS_H
