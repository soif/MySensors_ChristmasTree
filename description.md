# MySensors Christmas Tree

## Introduction

This [MySensors](https://www.mysensors.org/)  node controls a (5V) led strip, aka NeoPixels (based on WS2811 chips or similars) as well as one SSR (aka discrete Relay) output. Both output can be set OFF, ON, and  Animation mode.
Animation mode chains various sequences for the Led Strip output, and some blink patterns for the SSR output.

## Features

This [MySensors](https://www.mysensors.org/)  node provides the following features :

- Supports 3 modes for each output (NeoPixels or SSR) : Off,On, Animation
- Toggles between modes by pressing a dedicated hardware push button, or via Mysensorz radio messages
- Feedbacks the current output mode using a led
- In On mode (for NeoPixels), controls leds colors, via Mysensor RVB radio messages
- In Animation mode , controls animation speed using a potentiometer or via Mysensor radio messages


## User Guide

 - Each (short) click on a button switch between the 3 modes : OFF, ON, Animation
 - In animation mode, turning the potentiometer WHILE keeping the button held, changes its animation speed


## Schematic and Wiring

soon