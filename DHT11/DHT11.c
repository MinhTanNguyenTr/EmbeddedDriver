#include "DHT11.h"

/* Return codes:
 *  0 = OK
 * -1 = invalid parameters
 * -2 = response timeout
 * -3 = data timeout
 * -4 = checksum mismatch
 */

static int wait_for_level(DHT11_Config_t *cfg, uint8_t level, uint32_t timeout_us) {
    while (timeout_us--) {
        if (cfg->read_pin(cfg->port, cfg->pin, cfg->user_ctx) == level) {
            return 0;
        }
        cfg->delay_us(1);
    }
    return -1;
}

void DHT11_Init(DHT11_Config_t *cfg) {
    if (cfg == NULL) {
        return;
    }

    if (cfg->delay_us == NULL || cfg->set_pin_output == NULL ||
        cfg->set_pin_input == NULL || cfg->read_pin == NULL) {
        cfg->status = 1;
        return;
    }

    cfg->status = 0;
    cfg->set_pin_input(cfg->port, cfg->pin, cfg->user_ctx);
}

int DHT11_Read(DHT11_Config_t *cfg, uint8_t *temperature, uint8_t *humidity) {
    if (cfg == NULL || temperature == NULL || humidity == NULL) {
        return -1;
    }

    if (cfg->delay_us == NULL || cfg->set_pin_output == NULL ||
        cfg->set_pin_input == NULL || cfg->read_pin == NULL) {
        return -1;
    }

    cfg->status = 0;

    /* Start signal: pull bus low for at least 18ms, then release it. */
    cfg->set_pin_output(cfg->port, cfg->pin, cfg->user_ctx);
    cfg->delay_us(20000);

    cfg->set_pin_input(cfg->port, cfg->pin, cfg->user_ctx);
    cfg->delay_us(30);

    /* DHT response: low 80us then high 80us. */
    if (wait_for_level(cfg, 0, 100) != 0) {
        return -2;
    }
    if (wait_for_level(cfg, 1, 100) != 0) {
        return -2;
    }
    if (wait_for_level(cfg, 0, 100) != 0) {
        return -2;
    }

    uint8_t data[5] = {0};

    for (uint8_t bit = 0; bit < 40; ++bit) {
        if (wait_for_level(cfg, 1, 100) != 0) {
            return -3;
        }

        uint32_t duration = 0;
        while (cfg->read_pin(cfg->port, cfg->pin, cfg->user_ctx) == 1) {
            if (++duration > 100) {
                return -3;
            }
            cfg->delay_us(1);
        }

        uint8_t byte_index = bit / 8;
        data[byte_index] <<= 1;
        if (duration > 40) {
            data[byte_index] |= 1;
        }
    }

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return -4;
    }

    *humidity = data[0];
    *temperature = data[2];
    return 0;
}
