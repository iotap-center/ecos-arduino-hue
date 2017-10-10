# ecos-arduino-hue
This piece of software was written for the [ECOS project](http://iotap.mah.se/projects/ecos/). The sketch enables an Arduino (preferably an Arduino Zero, due to the amount of available memory) to subscribe to MQTT messages and control a Philips Hue setup based on those messages. No support for Wifi is in place for this version, so an Ethernet shield is required.

## Dependencies

This sketch uses [Nick O'Leary's Arduino Client for MQTT (PubSubClient)](https://pubsubclient.knolleary.net/) and [ArduinoJSON](https://bblanchon.github.io/ArduinoJson/). Make sure to install them before you try to compile the sketch. Also, make sure that PubSubClient's [MQTT_MAX_PACKET_SIZE](https://pubsubclient.knolleary.net/api.html#configoptions) is set to (at least) 1024.

## Acknowledgements

This code contains portions of code by [Nick O'Leary](https://github.com/knolleary) and [James Bruce](https://github.com/jamesabruce) (see [gist](https://gist.github.com/jamesabruce/8bc9faf3c06e30c3f6fc)).
