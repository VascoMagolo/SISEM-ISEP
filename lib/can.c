#include "can.h"
#include "DAVE.h"

uint8_t flag_data_rx = 0x00;
uint8_t length_rx = 0x00;
uint8_t data_rx[0x08] = { 0x00 };

void can_interrupt() {
	CAN_NODE_STATUS_t receive_status;
	CAN_NODE_STATUS_t status;

	const CAN_NODE_t *can_node = &CAN_NODE_A;
	XMC_CAN_MO_t *msg_obj = CAN_NODE_A->lmobj_ptr[0]->mo_ptr;

	status = CAN_NODE_MO_GetStatus(can_node->lmobj_ptr[0]);

	if (status & XMC_CAN_MO_STATUS_RX_PENDING) {
		XMC_CAN_MO_ResetStatus(msg_obj, XMC_CAN_MO_RESET_STATUS_RX_PENDING);

		receive_status = CAN_NODE_MO_Receive(can_node->lmobj_ptr[0]);

		if (receive_status == CAN_NODE_STATUS_SUCCESS) {
			memcpy(data_rx, &msg_obj->can_data[0],
					sizeof(msg_obj->can_data[0]));

			memcpy(data_rx + sizeof(msg_obj->can_data[0]),
					&msg_obj->can_data[1], sizeof(msg_obj->can_data[1]));

			length_rx = msg_obj->can_data_length;

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
