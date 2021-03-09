from mlx90640_evb9064x import *
from mlx90640 import *


def main():
    print("start")
    dev = MLX90640()
    print("dev", dev)

    # r = dev.i2c_init("/dev/i2c-1")
    # r = dev.i2c_init("mcp://mcp:2221/0")
    # r = dev.i2c_init("mlx://evb:9064x/COM6")
    r = dev.i2c_init(evb9064x_get_i2c_comport_url('auto'))
    print("init", r)
    r = dev.set_refresh_rate(1)
    print("setRefreshRate", r)

    refresh_rate = dev.get_refresh_rate()
    print("refresh rate: {}".format(refresh_rate))

    dev.dump_eeprom()
    dev.extract_parameters()

    for i in range(0, 10):
        dev.get_frame_data()
        ta = dev.get_ta() - 8.0
        emissivity = 1

        to = dev.calculate_to(emissivity, ta)

        print("{:02d}: {}".format(i, ','.join(format(x, ".2f") for x in to)))


if __name__ == "__main__":
    main()
