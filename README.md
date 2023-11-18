# K197Control
This is an Arduino library that allows to control a K197 voltmeter with the same two wire protocol used by the IEEE488 option. The communication is over a two wire bus (one wire for each direction).

The lower layer protocol could also be used as a generic two wire communication protocol between two microcontrollers (only an input pin supporting edge interrupt + output pin is needed on each microcontroller). 

At the moment the library only supports Arduino Uno and other boards with a compatible microcontroller.

DISCLAIMER: Please note that the purpose of this repository is educational. Any use of the information for any other purpose is under own responsibility.

Background information
----------------------
Releasing this sketch was inspired from discussions in the EEVBlog forum: https://www.eevblog.com/forum/testgear/keithley-197a-owners_-corner/ and https://www.eevblog.com/forum/projects/replacement-display-board-for-keithley-197a/msg4232665/#msg4232665

In the discussion, some forum members suggested to use the IEEE488 internal interface to control the voltmeter via bluetooth (inside the voltmeter there is a 6 pin connector used by an optional IEEE488 interface board). 
Using the IEEE interface instead of the display interface offers more possibilities to control the voltmeter, for example it is possible to select the range bypassing the front panel switches (only for volt and Ampere modes, it is also not posible to change modes or switch between Ac and DC Volts). With the help of some forum members, I managed to reverse engineer the protocol used. This library and the associated examples implement this protocol.

The protocol used by the K197 to communicate with the IEEE board uses two wires, one for each direction. As far as I know it is a unique protocol (at least, I never encountered it before). If you know of other applications using a similar protocol please comment in one of the forum threads linked above.

To understand the protocol is useful to think of three layers (this is somewhat arbitrary but bear with me for now), which I dubbed gemini, gemini frame and K197 control:
- The gemini protocol is the lowest layer and is responsible to send arbitrary bit sequences on the wire
- The gemini frame sits on top of gemini and is responsible for sending packing/unpacking a byte sequences into a frame and sending/receiving it using the lower layer
- The K197 control protocol encodes commands (to the K197) or measurement results (from the K197) as byte sequences that are sent or received using the gemini frame protocol

The K197 voltmener was designed at the time where hardware support for serial communication was rarely integrated in generic I/O peripherals. Rather than using a dedicated IC (e.g. a UUART), they designed a full duplex protocol which can be efficiently bit banged and is very tolerant of any delay from either side. In addition to the input and output I/O pins, the only other HW resource required is an edge sensitive interrupt. 

In fact, the two laower layers (gemini and gemini frame) could be useful even with modern micros when running out of serial ports.

The gemini and gemini frame protocols in a nutshell
---------------------------------------------------

In the following, let's be general and call the two peers Bob and Alice. Bob and Alice will keep their output pins LOW, while monitoring for a positive edge on their input pins. When Bob wants to send a bit to Alice, it will first set the outup pin high, then it will set the output pin according to the bit value and wait for an acknowledgement from Alice.

When Alice detects a positive edge, it will wait for a predetermined setup time and then read the value of the input pin. As soon as she has got the bit value, it will acknowledge generating a positive edge on its own output pin.  

The genius in this protocol is that the acknowledgment can also be used to send a bit in the opposite direction. Immediately after Alice sets the output pin high to ackbnowledge the reception, she has two choices: if she does not have any data to send, it will return the output pin to the idle state (LOW).

