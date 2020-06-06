/*
 * dyn_sensor.h
 *
 *  Created on: 18 mar. 2020
 *      Author: droma
 */

#ifndef DYN_SENSOR_H_
#define DYN_SENSOR_H_
#define OBSTACLE_DETECTED 0x20

#include <stdint.h>

typedef uint8_t byte;

void setDistanceToObject(byte ID, byte position, int dist);

uint8_t distanceToGetObstacle(byte ID, byte position);

uint8_t sensorRead(byte ID, byte sensor);

uint8_t getObstacleFlag(byte ID);


#endif /* DYN_SENSOR_H_ */
