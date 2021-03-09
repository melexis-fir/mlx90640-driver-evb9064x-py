#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>

#include "evb9064x.h"
#include "MLX90640_I2C_Driver_evb9064x.h"


static struct Evb9064x_t *g_handle;


struct MLX90640DriverRegister_t *
MLX90640_get_register_evb9064x(void)
{
  static struct MLX90640DriverRegister_t reg;

  strcpy(reg.name_, "mlx://evb:9064x/");
  reg.MLX90640_get_i2c_handle_  = MLX90640_get_i2c_handle_evb9064x;
  reg.MLX90640_set_i2c_handle_  = MLX90640_set_i2c_handle_evb9064x;
  reg.MLX90640_I2CInit_         = MLX90640_I2CInit_evb9064x;
  reg.MLX90640_I2CClose_        = MLX90640_I2CClose_evb9064x;
  reg.MLX90640_I2CRead_         = MLX90640_I2CRead_evb9064x;
  reg.MLX90640_I2CFreqSet_      = MLX90640_I2CFreqSet_evb9064x;
  reg.MLX90640_I2CGeneralReset_ = MLX90640_I2CGeneralReset_evb9064x;
  reg.MLX90640_I2CWrite_        = MLX90640_I2CWrite_evb9064x;
  return &reg;
}


void *
MLX90640_get_i2c_handle_evb9064x(void)
{
  return (void *)g_handle;
}


void
MLX90640_set_i2c_handle_evb9064x(void *handle)
{
  g_handle = (struct Evb9064x_t *)handle;
}


void 
MLX90640_I2CInit_evb9064x(const char *port) 
{
  const char *start = "mlx://evb:9064x/";
  if (strncmp(port, start, strlen(start)) != 0)
  {
    printf("ERROR: '%s' is not a valid port\n", port);
    return;
  }
  const char *port_id = &port[strlen(start)];
  if (strstr(port_id, "dev/tty") == port_id)
  {
    port_id--; // move the pointer one up, to include the '/' character.
  }

  printf ("using comport: '%s'\n", port_id);

  g_handle = evb9064x_open(port_id);
  if (g_handle == NULL) 
  {
    return;
  }

  // char buffer[256];
  // evb9064x_get_hardware_id(g_handle, buffer, sizeof(buffer));
  // printf ("evb9064x_get_hardware_id: '%s'\n", (char *)buffer);
  
  evb9064x_set_vdd(g_handle, 3.3);
  usleep(100000);
  evb9064x_i2c_init(g_handle);
  evb9064x_begin_conversion(g_handle, 0x33);
}


void MLX90640_I2CClose_evb9064x(void)
{
  evb9064x_close(g_handle);
  g_handle = NULL;
}


int MLX90640_I2CRead_evb9064x(uint8_t slaveAddr, uint16_t startAddr, uint16_t nMemAddressRead, uint16_t *data)
{
  return evb9064x_i2c_read(g_handle, slaveAddr, startAddr, data, nMemAddressRead);
}


void MLX90640_I2CFreqSet_evb9064x(int freq)
{
  if (freq){} 
}


int MLX90640_I2CGeneralReset_evb9064x(void)
{  
  return evb9064x_i2c_sent_general_reset(g_handle);
}


int MLX90640_I2CWrite_evb9064x(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{
  int delay_ms = 0;
  int16_t mem_section = writeAddress & 0xFF00;
  if ((mem_section == 0x2400) || 
      (mem_section == 0x2500) || 
      (mem_section == 0x2600) || 
      (mem_section == 0x2700))
  {
    delay_ms = 10; // 10ms for EEPROM write!
  }

  int r = evb9064x_i2c_write(g_handle, slaveAddr, writeAddress, &data, 1);

  if (delay_ms > 0)
  {
    usleep(delay_ms * 1000);
  }
  return r;
}
