import os
import sys
import ctypes
import serial.tools.list_ports
from mlx90640 import *

USB_VID = 1001
USB_PID = 32


def load_driver():
    dev = MLX90640()

    cfp = os.path.dirname(os.path.realpath(__file__))
    machine = 'windows'
    shared_lib_file = 'mlx90640_driver_evb9064x.dll'
    if os.environ.get('OS','') != 'Windows_NT':
        import platform
        machine = platform.machine()
        shared_lib_file = 'libmlx90640_driver_evb9064x.so'

    lib = ctypes.CDLL(os.path.join(cfp, 'libs', machine, shared_lib_file), mode=ctypes.RTLD_GLOBAL)

    # struct MLX90640DriverRegister_t *MLX90640_get_register_evb9064x(void);
    _get_register_evb9064x = lib.MLX90640_get_register_evb9064x
    _get_register_evb9064x.restype = ctypes.POINTER(ctypes.c_uint16)
    _get_register_evb9064x.argtypes = []
    dev._register_driver(_get_register_evb9064x())



def list_serial_ports(pid=None, vid=None):
    """ Lists serial port names which startswith serial_number like in argument.

    :raises EnvironmentError:
        On unsupported or unknown platforms
    :returns:
        A list of the serial ports available on the system
    """
    ports = []
    if sys.platform.startswith('win') or sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        for sp in serial.tools.list_ports.comports():
            if pid is None or vid is None:
                ports.append(str(sp.device))
            else:
                if sp.vid == vid and sp.pid == pid:
                    ports.append(str(sp.device))
        return ports
    elif sys.platform.startswith('darwin'):
        import glob
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result


def evb9064x_get_i2c_comport_url(comport):
    url = 'mlx://evb:9064x/'
    index = None
    try:
        index = int(comport)
    except Exception as e:
        pass
    if comport == 'auto':
        index = 0

    if index is not None:
        comports = list_serial_ports(USB_PID, USB_VID)
        if (index < len(comports)):
            comport = comports[index]
        else:
            return None

    if comport.startswith('/'):
        comport = comport[1:]
    url += comport
    return url
