/*
 * dyn_sensor.c
 *
 *  Created on: 18 mar. 2020
 *      Author: droma
 *
 * TODO: High-level functions like "distance_wall_front", etc
 * TODO: Generate another file for motors, with functions like "move_forward", etc
 */
#include "dyn_app_sensor.h"
#include "dyn_instr.h"


int distanceToGetObstacle(byte ID, byte postion) {
    byte param[1];
    param[0] = 1;
    //position will be 0x34(RAM) or 0x14(ROM)
    dyn_read_byte(ID, postion, param);

    return param[0];
}


int sensorRead(byte ID, byte sensor) {
    //sensor can be 1A Sensor Left; 1B Sensor Center; 1C Sensor Right
    byte param[1];
    param[0] = 1; //length of the read

    dyn_read_byte(ID, sensor, param); //get distance

    return param[0];
}


int getObstacleFlag(byte ID) {
    //flags
    // 2 detected in sensor right
    // 1 detected in sensor center
    // 0 detected in sensor left
    byte param[1];
    param[0] = 1;

    dyn_read_byte(ID, OBSTACLE_DETECTED, param);

    return param[0];
}