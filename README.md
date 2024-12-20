# Enterprisey Blinky

This is just a silly project to create an overly complicated blink program,
and test out a bunch of features in Zephyr. It serves no useful purpose 
otherwise, but please enjoy. So far, it has the following things that are 
different from what you might be used to in the Arduino world:

1. Leverages the [Zephyr](https://zephyrproject.org) embedded OS, although 
   this is planned to be the foundation of future versions of the Arduino 
   framework as well.
2. Uses the USB Communications Device Class (CDC) Abstract Control Model 
   (ACM) to implement a virtual serial port connection to the host.
3. Wires up the logging subsystem to use the USB console.
4. Leverages 
   [binary descriptors](https://docs.zephyrproject.org/latest/services/binary_descriptors/index.html)
   to embed build information into the binary and then display it when you 
   connect. 
