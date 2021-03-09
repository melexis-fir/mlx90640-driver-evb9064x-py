/* $Id: m_sp.cpp,v 1.8 2015/10/06 10:08:58 kva Exp $ */

#include "m_sp.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
  
#ifdef __MINGW32__
//#include "version.h"
#endif


const char *
m_spGetRevision (void)
{
#ifdef RELEASE_NUMBER
  return RELEASE_NUMBER;
#endif
  static char temp[64];
  strcpy (temp, "$Revision: 1.8 $");
  char *pTemp = strtok (temp, ":");
  pTemp = strtok (NULL, " $");
  return pTemp;
}

#ifdef __MINGW32__

/*  __  __ ___ _   _  ______        ___________                               */
/* |  \/  |_ _| \ | |/ ___\ \      / /___ /___ \                              */
/* | |\/| || ||  \| | |  _ \ \ /\ / /  |_ \ __) |                             */
/* | |  | || || |\  | |_| | \ V  V /  ___) / __/                              */
/* |_|  |_|___|_| \_|\____|  \_/\_/  |____/_____|                             */
/*                                                                            */


#include <windows.h>
#include <io.h>

struct MSpHandle
{
  char comportName_[32];
  HANDLE handle_;
  char sendLineEnding_[16];
  char receiveLineEnding_[16];
  char errorMessage_[256];
  char errorMessageBuffer_[256];
};

#ifdef DEBUG
//#include <wx/textctrl.h>
//extern wxTextCtrl *g_log;
//char msg[128];
#endif //DEBUG


static char g_errorMessage[256];
static char g_errorMessageBuffer[256];


static void
storeErrorMessage (struct MSpHandle *handle)
{
  LPVOID msgbuf;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		  FORMAT_MESSAGE_IGNORE_INSERTS |
		  FORMAT_MESSAGE_FROM_SYSTEM,
		  NULL,
		  GetLastError(),
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  (LPTSTR) & msgbuf, 0, NULL);

  if (handle == NULL)
  {
    strncpy(g_errorMessage, msgbuf, sizeof(g_errorMessage)-1);
    return;
  }
  strncpy(handle->errorMessage_, msgbuf, sizeof(handle->errorMessage_)-1);
}


static struct MSpHandle *
_spOpen (const char *comport)
{
  struct MSpHandle *handle = (struct MSpHandle *)malloc(sizeof(struct MSpHandle));
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return NULL;
  }
  strncpy(handle->comportName_, comport, sizeof(handle->comportName_)-1);

  char comportBuffer[32];
  strcpy (comportBuffer, comport);
  if (strstr (comport, "COM") == comport)
  {
    int portNo = atoi (comport + 3);
    if (portNo >= 10)
    {
      sprintf (comportBuffer, "\\\\.\\COM%d", portNo);
    }
  }

  int result = 1;
  if (!m_spSetSendLineEnding (handle, "\r")) result = 0;
  if (!m_spSetReceiveLineEnding (handle, "\r")) result = 0;
  if (!result)
    {
      snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: spOpen set default line endings");
      free(handle);
      return NULL;
    }

  handle->handle_ = CreateFile(comportBuffer, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

  if (handle->handle_ == INVALID_HANDLE_VALUE)
    {
      storeErrorMessage (NULL);
      CloseHandle (handle->handle_);
      handle->handle_ = NULL;
      free(handle);
      return NULL;
    }

  if (!SetupComm(handle->handle_, 2048, 2048) != 0)
    {
      storeErrorMessage (NULL);
      CloseHandle (handle->handle_);
      handle->handle_ = NULL;
      free(handle);
      return NULL;
    }

  DCB dcb;
  if (!GetCommState(handle->handle_, &dcb))
    {
      storeErrorMessage (NULL);
      CloseHandle (handle->handle_);
      handle->handle_ = NULL;
      free(handle);
      return NULL;
    }
  dcb.BaudRate=115200;
  dcb.ByteSize=8;
  dcb.StopBits=ONESTOPBIT;
  dcb.Parity=NOPARITY;
  if (!SetCommState(handle->handle_, &dcb))
    {
      storeErrorMessage (NULL);
      CloseHandle (handle->handle_);
      handle->handle_ = NULL;
      free(handle);
      return NULL;
    }

  COMMTIMEOUTS timeouts={0};
  timeouts.ReadIntervalTimeout=50;
  timeouts.ReadTotalTimeoutConstant=50;
  timeouts.ReadTotalTimeoutMultiplier=10;
  timeouts.WriteTotalTimeoutConstant=50;
  timeouts.WriteTotalTimeoutMultiplier=10;
  if(!SetCommTimeouts(handle->handle_, &timeouts)){
      storeErrorMessage (NULL);
      CloseHandle (handle->handle_);
      handle->handle_ = NULL;
      free(handle);
      return NULL;
  }

  return handle;
}


struct MSpHandle *
m_spOpen                (const char *comport, enum MBaudRate baudRate, enum MStopBits stopBits, enum MParity parity, enum MHandShake handshake)
{
  int result = 1;

  struct MSpHandle *handle = _spOpen (comport);
  if (handle == NULL) return NULL;

  if (!m_spSetBaudRate  (handle, baudRate))  result = 0;
  if (!m_spSetStopBits  (handle, stopBits))  result = 0;
  if (!m_spSetParity    (handle, parity))    result = 0;
  if (!m_spSetHandShake (handle, handshake)) result = 0;

  if (!result)
  {
    m_spClose (handle);
    return NULL;
  }
  return handle;
}


int
m_spClose               (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    free(handle);
    handle = NULL;
    return 0;
  }

  CloseHandle (handle->handle_);
  free(handle);
  handle = NULL;

  return 1;
}


int
m_spSetBaudRate         (struct MSpHandle *handle, enum MBaudRate baudRate)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if ((handle->handle_) == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  DWORD baudrate = CBR_9600;
  switch (baudRate)
  {
    case M_SP_110:     baudrate = CBR_110    ; break;
    case M_SP_300:     baudrate = CBR_300    ; break;
    case M_SP_600:     baudrate = CBR_600    ; break;
    case M_SP_1200:    baudrate = CBR_1200   ; break;
    case M_SP_2400:    baudrate = CBR_2400   ; break;
    case M_SP_4800:    baudrate = CBR_4800   ; break;
    case M_SP_9600:    baudrate = CBR_9600   ; break;
    case M_SP_14400:   baudrate = CBR_14400  ; break;
    case M_SP_19200:   baudrate = CBR_19200  ; break;
    case M_SP_38400:   baudrate = CBR_38400  ; break;
    case M_SP_56000:   baudrate = CBR_56000  ; break;
    case M_SP_57600:   baudrate = CBR_57600  ; break;
    case M_SP_115200:  baudrate = CBR_115200 ; break;
    case M_SP_128000:  baudrate = CBR_128000 ; break;
    case M_SP_230400:  baudrate = 230400 ; break;
    case M_SP_256000:  baudrate = CBR_256000 ; break;
    case M_SP_460800:  baudrate = 460800 ; break;
    case M_SP_921600:  baudrate = 921600 ; break;
    case M_SP_1000000:  baudrate = 1000000 ; break;

	default:
      printf ("sp: WARNING: not supported baudrate - 9600 used\n");
      break;
  }


  DCB dcb;
  if (!GetCommState(handle->handle_, &dcb))
  {
    storeErrorMessage (handle);
    return 0;
  }
  // See serial.h for an in-depth description of these params
  dcb.BaudRate = baudrate;

  if (!SetCommState(handle->handle_, &dcb))
  {
    storeErrorMessage (handle);
    return 0;
  }
  return 1;
}


int
m_spSetStopBits         (struct MSpHandle *handle, enum MStopBits stopBits)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  BYTE stopbits = ONESTOPBIT;
  switch (stopBits)
  {
    case M_SP_ONE:            stopbits = ONESTOPBIT; break;
    case M_SP_ONE_AND_A_HALF: stopbits = ONE5STOPBITS; break;
    case M_SP_TWO:            stopbits = TWOSTOPBITS; break;
  }


  DCB dcb;
  if (!GetCommState(handle->handle_, &dcb))
  {
    storeErrorMessage (handle);
    return 0;
  }
  // See serial.h for an in-depth description of these params
  dcb.StopBits = stopbits;

  if (!SetCommState(handle->handle_, &dcb))
  {
    storeErrorMessage (handle);
    return 0;
  }
  return 1;
}


int
m_spSetParity           (struct MSpHandle *handle, enum MParity parity)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  DCB dcb;
  if (!GetCommState(handle->handle_, &dcb))
  {
    storeErrorMessage (handle);
    return 0;
  }

  dcb.fParity = 0;
  dcb.Parity = NOPARITY;

  switch (parity)
  {
    case M_SP_NONE:   dcb.fParity = 0; dcb.Parity = NOPARITY;   break;
    case M_SP_EVEN:   dcb.fParity = 1;  dcb.Parity = EVENPARITY; break;
    case M_SP_ODD:    dcb.fParity = 1;  dcb.Parity = ODDPARITY;  break;
    case M_SP_IGNORE: printf ("PARITY IGNORE not supported\n"); break;
  }

  if (!SetCommState(handle->handle_, &dcb))
  {
    storeErrorMessage (handle);
    return 0;
  }
  return 1;

}


int
m_spSetHandShake        (struct MSpHandle *handle, enum MHandShake handShake)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  if (handShake ) {}

  return 1;
}


int
m_spWaitForData           (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }
/* TODO (#1#): implement wait for data (mingw32) */

#warning "TODO implement m_spWaitForData"
  return 0;
}


int
m_spFlushInput          (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  if (!PurgeComm (handle->handle_, PURGE_RXCLEAR))
  {
    storeErrorMessage (handle);
    return 0;
  }
  return 1;
}


int
m_spSend                (struct MSpHandle *handle, const void *buffer, const unsigned size)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  DWORD byteswritten = 0;

  if (!WriteFile(handle->handle_, buffer, size, &byteswritten, NULL))
  {
    storeErrorMessage (handle);
#ifdef DEBUG
	print_error(__LINE__);
#endif
    return 0;
  }
  return (int)byteswritten;
}


int
m_spReceive             (struct MSpHandle *handle, void *buffer, const unsigned size)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  DWORD bytesread = 0;

  if (!ReadFile(handle->handle_, buffer, size, &bytesread, NULL))
  {
    storeErrorMessage (handle->handle_);
#ifdef DEBUG
	print_error(__LINE__);
#endif
	return 0;
  }
  if (size < bytesread)
  {
    ((char *)buffer)[(int)bytesread] = '\0';
  }
  return (int)bytesread;
}


int
m_spSetTimeout          (struct MSpHandle *handle, long usec)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  int msec = usec / 1000;

  COMMTIMEOUTS ctout;
  if (!GetCommTimeouts(handle->handle_, &ctout))
  {
    storeErrorMessage (handle);
#ifdef DEBUG
        print_error(__LINE__);
#endif
     return 0;
  }
  ctout.ReadIntervalTimeout = msec;
  ctout.ReadTotalTimeoutMultiplier = 0;
  ctout.ReadTotalTimeoutConstant = msec;
  ctout.WriteTotalTimeoutMultiplier = 0;
  ctout.WriteTotalTimeoutConstant = 0;
  if (!SetCommTimeouts(handle->handle_, &ctout))
  {
    storeErrorMessage (handle);
#ifdef DEBUG
        print_error(__LINE__);
#endif
      return 0;
  }
  return 1;
}


int
m_spReadCts               (struct MSpHandle *handle, int *value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  DWORD modemStatus;
  if (!GetCommModemStatus(handle->handle_, &modemStatus))
  {
    storeErrorMessage (handle);
    return 0;
  }
  *value = (modemStatus & MS_CTS_ON) ? 1 : 0;
  return 1;
}


int
m_spReadDsr               (struct MSpHandle *handle, int *value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  DWORD modemStatus;
  if (!GetCommModemStatus(handle->handle_, &modemStatus))
  {
    storeErrorMessage (handle);
    return 0;
  }
  *value = (modemStatus & MS_DSR_ON) ? 1 : 0;
  return 1;
}


int
m_spReadRi                (struct MSpHandle *handle, int *value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  DWORD modemStatus;
  if (!GetCommModemStatus(handle->handle_, &modemStatus))
  {
    storeErrorMessage (handle);
    return 0;
  }
  *value = (modemStatus & MS_RING_ON) ? 1 : 0;
  return 1;
}


int
m_spReadDcd                (struct MSpHandle *handle, int *value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  DWORD modemStatus;
  if (!GetCommModemStatus(handle->handle_, &modemStatus))
  {
    storeErrorMessage (handle);
    return 0;
  }
  *value = (modemStatus & MS_RLSD_ON) ? 1 : 0;
  return 1;
}


int
m_spWriteDtr               (struct MSpHandle *handle, int value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  if (!EscapeCommFunction(handle->handle_, value ? CLRDTR : SETDTR))
  {
    storeErrorMessage (handle);
#ifdef DEBUG
    sprintf (msg, "sp: ERROR: escapecommf handle %d HANDLE %X(%d) %s\n", handle, handle->handle_, handle->handle_, m_spGetErrorMessage (0)); g_log->AppendText (msg);
#endif // DEBUG
    return 0;
  }
  return 1;
}


int
m_spWriteRts               (struct MSpHandle *handle, int value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  if (!EscapeCommFunction(handle->handle_, value ? CLRRTS : SETRTS))
  {
    return 0;
  }
  return 1;
}


int
m_spGetFileDescriptor   (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: handle outrange");
    return 0;
  }
  if (handle->handle_ == NULL)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: handle is not open");
    return 0;
  }

  int fd = _open_osfhandle ((intptr_t )(handle->handle_), 1);
  if (fd < 0)
  {
    storeErrorMessage (handle);
    return -1;
  }
  return fd;
}


const char *
m_spGetErrorMessage (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    strncpy(g_errorMessageBuffer, g_errorMessage, sizeof(g_errorMessageBuffer)-1);
    strcpy(g_errorMessage, "");
    if (!strcmp (g_errorMessageBuffer, ""))
    {
      return "handle is out of range";
    }

    return g_errorMessageBuffer;
  }
  strncpy(handle->errorMessageBuffer_, handle->errorMessage_, sizeof(handle->errorMessageBuffer_)-1);
  strcpy(handle->errorMessage_, "");
  return handle->errorMessageBuffer_;
}


#else // __MINGW32__

/*  _     ___ _   _ _   ___  __                                               */
/* | |   |_ _| \ | | | | \ \/ /                                               */
/* | |    | ||  \| | | | |\  /                                                */
/* | |___ | || |\  | |_| |/  \                                                */
/* |_____|___|_| \_|\___//_/\_\                                               */
/*                                                                            */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <string.h>
#include <errno.h>

#include <stdlib.h>

#ifndef strerr
#define strerr(a) strerror(a)
#endif

struct MSpHandle
{
  char comportName_[32];
  char sendLineEnding_[16];
  char receiveLineEnding_[16];
  char errorMessage_[256];
  char errorMessageBuffer_[256];

  int  fd_;               /* file descriptor/comms handle */
  long readTimeout_;     /* timeout for reading */
};


static char g_errorMessage[256];
static char g_errorMessageBuffer[256];


static void
storeErrorMessage (struct MSpHandle *handle)
{
  const char *msgbuf = strerr (errno);

  if (handle == NULL)
  {
    strncpy(g_errorMessage, msgbuf, sizeof(g_errorMessage));
    return;
  }
  strncpy(handle->errorMessage_, msgbuf, sizeof(handle->errorMessage_));
}


struct MSpHandle *
spOpen (const char *comport)
{
  struct MSpHandle *handle = (struct MSpHandle *)malloc(sizeof(struct MSpHandle));
  strncpy(handle->comportName_, comport, sizeof(handle->comportName_));

  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    free (handle);
    return NULL;
  }

  int result = 1;
  if (!m_spSetSendLineEnding (handle, "\r")) result = 0;
  if (!m_spSetReceiveLineEnding (handle, "\r")) result = 0;
  if (!result)
    {
      snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: spOpen set default line endings");
      free(handle);
      return NULL;
    }


  /* "blank" to start with */
  handle->fd_ = -1;          /* file descriptor/comms handle */
  handle->readTimeout_ = 10000;     /* timeout for reading */

  handle->fd_ = open (comport, O_RDWR | O_NDELAY | O_NOCTTY);

  if (handle->fd_ < 0)
  {
    storeErrorMessage (NULL);
    free(handle);
    return NULL;
  }


  int tmp = fcntl (handle->fd_, F_GETFL, 0);
  fcntl (handle->fd_, F_SETFL, tmp & ~O_NDELAY);

  struct termios ts;
  if (tcgetattr (handle->fd_, &ts) < 0)
  {
    storeErrorMessage (NULL);
    free(handle);
    return NULL;
  }


  /* set input modes so we get raw handling */
  ts.c_iflag &= ~(IGNPAR|PARMRK|INLCR|IGNCR|ICRNL|IXON);
  ts.c_iflag |= BRKINT;
  ts.c_oflag &= ~(OPOST|ONLCR|OCRNL|ONOCR|ONLRET);

  ts.c_lflag &= ~(ICANON|ECHO|ISTRIP|ISIG);
  ts.c_cc[VMIN] = 1;
  ts.c_cc[VTIME] = 1;
  ts.c_cflag &= ~(CSIZE|PARENB);
  ts.c_cflag |= CS8;

  if (tcsetattr (handle->fd_, TCSANOW, &ts) < 0)
  {
    storeErrorMessage (NULL);
    free(handle);
    return NULL;
  }

  return handle;
}


struct MSpHandle *
m_spOpen                (const char *comport, enum MBaudRate baudRate, enum MStopBits stopBits, enum MParity parity, enum MHandShake handshake)
{
  int result = 1;

  struct MSpHandle *handle = spOpen(comport);
  if (handle == NULL) return NULL;

  if (!m_spSetBaudRate  (handle, baudRate))  result = 0;
  if (!m_spSetStopBits  (handle, stopBits))  result = 0;
  if (!m_spSetParity    (handle, parity))    result = 0;
  if (!m_spSetHandShake (handle, handshake)) result = 0;

  if (!result)
  {
    m_spClose (handle);
    return NULL;
  }
  return handle;
}


int
m_spClose               (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  // printf ("m_spClose: fd: %d\n", handle->fd_);
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }

  close(handle->fd_);
  handle->fd_ = -1;
  free(handle);
  handle = NULL;

  return 1;
}


int
m_spSetBaudRate         (struct MSpHandle *handle, enum MBaudRate baudRate)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }

  int baudrate = B9600;
  switch (baudRate)
  {
    case M_SP_50    : baudrate = B50    ; break;
    case M_SP_75    : baudrate = B75    ; break;
    case M_SP_110   : baudrate = B110   ; break;
    case M_SP_134   : baudrate = B134   ; break;
    case M_SP_150   : baudrate = B150   ; break;
    case M_SP_200   : baudrate = B200   ; break;
    case M_SP_300   : baudrate = B300   ; break;
    case M_SP_600   : baudrate = B600   ; break;
    case M_SP_1200  : baudrate = B1200  ; break;
    case M_SP_1800  : baudrate = B1800  ; break;
    case M_SP_2400  : baudrate = B2400  ; break;
    case M_SP_4800  : baudrate = B4800  ; break;
    case M_SP_9600  : baudrate = B9600  ; break;
    case M_SP_19200 : baudrate = B19200 ; break;
    case M_SP_38400 : baudrate = B38400 ; break;
    case M_SP_57600 : baudrate = B57600 ; break;
    case M_SP_115200: baudrate = B115200; break;
#ifdef B14400
    case M_SP_14400 : baudrate = B14400; break;
#endif
#ifdef B56000
    case M_SP_56000 : baudrate = B56000; break;
#endif
#ifdef B128000
    case M_SP_128000: baudrate = B128000; break;
#endif
#ifdef B230400
    case M_SP_230400: baudrate = B230400; break;
#endif
#ifdef B256000
    case M_SP_256000: baudrate = B256000; break;
#endif
#ifdef B460800
    case M_SP_460800:   baudrate = B460800; break;
#endif
#ifdef B912600
    case M_SP_912600: baudrate = B912600; break;
#endif


    case M_SP_1000000: baudrate = 1000000; break;


	default:
      printf ("sp: ERROR: unsupported baudrate; 9600 used\n");
      break;

  }

  struct termios ts;
  if (tcgetattr (handle->fd_, &ts) < 0)
  {
    storeErrorMessage (handle);
    return 0;
  }

  cfsetospeed (&ts, baudrate);
  cfsetispeed (&ts, baudrate);

  if (tcsetattr (handle->fd_, TCSANOW, &ts) < 0)
  {
    storeErrorMessage (handle);
    return 0;
  }
  return 1;
}


int
m_spSetStopBits         (struct MSpHandle *handle, enum MStopBits stopBits)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }


  struct termios ts;
  if (tcgetattr (handle->fd_, &ts) < 0)
  {
    storeErrorMessage (handle);
    return 0;
  }

  switch (stopBits)
  {
    case M_SP_ONE:            ts.c_cflag &= ~CSTOPB; break;
    case M_SP_ONE_AND_A_HALF: ts.c_cflag &= ~CSTOPB; break;
    case M_SP_TWO:            ts.c_cflag |= CSTOPB; break;
  }


  if (tcsetattr (handle->fd_, TCSANOW, &ts) < 0)
  {
    storeErrorMessage (handle);
    return 0;
  }
  return 1;
}


int
m_spSetParity           (struct MSpHandle *handle, enum MParity parity)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }

  struct termios ts;
  if (tcgetattr (handle->fd_, &ts) < 0)
  {
    storeErrorMessage (handle);
    return 0;
  }

  ts.c_iflag &= ~(IGNPAR);
  ts.c_cflag &= (~PARODD);
  ts.c_cflag &= (~PARENB);

  switch (parity)
  {
    case M_SP_ODD:
      ts.c_cflag |= PARODD;
      ts.c_cflag |= PARENB;
      break;
    case M_SP_EVEN:
      ts.c_cflag &= (~PARODD);
      ts.c_cflag |= PARENB;
      break;
    case M_SP_NONE:
      break;
    case M_SP_IGNORE:
      ts.c_iflag |= IGNPAR;
      break;
  }


  if (tcsetattr (handle->fd_, TCSANOW, &ts) < 0)
  {
    storeErrorMessage (handle);
    return 0;
  }
  return 1;
}


int
m_spSetHandShake        (struct MSpHandle *handle, enum MHandShake handShake)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }

  if (handShake ) {}

  return 1;
}


int
m_spWaitForData           (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }


//  struct termios ts;
//  if (tcgetattr (handle->fd_, &ts) < 0)
//  {
//    storeErrorMessage (handle);
//    return 0;
//  }

//  printf ("c_iflag %08X\n", ts.c_iflag);
//  printf ("c_oflag %08X\n", ts.c_oflag);
//  printf ("c_cflag %08X\n", ts.c_cflag);
//  printf ("c_lflag %08X\n", ts.c_lflag);
//  for (int i=0; i<NCCS; i++)
//  {
//    printf ("ts.c_cc[%2d] = %d\n", i, ts.c_cc[i]);
//  }

//  printf ("iflag: IGNBRK   %d  ignore BREAK condition on input\n", ts.c_iflag & IGNBRK  ? 1 : 0);
//  printf ("iflag: BRKINT   %d  If IGNBRK is not set, generate SIGINT on BREAK condition, else read BREAK as character \\0.\n", ts.c_iflag & BRKINT  ? 1 : 0);
//  printf ("iflag: IGNPAR   %d  ignore framing errors and parity errors.\n", ts.c_iflag & IGNPAR  ? 1 : 0);
//  printf ("iflag: PARMRK   %d  if IGNPAR is not set, prefix a character with a parity error or framing error with \\377 \\0.   If  neither  IGNPAR  nor PARMRK is set, read a character with a parity error or framing error as '\\0'.\n", ts.c_iflag & PARMRK  ? 1 : 0);
//  printf ("iflag: INPCK    %d  enable input parity checking\n", ts.c_iflag & INPCK   ? 1 : 0);
//  printf ("iflag: ISTRIP   %d  strip off eighth bit\n", ts.c_iflag & ISTRIP  ? 1 : 0);
//  printf ("iflag: INLCR    %d  translate NL to CR on input\n", ts.c_iflag & INLCR   ? 1 : 0);
//  printf ("iflag: IGNCR    %d  ignore carriage return on input\n", ts.c_iflag & IGNCR   ? 1 : 0);
//  printf ("iflag: ICRNL    %d  translate carriage return to newline on input (unless IGNCR is set)\n", ts.c_iflag & ICRNL   ? 1 : 0);
//  printf ("iflag: IUCLC    %d  map uppercase characters to lowercase on input\n", ts.c_iflag & IUCLC   ? 1 : 0);
//  printf ("iflag: IXON     %d  enable XON/XOFF flow control on output\n", ts.c_iflag & IXON    ? 1 : 0);
//  printf ("iflag: IXANY    %d  enable any character to restart output\n", ts.c_iflag & IXANY   ? 1 : 0);
//  printf ("iflag: IXOFF    %d  enable XON/XOFF flow control on input\n", ts.c_iflag & IXOFF   ? 1 : 0);
//  printf ("iflag: IMAXBEL  %d  ring bell when input queue is full\n", ts.c_iflag & IMAXBEL ? 1 : 0);
//
//
//  printf ("oflag: OPOST    %d  enable implementation-defined output processing\n"                                                                                                                  , ts.c_oflag & OPOST  ? 1 : 0);
//  printf ("oflag: OLCUC    %d  map lowercase characters to uppercase on output\n"                                                                                                                  , ts.c_oflag & OLCUC  ? 1 : 0);
//  printf ("oflag: ONLCR    %d  map NL to CR-NL on output\n"                                                                                                                                        , ts.c_oflag & ONLCR  ? 1 : 0);
//  printf ("oflag: OCRNL    %d  map CR to NL on output\n"                                                                                                                                           , ts.c_oflag & OCRNL  ? 1 : 0);
//  printf ("oflag: ONOCR    %d  don't output CR at column 0\n"                                                                                                                                      , ts.c_oflag & ONOCR  ? 1 : 0);
//  printf ("oflag: ONLRET   %d  don't output CR\n"                                                                                                                                                  , ts.c_oflag & ONLRET ? 1 : 0);
//  printf ("oflag: OFILL    %d  send fill characters for a delay, rather than using a timed delay\n"                                                                                                , ts.c_oflag & OFILL  ? 1 : 0);
//  printf ("oflag: OFDEL    %d  fill character is ASCII DEL.  If unset, fill character is ASCII NUL\n"                                                                                              , ts.c_oflag & OFDEL  ? 1 : 0);
//  printf ("oflag: NLDLY    %d  newline delay mask.  Values are NL0 and NL1.\n"                                                                                                                     , ts.c_oflag & NLDLY  ? 1 : 0);
//  printf ("oflag: CRDLY    %d  carriage return delay mask.  Values are CR0, CR1, CR2, or CR3.\n"                                                                                                   , ts.c_oflag & CRDLY  ? 1 : 0);
//  printf ("oflag: TABDLY   %d  horizontal  tab  delay  mask.   Values  are TAB0, TAB1, TAB2, TAB3, or XTABS.  A value of XTABS expands tabs to spaces (with tab stops every eight columns).\n"     , ts.c_oflag & TABDLY ? 1 : 0);
//  printf ("oflag: BSDLY    %d  backspace delay mask.  Values are BS0 or BS1.\n"                                                                                                                    , ts.c_oflag & BSDLY  ? 1 : 0);
//  printf ("oflag: VTDLY    %d  vertical tab delay mask.  Values are VT0 or VT1.\n"                                                                                                                 , ts.c_oflag & VTDLY  ? 1 : 0);
//  printf ("oflag: FFDLY    %d  form feed delay mask.  Values are FF0 or FF1.\n"                                                                                                                    , ts.c_oflag & FFDLY  ? 1 : 0);
//
//
//  int sizeb = 0;
//  if (ts.c_cflag & CS5) sizeb = 5;
//  if (ts.c_cflag & CS6) sizeb = 6;
//  if (ts.c_cflag & CS7) sizeb = 7;
//  if (ts.c_cflag & CS8) sizeb = 8;
//
//  printf ("cflag: CSIZE    %d character size mask.  Values are CS5, CS6, CS7, or CS8.\n"                   , sizeb);
//  printf ("cflag: CSTOPB   %d set two stop bits, rather than one.\n"                                       , ts.c_cflag & CSTOPB  ? 1 : 0);
//  printf ("cflag: CREAD    %d enable receiver.\n"                                                          , ts.c_cflag & CREAD   ? 1 : 0);
//  printf ("cflag: PARENB   %d enable parity generation on output and parity checking for input.\n"         , ts.c_cflag & PARENB  ? 1 : 0);
//  printf ("cflag: PARODD   %d parity for input and output is odd.\n"                                       , ts.c_cflag & PARODD  ? 1 : 0);
//  printf ("cflag: HUPCL    %d lower modem control lines after last process closes the device (hang up).\n" , ts.c_cflag & HUPCL   ? 1 : 0);
//  printf ("cflag: CLOCAL   %d ignore modem control lines\n"                                                , ts.c_cflag & CLOCAL  ? 1 : 0);
//  printf ("cflag: CIBAUD   %d mask for input speeds (not used).\n"                                         , ts.c_cflag & CIBAUD  ? 1 : 0);
//  printf ("cflag: CRTSCTS  %d flow control.\n"                                                             , ts.c_cflag & CRTSCTS ? 1 : 0);
//
//
//  printf ("lfags: ISIG     %d when any of the characters INTR, QUIT, SUSP, or DSUSP are received, generate the corresponding signal.\n"                , ts.c_lflag & ISIG    ? 1 : 0);
//  printf ("lfags: ICANON   %d enable canonical mode.  This enables the special characters EOF, EOL, EOL2, ERASE, KILL, REPRINT, STATUS, and  WERASE,\n", ts.c_lflag & ICANON  ? 1 : 0);
//  printf ("lfags:             and buffers by lines.\n"                                                                                                                               );
//  printf ("lfags: XCASE    %d if ICANON is also set, terminal is uppercase only.  Input is converted to lowercase, except for characters preceded by\n", ts.c_lflag & XCASE   ? 1 : 0);
//  printf ("lfags:             \\.  On output, uppercase characters are preceded by \\ and lowercase characters are converted to uppercase.\n"                                        );
//  printf ("lfags: ECHO     %d echo input characters.\n"                                                                                                , ts.c_lflag & ECHO    ? 1 : 0);
//  printf ("lfags: ECHOE    %d if ICANON is also set, the ERASE character erases the preceding input character, and WERASE erases the preceding word.\n", ts.c_lflag & ECHOE   ? 1 : 0);
//  printf ("lfags: ECHOK    %d if ICANON is also set, the KILL character erases the current line.\n"                                                    , ts.c_lflag & ECHOK   ? 1 : 0);
//  printf ("lfags: ECHONL   %d if ICANON is also set, echo the NL character even if ECHO is not set.\n"                                                 , ts.c_lflag & ECHONL  ? 1 : 0);
//  printf ("lfags: ECHOCTL  %d if  ECHO is also set, ASCII control signals other than TAB, NL, START, and STOP are echoed as ^X, where X is the char-\n", ts.c_lflag & ECHOCTL ? 1 : 0);
//  printf ("lfags:             acter with ASCII code 0x40 greater than the control signal.  For example, character 0x08 (BS) is echoed as ^H.\n"                                      );
//  printf ("lfags: ECHOPRT  %d if ICANON and IECHO are also set, characters are printed as they are being erased.\n"                                    , ts.c_lflag & ECHOPRT ? 1 : 0);
//  printf ("lfags: ECHOKE   %d if ICANON is also set, KILL is echoed by erasing each character on the line, as specified by ECHOE and ECHOPRT.\n"       , ts.c_lflag & ECHOKE  ? 1 : 0);
//  printf ("lfags: FLUSHO   %d output is being flushed.  This flag is toggled by typing the DISCARD character.\n"                                       , ts.c_lflag & FLUSHO  ? 1 : 0);
//  printf ("lfags: NOFLSH   %d disable flushing the input and output queues when generating the SIGINT and SIGQUIT signals, and  flushing  the  input\n", ts.c_lflag & NOFLSH  ? 1 : 0);
//  printf ("lfags:             queue when generating the SIGSUSP signal.\n"                                                                                                           );
//  printf ("lfags: TOSTOP   %d send the SIGTTOU signal to the process group of a background process which tries to write to its controlling terminal.\n", ts.c_lflag & TOSTOP  ? 1 : 0);
//  printf ("lfags: PENDIN   %d all characters in the input queue are reprinted when the next character is read.  (bash handles typeahead this way.)\n"  , ts.c_lflag & PENDIN  ? 1 : 0);
//  printf ("lfags: IEXTEN   %d enable implementation-defined input processing.\n"                                                                       , ts.c_lflag & IEXTEN  ? 1 : 0);


  fd_set readfds;
  struct timeval tv, *tvwait;

  /* isn't the Unix way of doing this so much more elegant :-) */
  FD_ZERO (&readfds);
  FD_SET (handle->fd_, &readfds);

  tv.tv_sec = handle->readTimeout_ / 1000000;
  tv.tv_usec = handle->readTimeout_ % 1000000;

  if (handle->readTimeout_ < 0)
  {
    tvwait = NULL;
  }
  else
  {
    tvwait = &tv;
  }

  int ret = select (handle->fd_ + 1, &readfds, NULL, NULL, tvwait);

  if (ret < 0)
    {
      storeErrorMessage (handle);
      return 0;
    }
  if (ret == 0)
    {
      return 0;
    }
  return 1;
}


int
m_spFlushInput          (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }

  unsigned char byte;
  int timeout = handle->readTimeout_;

  m_spSetTimeout (handle, 0);

  while (m_spReceive(handle, &byte, 1) > 0);

  m_spSetTimeout (handle, timeout);

  return 1;
}


int
m_spSend                (struct MSpHandle *handle, const void *buffer, const unsigned size)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }

  int ret = write (handle->fd_, buffer, size);
  if (ret < 0)
    {
      storeErrorMessage (handle);
      return 0;
    }

  return ret;
}


int
m_spReceiveByte(struct MSpHandle *handle, unsigned char *ch)
{ 
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }
  if (m_spWaitForData(handle) > 0)
  {
    int ret = read(handle->fd_, ch, 1);
    if (ret < 0)
    {
      storeErrorMessage(handle);
      return 0;
    }
    if (ret == 0)
    {
      return 0;
    }
    return 1;
  }
  return 0;
}


int
m_spReceive             (struct MSpHandle *handle, void *buffer, const unsigned size)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }

  unsigned int i = 0;
  for(; i < size; i++)
  {
    unsigned char ch;
    int ret = m_spReceiveByte(handle, &ch);
    if (ret == 0) break;
    ((unsigned char *)buffer)[i] = ch;
  }
  return i;
}


int
m_spSetTimeout          (struct MSpHandle *handle, long usec)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }

  handle->readTimeout_ = usec;

  return 1;
}


int
m_spReadCts               (struct MSpHandle *handle, int *value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }


  int val;
  if (ioctl (handle->fd_, TIOCMGET, &val) < 0)
  {
    storeErrorMessage (handle);
    return 0;
  }

  *value = (val & TIOCM_CTS) ? 1 : 0;
  return 1;
}


int
m_spReadDsr               (struct MSpHandle *handle, int *value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }


  int val;
  if (ioctl (handle->fd_, TIOCMGET, &val) < 0)
  {
    storeErrorMessage (handle);
    return 0;
  }

  *value = (val & TIOCM_DSR) ? 1 : 0;
  return 1;
}


int
m_spReadRi                (struct MSpHandle *handle, int *value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }


  int val;
  if (ioctl (handle->fd_, TIOCMGET, &val) < 0)
  {
    storeErrorMessage (handle);
    return 0;
  }

  *value = (val & TIOCM_RI) ? 1 : 0;
  return 1;
}


int
m_spReadDcd                (struct MSpHandle *handle, int *value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }


  int val;
  if (ioctl (handle->fd_, TIOCMGET, &val) < 0)
  {
    storeErrorMessage (handle);
    return 0;
  }

  *value = (val & TIOCM_CD) ? 1 : 0;
  return 1;
}


int
m_spWriteDtr               (struct MSpHandle *handle, int value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }

  int val;
  if (ioctl(handle->fd_, TIOCMGET, &val) < 0)
    {
      storeErrorMessage (handle);
      return 0;
    }

  if (!value) val |= TIOCM_DTR;
  if ( value) val &= ~(TIOCM_DTR);

  if (ioctl(handle->fd_, TIOCMSET, &val) < 0)
    {
      storeErrorMessage (handle);
      return 0;
    }
  return 1;
}


int
m_spWriteRts               (struct MSpHandle *handle, int value)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }

  int val;
  if (ioctl(handle->fd_, TIOCMGET, &val) < 0)
    {
      storeErrorMessage (handle);
      return 0;
    }

  if (!value) val |= TIOCM_RTS;
  if ( value) val &= ~(TIOCM_RTS);

  if (ioctl(handle->fd_, TIOCMSET, &val) < 0)
    {
      storeErrorMessage (handle);
      return 0;
    }
  return 1;
}


int
m_spGetFileDescriptor   (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  if (handle->fd_ < 0)
  {
    snprintf (handle->errorMessage_, sizeof(handle->errorMessage_), "sp: ERROR: no file descriptor (%s)", handle->comportName_);
    return 0;
  }
  return handle->fd_;
}


const char *
m_spGetErrorMessage (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    strcpy(g_errorMessageBuffer, g_errorMessage);
    strcpy(g_errorMessage, "");
    if (!strcmp (g_errorMessageBuffer, ""))
    {
      return "handle is out of range";
    }
    return g_errorMessageBuffer;
  }
  strcpy(handle->errorMessageBuffer_, handle->errorMessage_);
  strcpy(handle->errorMessage_, "");
  return handle->errorMessageBuffer_;
}


#endif // __MINGW32__


/*   ___  ____    _           _                           _            _      */
/*  / _ \/ ___|  (_)_ __   __| | ___ _ __   ___ _ __   __| | ___ _ __ | |_    */
/* | | | \___ \  | | '_ \ / _` |/ _ \ '_ \ / _ \ '_ \ / _` |/ _ \ '_ \| __|   */
/* | |_| |___) | | | | | | (_| |  __/ |_) |  __/ | | | (_| |  __/ | | | |_    */
/*  \___/|____/  |_|_| |_|\__,_|\___| .__/ \___|_| |_|\__,_|\___|_| |_|\__|   */
/*                                  |_|                                       */


const char *
m_spGetReceiveLineEnding (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return "";
  }
  return handle->receiveLineEnding_;
}


int
m_spSetReceiveLineEnding (struct MSpHandle *handle, const char *lineEnding)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  strncpy(handle->receiveLineEnding_, lineEnding, sizeof(handle->receiveLineEnding_)-1);
  return 1;
}


const char *
m_spGetSendLineEnding    (struct MSpHandle *handle)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return "";
  }
  return handle->sendLineEnding_;
}


int
m_spSetSendLineEnding    (struct MSpHandle *handle, const char *lineEnding)
{
  if (handle == NULL)
  {
    snprintf (g_errorMessage, sizeof(g_errorMessage), "sp: ERROR: no handle");
    return 0;
  }
  strncpy(handle->sendLineEnding_, lineEnding, sizeof(handle->sendLineEnding_)-1);
  return 1;
}


int
m_spSendLine             (struct MSpHandle *handle, const char *data)
{
  const char *end = m_spGetSendLineEnding (handle);
  char buffer[strlen (end) + strlen(data) + 10];
  sprintf (buffer, "%s%s", data, end);

//   printf ("m_spSendLine: ");
//   for (int i=0; i<strlen(buffer); i++) printf ("'%c'(%d), ", buffer[i], buffer[i]);
//   printf ("\n");
  int bytesSend = m_spSend (handle, buffer, strlen (buffer));
  return bytesSend;
}


int
m_spReceiveLine          (struct MSpHandle *handle, char *data, unsigned int maxLength)
{
  const char *end = m_spGetReceiveLineEnding (handle);

  unsigned char received = 0;
  unsigned int counter = 0;
  int receivedLineEnding = 0;
  //char data2[maxLength];
  memset (data, 0, maxLength);
  while (!receivedLineEnding)
  {
    int ret = m_spReceive (handle, (void *)&received, 1);

    // check for correct data;
    if (ret <= 0) { return 0; }
    // add to buffer
    data[counter] = received;

    // check for line ending!
    char *temp = &data[strlen(data) - strlen(end)];
    if (!strcmp (temp, end))
    { // found line ending!
      receivedLineEnding = 1;
      data[strlen(data) - strlen(end)] = '\0';
      //counter -= strlen (receiveLineEnding_);
    }
    counter++;
    // check if buffer is full
    if (counter >= maxLength)
    {
      return 0;
    }
  }

  return counter;
}


int
m_spSetLineEndings        (struct MSpHandle *handle, const char *lineEnding)
{
  if (!m_spSetSendLineEnding    (handle, lineEnding)) return 0;
  if (!m_spSetReceiveLineEnding (handle, lineEnding)) return 0;
  return 1;
}
