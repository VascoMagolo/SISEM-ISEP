#include "can.h"
#include "DAVE.h"
#include <stdint.h>

volatile uint8_t  flag_data_rx = 0x00;
volatile uint16_t can_id_rx    = 0x00;
uint8_t length_rx = 0x00;
uint8_t data_rx[0x08] = { 0x00 };

void can_interrupt() {
	CAN_NODE_STATUS_t receive_status;
	CAN_NODE_STATUS_t status;

	const CAN_NODE_t *can_node = &CAN_NODE_0;
	XMC_CAN_MO_t *msg_obj = CAN_NODE_0.lmobj_ptr[0]->mo_ptr;

	status = CAN_NODE_MO_GetStatus(can_node->lmobj_ptr[0]);

	if (status & XMC_CAN_MO_STATUS_RX_PENDING) {
		XMC_CAN_MO_ResetStatus(msg_obj, XMC_CAN_MO_RESET_STATUS_RX_PENDING);

		receive_status = CAN_NODE_MO_Receive(can_node->lmobj_ptr[0]);

		if (receive_status == CAN_NODE_STATUS_SUCCESS) {
			memcpy(data_rx, &msg_obj->can_data[0],
					sizeof(msg_obj->can_data[0]));

			memcpy(data_rx + sizeof(msg_obj->can_data[0]),
					&msg_obj->can_data[1], sizeof(msg_obj->can_data[1]));

			length_rx  = msg_obj->can_data_length;
			can_id_rx  = (uint16_t)msg_obj->can_identifier;

			flag_data_rx = 0x01;
		} else {
			// message object failed to receive.
		}
	}
}

void can_send(const CAN_NODE_t* can_node, uint16_t can_id, uint8_t *data,
		uint8_t length) {
	CAN_NODE_STATUS_t can_msg_tx_status;
	CAN_NODE_STATUS_t status;

	XMC_CAN_MO_t *can_msg = can_node->lmobj_ptr[1]->mo_ptr;

	//Application code.
	//can_msg->can_mo_type = XMC_CAN_MO_TYPE_TRANSMSGOBJ;  // configure msg type as transmit type
	//can_msg->can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS; // configure msg identifier type

	can_msg->can_data_length = length; // configure CAN transmit msg data length field
	can_msg->can_identifier = can_id;

	// most significant 4 bytes
	can_msg->can_data[0] = data[0] + (data[1] << 8) + (data[2] << 16)
			+ (data[3] << 24);

	// least significant 4 bytes
	can_msg->can_data[1] = data[4] + (data[5] << 8) + (data[6] << 16)
			+ (data[7] << 24);

	// runtime change the msg configuration
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

	return;
}

void can_send_sensor(const CAN_NODE_t* can_node, uint16_t can_id, float temperature, float humidity) {
	// DBC encoding:
	// Temperature [0..31]  = (celsius + 55) * 10  (factor 0.1, offset -55)
	// Humidity    [32..63] = percent * 10          (factor 0.1, offset 0)
    uint32_t temp_enc = (uint32_t)((temperature + 55.0f) * 10.0f);
    uint32_t hum_enc  = (uint32_t)(humidity * 10.0f);

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

void can_decode_sensor(const uint8_t data[8], float *temp, float *hum) {
    uint32_t temp_raw = (uint32_t)data[0] | ((uint32_t)data[1] << 8)
                      | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
    uint32_t hum_raw  = (uint32_t)data[4] | ((uint32_t)data[5] << 8)
                      | ((uint32_t)data[6] << 16) | ((uint32_t)data[7] << 24);
    *temp = (float)temp_raw / 10.0f - 55.0f;
    *hum  = (float)hum_raw  / 10.0f;
}
