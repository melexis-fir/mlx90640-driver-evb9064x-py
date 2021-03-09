#ifndef SERIALPORT_SP_H
#define SERIALPORT_SP_H

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *m_spGetRevision (void);

/** \ingroup serialport
 The baudrate constants
*/
enum MBaudRate
{ M_SP_50,       /**< 50 baud */
  M_SP_75,       /**< 75 baud */
  M_SP_110,      /**< 110 baud */
  M_SP_134,      /**< 134 baud */
  M_SP_150,      /**< 150 baud */
  M_SP_200,      /**< 200 baud */
  M_SP_300,      /**< 300 baud */
  M_SP_600,      /**< 600 baud */
  M_SP_1200,     /**< 1200 baud */
  M_SP_1800,     /**< 1800 baud */
  M_SP_2400,     /**< 2400 baud */
  M_SP_4800,     /**< 4800 baud */
  M_SP_9600,     /**< 9600 baud */
  M_SP_14400,    /**< 14400 baud */
  M_SP_19200,    /**< 19200 baud */
  M_SP_38400,    /**< 38400 baud */
  M_SP_56000,    /**< 56000 baud */
  M_SP_57600,    /**< 57600 baud */
  M_SP_115200,   /**< 115200 baud */
  M_SP_128000,   /**< 128000 baud */
  M_SP_230400,   /**< 230400 baud */
  M_SP_256000,   /**< 256000 baud */
  M_SP_460800,   /**< 460800 baud */
  M_SP_921600,   /**< 921600 baud */
  M_SP_1000000,  /**< 1000000 baud */
};

/** \ingroup serialport
 The stopbits constants
*/
enum MStopBits
{ M_SP_ONE,              /**< 1 stop bit */
  M_SP_ONE_AND_A_HALF,   /**< 1.5 stop bit */
  M_SP_TWO               /**< 2 stop bit */
};


/** \ingroup serialport
 The parity constants
 */
enum MParity
{ M_SP_NONE,  /**< no parity */
  M_SP_EVEN,  /**< even parity */
  M_SP_ODD,    /**< odd parity */
  M_SP_IGNORE
};


/** \ingroup serialport
 The handshake constants
 */
enum MHandShake
{ M_SP_OFF,              /**< no handshake */
  M_SP_XON_XOFF,         /**< Xon/Xoff handshaking */
  M_SP_RTC_CTS,          /**< RTC/CTS handshaking */
  M_SP_RTC_CTS_XON_XOFF, /**< RTC/CTS handshaking together with Xon/Xoff */
  M_SP_RTS_ON_TX         /**< RTS on TX handshaking */
};

#define M_SP_BLOCK = -1;

struct MSpHandle;

struct MSpHandle *m_spOpen   (const char *comPort, enum MBaudRate baudRate, enum MStopBits stopBits, enum MParity parity, enum MHandShake handshake);
int m_spClose                (struct MSpHandle *handle);
int m_spSetBaudRate          (struct MSpHandle *handle, enum MBaudRate baudRate);
int m_spSetStopBits          (struct MSpHandle *handle, enum MStopBits stopBits);
int m_spSetParity            (struct MSpHandle *handle, enum MParity parity);
int m_spSetHandShake         (struct MSpHandle *handle, enum MHandShake handShake);
int m_spWaitForData          (struct MSpHandle *handle);
int m_spFlushInput           (struct MSpHandle *handle);
int m_spSend                 (struct MSpHandle *handle, const void *buffer, const unsigned size);
int m_spReceive              (struct MSpHandle *handle, void *buffer, const unsigned size);
int m_spSetTimeout           (struct MSpHandle *handle, long usec);
int m_spReadCts              (struct MSpHandle *handle, int *value);
int m_spReadDsr              (struct MSpHandle *handle, int *value);
int m_spReadRi               (struct MSpHandle *handle, int *value);
int m_spReadDcd              (struct MSpHandle *handle, int *value);
int m_spWriteDtr             (struct MSpHandle *handle, int value);
int m_spWriteRts             (struct MSpHandle *handle, int value);
int  m_spGetFileDescriptor   (struct MSpHandle *handle);
const char *m_spGetErrorMessage (struct MSpHandle *handle);

int         m_spSendLine             (struct MSpHandle *handle, const char *data);
int         m_spReceiveLine          (struct MSpHandle *handle, char *data, unsigned int maxLength);

const char *m_spGetReceiveLineEnding (struct MSpHandle *handle);
int         m_spSetReceiveLineEnding (struct MSpHandle *handle, const char *lineEnding);
const char *m_spGetSendLineEnding    (struct MSpHandle *handle);
int         m_spSetSendLineEnding    (struct MSpHandle *handle, const char *lineEnding);
int         m_spSetLineEndings       (struct MSpHandle *handle, const char *lineEnding);

#ifdef __cplusplus
}
#endif

#endif // SERIALPORT_SP_H
