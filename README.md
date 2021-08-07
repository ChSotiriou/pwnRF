# pwnRF | Easy to use SUBGHZ transceiver

## Quick Description

This repository contains the firmware for a [Nucleo-WL55](https://www.st.com/en/evaluation-tools/nucleo-wl55jc.html) development board that lets you, using a UART terminal on your computer to transmit an arbitrary message on any frequency below 1GHz.

I'm looking to add many more features in the future.

## Usage

To use the device and access the command line, connect the board to your computer and using a serial terminal (I use [hterm](https://www.der-hammer.info/pages/terminal.html)) with a baudrate of 115200. Make sure to terminate each line with `"\r\n"`.

## Command Description

- `help`: Lists all registed commands
- `freq [Hz]`: Get/Set the transmitting frequency (1MHz - 1GHz)
- `power [dBm]`: Get/Set the transmitter power (1dBm - 22dBm)
- `transmit <msg>`: Transmits a digital message 