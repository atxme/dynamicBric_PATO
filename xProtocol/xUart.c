////////////////////////////////////////////////////////////
//  uart source file
//  implements uart functions for Raspberry Pi
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////



#include "xUart.h"
#include "assert/xAssert.h"
#include <sys/ioctl.h>

////////////////////////////////////////////////////////////
/// uartInit
////////////////////////////////////////////////////////////
int uartInit(xos_uart_t* p_ptUart)
{
    X_ASSERT(p_ptUart != NULL);

    memset(p_ptUart, 0, sizeof(xos_uart_t));
    p_ptUart->t_iHandle = -1;
    p_ptUart->t_iBaudRate = UART_DEFAULT_BAUD;
    p_ptUart->t_iBits = UART_DEFAULT_BITS;
    p_ptUart->t_iParity = UART_DEFAULT_PARITY;
    p_ptUart->t_iStop = UART_DEFAULT_STOP;
    p_ptUart->t_ulTimeout = 1000; // 1 second default timeout

    return UART_OK;
}

////////////////////////////////////////////////////////////
/// uartOpen
////////////////////////////////////////////////////////////
int uartOpen(xos_uart_t* p_ptUart, const char* p_pcDevice)
{
    X_ASSERT(p_ptUart != NULL);
    X_ASSERT(p_pcDevice != NULL);

    if (p_ptUart->t_iHandle >= 0) {
        return UART_ALREADY_OPEN;
    }

    // Open device
    p_ptUart->t_iHandle = open(p_pcDevice, O_RDWR | O_NOCTTY | O_NDELAY);
    if (p_ptUart->t_iHandle < 0) {
        return UART_ERROR;
    }

    // Save device name
    strncpy(p_ptUart->t_cDevice, p_pcDevice, sizeof(p_ptUart->t_cDevice) - 1);

    // Get current options
    if (tcgetattr(p_ptUart->t_iHandle, &p_ptUart->t_tOptions) != 0) {
        close(p_ptUart->t_iHandle);
        p_ptUart->t_iHandle = -1;
        return UART_ERROR;
    }

    return UART_OK;
}

////////////////////////////////////////////////////////////
/// uartConfigure
////////////////////////////////////////////////////////////
int uartConfigure(xos_uart_t* p_ptUart, speed_t p_iBaudRate, int p_iBits, int p_iParity, int p_iStop)
{
    X_ASSERT(p_ptUart != NULL);
    X_ASSERT(p_ptUart->t_iHandle >= 0);

    // Save configuration
    p_ptUart->t_iBaudRate = p_iBaudRate;
    p_ptUart->t_iBits = p_iBits;
    p_ptUart->t_iParity = p_iParity;
    p_ptUart->t_iStop = p_iStop;

    // Configure port
    tcgetattr(p_ptUart->t_iHandle, &p_ptUart->t_tOptions);

    // Set baud rate
    cfsetispeed(&p_ptUart->t_tOptions, p_iBaudRate);
    cfsetospeed(&p_ptUart->t_tOptions, p_iBaudRate);

    // Set control flags
    p_ptUart->t_tOptions.c_cflag = p_iBaudRate | p_iBits | CLOCAL | CREAD;
    if (p_iParity) {
        p_ptUart->t_tOptions.c_cflag |= PARENB;
    }
    if (p_iStop) {
        p_ptUart->t_tOptions.c_cflag |= CSTOPB;
    }

    // Set input flags
    p_ptUart->t_tOptions.c_iflag = IGNPAR;

    // Set output flags
    p_ptUart->t_tOptions.c_oflag = 0;

    // Set local flags
    p_ptUart->t_tOptions.c_lflag = 0;

    // Set read timeout
    p_ptUart->t_tOptions.c_cc[VTIME] = (p_ptUart->t_ulTimeout / 100); // Convert ms to deciseconds
    p_ptUart->t_tOptions.c_cc[VMIN] = 0;

    // Flush port and apply settings
    tcflush(p_ptUart->t_iHandle, TCIFLUSH);
    if (tcsetattr(p_ptUart->t_iHandle, TCSANOW, &p_ptUart->t_tOptions) != 0) {
        return UART_ERROR;
    }

    return UART_OK;
}

////////////////////////////////////////////////////////////
/// uartSend
////////////////////////////////////////////////////////////
int uartSend(xos_uart_t* p_ptUart, const char* p_pcBuffer, unsigned long p_ulSize)
{
    X_ASSERT(p_ptUart != NULL);
    X_ASSERT(p_pcBuffer != NULL);
    X_ASSERT(p_ulSize > 0);

    if (p_ptUart->t_iHandle < 0) {
        return UART_NOT_OPEN;
    }

    ssize_t l_iWritten = write(p_ptUart->t_iHandle, p_pcBuffer, p_ulSize);
    if (l_iWritten < 0) {
        return UART_ERROR;
    }

    return (int)l_iWritten;
}

////////////////////////////////////////////////////////////
/// uartReceive
////////////////////////////////////////////////////////////
int uartReceive(xos_uart_t* p_ptUart, char* p_pcBuffer, unsigned long p_ulSize)
{
    X_ASSERT(p_ptUart != NULL);
    X_ASSERT(p_pcBuffer != NULL);
    X_ASSERT(p_ulSize > 0);

    if (p_ptUart->t_iHandle < 0) {
        return UART_NOT_OPEN;
    }

    ssize_t l_iRead = read(p_ptUart->t_iHandle, p_pcBuffer, p_ulSize);
    if (l_iRead < 0) {
        return UART_ERROR;
    }
    if (l_iRead == 0) {
        return UART_TIMEOUT;
    }

    return (int)l_iRead;
}

////////////////////////////////////////////////////////////
/// uartSetTimeout
////////////////////////////////////////////////////////////
int uartSetTimeout(xos_uart_t* p_ptUart, unsigned long p_ulTimeout)
{
    X_ASSERT(p_ptUart != NULL);

    p_ptUart->t_ulTimeout = p_ulTimeout;
    if (p_ptUart->t_iHandle >= 0) {
        p_ptUart->t_tOptions.c_cc[VTIME] = (p_ulTimeout / 100);
        if (tcsetattr(p_ptUart->t_iHandle, TCSANOW, &p_ptUart->t_tOptions) != 0) {
            return UART_ERROR;
        }
    }

    return UART_OK;
}

////////////////////////////////////////////////////////////
/// uartFlush
////////////////////////////////////////////////////////////
int uartFlush(xos_uart_t* p_ptUart)
{
    X_ASSERT(p_ptUart != NULL);

    if (p_ptUart->t_iHandle < 0) {
        return UART_NOT_OPEN;
    }

    if (tcflush(p_ptUart->t_iHandle, TCIOFLUSH) != 0) {
        return UART_ERROR;
    }

    return UART_OK;
}

////////////////////////////////////////////////////////////
/// uartClose
////////////////////////////////////////////////////////////
int uartClose(xos_uart_t* p_ptUart)
{
    X_ASSERT(p_ptUart != NULL);

    if (p_ptUart->t_iHandle < 0) {
        return UART_NOT_OPEN;
    }

    close(p_ptUart->t_iHandle);
    p_ptUart->t_iHandle = -1;

    return UART_OK;
}


