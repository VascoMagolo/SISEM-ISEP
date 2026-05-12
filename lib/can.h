#ifndef LIB_CAN_H_
#define LIB_CAN_H_

#define CAN_ID_GROUP ((uint16_t) 0x4c0)

void can_interrupt(); // function name defined in dave APP
void can_send(const CAN_NODE_t* can_node, uint16_t can_id, uint8_t *data,
		uint8_t length);

#endif /* LIB_CAN_H_ */
