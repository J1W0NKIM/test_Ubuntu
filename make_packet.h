//make_packet.h
#ifndef MAKE_PACKET_H
#define MAKE_PACKET_H

#include <Windows.h>
#define DATA_SIZE 20

#define STX 0x02
#define ETX 0x03

#define OS_REQ 0x80
#define CPU_REQ 0x85
#define OS_INFO 0x90
#define CPU_INFO 0x95

#define ACK 0x06
#define NACK 0x15

#define DATA 0x8A
#define FLOW 0x8B

#define DATA_ERROR 0x96

typedef struct Packet {
	BYTE stx;
	BYTE len;
	BYTE type;
	BYTE data[DATA_SIZE];
	BYTE sum;
	BYTE etx;
}Packet;

Packet ack_packet;
Packet nack_packet;
Packet infoOS_packet;
Packet reqOS_packet;
Packet infoCPU_packet;
Packet reqCPU_packet;

void create_packet(Packet*, BYTE);
BYTE check_sum(const unsigned char*);

#endif //MAKE_PACKET_H