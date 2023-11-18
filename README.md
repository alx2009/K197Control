# K197Control
This is an Arduino library that allows to control a K197 voltmeter with the same two wire protocol used by the IEEE488 option. The communication is over a two wire bus (one wire for each direction).

The lower layer protocol could also be used as a generic two wire communication protocol between two microcontrollers (only an input pin supporting edge interrupt + output pin is needed on each microcontroller). 

At the moment the library only supports Arduino Uno and other boards with a compatible microcontroller.

DISCLAIMER: Please note that the purpose of this repository is educational. Any use of the information for any other purpose is under own responsibility.

Furthermore, this library has been tested with only one card and voltmeter. It may or may not work elsewhere, e.g. due to different firmware revisions of the card or the voltmeter. 

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

The above model is useful to understand the protocol and the library, but the original designers of the protocol might have had a completely different model in mind.

The K197 voltmener was designed at the time where hardware support for serial communication was rarely integrated in generic I/O peripherals. Rather than using a dedicated IC (e.g. a UUART), they designed a full duplex protocol which can be efficiently bit banged and is very tolerant of any delay from either side. In addition to the input and output I/O pins, the only other HW resource required is an edge sensitive interrupt. 

In fact, the two laower layers (gemini and gemini frame) could be useful even with modern micros when running out of serial ports.

The gemini protocol in a nutshell
---------------------------------------------------

In the following, let's be general and call the two peers Bob and Alice. Bob and Alice will keep their output pins LOW, while monitoring for a positive edge on their input pins. When Bob wants to send a bit to Alice, it will first set the outup pin high, then it will set the output pin according to the bit value and wait for an acknowledgement from Alice (up to a maximum time, then it will time-out).

When Alice detects a positive edge, it will wait for a predetermined setup time and then read the value of the input pin. As soon as she has got the bit value, it will acknowledge generating a positive edge on its own output pin.  

The genius in this protocol is that the acknowledgment can also be used to send a bit in the opposite direction. Immediately after Alice sets the output pin high to ackbnowledge the reception, she has two choices: if she does not have any data to send, it will return the output pin to the idle state (LOW). If she has data to send, she will set the output pin according to the bit value and wait for an acknowledgement from Bob (which is also including a bit to alice and so forth).

But wait a minute, doesn't this result in an endless sequence of transmissions? Indeed, my first attempts of aping the protocol resulted in a never ending back and forth transmission... the way it works is that the peer that initiates the communication always expects an acknowledgment but once the last bit is sent, it will not acknowledge the last bit received (if for some reason the peer was still sending data, it will time-out).

When a timout is detected, the higher layer (gemini frame) is informed.

Note that this creates a race condition if both Alice and Bob initiate a transmission at the same time. While this could be handled in a number of ways, the K197 appears to solve it by not allowing the IEE488 board to initiate the communication "out of the blue". The K197 is the one initiating the communication, the IEEE488 board can only send a bit when acknowledging a bit from the K197.

The gemini frame protocol
---------------------------------------------------
The gemini frame protocol packs and unpacks sequence of bytes in frames. A frame is composed of sub-frames and synchronization sequences. The sub-frame is composed by 9 bits, a start bit (set to 1) and 8 data bits encoding a byte of data. Any consecutive 0 bits outside a sub-frame are interpreted as synchronization sequences. Both the sub-frame rapresenting the bytes and the bits within a subframe are sent MSB first.

In principle there are a number of ways to detect a frame boundary:
- detect an initial synchronization sequence with 9 or more consecutive zeroes (this seems to be the case for the K197 but I have not used thie method in the library).  
- rely on a frame timout period. Once the communication is idle (both pins are LOW) for more than the frame timeout period, the next bit will start a new frame (this is the method implemented in the library).
- use frames of known lenght. Once all the bytes of a frame have been received we know that the frame reception is complete (this is also supported by the library so that we can process a frame as soon as possible)

When sending a frame, if an acknowledgment timeout is detected by the lower layer the frame layer will assume that the peer is unavailable and skip the rest of the frame. If an incomplete frame has been received the entire frame must be ingnored. 

It is also possible to send an empty frame (a frame only including a synchronization sequence but no data). 

The K197 control protocol
---------------------------------------------------
As anticipated, the K197 is always the initiator of the comunication. When it is triggered, it will attempt to send the measurement result encoded in a frame including an initial synchronization sequence of 16 '0' and 4 bytes of data. When the K197 is not triggered, it will still periodically "poll" the IEEE board sending an empty frame.

In both cases, the K197 will monitor the first bit sent by the K197 card. If the first bit received is zero, it will interprete it as if the IEEE488 board does not have any data to send. If the first bit is one, 

 




