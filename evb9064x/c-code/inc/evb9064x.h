#ifndef __EVB9064x_H__
#define __EVB9064x_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct Evb9064x_t;

struct Evb9064x_t *evb9064x_open(const char *serial_port);
void evb9064x_close  (struct Evb9064x_t *handle);
int  evb9064x_send   (struct Evb9064x_t *handle, uint8_t *data, uint16_t size);
int  evb9064x_receive(struct Evb9064x_t *handle, uint8_t *data, uint16_t max_size, uint16_t *size);

int  evb9064x_get_hardware_id(struct Evb9064x_t *handle, char *data, uint16_t max_size);
int  evb9064x_get_software_id(struct Evb9064x_t *handle, char *data, uint16_t max_size);

int  evb9064x_set_vdd(struct Evb9064x_t *handle, float vdd);

int  evb9064x_i2c_init        (struct Evb9064x_t *handle);
int  evb9064x_begin_conversion(struct Evb9064x_t *handle, uint8_t slave_address);
int  evb9064x_i2c_read        (struct Evb9064x_t *handle, uint8_t slave_address, uint16_t memory_address, uint16_t *data, uint16_t size);
int  evb9064x_i2c_write       (struct Evb9064x_t *handle, uint8_t slave_address, uint16_t memory_address, uint16_t *data, uint16_t size);
int  evb9064x_i2c_sent_general_reset(struct Evb9064x_t *handle);


#ifdef __cplusplus
}
#endif

#endif // __EVB9064x_H__
