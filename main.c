#include "open_port.h"
#include "make_packet.h"
#include<stdbool.h>

#define QUEUE_SIZE 100

bool rq_flag = false;
bool oq_flag = false;

int ack_cnt = 0;

CRITICAL_SECTION g_cs;

Packet recent_send = { 0 };

typedef struct Queue {
	int rear, front;
	Packet index[QUEUE_SIZE];
}Queue;

Queue input_Queue = { 0 };
Queue output_Queue = { 0 };
Queue req_Queue = { 0 };

BOOL isFull() {
	Queue* rq = &req_Queue;
	if (rq->front == (rq->rear + 1) % QUEUE_SIZE) {
		return FALSE;
	}
	return TRUE;
}

void init_Queue() {
	input_Queue.front = input_Queue.rear = 0;
	output_Queue.front = output_Queue.rear = 0;
	req_Queue.front = req_Queue.rear = 0;
}

BYTE type_classification(Queue *iq) {
	DWORD bytesWritten;

	if (iq->index[iq->front].type == DATA) {
		if (iq->index[iq->front].sum != check_sum(iq->index[iq->front].data))
			return DATA_ERROR;
		else if (iq->index[iq->front].data[0] == OS_REQ) {
			return OS_REQ;
		}
		else if (iq->index[iq->front].data[0] == OS_INFO) {
			return OS_INFO;
		}
		else if (iq->index[iq->front].data[0] == CPU_REQ) {
			return CPU_REQ;
		}
		else if (iq->index[iq->front].data[0] == CPU_INFO) {
			return CPU_INFO;
		}
	}
	else if (iq->index[iq->front].type == FLOW) {
		if (iq->index[iq->front].data[0] == ACK) {
			return ACK;
		}
		else if (iq->index[iq->front].data[0] == NACK) {
			return NACK;
		}
	}
	return 0;
}

BOOL packet_inspection(Queue* iq){
	if (iq->index[iq->front].stx != STX) return FALSE;
	if (iq->index[iq->front].etx != ETX) return FALSE;
	return TRUE;
}

DWORD WINAPI CmdThread(LPVOID lpParam) {
	HANDLE hCOM = lpParam;
	BOOL ret;
	DWORD bytesWritten;
	Packet* os_req = &reqOS_packet;
	Packet* cpu_req = &reqCPU_packet;
	Queue* rq = &req_Queue;

	while (1) {
		printf("\n\t\t동작 선택 [q] [w] [e] [r]");
		int input_key = getch();
		switch (input_key) {
		case 'q':
			if (isFull() == TRUE) {
				rq->index[rq->rear] = *os_req;
				rq->rear = (rq->rear + 1) % QUEUE_SIZE;
			}
			else {
				printf("\n\t\t요청 실패. 잠시 후 다시 시도해 주세요.");
				Sleep(3000);
			}
			break;
		case 'w':
			if (isFull() == TRUE) {
				rq->index[rq->rear] = *cpu_req;
				rq->rear = (rq->rear + 1) % QUEUE_SIZE;
			}
			else {
				printf("\n\t\t요청 실패. 잠시 후 다시 시도해 주세요.");
				Sleep(3000);
			}
			break;
		case 'e':
			printf("\n\t\t프로그램 종료...");
			return 0;

		case 'r':
			EnterCriticalSection(&g_cs);
			system("cls");
			rq->rear = rq->front = 0;
			printf("\n\t\tACK : %d", ack_cnt);
			ack_cnt = 0;
			LeaveCriticalSection(&g_cs);
			break;
		//테스트 커맨드
		case 't':
		{
			EnterCriticalSection(&g_cs);
			for (int i = 0; i < QUEUE_SIZE - 1; i++)
			{
				if (i % 2 == 0)
					rq->index[i] = *os_req;
				else
					rq->index[i] = *cpu_req;
			}
			rq->rear = QUEUE_SIZE - 1;
			LeaveCriticalSection(&g_cs);
			break;
		}
		default:
			printf("\n\t\t올바른 값 입력");
		}
	}
}

DWORD WINAPI RecvThread(LPVOID lpParam) {
	HANDLE hCOM = lpParam;
	DWORD first_bytesRead, second_bytesRead;
	Queue* iq = &input_Queue;
	BOOL ret;
	Packet packet;

	while (1) {
		first_bytesRead = second_bytesRead = 0;
		ret = ReadFile(hCOM, &packet, sizeof(Packet), &first_bytesRead, NULL);
		if (ret == FALSE) {
			fprintf(stderr, "\n\t\tReadFile Error : %d", GetLastError());
			return - 1;
		}
		if (first_bytesRead > 0 && packet.len != first_bytesRead) {
			ret = ReadFile(hCOM, ((BYTE*)&packet) + first_bytesRead, packet.len - first_bytesRead, &second_bytesRead, NULL);
			if (ret == FALSE) {
				fprintf(stderr, "\n\t\tReadFile Error : %d", GetLastError());
				return -1;
			}
		}

		if (first_bytesRead + second_bytesRead > 0)
		{
			printf("\n\t\t[INPUT BUFFER READ]");
			iq->index[iq->rear] = packet;
			iq->rear = (iq->rear + 1) % QUEUE_SIZE;
		}
	}
}

DWORD WINAPI Data_Processing(LPVOID lpParam) {
	int cnt = 0;
	HANDLE hCOM = lpParam;
	Queue* oq = &output_Queue;
	Queue* iq = &input_Queue;
	DWORD bytesWritten;
	while (1) {
		if (iq->front != iq->rear) {
			if (packet_inspection(iq) == FALSE) {
				iq->front = (iq->front + 1) % QUEUE_SIZE;
			}
			else {
				switch (type_classification(iq))
				{
				case DATA_ERROR:
					printf("\n\t\t[SEND][NACK]");
					WriteFile(hCOM, &nack_packet, sizeof(Packet), &bytesWritten, NULL);
					break;
				case OS_REQ:
					printf("\n\t\t[READ][REQ]");
					oq->index[oq->rear] = infoOS_packet;
					oq->rear = (oq->rear + 1) % QUEUE_SIZE;
					break;

				case CPU_REQ:
					printf("\n\t\t[READ][REQ]");
					oq->index[oq->rear] = infoCPU_packet;
					oq->rear = (oq->rear + 1) % QUEUE_SIZE;
					break;

				case OS_INFO:
					WriteFile(hCOM, &ack_packet, sizeof(Packet), &bytesWritten, NULL);
					printf("\n\t\t[SEND][ACK]");
					printf("\n\t\t[READ][INFO][%s]", (iq->index[iq->front].data) + 1);
					rq_flag = false;
					break;

				case CPU_INFO:
					WriteFile(hCOM, &ack_packet, sizeof(Packet), &bytesWritten, NULL);
					printf("\n\t\t[SEND][ACK]");
					printf("\n\t\t[READ][INFO][%s]", (iq->index[iq->front].data) + 1);
					rq_flag = false;
					break;

				case ACK:
					printf("\n\t\t[READ][ACK]");
					oq_flag = false;
					ack_cnt++;
					break;
				
				case NACK:
					printf("\n\t\t!!![READ][NACK]");
					WriteFile(hCOM, &recent_send, sizeof(Packet), &bytesWritten, NULL);
					cnt++;
					printf("\n\t\t%d번째 NACK 발생", cnt);
					if (cnt > 3) {
						printf("\n\t\tNACK 3회 발생으로 인한 프로그램 종료.");
						return -1;
					}
					break;
				}
				iq->front = (iq->front + 1) % QUEUE_SIZE;
			}
		}
	}
}

DWORD WINAPI SendThread(LPVOID lpParam) {
	HANDLE hCOM = lpParam;
	DWORD bytesWritten;
	Queue* oq = &output_Queue;
	Queue* rq = &req_Queue;
	BOOL ret;
	while (1)
	{
		EnterCriticalSection(&g_cs);
		if ((oq->front != oq->rear) && oq_flag == false) {
			ret = WriteFile(hCOM, &oq->index[oq->front], sizeof(Packet), &bytesWritten, NULL);
			if (ret == FALSE) {
				fprintf(stderr, "WriteFile Error %d", GetLastError());
				LeaveCriticalSection(&g_cs);
				return -1;
			}
			LeaveCriticalSection(&g_cs);
			printf("\n\t\t[SEND][INFO]");
			recent_send = oq->index[oq->front];
			oq_flag = true;
			oq->front = (oq->front + 1) % QUEUE_SIZE;
		}

		if ((rq->front != rq->rear) && rq_flag == false) {
			ret = WriteFile(hCOM, &rq->index[rq->front], sizeof(Packet), &bytesWritten, NULL);
			if (ret == FALSE) {
				fprintf(stderr, "WriteFile Error %d", GetLastError());
				return -1;
			}
			LeaveCriticalSection(&g_cs);
			printf("\n\t\t[SEND][REQ]");
			recent_send = rq->index[rq->front];
			rq_flag = true;
			rq->front = (rq->front + 1) % QUEUE_SIZE;
			if (rq->front == rq->rear)
				printf("\n\t\tReq 패킷 %d회 전송 완료.", QUEUE_SIZE-1);
		}
		LeaveCriticalSection(&g_cs);
	}
}

int main() {
	LPCSTR portNum = get_PortNum();
	DWORD baud = get_baudrate();
	HANDLE hCOM = Open_Port(portNum, baud);
	if (hCOM == NULL) {
		printf("\n\t\t연결 실패...");
		return -1;
	}

	init_Queue();
	create_packet(&ack_packet, ACK);
	create_packet(&nack_packet, NACK);
	create_packet(&infoOS_packet, OS_INFO);
	create_packet(&reqOS_packet, OS_REQ);
	create_packet(&infoCPU_packet, CPU_INFO);
	create_packet(&reqCPU_packet, CPU_REQ);

	InitializeCriticalSection(&g_cs);

	HANDLE hThread[4];
	DWORD hThreadID[4];

	hThread[0] = CreateThread(NULL, 0, CmdThread, (LPVOID)hCOM, 0, &hThreadID[0]);
	if (hThread[0] == NULL) {
		fprintf(stderr, "\n\t\t스레드1 생성 에러 : %d", GetLastError());
		return -1;
	}

	hThread[1] = CreateThread(NULL, 0, RecvThread, (LPVOID)hCOM, 0, &hThreadID[1]);
	if (hThread[1] == NULL) {
		fprintf(stderr, "\n\t\t스레드2 생성 에러 : %d", GetLastError());
		return -1;
	}

	hThread[2] = CreateThread(NULL, 0, Data_Processing, (LPVOID)hCOM, 0, &hThreadID[2]);
	if (hThread[2] == NULL) {
		fprintf(stderr, "\n\t\t스레드3 생성 에러 : %d", GetLastError());
		return -1;
	}

	hThread[3] = CreateThread(NULL, 0, SendThread, (LPVOID)hCOM, 0, &hThreadID[3]);
	if (hThread[3] == NULL) {
		fprintf(stderr, "\n\t\t스레드4 생성 에러 : %d", GetLastError());
		return -1;
	}

	WaitForMultipleObjects(4, hThread, FALSE, INFINITE);

	for (int i = 0; i < 4; i++) {
		CloseHandle(hThread[i]);
		printf("\n\t\tThread %d 종료...", i);
		Sleep(1000);
	}

	DeleteCriticalSection(&g_cs);
	CloseHandle(hCOM);
	return 0;
}