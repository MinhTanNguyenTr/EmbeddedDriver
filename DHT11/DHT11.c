#include <stddef.h>
#include "DHT11.h"

/*----------------------------------------------------------------------------
 * Protocol Timing (us)
 *---------------------------------------------------------------------------*/
#define DHT11_START_SIGNAL_US         (20000U)
#define DHT11_HOST_RELEASE_US         (30U)

#define DHT11_RESPONSE_TIMEOUT_US     (100U)
#define DHT11_BIT_TIMEOUT_US          (100U)

#define DHT11_BIT_THRESHOLD_US        (40U)

#define DHT11_DATA_BYTES              (5U)
#define DHT11_TOTAL_BITS              (40U)

/*----------------------------------------------------------------------------
 * Private Functions
 *---------------------------------------------------------------------------*/

static bool DHT11_IsConfigValid(const DHT11_Config_t *cfg)
{
    return (cfg != NULL) &&
           (cfg->delay_us != NULL) &&
           (cfg->set_pin_output != NULL) &&
           (cfg->set_pin_input != NULL) &&
           (cfg->read_pin != NULL);
}

static DHT11_Status DHT11_WaitForLevel(DHT11_Config_t *cfg,
                                       uint8_t level,
                                       uint32_t timeout_us)
{
    while (timeout_us--)
    {
        if (cfg->read_pin(cfg->port, cfg->pin, cfg->user_ctx) == level)
        {
            return DHT11_OK;
        }

        cfg->delay_us(1);
    }

    return DHT11_TIMEOUT;
}

static DHT11_Status DHT11_Start(DHT11_Config_t *cfg)
{
    cfg->set_pin_output(cfg->port, cfg->pin, cfg->user_ctx);

    cfg->delay_us(DHT11_START_SIGNAL_US);

    cfg->set_pin_input(cfg->port, cfg->pin, cfg->user_ctx);

    cfg->delay_us(DHT11_HOST_RELEASE_US);

    return DHT11_OK;
}

static DHT11_Status DHT11_WaitResponse(DHT11_Config_t *cfg)
{
    if (DHT11_WaitForLevel(cfg, 0, DHT11_RESPONSE_TIMEOUT_US) != DHT11_OK)
    {
        return DHT11_RESPONSE_TIMEOUT;
    }

    if (DHT11_WaitForLevel(cfg, 1, DHT11_RESPONSE_TIMEOUT_US) != DHT11_OK)
    {
        return DHT11_RESPONSE_TIMEOUT;
    }

    if (DHT11_WaitForLevel(cfg, 0, DHT11_RESPONSE_TIMEOUT_US) != DHT11_OK)
    {
        return DHT11_RESPONSE_TIMEOUT;
    }

    return DHT11_OK;
}

/* Read one bit (MSB first) */
static DHT11_Status DHT11_ReadBit(DHT11_Config_t *cfg, uint8_t *bit)
{
    uint32_t high_time = 0;

    if (DHT11_WaitForLevel(cfg, 1, DHT11_BIT_TIMEOUT_US) != DHT11_OK)
    {
        return DHT11_DATA_TIMEOUT;
    }

    while (cfg->read_pin(cfg->port, cfg->pin, cfg->user_ctx))
    {
        if (++high_time > DHT11_BIT_TIMEOUT_US)
        {
            return DHT11_DATA_TIMEOUT;
        }

        cfg->delay_us(1);
    }

    *bit = (high_time > DHT11_BIT_THRESHOLD_US) ? 1U : 0U;

    return DHT11_OK;
}

static DHT11_Status DHT11_ReadByte(DHT11_Config_t *cfg, uint8_t *value)
{
    uint8_t bit;
    *value = 0;

    for (uint8_t i = 0; i < 8; i++)
    {
        DHT11_Status status = DHT11_ReadBit(cfg, &bit);

        if (status != DHT11_OK)
        {
            return status;
        }

        *value <<= 1;
        *value |= bit;
    }

    return DHT11_OK;
}

static DHT11_Status DHT11_ReadFrame(DHT11_Config_t *cfg,
                                    uint8_t data[DHT11_DATA_BYTES])
{
    for (uint8_t i = 0; i < DHT11_DATA_BYTES; i++)
    {
        DHT11_Status status = DHT11_ReadByte(cfg, &data[i]);

        if (status != DHT11_OK)
        {
            return status;
        }
    }

    return DHT11_OK;
}

static bool DHT11_CheckChecksum(const uint8_t data[DHT11_DATA_BYTES])
{
    uint8_t checksum;

    checksum = (uint8_t)(data[0] +
                         data[1] +
                         data[2] +
                         data[3]);

    return (checksum == data[4]);
}

/*----------------------------------------------------------------------------
 * Public Functions
 *---------------------------------------------------------------------------*/

void DHT11_Init(DHT11_Config_t *cfg)
{
    if (!DHT11_IsConfigValid(cfg))
    {
        return;
    }

    cfg->status = DHT11_OK;

    cfg->set_pin_input(cfg->port,
                       cfg->pin,
                       cfg->user_ctx);
}

DHT11_Status DHT11_Read(DHT11_Config_t *cfg,
                        uint8_t *temperature,
                        uint8_t *humidity)
{
    uint8_t data[DHT11_DATA_BYTES];
    DHT11_Status status;

    if (!DHT11_IsConfigValid(cfg) ||
        (temperature == NULL) ||
        (humidity == NULL))
    {
        return DHT11_INVALID_PARAM;
    }

    status = DHT11_Start(cfg);
    if (status != DHT11_OK)
    {
        return status;
    }

    status = DHT11_WaitResponse(cfg);
    if (status != DHT11_OK)
    {
        return status;
    }

    status = DHT11_ReadFrame(cfg, data);
    if (status != DHT11_OK)
    {
        return status;
    }

    if (!DHT11_CheckChecksum(data))
    {
        return DHT11_CHECKSUM_ERROR;
    }

    *humidity = data[0];
    *temperature = data[2];

    return DHT11_OK;
}