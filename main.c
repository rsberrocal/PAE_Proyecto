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

int isOnWall(const uint8_t *dist, int min, int max);

//return to 0 if min dist is on left, 1 on center, 2 on right
int getNearWall(uint8_t *minDist) {
    uint8_t distLeft = sensorRead(ID_SENSOR, DYN_REG__IR_LEFT);
    //printf("LEFT: %hu \n", distLeft);
    uint8_t distCenter = sensorRead(ID_SENSOR, DYN_REG__IR_CENTER);
    //printf("CENTER: %hu \n", distCenter);
    uint8_t distRight = sensorRead(ID_SENSOR, DYN_REG__IR_RIGHT);
    //printf("RIGHT: %hu \n", distRight);
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

int isOnWall(const uint8_t *minDist, int min, int max) {
    if (*minDist > min && *minDist < max) {
        return 1;
    }
    return 0;
}

void searchingWall(uint8_t *minDist, int *stage, int *lastWall, int *isStopped) {
    if (isOnWall(minDist, 30 , 45) == 0) {//Si no esta en la pared
        printf("Buscamos pared\n");
        int wall = getNearWall(minDist);//Miramos en que pared esta, tambien pillamos la distancia minima
        if (*lastWall != wall) {//Si la pared ha cambiado es que necesitamos hacer un giro
            switch (wall) {
                case 0://Cambiamos a la izquierda
                    //Hacemos un stop y en la siguiente iteracion giramos
                    if (*isStopped == 0) {//No esta parado
                        stop();
                        *isStopped = 1;
                    } else {//Esta parado
                        turnLeft(MAIN_SPEED);//giramos
                        *lastWall = wall;
                        *isStopped = 0;// ya no esta quieto
                    }
                    break;
                case 1://Cambiamos hacia adelante
                    //Hacemos un stop y en la siguiente iteracion giramos
                    if (*isStopped == 0) {//No esta parado
                        stop();
                        *isStopped = 1;
                    } else {//Esta parado
                        forward(MAIN_SPEED);//giramos
                        *lastWall = wall;
                        *isStopped = 0;// ya no esta quieto
                    }
                    break;
                case 2://Cambiamos a la izquierda
                    //Hacemos un stop y en la siguiente iteracion giramos
                    if (*isStopped == 0) {//No esta parado
                        stop();
                        *isStopped = 1;
                    } else {//Esta parado
                        turnRight(MAIN_SPEED);//giramos
                        *lastWall = wall;
                        *isStopped = 0;// ya no esta quieto
                    }
                    break;
            }
        }
    } else {//de estarlo pasamos al siguiente stage
        if (*isStopped == 0) { //Si no esta parado, lo paramos
            stop();
            *isStopped = 1;
        }
        *lastWall = -1;//Reset de wall
        *stage = 1;
    }
}

void followWall(uint8_t *minDist, int *stage, int *lastWall, int *isStopped) {
    int wall = getNearWall(minDist);//Miramos en que pared esta, tambien pillamos la distancia minima
    if (isOnWall(minDist, 35, 45 ) == 1) {//Si esta en la pared
        //Corregimos posicion
        if (wall != 0) {//La pared mas cercana no es la de la izquierda, corregimos
            //Hacemos un stop y en la siguiente iteracion giramos
            if (*lastWall != 2) {
                if (*isStopped == 0) {//No esta parado
                    stop();
                    *isStopped = 1;
                } else {//Esta parado
                    turnRight(MAIN_SPEED);//giramos
                    *lastWall = 2;
                    *isStopped = 0;// ya no esta quieto
                }
            }
        } else {// estamos alineados con la pared nos movemos adelante
            //Hacemos un stop y en la siguiente iteracion giramos
            printf("Adelante\n\n");
            if (*lastWall != 0) {
                if (*isStopped == 0) {//No esta parado
                    stop();
                    *isStopped = 1;
                } else {//Esta parado
                    forward(MAIN_SPEED);//giramos
                    *lastWall = 0;
                    *isStopped = 0;// ya no esta quieto
                }
            }
        }
    } else {//Si no esta en la pared
        printf("No esta en pared %d \n", *minDist);
        if (*isStopped == 0) { //Si no esta parado, lo paramos
            stop();
            *isStopped = 1;
        }
        *lastWall = -1;//Reset de wall
        *stage = 2;
    }
}

void correctPosition(uint8_t *minDist, int *stage, int *lastWall, int *isStopped) {
    uint8_t distCenter = sensorRead(ID_SENSOR, DYN_REG__IR_CENTER);
    int wall = getNearWall(minDist);//Miramos en que pared esta, tambien pillamos la distancia minima
    if (isOnWall(minDist,35 , 45) == 0) {
        if(*minDist >=45){//Si se ha desviado para la derecha, giramos en lado contrario
            if (*lastWall != 2) {
                if (*isStopped == 0) {//No esta parado
                    stop();
                    *isStopped = 1;
                } else {//Esta parado
                    turnLeft(MAIN_SPEED);//giramos
                    *lastWall = 2;
                    *isStopped = 0;// ya no esta quieto
                }
            }
        }else{//No se desvia hacia derecha, quizas ha de girar a derecha para corregir posicion o quizas para esquivar obstaculo
            if (distCenter <45)
                *stage = 3;
            else if (*lastWall != 0) {
                if (*isStopped == 0) {//No esta parado
                    stop();
                    *isStopped = 1;
                } else {//Esta parado
                    turnRight(MAIN_SPEED);//giramos
                    *lastWall = 0;
                    *isStopped = 0;// ya no esta quieto
                }
            }
        }
    }else {//esta corregido la posicion
        if (*isStopped == 0) { //Si no esta parado, lo paramos
            stop();
            *isStopped = 1;
        }
        *lastWall = -1;//Reset de wall
        *stage = 1;
    }
}
rotateRight(uint8_t *minDist, int *stage, int *lastWall, int *isStopped){
    uint8_t distCenter = sensorRead(ID_SENSOR, DYN_REG__IR_CENTER);
    if (distCenter < 45){
        turnRight(90);//giramos
        *isStopped = 0;// ya no esta quieto
    }
    else {//esta corregido la posicion

        if (*isStopped == 0) { //Si no esta parado, lo paramos
            stop();
            *isStopped = 1;
        }
        *lastWall = -1;//Reset de wall
        *stage = 1;
    }
}

/**
 * main.c
 */
int main(void) {
    pthread_t tid, jid;
    uint8_t tmp;
    int main_speed = 190;
    uint8_t distCenter = 0;
    uint8_t distRight = 0;
    uint8_t distLeft = 0;
    int flagRotation = -1;
    int lastWall = -1;
    int isStopped = 1;
    //int searchingWall = 1;
    int hasStop = -1;
    int stage = 0; //Estado, 0 -> Buscando pared.
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
    int test = 0;
    while (estado != Quit) {
        if (simulator_finished) {
            break;
        }
        Get_estado(&estado, &estado_anterior);
        switch (stage) {
            case 0:// buscamos la pared mas cercana
                searchingWall(&minDistance, &stage, &lastWall, &isStopped);
                break;
            case 1://Estamos en la pared, solo tenemos que seguirla
                printf("Estamos en pared\n");
                followWall(&minDistance, &stage, &lastWall, &isStopped);
                break;
            case 2:
                printf("Hay que corregir posicion\n");
                correctPosition(&minDistance, &stage, &lastWall, &isStopped);
                break;
            case 3:
                printf("Giramos en obstaculo");
                rotateRight(&minDistance, &stage, &lastWall, &isStopped);

        }

        /*** Teclado ***/
        if (estado != estado_anterior) {
            Set_estado_anterior(estado);
            printf("estado = %d\n", estado);
            switch (estado) {
                case Sw1:
                    printf("Boton Sw1 ('a') apretado\n");
                    printf("\n");
                    break;
                case Sw2:
                    printf("Boton Sw2 ('s') apretado\n");
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
            }
            fflush(stdout);
        }
    }
    pthread_kill(tid, SIGTERM);
    pthread_kill(jid, SIGTERM);
    printf("Programa terminado\n");
    fflush(stdout);
}
