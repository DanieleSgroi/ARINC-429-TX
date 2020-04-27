# ARINC-429-TX

Arduino based ARINC 429 Transmitter

#DESCRIPTION 

This sketch demonstrates transmission of ARINC 429 data with an Arduino Pro Mini clocked at 16 MHz. 

Both Low-Speed (12,5 kbps) and High-Speed (100 kbps) data rates are supported.

The digital outputs shall be connected to proper level shifters in order to achieve the necessary electrical-level compatibility to the standard (differential +/- 10 V).

It has been succesfully used to emulate ARINC 429 data output of Rockwell-Collins DME-42 in order to troubleshoot A/C system integration.

This sketch can be used as skeleton for more complex data simulations.
