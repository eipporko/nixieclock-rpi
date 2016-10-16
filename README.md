6 digit Nixie Clock
===================

## Motivation

* [Alex's Nixie Clock](https://www.raspberrypi.org/blog/alexs-nixie-clock/)


## Project structure

 * `hardware/eagle` - PCB design and schematics
 * `hardware/docs`  - various PDF datasheets for components used
 * `software`       - clock program

## BOM Summary
Main components

 * IN-8-2 Nixie tubes
 * MH74141 Nixie driver
 * 74HCT259 8-bit addressable latch
 * SN74LS77 4-bit D latch
 * Raspberry Pi Zero, 1, 2 or 3 (Models B, B+)

The BCD decoder IC and Nixie tubes is hard to obtain, so far they're only
available on ebay and most of them are NOS(new old stock)

Check the schematics for the rest of the passives

## Theory of operation


### Architecture
Every digit of the display is composed by a Nixie tube (IN-8-2) driven by a decimal decoder designed for directly drive this kind of gas-filled cold-cathode indicators (MH74141) which input is supplied by a 4-bit D Latch (SN74LS77).

Each latch is multiplexed, with one 3-line to 8-line demux with latch (74HCT259), in order to share the 4 bit bus with all the drivers.

In this way it's only needed 8 bits to control the clock's display.

```
                            .___.
                   +3V3---1-|O O|--2--+5V
           (SDA)  GPIO2---3-|O O|--4--+5V
          (SCL1)  GPIO3---5-|O O|--6--_
     (GPIO_GLCK)  GPIO4---7-|O O|--8-----GPIO14 (TXD0)
                       _--9-|O.O|-10-----GPIO15 (RXD0)
     (GPIO_GEN0) GPIO17--11-|O O|-12-----GPIO18 (GPIO_GEN1)
     (GPIO_GEN2) GPIO27--13-|O O|-14--_
     (GPIO_GEN3) GPIO22--15-|O O|-16-----GPIO23 (GPIO_GEN4)
                   +3V3--17-|O O|-18-----GPIO24 (GPIO_GEN5)
      (SPI_MOSI) GPIO10--19-|O.O|-20--_
      (SPI_MOSO) GPIO9 --21-|O O|-22-----GPIO25 (GPIO_GEN6)
      (SPI_SCLK) GPIO11--23-|O O|-24-----GPIO8  (SPI_C0_N)
                       _-25-|O O|-26-----GPIO7  (SPI_C1_N)
        (EEPROM) ID_SD---27-|O O|-28-----ID_SC Reserved for ID EEPROM
                 GPIO5---29-|O.O|-30--_
                 GPIO6---31-|O O|-32-----GPIO12
                 GPIO13--33-|O O|-34--_
                 GPIO19--35-|O O|-36-----GPIO16
                 GPIO26--37-|O O|-38-----GPIO20
                       _-39-|O O|-40-----GPIO21
                            '---'


** NIXIE DIGIT MESSAGE **

                 DATA            ADDRESS    STRBE
           ,--------------,    ,---------,  ,---,
           A    B    C    D    A    B    C    |
           |    |    |    |    |    |    |    |
        | $22  $18  $16  $15  $13  $12  $11  $7 |
        '---------------------------------------'

```
