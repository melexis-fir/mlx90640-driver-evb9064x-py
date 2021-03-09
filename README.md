# MLX90640 driver for EVB90640-41

[![GitHub release (latest by date)](https://img.shields.io/github/v/release/melexis-fir/mlx90640-driver-evb9064x-py?label=github-latest-release-tag)](https://github.com/melexis-fir/mlx90640-driver-evb9064x-py/releases) ![Lines of code](https://img.shields.io/tokei/lines/github/melexis-fir/mlx90640-driver-evb9064x-py)  

[![PyPI](https://img.shields.io/pypi/v/mlx90640-driver-evb9064x)](https://pypi.org/project/mlx90640-driver-evb9064x) ![PyPI - Python Version](https://img.shields.io/pypi/pyversions/mlx90640-driver-evb9064x) ![PyPI - License](https://img.shields.io/pypi/l/mlx90640-driver-evb9064x)  

![platform](https://img.shields.io/badge/platform-Win10%20PC%20%7C%20linux%20PC%20%7C%20rasberry%20pi%204%20%7C%20Jetson%20Nano%20%7C%20beagle%20bone-lightgrey)  

MLX90640 is a thermal camera (32x24 pixels) using Far InfraRed radiation from objects to measure the object temperature.  
https://www.melexis.com/mlx90640  
The python package "[mlx90640-driver](https://github.com/melexis-fir/mlx90640-driver-py)" driver interfaces the MLX90640 and aims to facilitate rapid prototyping.

This package provide the I2C low level routines.
It uses the I2C from the EVB90640-41 which is connected via the USB cable to the computer.  
https://www.melexis.com/en/product/EVB90640-41/  

## Getting started

### Installation


```bash
pip install mlx90640-driver-evb9064x
```

https://pypi.org/project/mlx90640-driver-evb9064x  
https://pypistats.org/packages/mlx90640-driver-evb9064x

__Note:__  
On linux OS, make sure the user has access to the comport  `/dev/ttyUSB<x>` or `/dev/ttyACM<x>`.  
And easy way to do this is by adding the user to the group `dialout`.

### Running the driver demo

* Connect the MLX90640 to the EVB
* Connect the EVB to your PC with the USB cable.  
* Open a terminal and run following command:  

```bash
mlx90640-evb9064x-dump auto
```

This program takes 1 optional argument.

```bash
mlx90640-evb9064x-dump <communication-port>
```

Note: this dump command is not yet available!
