# STM32F469I-DISCO external signals

Reported board marking: **MB1189 B-01**. ST's currently published MB1189 schematics
list B-07, B-08, and C-01. The table below comes from UM1932 Rev 5 and is
**provisional for B-01**. Do not connect vehicle wiring before confirming the
connector labels and signal continuity on the actual board. The default build
uses the Discovery Rev B 8 MHz HSE setting, which also needs physical checking.

| Signal | MCU alternate function | Connector | Conflict check |
| --- | --- | --- | --- |
| MSP_TX | PC6 / USART6_TX | CN12 pin 6 | Connect to flight controller RX; USART6 is no longer on CN7 |
| MSP_RX | PC7 / USART6_RX | CN12 pin 8 | Connect to flight controller TX |
| WS_CHAIN_A | PB4 / TIM3_CH1 | CN12 pin 5 | Confirm B-01 routing and SWD/JTAG configuration |
| WS_CHAIN_B | PB5 / TIM3_CH2 | CN12 pin 9 | Confirm B-01 routing |
| LAMP_FRONT | PC13 / GPIO | CN12 pin 13 | Confirm anti-tamper/on-board circuit on B-01 |
| LAMP_ROOF | PC1 / GPIO | CN12 pin 14 | Confirm speaker/analog shared circuitry on B-01 |

LCD MIPI DSI, SDRAM FMC, and capacitive touch I2C/reset pins are dedicated to
the board and must remain assigned to the official BSP. CN12 pins 1 and 2
provide 3.3 V and GND for reference only; external 5 V LEDs and 12 V lamps
must use separate protected supplies. All grounds join at a suitable power
distribution point; lamp current must not return through this header.
