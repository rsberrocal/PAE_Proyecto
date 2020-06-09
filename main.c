#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <posicion.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else

#include <unistd.h>

#endif

#include "main.h"
#include "dyn/dyn_app_common.h"
#include "dyn_test/dyn_emu.h"
#include "dyn_test/b_queue.h"
#include "dyn_motors.h"
#include "dyn_instr.h"
#include "dyn_app_sensor.h"
#include "movement_simulator.h"
#include "joystick_emu/joystick.h"
#include "habitacion_001.h"

uint8_t estado = Ninguno, estado_anterior = Ninguno, finalizar = 0;
uint32_t indice;

//return to 0 if min dist is on left, 1 on center, 2 on right
int getNearWall(uint8_t *minDist) {
    uint8_t distLeft = sensorRead(ID_SENSOR, DYN_REG__IR_LEFT);
    printf("Distance left %d\n", distLeft);
    uint8_t distCenter = sensorRead(ID_SENSOR, DYN_REG__IR_CENTER);
    printf("Distance center %d\n", distCenter);
    uint8_t distRight = sensorRead(ID_SENSOR, DYN_REG__IR_RIGHT);
    printf("Distance right %d\n", distRight);
    if (distLeft <= distCenter && distLeft < distRight) {
        *minDist = distLeft;
        return 0;
    }
    if (distCenter <= distLeft && distCenter < distRight) {
        *minDist = distCenter;
        return 1;
    }
    if (distRight <= distCenter && distRight < distLeft) {
        *minDist = distRight;
        return 2;
    }
}

int isOnWall(const uint8_t *minDist){
    if (*minDist >= 30 && *minDist < 70){
        return 1;
    }
    return 0;
}

int isCorrectPosition(uint8_t *minDist){
    if(getNearWall(minDist) == 0){
        return 1;
    }
    return 0;
}

/**
 * main.c
 */
int main(void) {
    pthread_t tid, jid;
    uint8_t tmp;
    int main_speed = 120;
    uint8_t distCenter = 0;
    uint8_t distRight = 0;
    uint8_t distLeft = 0;
    int flagRotation = -1;
    int searchingWall = 1;
    uint8_t minDistance = 255;

    //Init queue for TX/RX data
    init_queue(&q_tx);
    init_queue(&q_rx);

    //Start thread for dynamixel module emulation
    // Passing the room information to the dyn_emu thread
    pthread_create(&tid, NULL, dyn_emu, (void *) datos_habitacion);
    pthread_create(&jid, NULL, joystick_emu, (void *) &jid);

    //Testing some high level function
    printf("\nSetting LED to 0 \n");
    dyn_led_control(1, 0);
    printf("\nGetting LED value \n");
    dyn_led_read(1, &tmp);
    //assert(tmp == 0);
    printf("\nSetting LED to 1 \n");
    dyn_led_control(1, 1);
    printf("\nGetting LED value \n");
    dyn_led_read(1, &tmp);
    //assert(tmp == 1);

    printf("\n************************\n");
    printf("Test passed successfully\n");

    //Setting motors
    endlessTurn(ID_MOTOR_R);
    endlessTurn(ID_MOTOR_L);

    printf("\nDimensiones habitacion %d ancho x %d largo mm2\n", ANCHO, LARGO);
    printf("En memoria: %I64u B = %I64u MiB\n", sizeof(datos_habitacion), sizeof(datos_habitacion) >> 20);

    printf("Pulsar 'q' para terminar, qualquier tecla para seguir\n");
    fflush(stdout);//	return 0;

    while (estado != Quit) {
        if (simulator_finished) {
            break;
        }
        Get_estado(&estado, &estado_anterior);

        //set distances
        if (!isOnWall(&minDistance)) {//Si no hemos llegado al la distancia minima para no buscar la pared
            int wall = getNearWall(&minDistance);
            if (flagRotation != wall) {
                stop();
                switch (wall) {
                    case 0:
                        printf("Es la izquierda\n");
                        if(minDistance < 15){
                            turnRight(main_speed);
                        }else{
                            turnLeft(main_speed);
                        }
                        break;
                    case 1:
                        printf("Es el centro \n");
                        forward(main_speed);
                        break;
                    case 2:
                        printf("Es la derecha \n");
                        if(minDistance < 10){
                            turnLeft(main_speed);
                        }else{
                            turnRight(main_speed);
                        }
                        break;
                }
                flagRotation = wall;
            }
            printf("min distance %d \n", minDistance);
            searchingWall = 1;
        } else {
            //Follow the wall
            printf("Stop searching\n");
            stop();
            searchingWall==0;
            flagRotation = -1;
            if (isCorrectPosition(&minDistance) == 1){
                printf("Correct position\n");
                if (flagRotation != 0){
                    stop();
                    forward(main_speed);
                    flagRotation = 0;
                }
            }else{
                printf("Correcting position to right \n");
                if(flagRotation != 1){
                    stop();
                    turnRight(80);
                    flagRotation = 1;
                }
            }
            //Rotate to the left to follow the wall
            /*turnRight(main_speed);
            stop();*/

            //Signal the emulation thread to stop
        }



        //1er caso, no hay ningun obstaculo, go forward
        /*if (distLeft > MAX_DISTANCE_OBSTACLE && distCenter > MAX_DISTANCE_OBSTACLE &&
            distRight > MAX_DISTANCE_OBSTACLE) {
            printf("No hay obstaculos \n");
            forward(main_speed);
        } else {
            if (distLeft >= distCenter && distLeft >= distRight) {//2do caso, en la izquierda esta mas properoç
                printf("La izq es mayor \n");
                stop();
                turnLeft(10);
                stop();
                forward(10);
                stop();
            } else if (distCenter >= distLeft && distCenter >= distRight) {//3er caso, el centro esta mas propero
                printf("center es mayor \n");
                stop();
                turnLeft(10);
                stop();
                forward(10);
                stop();
            } else if (distRight >= distCenter && distRight >= distLeft) {//4to caso, en la derecha esta mas propero
                printf("La der es mayor \n");
                stop();
                turnRight(10);
                stop();
                forward(10);
                stop();
            }
        }*/
        /*** Teclado ***/
        if (estado != estado_anterior) {
            Set_estado_anterior(estado);
            printf("estado = %d\n", estado);
            switch (estado) {
                case Sw1:
                    printf("Boton Sw1 ('a') apretado\n");
                    //dyn_led_control(1, 1); //Probaremos de encender el led del motor 2
                    printf("\n");
                    break;
                case Sw2:
                    printf("Boton Sw2 ('s') apretado\n");
                    //dyn_led_control(1, 0); //Probaremos de apagar el led del motor 2
                    printf("\n");
                    break;
                case Up:

                    break;
                case Down:

                    break;
                case Left:
                    //Comprobaremos si detectamos las esquinas de la pared izquierda:
                    printf("Esquina inferior izquierda:\n");
                    printf("(1, 1): %d (fuera pared)\n", obstaculo(1, 1, datos_habitacion));
                    printf("(0, 1): %d (pared izq.)\n", obstaculo(0, 1, datos_habitacion));
                    printf("(1, 0): %d (pared del.)\n", obstaculo(1, 0, datos_habitacion));
                    printf("(0, 0): %d (esquina)\n", obstaculo(0, 0, datos_habitacion));
                    printf("Esquina superior izquierda:\n");
                    printf("(1, 4094): %d (fuera pared)\n", obstaculo(1, 4094, datos_habitacion));
                    printf("(0, 4094): %d (pared izq.)\n", obstaculo(0, 4094, datos_habitacion));
                    printf("(1, 4095): %d (pared fondo.)\n", obstaculo(1, 4095, datos_habitacion));
                    printf("(0, 4095): %d (esquina)\n", obstaculo(0, 4095, datos_habitacion));
                    //stop();
                    break;
                case Right:
                    //Comprobaremos si detectamos las esquinas de la pared derecha:
                    printf("Esquina inferior derecha:\n");
                    printf("(4094, 1): %d (fuera pared)\n", obstaculo(4094, 1, datos_habitacion));
                    printf("(4094, 0): %d (pared del.)\n", obstaculo(4094, 0, datos_habitacion));
                    printf("(4095, 1): %d (pared der.)\n", obstaculo(4095, 1, datos_habitacion));
                    printf("(4095, 0): %d (esquina)\n", obstaculo(4095, 0, datos_habitacion));
                    printf("Esquina superior derecha:\n");
                    printf("(4094, 4094): %d (fuera pared)\n", obstaculo(4094, 4094, datos_habitacion));
                    printf("(4094, 4095): %d (pared fondo)\n", obstaculo(4094, 4095, datos_habitacion));
                    printf("(4095, 4094): %d (pared der.)\n", obstaculo(4095, 4094, datos_habitacion));
                    printf("(4095, 4095): %d (esquina)\n", obstaculo(4095, 4095, datos_habitacion));
                    break;
                case Center:

                    break;
                case Quit:
                    printf("Adios!\n");
                    break;
                    //etc, etc...
                    pthread_kill(tid, SIGTERM);
                    pthread_kill(jid, SIGTERM);
                    printf("Programa terminado\n");
                    fflush(stdout);
            }
            fflush(stdout);
        }

    }
    pthread_kill(tid, SIGTERM);
    pthread_kill(jid, SIGTERM);
    printf("Programa terminado\n");
    fflush(stdout);

}
