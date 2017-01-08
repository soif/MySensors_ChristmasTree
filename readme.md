# MySensors Christmas Tree

This [MySensors](https://www.mysensors.org/)  node controls a (5V) led strip, aka NeoPixels (based on WS2811 chips or similars) as well as one SSR (aka discrete Relay) output.

It allows the following features :

- Supports 3 modes for each output (NeoPixels or SSR) : Off,On, Animation
- Toggles between modes by pressing a dedicated hardware push button, or via Mysensorz radio messages
- Feedbacks the current output mode using a led
- In On mode (for NeoPixels), controls leds colors, via Mysensor RVB radio messages
- In Animation mode , controls animation speed using a potentiometer or via Mysensor radio messages

Please read the [documentation](description.md), for more information.
