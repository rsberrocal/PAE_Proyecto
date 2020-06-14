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

//Setting the distance to consider an object
void setDistanceToObject(byte ID, byte position, int dist) {
    byte params[1] = {dist};
    //position will be 0x34(RAM) or 0x14(ROM)
    dyn_write(ID, position, params, 1);
}

uint8_t distanceToGetObstacle(byte ID, byte postion) {
    uint8_t dist = 0;
    //position will be 0x34(RAM) or 0x14(ROM)
    dyn_read_byte(ID, postion, &dist);

    return dist;
}

//Return distance from sensor to object
uint8_t sensorRead(byte ID, byte sensor) {
    //sensor can be 1A Sensor Left; 1B Sensor Center; 1C Sensor Right
    uint8_t dist = 0;

    dyn_read_byte(ID, sensor, &dist); //get distance

    return dist;
}

uint8_t getObstacleFlag(byte ID) {
    //flags
    // 2 detected in sensor right
    // 1 detected in sensor center
    // 0 detected in sensor left
    uint8_t flag = 0;

    dyn_read_byte(ID, OBSTACLE_DETECTED, &flag);

    return flag;
}