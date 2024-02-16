#include "open_port.h"

LPCSTR get_PortNum() {
    int x;
    LPCSTR port = "";
    printf("\n\n\t\t연결 포트 번호 입력 : ");
    scanf_s("%d", &x);
    if (x == 3)
        return port = L"COM3";
    else if (x == 4)
        return port = L"COM4";
    else if (x == 5)
        return port = L"COM5";
    else
        return NULL;
}
DWORD get_baudrate() {
    DWORD x;
    printf("\n\t\tbaudrate 입력 : ");
    scanf_s("%lu", &x);
    return x;
}

HANDLE Open_Port(LPCSTR portName, DWORD baudRate) {
    HANDLE hCOM = CreateFile(portName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hCOM == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    else {
        printf("\n\t\t%lS 포트 연결 성공!!", portName);
        printf("\n\t\tbaudrate  is  %lu", baudRate);
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hCOM, &dcbSerialParams)) {
        fprintf(stderr, "Error getting serial port state.\n");
        CloseHandle(hCOM);
        return NULL;
    }

    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hCOM, &dcbSerialParams)) {
        fprintf(stderr, "Error setting serial port state.\n");
        CloseHandle(hCOM);
        return NULL;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hCOM, &timeouts)) {
        fprintf(stderr, "Error setting timeouts.\n");
        CloseHandle(hCOM);
        return NULL;
    }
    return hCOM;
}