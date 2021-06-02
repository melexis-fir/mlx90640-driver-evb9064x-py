#define _DEFAULT_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <unistd.h>

#include "rs232.h"

// debugging terminal:
//
// sudo apt-get install gtkterm
// gtkterm -b 8 -t 1 -s 115200 -p /dev/ttyACM0

int main(void)
{

  int port = RS232_GetPortnr("/dev/ttyACM0");

  char mode[]={'8','N','1',0};
  if(RS232_OpenComport(port, 115200, mode, 0))
  {
    printf("Can not open comport\n");
    return(0);
  }

  RS232_flushRXTX(port);

  unsigned char buffer[256];
  memset(buffer, 0, sizeof(buffer));
  buffer[0] = 0x01;
  buffer[1] = 0x01;
  buffer[2] = 0xFE;
  buffer[3] = '\0';

  int r = RS232_SendBuf(port, buffer, 4);
  printf ("send: %d\n", r);
  // usleep(100000);
  memset(buffer, 0, sizeof(buffer));
  r = RS232_PollComport(port, buffer, 256);
  printf ("receive: %d\n", r);

  printf ("msg: '%s'\n", buffer);
  for (int i = 0; i < r; ++i)
  {
    printf ("%2d %02X '%c'\n", i, buffer[i], buffer[i]);
  }


  // usleep(100000); // wait between commands!


  memset(buffer, 0, sizeof(buffer));
  buffer[0] = 0x01;
  buffer[1] = 0x01;
  buffer[2] = 0xFE;
  buffer[3] = '\0';

  r = RS232_SendBuf(port, buffer, 4);
  printf ("send: %d\n", r);
  memset(buffer, 0, sizeof(buffer));
  r = RS232_PollComport(port, buffer, 256);
  printf ("receive: %d\n", r);

  printf ("msg: '%s'\n", buffer);
  for (int i = 0; i < r; ++i)
  {
    printf ("%2d %02X '%c'\n", i, buffer[i], buffer[i]);
  }

  RS232_CloseComport(port);
  return 0;
}
