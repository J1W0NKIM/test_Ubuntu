//OPEN_PORT_H
#ifndef OPEN_PORT_H
#define OPEN_PORT_H

#include <Windows.h>
#include <stdio.h>

LPCSTR get_PortNum();
DWORD get_baudrate();
HANDLE Open_Port(LPCSTR portNum, DWORD baudrate);

#endif //OPEN_PORT_H