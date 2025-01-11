////////////////////////////////////////////////////////////
//  gpio source file
//  implements the gpio functions for Raspberry Pi
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xGpio.h"
#include "xAssert.h"


// Define the GPIO chip based on Raspberry Pi version
#if defined(RPI_5)
#define GPIO_CHIP_NAME "/dev/gpiochip4"
#elif defined(RPI_4)
#define GPIO_CHIP_NAME "/dev/gpiochip0"
#else
#error "Unsupported Raspberry Pi version. Define RPI_4 or RPI_5"
#endif

int gpioInit(xos_gpio_t* p_pttGpio)
{
    X_ASSERT_RETURN(p_pttGpio != NULL, GPIO_INVALID);

    p_pttGpio->t_ptChip = NULL;
    p_pttGpio->t_ptLine = NULL;
    p_pttGpio->t_ulPin = 0;
    p_pttGpio->t_ucDirection = GPIO_INPUT;
    p_pttGpio->t_ucValue = GPIO_LOW;
    p_pttGpio->t_ucPull = GPIO_PULL_OFF;
    strcpy(p_pttGpio->t_cChipName, GPIO_CHIP_NAME);

    return GPIO_OK;
}

int gpioOpen(xos_gpio_t* p_pttGpio, uint32_t p_ulPin, uint8_t p_ucDirection)
{
    X_ASSERT_RETURN(p_pttGpio != NULL, GPIO_INVALID);
    X_ASSERT_RETURN(p_pttGpio->t_ptChip == NULL, GPIO_ALREADY_OPEN);

    // Open GPIO chip
    p_pttGpio->t_ptChip = gpiod_chip_open(p_pttGpio->t_cChipName);
    X_ASSERT_RETURN(p_pttGpio->t_ptChip != NULL, GPIO_ERROR);

    // Get GPIO line
    p_pttGpio->t_ptLine = gpiod_chip_get_line(p_pttGpio->t_ptChip, p_ulPin);
    if (p_pttGpio->t_ptLine == NULL)
    {
        gpiod_chip_close(p_pttGpio->t_ptChip);
        p_pttGpio->t_ptChip = NULL;
        return GPIO_ERROR;
    }

    // Configure GPIO direction
    int ret;
    if (p_ucDirection == GPIO_INPUT)
    {
        ret = gpiod_line_request_input(p_pttGpio->t_ptLine, "dynamicBric_PATO");
    }
    else
    {
        ret = gpiod_line_request_output(p_pttGpio->t_ptLine, "dynamicBric_PATO", 0);
    }

    if (ret < 0)
    {
        gpiod_chip_close(p_pttGpio->t_ptChip);
        p_pttGpio->t_ptChip = NULL;
        p_pttGpio->t_ptLine = NULL;
        return GPIO_ERROR;
    }

    p_pttGpio->t_ulPin = p_ulPin;
    p_pttGpio->t_ucDirection = p_ucDirection;

    return GPIO_OK;
}

int gpioSetDirection(xos_gpio_t* p_pttGpio, uint8_t p_ucDirection)
{
    X_ASSERT_RETURN(p_pttGpio != NULL && p_pttGpio->t_ptLine != NULL, GPIO_NOT_OPEN);

    // Release current configuration
    gpiod_line_release(p_pttGpio->t_ptLine);

    // Configure new direction
    int ret;
    if (p_ucDirection == GPIO_INPUT)
    {
        ret = gpiod_line_request_input(p_pttGpio->t_ptLine, "dynamicBric_PATO");
    }
    else
    {
        ret = gpiod_line_request_output(p_pttGpio->t_ptLine, "dynamicBric_PATO", 0);
    }

    if (ret < 0)
    {
        return GPIO_ERROR;
    }

    p_pttGpio->t_ucDirection = p_ucDirection;
    return GPIO_OK;
}

int gpioSetValue(xos_gpio_t* p_pttGpio, uint8_t p_ucValue)
{
    X_ASSERT_RETURN(p_pttGpio != NULL && p_pttGpio->t_ptLine != NULL, GPIO_NOT_OPEN);
    X_ASSERT_RETURN(p_pttGpio->t_ucDirection == GPIO_OUTPUT, GPIO_INVALID);

    int ret = gpiod_line_set_value(p_pttGpio->t_ptLine, p_ucValue);
    if (ret < 0)
    {
        return GPIO_ERROR;
    }

    p_pttGpio->t_ucValue = p_ucValue;
    return GPIO_OK;
}

int gpioGetValue(xos_gpio_t* p_pttGpio, uint8_t* p_ptucValue)
{
    X_ASSERT_RETURN(p_pttGpio != NULL && p_pttGpio->t_ptLine != NULL, GPIO_NOT_OPEN);
    X_ASSERT_RETURN(p_ptucValue != NULL, GPIO_INVALID);

    int value = gpiod_line_get_value(p_pttGpio->t_ptLine);
    if (value < 0)
    {
        return GPIO_ERROR;
    }

    *p_ptucValue = (uint8_t)value;
    p_pttGpio->t_ucValue = *p_ptucValue;
    return GPIO_OK;
}

int gpioSetPull(xos_gpio_t* p_pttGpio, uint8_t p_ucPull)
{
    X_ASSERT_RETURN(p_pttGpio != NULL && p_pttGpio->t_ptLine != NULL, GPIO_NOT_OPEN);
    X_ASSERT_RETURN(p_pttGpio->t_ucDirection == GPIO_INPUT, GPIO_INVALID);

    // Release current configuration
    gpiod_line_release(p_pttGpio->t_ptLine);

    // Configure new pull-up/down
    struct gpiod_line_request_config config = {
        .consumer = "dynamicBric_PATO",
        .request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT,
        .flags = 0,
    };

    if (p_ucPull == GPIO_PULL_UP)
    {
        config.flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
    }
    else if (p_ucPull == GPIO_PULL_DOWN)
    {
        config.flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
    }

    int ret = gpiod_line_request(p_pttGpio->t_ptLine, &config, 0);
    if (ret < 0)
    {
        return GPIO_ERROR;
    }

    p_pttGpio->t_ucPull = p_ucPull;
    return GPIO_OK;
}

int gpioClose(xos_gpio_t* p_pttGpio)
{
    X_ASSERT_RETURN(p_pttGpio != NULL, GPIO_INVALID);

    if (p_pttGpio->t_ptLine != NULL)
    {
        gpiod_line_release(p_pttGpio->t_ptLine);
        p_pttGpio->t_ptLine = NULL;
    }

    if (p_pttGpio->t_ptChip != NULL)
    {
        gpiod_chip_close(p_pttGpio->t_ptChip);
        p_pttGpio->t_ptChip = NULL;
    }

    return GPIO_OK;
}

