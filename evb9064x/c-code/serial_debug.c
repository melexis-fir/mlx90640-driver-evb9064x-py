#define _DEFAULT_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <unistd.h>

#include "m_sp.h"

// debugging terminal:
//
// sudo apt-get install gtkterm
// gtkterm -b 8 -t 1 -s 115200 -p /dev/ttyACM0

int main(void)
{
  struct MSpHandle *sp = m_spOpen("COM6", M_SP_115200, M_SP_ONE, M_SP_NONE, M_SP_OFF);
  //struct MSpHandle *sp = m_spOpen("/dev/ttyACM0", M_SP_115200, M_SP_ONE, M_SP_NONE, M_SP_OFF);
  char buffer[256];
  memset(buffer, 0, sizeof(buffer));
  buffer[0] = 0x01;
  buffer[1] = 0x01;
  buffer[2] = 0xFE;
  int r = m_spSend(sp, buffer, 3);
  printf ("send: %d\n", r);
  r = m_spReceive(sp, buffer, 256);
  printf ("receive: %d\n", r);

  printf ("msg: '%s'\n", buffer);
  for (int i = 0; i < r; ++i)
  {
    printf ("%2d %02X '%c'\n", i, buffer[i], buffer[i]);
  }
  m_spClose(sp);
  return 0;
}
