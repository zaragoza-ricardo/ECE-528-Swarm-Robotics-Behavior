/**
 * @file Swarm.h
 * @brief Header file for Swarm.c
 *
 *
 *
 * @author Ricardo Zaragoza
 *
 */



#ifndef SWARM_H_
#define SWARM_H_

#include <stdint.h>
#include <stdbool.h>

// This is the current autonomous state inside the autonomous state machine.
typedef enum
{
    AUTO_IDLE = 0,
    AUTO_FORWARD,
    AUTO_WAIT,
    AUTO_TURN_LEFT,
    AUTO_TURN_RIGHT,
    AUTO_AVOID,
    AUTO_FOLLOW,
    AUTO_REGROUP,
    AUTO_REVERSE,
    AUTO_CHECK_LEFT,
    AUTO_CHECK_RIGHT,
    AUTO_RECENTER,
    AUTO_STOP
} auto_state_t;

typedef struct
{
    int robot_id;
    auto_state_t state;
    int lx;
    int ly;
    int brake;
    int throttle;
    uint8_t valid;
} peer_state_t;

/* Parse one incoming peer message and update outputs */
bool Parse_Peer_Message(const char *msg,
                        char *mode,
                        int *robot_id,
                        int *lx,
                        int *ly,
                        int *brake,
                        int *throttle);

/* Store the latest state for one peer */
void Update_Peer_Table(int robot_id,
                       auto_state_t state,
                       int lx,
                       int ly,
                       int brake,
                       int throttle);

/* Read local sensors and update local robot flags/data */
void Update_Local_Sensors(void);

/* Decide next autonomous state */
auto_state_t Decide_Next_State(void);

/* Execute motor behavior for current autonomous state */
void Run_Autonomous_StateMachine(void);

/* Send this robot's current state/status outward */
void Broadcast_My_State(void);

#endif
