# Linux I2C driver for the 24FC02 EEPROM

An out-of-tree Linux kernel module for the Microchip 24xx02 family of 2 Kbit
(256 x 8) I2C serial EEPROMs (24AA02 / 24LC02B / 24FC02). It binds to an I2C
client, exposes the EEPROM through sysfs, and has an optional character device
interface.

- [Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/24AA02-24LC02B-24FC02-2K-I2C-Serial-EEPROM-20001709N.pdf)

## Features

- Binds via I2C device ID table or Device Tree (`ee24c02`).
- sysfs attributes to read and write the whole EEPROM as hex bytes.
- Page-aware writes: data is split on 8-byte page boundaries, with a 10 ms wait
  after each page write for the internal write cycle.
- Optional character device (`/dev/eeprom`), selectable at build time.

## Building

Build against the running kernel:

```sh
make host
```

Build against a specific kernel build tree:

```sh
make KERNELDIR=/path/to/kernel/build modules
```

Cross-compile for ARM (for example, to run under QEMU):

```sh
make qemu QEMUDIR=/path/to/kernel/build QEMU_CC=arm-linux-gnueabihf-
```

> **Note:** the default `KERNELDIR`, `QEMUDIR` and `QEMU_CC` values in the
> Makefile point at paths on my machine. Override them as shown above
> or edit the Makefile.

Disable the character device interface:

```sh
make host CONFIG_AT24C02_CHARDEV=n
```

Clean:

```sh
make clean
```

## Usage

### 1. Load the module

```sh
sudo insmod at24c02_drv.ko
```

### 2. Describe the device

**Device Tree**, as a child of your I2C controller node:

```dts
&i2c1 {
	eeprom@50 {
		compatible = "ee24c02";
		reg = <0x50>;
	};
};
```

**Or instantiate it at runtime** (the I2C ID is `ee24c02`):

```sh
echo ee24c02 0x50 | sudo tee /sys/bus/i2c/devices/i2c-1/new_device
```

Replace `i2c-1` with the bus your EEPROM is on. On a successful probe the log
shows `at24c02 probed`. To remove it again:

```sh
echo 0x50 | sudo tee /sys/bus/i2c/devices/i2c-1/delete_device
```

### 3. Use the sysfs interface

The attributes appear under the I2C client directory, for example
`/sys/bus/i2c/devices/1-0050/`.

| Attribute | Access | Description |
|-----------|--------|-------------|
| `eeprom` | read/write | Read the full 256 bytes, or write bytes starting at a given address |
| `address` | read/write | Address pointer in hex, also exposed as the `reg` module parameter |
| `capacity` | read-only | EEPROM size in bytes (`256`) |
| `page_size` | read-only | Page write size in bytes (`8`) |

**Read** the whole EEPROM as hex bytes:

```sh
cat /sys/bus/i2c/devices/1-0050/eeprom
# 0xff 0xff 0xff ...
```

**Write** by passing the start address followed by the data bytes, all in hex
and separated by spaces:

```sh
# Write 0xde 0xad 0xbe 0xef starting at address 0x10
echo "0x10 0xde 0xad 0xbe 0xef" | sudo tee /sys/bus/i2c/devices/1-0050/eeprom
```

Notes on writes:

- The first value is always the start address; at least one data byte must
  follow it.
- Writes that cross an 8-byte page boundary are split into separate page
  writes automatically.
- `eeprom` reads always start at address `0x00`; the `address` attribute does
  not affect them.

**Address pointer:**

```sh
cat /sys/bus/i2c/devices/1-0050/address        # 0x00
echo 20 | sudo tee /sys/bus/i2c/devices/1-0050/address   # sets 0x20 (hex)
```

### 4. Character device (experimental)

With `CONFIG_AT24C02_CHARDEV` enabled (the default), loading the module also
creates the `eeprom` class and the `/dev/eeprom` node. `llseek` updates the
address pointer, but `read` and `write` do not transfer data yet. Use the sysfs
interface for real I/O.

## Unloading

```sh
sudo rmmod at24c02_drv
```

## License

Licensed under the GNU General Public License v2.0 only. See [LICENSE](LICENSE).
