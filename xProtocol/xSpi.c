////////////////////////////////////////////////////////////
//  spi source file
//  implements the spi functions for Raspberry Pi
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xSpi.h"
#include "xAssert.h"


int spiInit(xos_spi_t* p_pttSpi)
{
	X_ASSERT_RETURN(p_pttSpi != NULL, SPI_INVALID);

    p_pttSpi->t_iHandle = -1;
    strcpy(p_pttSpi->t_cDevice, SPI_DEFAULT_DEVICE);
    p_pttSpi->t_ucMode = SPI_DEFAULT_MODE;
    p_pttSpi->t_ucBits = SPI_DEFAULT_BITS;
    p_pttSpi->t_ulSpeed = SPI_DEFAULT_SPEED;
    p_pttSpi->t_usDelay = SPI_DEFAULT_DELAY;
    memset(&p_pttSpi->t_tTransfer, 0, sizeof(struct spi_ioc_transfer));

    return SPI_OK;
}

int spiOpen(xos_spi_t* p_pttSpi, const char* p_ptkcDevice)
{
	X_ASSERT_RETURN(p_pttSpi != NULL, SPI_INVALID);
	X_ASSERT_RETURN(p_pttSpi->t_iHandle < 0, SPI_ALREADY_OPEN);

    p_pttSpi->t_iHandle = open(p_ptkcDevice, O_RDWR);
	X_ASSERT_RETURN(p_pttSpi->t_iHandle >= 0, SPI_ERROR);

    strcpy(p_pttSpi->t_cDevice, p_ptkcDevice);
    return SPI_OK;
}

int spiConfigure(xos_spi_t* p_pttSpi, uint8_t p_ucMode, uint32_t p_ulSpeed, uint8_t p_ucBits, uint16_t p_usDelay)
{
    X_ASSERT_RETURN(p_pttSpi != NULL && p_pttSpi->t_iHandle > 0, SPI_NOT_OPEN);

    // Set SPI mode
    if (ioctl(p_pttSpi->t_iHandle, SPI_IOC_WR_MODE, &p_ucMode) < 0)
    {
        return SPI_ERROR;
    }

    // Set bits per word
    if (ioctl(p_pttSpi->t_iHandle, SPI_IOC_WR_BITS_PER_WORD, &p_ucBits) < 0)
    {
        return SPI_ERROR;
    }

    // Set max speed
    if (ioctl(p_pttSpi->t_iHandle, SPI_IOC_WR_MAX_SPEED_HZ, &p_ulSpeed) < 0)
    {
        return SPI_ERROR;
    }

    p_pttSpi->t_ucMode = p_ucMode;
    p_pttSpi->t_ucBits = p_ucBits;
    p_pttSpi->t_ulSpeed = p_ulSpeed;
    p_pttSpi->t_usDelay = p_usDelay;

    return SPI_OK;
}

int spiTransfer(xos_spi_t* p_pttSpi, const uint8_t* p_ptucTxBuffer, uint8_t* p_ptucRxBuffer, unsigned long p_ulSize)
{
    X_ASSERT_RETURN(p_pttSpi != NULL && p_pttSpi->t_iHandle > 0, SPI_NOT_OPEN);

    if (p_ptucTxBuffer == NULL || p_ptucRxBuffer == NULL || p_ulSize == 0)
    {
        return SPI_INVALID;
    }

    struct spi_ioc_transfer tr = 
    {
        .tx_buf = (unsigned long)p_ptucTxBuffer,
        .rx_buf = (unsigned long)p_ptucRxBuffer,
        .len = p_ulSize,
        .delay_usecs = p_pttSpi->t_usDelay,
        .speed_hz = p_pttSpi->t_ulSpeed,
        .bits_per_word = p_pttSpi->t_ucBits,
    };

    int ret = ioctl(p_pttSpi->t_iHandle, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0)
    {
        return SPI_ERROR;
    }

    return ret;
}

int spiWrite(xos_spi_t* p_pttSpi, const uint8_t* p_ptucBuffer, unsigned long p_ulSize)
{
    uint8_t* rxBuffer = (uint8_t*)malloc(p_ulSize);
    if (rxBuffer == NULL)
    {
        return SPI_ERROR;
    }

    int ret = spiTransfer(p_pttSpi, p_ptucBuffer, rxBuffer, p_ulSize);
    free(rxBuffer);
    return ret;
}

int spiRead(xos_spi_t* p_pttSpi, uint8_t* p_ptucBuffer, unsigned long p_ulSize)
{
    uint8_t* txBuffer = (uint8_t*)calloc(p_ulSize, sizeof(uint8_t));
    if (txBuffer == NULL)
    {
        return SPI_ERROR;
    }

    int ret = spiTransfer(p_pttSpi, txBuffer, p_ptucBuffer, p_ulSize);
    free(txBuffer);
    return ret;
}

int spiSetCS(xos_spi_t* p_pttSpi, int p_iState)
{
    if (p_pttSpi == NULL || p_pttSpi->t_iHandle < 0)
    {
        return SPI_NOT_OPEN;
    }

    // Implementation depends on hardware setup
    // This is a placeholder for GPIO-based CS control
    return SPI_OK;
}

int spiClose(xos_spi_t* p_pttSpi)
{
    if (p_pttSpi == NULL)
    {
        return SPI_INVALID;
    }

    if (p_pttSpi->t_iHandle < 0)
    {
        return SPI_NOT_OPEN;
    }

    close(p_pttSpi->t_iHandle);
    p_pttSpi->t_iHandle = -1;
    return SPI_OK;
}


