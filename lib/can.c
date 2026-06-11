#include "can.h"

#include <stdint.h>

#include "DAVE.h"

volatile uint8_t flag_data_rx = 0x00;
volatile uint16_t can_id_rx   = 0x00;
uint8_t length_rx             = 0x00;
uint8_t data_rx[8]         = {0x00};

void can_interrupt() {
    CAN_NODE_STATUS_t receive_status;
    CAN_NODE_STATUS_t status;

    const CAN_NODE_t* can_node = &CAN_NODE;
    XMC_CAN_MO_t* msg_obj = CAN_NODE.lmobj_ptr[0]->mo_ptr;

    status = CAN_NODE_MO_GetStatus(can_node->lmobj_ptr[0]);

    if (status & XMC_CAN_MO_STATUS_RX_PENDING) {
        XMC_CAN_MO_ResetStatus(msg_obj, XMC_CAN_MO_RESET_STATUS_RX_PENDING);

        receive_status = CAN_NODE_MO_Receive(can_node->lmobj_ptr[0]);

        if (receive_status == CAN_NODE_STATUS_SUCCESS) {
            memcpy(data_rx, &msg_obj->can_data[0], sizeof(msg_obj->can_data[0]));

            memcpy(data_rx + sizeof(msg_obj->can_data[0]), &msg_obj->can_data[1],
                         sizeof(msg_obj->can_data[1]));

            length_rx = msg_obj->can_data_length;
            can_id_rx = (uint16_t)msg_obj->can_identifier;

            flag_data_rx = 0x01;
        } else {
            // message object failed to receive.
        }
    }
}

void can_send(const CAN_NODE_t* can_node, uint16_t can_id, uint8_t* data,
              uint8_t length) {
    CAN_NODE_STATUS_t can_msg_tx_status;
    CAN_NODE_STATUS_t status;

    XMC_CAN_MO_t* can_msg = can_node->lmobj_ptr[1]->mo_ptr;

    can_msg->can_data_length = length;
    can_msg->can_identifier = can_id;

    // load payload into the two CAN data registers
    can_msg->can_data[0] = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    can_msg->can_data[1] = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);

    CAN_NODE_MO_Init(can_node->lmobj_ptr[1]);

    can_msg_tx_status = CAN_NODE_MO_Transmit(can_node->lmobj_ptr[1]);

    if (can_msg_tx_status == CAN_NODE_STATUS_SUCCESS) {
        status = CAN_NODE_MO_GetStatus(can_node->lmobj_ptr[1]);

        if (status & XMC_CAN_MO_STATUS_TX_PENDING) {
            XMC_CAN_MO_ResetStatus(can_msg, XMC_CAN_MO_RESET_STATUS_TX_PENDING);
        } else {
            // message object failed to transmit.
        }
    }
}

void can_send_sensor(const CAN_NODE_t* can_node, uint16_t can_id,
                     float temperature, float humidity) {
    // DBC encoding:
    // Temperature [0..31]  = (celsius + 55) * 10  (factor 0.1, offset -55)
    // Humidity    [32..63] = percent * 10          (factor 0.1, offset 0)
    uint32_t temp_enc = (uint32_t)((temperature + 55.0f) * 10.0f);
    uint32_t hum_enc = (uint32_t)(humidity * 10.0f);

    uint8_t payload[8];
    payload[0] = (uint8_t)(temp_enc);
    payload[1] = (uint8_t)(temp_enc >> 8);
    payload[2] = (uint8_t)(temp_enc >> 16);
    payload[3] = (uint8_t)(temp_enc >> 24);
    payload[4] = (uint8_t)(hum_enc);
    payload[5] = (uint8_t)(hum_enc >> 8);
    payload[6] = (uint8_t)(hum_enc >> 16);
    payload[7] = (uint8_t)(hum_enc >> 24);

    can_send(can_node, can_id, payload, 8);
}

void can_send_gps(const CAN_NODE_t* can_node, uint16_t can_id, float lat,
                  float lon) {
    // encoding: int32_t * 1e6 -> physical = raw * 0.000001 (degrees)
    int32_t lat_enc = (int32_t)(lat * 1e6f);
    int32_t lon_enc = (int32_t)(lon * 1e6f);

    uint8_t payload[8];
    payload[0] = (uint8_t)(lat_enc);
    payload[1] = (uint8_t)(lat_enc >> 8);
    payload[2] = (uint8_t)(lat_enc >> 16);
    payload[3] = (uint8_t)(lat_enc >> 24);
    payload[4] = (uint8_t)(lon_enc);
    payload[5] = (uint8_t)(lon_enc >> 8);
    payload[6] = (uint8_t)(lon_enc >> 16);
    payload[7] = (uint8_t)(lon_enc >> 24);

    can_send(can_node, can_id, payload, 8);
}

void can_decode_sensor(const uint8_t data[8], float* temp, float* hum) {
    uint32_t temp_b0 = (uint32_t)data[0];
    uint32_t temp_b1 = (uint32_t)data[1] << 8;
    uint32_t temp_b2 = (uint32_t)data[2] << 16;
    uint32_t temp_b3 = (uint32_t)data[3] << 24;
    uint32_t temp_raw = temp_b0 | temp_b1 | temp_b2 | temp_b3;

    uint32_t hum_b0 = (uint32_t)data[4];
    uint32_t hum_b1 = (uint32_t)data[5] << 8;
    uint32_t hum_b2 = (uint32_t)data[6] << 16;
    uint32_t hum_b3 = (uint32_t)data[7] << 24;
    uint32_t hum_raw = hum_b0 | hum_b1 | hum_b2 | hum_b3;

    *temp = (float)temp_raw / 10.0f - 55.0f;
    *hum = (float)hum_raw / 10.0f;
}
