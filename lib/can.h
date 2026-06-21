#ifndef LIB_CAN_H_
#define LIB_CAN_H_

#include <stdint.h>

#include "config.h"
#include "DAVE.h"

#define CAN_ID_BASE ((uint16_t)0x4C0)

#if defined(FEATURE_AHT10)
#   define CAN_ID_SENSOR (CAN_ID_BASE + 0) // 4c0
#endif // FEATURE_AHT10

#if defined(FEATURE_GPS)
#   include "gps.h"
#   define CAN_ID_GPS_POS  (CAN_ID_BASE + 1) // 4c1
#   define CAN_ID_GPS_META (CAN_ID_BASE + 2) // 4c2
#endif // FEATURE_GPS

extern volatile uint8_t flag_data_rx;
extern volatile uint16_t can_id_rx;
extern uint8_t data_rx[8];
extern uint8_t length_rx;

void can_interrupt();
void can_send(const CAN_NODE_t* can_node, uint16_t can_id, uint8_t* data, uint8_t dlc);

#if defined(FEATURE_AHT10)
    void can_send_sensor(const CAN_NODE_t* can_node, uint16_t can_id, float temperature, float humidity);
    void can_decode_sensor(const uint8_t data[8], float* temp, float* hum);
#endif // FEATURE_AHT10

#if defined(FEATURE_GPS)
    void can_send_gps(const CAN_NODE_t* can_node, uint16_t can_id, gps_data_t data);
    void can_send_gps_meta(const CAN_NODE_t* can_node, uint16_t can_id, gps_data_t data);
#endif // FEATURE_GPS

#endif /* LIB_CAN_H_ */
