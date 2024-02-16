#include "make_packet.h"
#pragma warning(disable:4996)

BYTE check_sum(const unsigned char* data) {
	int sum = 0;
	for (int i = 0; i < DATA_SIZE; i++)
		sum += data[i];
	return (BYTE)sum;
}

void create_packet(Packet* packet, BYTE type) {
	memset(packet, 0, sizeof(Packet));

	packet->stx = STX;
	packet->len = sizeof(Packet);
	if (type == OS_REQ || type == OS_INFO || type == CPU_REQ || type == CPU_INFO) {
		packet->type = DATA;
		if (type == OS_REQ) 
			packet->data[0] = OS_REQ;

		else if (type == CPU_REQ)
			packet->data[0] = CPU_REQ;

		else if (type == OS_INFO) {
			const char* os = "windows";
			packet->data[0] = OS_INFO;
			strcpy(&packet->data[1], os);

		}
		else if (type == CPU_INFO) {
			const char* cpu = "AMD64";
			packet->data[0] = CPU_INFO;
			strcpy(&packet->data[1], cpu);
		}

	}
	else if (type == ACK) {
		packet->type = FLOW;
		packet->data[0] = ACK;
	}
	else if (type == NACK) {
		packet->type = FLOW;
		packet->data[0] = NACK;
	}
	packet->sum = check_sum(packet->data);
	packet->etx = ETX;
};

