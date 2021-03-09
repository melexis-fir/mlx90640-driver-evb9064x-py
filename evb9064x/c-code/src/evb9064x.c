#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>

#include "m_sp.h"
#include "evb9064x.h"

enum Evb9064x_CMD
{
  CMD_ResetHardware = 0,
  CMD_GetHardwareID = 1,
  CMD_GetSoftwareID = 3,
  CMD_ReadEEPROM = 10,
  CMD_Init_I2C_SW = 30,
  CMD_I2C_Master_SW = 31,
  CMD_SetVdd = 150,
  CMD_StopDAQ = 152,
  CMD_ReadDAQ = 153,
  CMD_MeasureVddIdd = 156,
  CMD_StartDAQ_90640 = 171,
  CMD_I2C_Master_90640 = 174,
  CMD_StartDAQ_90641 = 179,  
};

struct Evb9064x_t
{
  struct MSpHandle *sp_handle_;
};


struct Evb9064x_t *
evb9064x_open(const char *serial_port)
{
  struct Evb9064x_t *handle = (struct Evb9064x_t *)malloc(sizeof (struct Evb9064x_t));
  handle->sp_handle_ = m_spOpen(serial_port, M_SP_115200, M_SP_ONE, M_SP_NONE, M_SP_OFF);

  m_spSetTimeout(handle->sp_handle_, 1000000);
  return handle;
}


void
evb9064x_close(struct Evb9064x_t *handle)
{
  m_spClose(handle->sp_handle_);
  handle->sp_handle_ = NULL;
  free(handle);
  handle = NULL;
}


static uint8_t 
mlx_crc(uint8_t start, uint8_t *data, uint16_t size)
{
  uint16_t crc = start;
  for (uint16_t i = 0; i < size; i++)
  {
    crc += data[i];
    if (crc > 255)
    {
      crc -= 255;
    }
  }
  return 255-crc;
}


int
evb9064x_send(struct Evb9064x_t *handle, uint8_t *data, uint16_t size)
{
  if (size < 1)
  {
    return -1; // nothing to send!
  }
  if (size > 253)
  {
    return -2; // too long
  }

  uint8_t crc = mlx_crc(0, data, size);

  uint8_t buffer[256];
  memset(buffer, 0, sizeof(buffer));

  buffer[0] = (uint8_t)size;
  memcpy(&buffer[1], data, size);
  buffer[size+1] = crc;

  m_spFlushInput(handle->sp_handle_);
  m_spSend(handle->sp_handle_, buffer, size+2);
  return 0;
}




int
evb9064x_receive(struct Evb9064x_t *handle, uint8_t *data, uint16_t max_size, uint16_t *size)
{
  uint8_t buffer[16];
  uint16_t crc = 0;

  int r = m_spReceive(handle->sp_handle_, buffer, 1);
  if (r != 1) return -6;
  uint16_t n = buffer[0];
  if (n == 0)
  {
    return -1;
  }
  if (n == 255)
  { // Special case; n is 16 bits wide
    m_spReceive(handle->sp_handle_, buffer, 2);
    n = buffer[0];
    n *= 256;
    n += buffer[1];
    crc = 1;
    crc += buffer[0];
    if (crc > 255) crc -= 255;
    crc += buffer[1];
    if (crc > 255) crc -= 255;
  }

  if (n+1 > max_size)
  { // buffer given is too small!
    return -2;
  }

  r = m_spReceive(handle->sp_handle_, data, n);
  if (r != n) return -4;
  uint8_t crc_received = 0;
  r = m_spReceive(handle->sp_handle_, &crc_received, 1);
  if (r != 1) return -5;

  crc = mlx_crc(crc, data, n);
  *size = n;

  if (crc != crc_received)
  {
    return -3;
  }

  return 0;
}


int
evb9064x_get_hardware_id(struct Evb9064x_t *handle, char *data, uint16_t max_size)
{
  int r = 0;
  uint8_t buffer[256];
  uint16_t size = 0;
  memset(buffer, 0, sizeof(buffer));
  memset(data, 0, max_size);

  buffer[0] = CMD_GetHardwareID;
  r = evb9064x_send(handle, buffer, 1);
  if (r != 0)return r;
  r = evb9064x_receive(handle, buffer, sizeof(buffer), &size);
  if (r != 0)return r;

  if (size < 6)
  {
    return -1;
  }

  memcpy(data, &buffer[6], size-6);
  for (int i = 0; i < size-6; i++)
  {
    if (data[i] < ' ')
    {
      data[i] = '|';
    }
  }
  return 0;
}


int
evb9064x_get_software_id(struct Evb9064x_t *handle, char *data, uint16_t max_size)
{
  int r = 0;
  uint8_t buffer[256];
  uint16_t size = 0;
  memset(buffer, 0, sizeof(buffer));
  memset(data, 0, max_size);

  buffer[0] = CMD_GetSoftwareID;
  r = evb9064x_send(handle, buffer, 1);
  if (r != 0)return r;
  r = evb9064x_receive(handle, buffer, sizeof(buffer), &size);
  if (r != 0)return r;

  if (size < 6)
  {
    return -1;
  }

  memcpy(data, &buffer[6], size-6);
  for (int i = 0; i < size-6; i++)
  {
    if (data[i] < ' ')
    {
      data[i] = '|';
    }
  }
  return 0;
}


int 
evb9064x_set_vdd(struct Evb9064x_t *handle, float vdd)
{
  int r = 0;
  uint8_t buffer[16];
  uint16_t readed_size = 0;
  memset(buffer, 0, sizeof(buffer));

  uint8_t *p = (uint8_t *)&vdd;

  buffer[0] = CMD_SetVdd;
  buffer[1] = 0;
  buffer[2] = p[0];
  buffer[3] = p[1];
  buffer[4] = p[2];
  buffer[5] = p[3];
  r = evb9064x_send(handle, buffer, 6);
  if (r != 0) return r;
  memset(buffer, 0, sizeof(buffer));
  r = evb9064x_receive(handle, buffer, sizeof(buffer), &readed_size);
  if (r != 0) return r;
  return 0;
}


int
evb9064x_i2c_init(struct Evb9064x_t *handle)
{
  int r = 0;
  uint8_t buffer[256];
  uint16_t readed_size = 0;
  memset(buffer, 0, sizeof(buffer));
  // memset(data, 0, size);

  buffer[0] = CMD_Init_I2C_SW;
  buffer[1] = 2;
  buffer[2] = 0;
  buffer[3] = 0;
  buffer[4] = 0;
  buffer[5] = 6;
  buffer[6] = 0;
  buffer[7] = 0;
  buffer[8] = 0;
  buffer[9] = 8;
  buffer[10] = 0;
  buffer[11] = 0;
  buffer[12] = 0;
  buffer[13] = 5;
  buffer[14] = 0;
  buffer[15] = 0;
  buffer[16] = 0;
  r = evb9064x_send(handle, buffer, 17);
  if (r != 0) return r;
  r = evb9064x_receive(handle, buffer, sizeof(buffer), &readed_size);
  if (r != 0) return r;

  if (readed_size != 1)
  {
    return -1;
  }
  if (buffer[0] != 0x1E)
  {
    return -2;
  }
  return 0;
}


int
evb9064x_begin_conversion(struct Evb9064x_t *handle, uint8_t slave_address)
{
  int r = 0;
  uint8_t buffer[256];
  uint16_t readed_size = 0;
  memset(buffer, 0, sizeof(buffer));

  uint16_t memory_address = 0x8000;  

  buffer[0] = CMD_I2C_Master_90640;
  buffer[1] = slave_address;
  buffer[2] = (memory_address >> 8) & 0xFF;
  buffer[3] = memory_address & 0xFF;
  buffer[4] = 0x00;
  buffer[5] = 0x20;
  buffer[6] = 0x00;
  buffer[7] = 0x00;
  r = evb9064x_send(handle, buffer, 8);
  if (r != 0) return r;
  memset(buffer, 0, sizeof(buffer));
  r = evb9064x_receive(handle, buffer, sizeof(buffer), &readed_size);
  if (r != 0) return r;

  if (readed_size != 2)
  {
    return -1;
  }
  if (buffer[0] != 0xAE)
  {
    return -2;
  }

  if (buffer[1] != 0x00)
  {
    return -3;
  }

  buffer[0] = CMD_I2C_Master_90640;
  buffer[1] = slave_address;
  buffer[2] = 0x24;
  buffer[3] = 0x00;
  buffer[4] = 0x80;
  buffer[5] = 0x06;
  r = evb9064x_send(handle, buffer, 8);
  if (r != 0) return r;
  memset(buffer, 0, sizeof(buffer));
  r = evb9064x_receive(handle, buffer, sizeof(buffer), &readed_size);
  if (r != 0) return r;

  buffer[0] = CMD_I2C_Master_90640;
  buffer[1] = slave_address;
  buffer[2] = 0x80;
  buffer[3] = 0x00;
  buffer[4] = 0x22;
  buffer[5] = 0x00;

  r = evb9064x_send(handle, buffer, 8);
  if (r != 0) return r;
  memset(buffer, 0, sizeof(buffer));
  r = evb9064x_receive(handle, buffer, sizeof(buffer), &readed_size);
  if (r != 0) return r;
  return 0;
}


int 
evb9064x_i2c_read(struct Evb9064x_t *handle, uint8_t slave_address, uint16_t memory_address, uint16_t *data, uint16_t size)
{
  uint8_t buffer[2*1024];
  int r = 0;
  uint16_t readed_size = 0;
  memset(buffer, 0, sizeof(buffer));

  m_spSetTimeout(handle->sp_handle_, 1000000);

  buffer[0] = CMD_I2C_Master_90640;
  buffer[1] = slave_address;
  buffer[2] = (memory_address >> 8) & 0xFF;
  buffer[3] = memory_address & 0xFF;
  buffer[4] = (size*2) & 0xFF;
  buffer[5] = ((size*2) >> 8) & 0xFF;
  r = evb9064x_send(handle, buffer, 6);
  if (r != 0) return r;

  memset(buffer, 0, sizeof(buffer));
  r = evb9064x_receive(handle, buffer, sizeof(buffer), &readed_size);
  if (r != 0) return r;

  for (int i = 2; i < readed_size; i += 2)
  {
    data[(i-2)/2] = 256 * buffer[i] + buffer[i+1];
  }

  return 0;
}


int 
evb9064x_i2c_write(struct Evb9064x_t *handle, uint8_t slave_address, uint16_t memory_address, uint16_t *data, uint16_t size)
{
  uint8_t buffer[256];
  int r = 0;
  uint16_t readed_size = 0;
  memset(buffer, 0, sizeof(buffer));

  buffer[0] = CMD_I2C_Master_90640;
  buffer[1] = slave_address;
  buffer[2] = (memory_address >> 8) & 0xFF;
  buffer[3] = memory_address & 0xFF;
  int j=4;
  for (int i=0; i<size; i++)
  {
    buffer[j] = (data[i] >> 8) & 0xFF;
    j++;
    buffer[j] = data[i] & 0xFF;
    j++;
  }
  buffer[j] = 0x00;
  j++;
  buffer[j] = 0x00;
  j++;

  r = evb9064x_send(handle, buffer, j);
  if (r != 0) return r;

  memset(buffer, 0, sizeof(buffer));
  r = evb9064x_receive(handle, buffer, sizeof(buffer), &readed_size);
  if (r != 0) return r;

  return 0;
}


int
evb9064x_i2c_sent_general_reset(struct Evb9064x_t *handle)
{ // See I2C Spec => 3.1.14 Software reset
  // http://www.nxp.com/documents/user_manual/UM10204.pdf
  uint8_t buffer[16];
  int r = 0;
  uint16_t readed_size = 0;
  memset(buffer, 0, sizeof(buffer));

  buffer[0] = CMD_I2C_Master_90640;
  buffer[1] = 0x00; // slave_address;
  buffer[2] = 0x06; // data;
  buffer[3] = 0x00;
  buffer[4] = 0x00;

  r = evb9064x_send(handle, buffer, 5);
  if (r != 0) return r;

  memset(buffer, 0, sizeof(buffer));
  r = evb9064x_receive(handle, buffer, sizeof(buffer), &readed_size);
  if (r != 0) return r;

  return 0;
}
