#ifndef LIB_CAN_H_
#define LIB_CAN_H_

#include <stdint.h>

#include "DAVE.h"

#define CAN_ID_GROUP ((uint16_t)0x4C0)

extern volatile uint8_t flag_data_rx;
extern volatile uint16_t can_id_rx;
extern uint8_t data_rx[8];
extern uint8_t length_rx;

void can_interrupt();
void can_send(const CAN_NODE_t* can_node, uint16_t can_id, uint8_t* data,
              uint8_t length);
void can_send_sensor(const CAN_NODE_t* can_node, uint16_t can_id,
                     float temperature, float humidity);
void can_decode_sensor(const uint8_t data[8], float* temp, float* hum);
void can_send_gps(const CAN_NODE_t* can_node, uint16_t can_id, float lat,
                  float lon);

#endif /* LIB_CAN_H_ */
