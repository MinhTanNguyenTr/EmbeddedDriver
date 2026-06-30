#ifndef _DHT11_H_
#define _DHT11_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    DHT11_OK = 0,
    DHT11_INVALID_PARAM,
    DHT11_TIMEOUT,
    DHT11_RESPONSE_TIMEOUT,
    DHT11_DATA_TIMEOUT,
    DHT11_CHECKSUM_ERROR
} DHT11_Status;

/*
 * Portable DHT11 driver configuration structure.
 * - `port` is framework-specific (e.g. GPIOA pointer) and kept as void* for portability.
 * - `pin` is the pin mask or number (use framework convention).
 * - `status` can be used by callers to store driver state.
 * - Function pointers allow the driver to be used with different HALs/RTOSes.
 */
typedef struct {
    void *port;                         /* framework-specific GPIO port (e.g. GPIOA) */
    uint16_t pin;                       /* pin mask or number */
    uint8_t status;                     /* user/driver status flag */

    /* timing function: delay in microseconds */
    void (*delay_us)(uint32_t us);

    /* configure pin as output (pulling bus low/high) */
    void (*set_pin_output)(void *port, uint16_t pin, void *user_ctx);

    /* configure pin as input (for reading data) */
    void (*set_pin_input)(void *port, uint16_t pin, void *user_ctx);

    /* read pin level: return 0 or 1 */
    uint8_t (*read_pin)(void *port, uint16_t pin, void *user_ctx);

    /* optional opaque pointer passed to callbacks */
    void *user_ctx;
} DHT11_Config_t;

/* Simple C API using the portable config struct */
/* Initialize driver state if needed (may be a no-op) */
void DHT11_Init(DHT11_Config_t *cfg);

/* Read temperature and humidity. Returns DHT11_OK on success, other values on error. */
DHT11_Status DHT11_Read(DHT11_Config_t *cfg, uint8_t *temperature, uint8_t *humidity);

#endif /* _DHT11_H_ */