# MySensors Christmas Tree

This [MySensors](https://www.mysensors.org/)  node controls a (5V) led strip, aka NeoPixels (based on WS2811 chips or similars) as well as one SSR (aka discrete Relay) output. Both output can be set OFF, ON, and  Animation mode.
Animation mode chains various sequences for the Led Strip output, and some blink patterns for the SSR output.

It allows the following features :

- Supports 3 modes for each output (NeoPixels or SSR) : Off,On, Animation
- Toggles between modes by pressing a dedicated hardware push button, or via Mysensorz radio messages
- Feedbacks the current output mode using a led
- In On mode (for NeoPixels), controls leds colors, via Mysensor RVB radio messages
- In Animation mode , controls animation speed using a potentiometer or via Mysensor radio messages

Please read the [documentation](description.md), for more information.


## Disclaimer

**THIS PROJECT IS CURRENTLY AN APHA VERSION !** The sotware is mostly finished, and breadboard testing run fine, but the final Schematic and wiring is a work in progress...


## Licence

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

