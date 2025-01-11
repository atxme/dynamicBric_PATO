////////////////////////////////////////////////////////////
//  i2c source file
//  implements the i2c functions for Raspberry Pi
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xI2c.h"
#include "xAssert.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>

#ifdef _RASPBERRY

int i2cInit(xos_i2c_t* p_pttI2c)
{
    X_ASSERT_RETURN(p_pttI2c != NULL, XOS_I2C_INVALID);

    p_pttI2c->t_iHandle = -1;
    p_pttI2c->t_ulSpeed = XOS_I2C_DEFAULT_SPEED;
    p_pttI2c->t_ucAddress = 0;
    p_pttI2c->t_ulTimeout = XOS_I2C_DEFAULT_TIMEOUT;
    strcpy(p_pttI2c->t_cDevice, "/dev/i2c-");

    return XOS_I2C_OK;
}

int i2cOpen(xos_i2c_t* p_pttI2c, uint8_t p_ucBus)
{
    X_ASSERT_RETURN(p_pttI2c != NULL, XOS_I2C_INVALID);
    X_ASSERT_RETURN(p_pttI2c->t_iHandle < 0, XOS_I2C_ALREADY_OPEN);

    char device[20];
    snprintf(device, sizeof(device), "/dev/i2c-%d", p_ucBus);
    strcpy(p_pttI2c->t_cDevice, device);

    p_pttI2c->t_iHandle = open(device, O_RDWR);
    X_ASSERT_RETURN(p_pttI2c->t_iHandle >= 0, XOS_I2C_ERROR);

    return XOS_I2C_OK;
}

int i2cSetSlaveAddress(xos_i2c_t* p_pttI2c, uint8_t p_ucAddress)
{
    X_ASSERT_RETURN(p_pttI2c != NULL && p_pttI2c->t_iHandle >= 0, XOS_I2C_NOT_OPEN);

    p_pttI2c->t_ucAddress = p_ucAddress;
    int ret = ioctl(p_pttI2c->t_iHandle, I2C_SLAVE, p_ucAddress);
    return (ret < 0) ? XOS_I2C_ERROR : XOS_I2C_OK;
}

int i2cWrite(xos_i2c_t* p_pttI2c, const uint8_t* p_ptucBuffer, unsigned long p_ulSize)
{
    X_ASSERT_RETURN(p_pttI2c != NULL && p_pttI2c->t_iHandle >= 0, XOS_I2C_NOT_OPEN);
    X_ASSERT_RETURN(p_ptucBuffer != NULL && p_ulSize > 0, XOS_I2C_INVALID);

    int ret = write(p_pttI2c->t_iHandle, p_ptucBuffer, p_ulSize);
    return (ret < 0) ? XOS_I2C_ERROR : ret;
}

int i2cRead(xos_i2c_t* p_pttI2c, uint8_t* p_ptucBuffer, unsigned long p_ulSize)
{
    X_ASSERT_RETURN(p_pttI2c != NULL && p_pttI2c->t_iHandle >= 0, XOS_I2C_NOT_OPEN);
    X_ASSERT_RETURN(p_ptucBuffer != NULL && p_ulSize > 0, XOS_I2C_INVALID);

    int ret = read(p_pttI2c->t_iHandle, p_ptucBuffer, p_ulSize);
    return (ret < 0) ? XOS_I2C_ERROR : ret;
}

int i2cWriteRegister(xos_i2c_t* p_pttI2c, uint8_t p_ucRegister, uint8_t p_ucValue)
{
    X_ASSERT_RETURN(p_pttI2c != NULL && p_pttI2c->t_iHandle >= 0, XOS_I2C_NOT_OPEN);

    uint8_t buffer[2];
    buffer[0] = p_ucRegister;
    buffer[1] = p_ucValue;

    return i2cWrite(p_pttI2c, buffer, sizeof(buffer));
}

int i2cReadRegister(xos_i2c_t* p_pttI2c, uint8_t p_ucRegister, uint8_t* p_ptucValue)
{
    X_ASSERT_RETURN(p_pttI2c != NULL && p_pttI2c->t_iHandle >= 0, XOS_I2C_NOT_OPEN);
    X_ASSERT_RETURN(p_ptucValue != NULL, XOS_I2C_INVALID);

    int ret = i2cWrite(p_pttI2c, &p_ucRegister, 1);
    if (ret < 0) return ret;

    return i2cRead(p_pttI2c, p_ptucValue, 1);
}

int i2cSetSpeed(xos_i2c_t* p_pttI2c, uint32_t p_ulSpeed)
{
    X_ASSERT_RETURN(p_pttI2c != NULL && p_pttI2c->t_iHandle >= 0, XOS_I2C_NOT_OPEN);

    p_pttI2c->t_ulSpeed = p_ulSpeed;
    return XOS_I2C_OK;
}

int i2cClose(xos_i2c_t* p_pttI2c)
{
    X_ASSERT_RETURN(p_pttI2c != NULL, XOS_I2C_INVALID);

    if (p_pttI2c->t_iHandle >= 0)
    {
        close(p_pttI2c->t_iHandle);
        p_pttI2c->t_iHandle = -1;
    }

    return XOS_I2C_OK;
}

#endif // _RASPBERRY
