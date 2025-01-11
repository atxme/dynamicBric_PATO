////////////////////////////////////////////////////////////
//  pwm source file
//  implements the pwm functions for Raspberry Pi
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xPwm.h"
#include "xAssert.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#ifdef _RASPBERRY

#define PWM_PATH "/sys/class/pwm/pwmchip0"

int pwmInit(xos_pwm_t* p_pttPwm)
{
    X_ASSERT_RETURN(p_pttPwm != NULL, XOS_PWM_INVALID);

    p_pttPwm->t_iHandle = -1;
    p_pttPwm->t_ucChannel = 0;
    p_pttPwm->t_ulFrequency = XOS_PWM_DEFAULT_FREQ;
    p_pttPwm->t_ulDutyCycle = XOS_PWM_DEFAULT_DUTY;
    p_pttPwm->t_ulPeriod = XOS_PWM_DEFAULT_PERIOD;
    p_pttPwm->t_iEnabled = 0;

    return XOS_PWM_OK;
}

int pwmOpen(xos_pwm_t* p_pttPwm, uint8_t p_ucChannel)
{
    X_ASSERT_RETURN(p_pttPwm != NULL, XOS_PWM_INVALID);
    X_ASSERT_RETURN(p_ucChannel <= XOS_PWM_CHANNEL_1, XOS_PWM_INVALID);

    char buffer[128];
    FILE* fp;

    // Export PWM channel
    fp = fopen("/sys/class/pwm/pwmchip0/export", "w");
    if (fp == NULL) return XOS_PWM_ERROR;
    fprintf(fp, "%d", p_ucChannel);
    fclose(fp);

    // Set period
    snprintf(buffer, sizeof(buffer), "%s/pwm%d/period", PWM_PATH, p_ucChannel);
    fp = fopen(buffer, "w");
    if (fp == NULL) return XOS_PWM_ERROR;
    fprintf(fp, "%d", p_pttPwm->t_ulPeriod);
    fclose(fp);

    p_pttPwm->t_ucChannel = p_ucChannel;
    p_pttPwm->t_iHandle = 1;

    return XOS_PWM_OK;
}

int pwmSetFrequency(xos_pwm_t* p_pttPwm, uint32_t p_ulFrequency)
{
    X_ASSERT_RETURN(p_pttPwm != NULL && p_pttPwm->t_iHandle > 0, XOS_PWM_NOT_OPEN);

    // Convert frequency to period (nanoseconds)
    p_pttPwm->t_ulPeriod = 1000000000 / p_ulFrequency;
    p_pttPwm->t_ulFrequency = p_ulFrequency;

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s/pwm%d/period", PWM_PATH, p_pttPwm->t_ucChannel);

    FILE* fp = fopen(buffer, "w");
    if (fp == NULL) return XOS_PWM_ERROR;
    fprintf(fp, "%d", p_pttPwm->t_ulPeriod);
    fclose(fp);

    return XOS_PWM_OK;
}

int pwmSetDutyCycle(xos_pwm_t* p_pttPwm, uint32_t p_ulDutyCycle)
{
    X_ASSERT_RETURN(p_pttPwm != NULL && p_pttPwm->t_iHandle > 0, XOS_PWM_NOT_OPEN);
    X_ASSERT_RETURN(p_ulDutyCycle <= 100, XOS_PWM_INVALID);

    // Convert duty cycle to active time
    uint32_t duty_ns = (p_pttPwm->t_ulPeriod * p_ulDutyCycle) / 100;

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s/pwm%d/duty_cycle", PWM_PATH, p_pttPwm->t_ucChannel);

    FILE* fp = fopen(buffer, "w");
    if (fp == NULL) return XOS_PWM_ERROR;
    fprintf(fp, "%d", duty_ns);
    fclose(fp);

    p_pttPwm->t_ulDutyCycle = p_ulDutyCycle;
    return XOS_PWM_OK;
}

int pwmEnable(xos_pwm_t* p_pttPwm)
{
    X_ASSERT_RETURN(p_pttPwm != NULL && p_pttPwm->t_iHandle > 0, XOS_PWM_NOT_OPEN);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s/pwm%d/enable", PWM_PATH, p_pttPwm->t_ucChannel);

    FILE* fp = fopen(buffer, "w");
    if (fp == NULL) return XOS_PWM_ERROR;
    fprintf(fp, "1");
    fclose(fp);

    p_pttPwm->t_iEnabled = 1;
    return XOS_PWM_OK;
}

int pwmDisable(xos_pwm_t* p_pttPwm)
{
    X_ASSERT_RETURN(p_pttPwm != NULL && p_pttPwm->t_iHandle > 0, XOS_PWM_NOT_OPEN);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s/pwm%d/enable", PWM_PATH, p_pttPwm->t_ucChannel);

    FILE* fp = fopen(buffer, "w");
    if (fp == NULL) return XOS_PWM_ERROR;
    fprintf(fp, "0");
    fclose(fp);

    p_pttPwm->t_iEnabled = 0;
    return XOS_PWM_OK;
}

int pwmClose(xos_pwm_t* p_pttPwm)
{
    X_ASSERT_RETURN(p_pttPwm != NULL, XOS_PWM_INVALID);

    if (p_pttPwm->t_iHandle > 0)
    {
        // Unexport PWM channel
        FILE* fp = fopen("/sys/class/pwm/pwmchip0/unexport", "w");
        if (fp != NULL)
        {
            fprintf(fp, "%d", p_pttPwm->t_ucChannel);
            fclose(fp);
        }
        p_pttPwm->t_iHandle = -1;
    }

    return XOS_PWM_OK;
}

#endif // _RASPBERRY
