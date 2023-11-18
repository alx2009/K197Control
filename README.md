# K197Control
This is an Arduino library that allows to control a K197 voltmeter with the same two wire protocol used by the IEEE488 option. The communication is over a two wire bus (one wire for each direction).

The lower layer protocol could also be used as a generic two wire communication protocol between two microcontrollers (only an input pin supporting edge interrupt + output pin is needed on each microcontroller). 

At the moment the library only supports Arduino Uno and other boards with a compatible microcontroller.

DISCLAIMER: Please note that the purpose of this repository is educational. Any use of the information for any other purpose is under own responsibility.

Furthermore, this library has been tested with only one card and one voltmeter. It may or may not work elsewhere, e.g. due to different firmware revisions of the card or the voltmeter. 

Installation and use
----------------------
Please note the dislcaimer above. These instructions explain how to download the library and compile under the Arduino IDE. Any other use is your own responsibility.

From the GitHub repository (https://github.com/alx2009/K197Control), select the "Releases", decide what release you want (the latest one usually) and download the .zip file with the source code for that release. Then follow the instructions on how to import a .zip library here: https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries#importing-a-zip-library

Once the library is installed, the example programs can be found in the Arduino IDE under File > Examples > K197control

- The K197Probe example demonstrate bhow to probe the interface at low level, displaying raw frame data.
- The K197ControlDataLogger example demonstrate how to log measurements results to Serial (Add an Arduino mini and a blutooth module and suddently your voltmeter can log data via bluetooth)
- The K197DataAcquisition example is similar to K197ControlDataLogger but in addition can send command to the voltmeter, including setting trigger mode and overriding the range.

Limitations

This is a very early release, developed at the same time the protocl was reverse engineered. Only a limited amount of tests has been performed. As such the API is not fully consistent and may have to be changed in futurtr revisions. I also expect that there are several bugs lingering around. One known limitation is that the library does not implement the acknowdlege timeout. This means that if the other party does not acknowledge a bit the library will hang. In practice this is rare as the K197 seems to be very well behaved.  

Background information
----------------------
Releasing this sketch was inspired from discussions in the EEVBlog forum: https://www.eevblog.com/forum/testgear/keithley-197a-owners_-corner/ and https://www.eevblog.com/forum/projects/replacement-display-board-for-keithley-197a/msg4232665/#msg4232665

In the discussion, some forum members suggested to use the IEEE488 internal interface to control the voltmeter via bluetooth (inside the voltmeter there is a 6 pin connector used by an optional IEEE488 interface board). 
Using the IEEE interface instead of the display interface offers more possibilities to control the voltmeter, for example it is possible to select the range bypassing the front panel switches (only for volt and Ampere modes, it is also not posible to change modes or switch between Ac and DC Volts). With the help of some forum members, I managed to reverse engineer the protocol used. This library and the associated examples implement this protocol.

The protocol used by the K197 to communicate with the IEEE board uses two wires, one for each direction. As far as I know it is a unique protocol (at least, I never encountered it before). If you know of other applications using a similar protocol please comment in one of the forum threads linked above.

To understand the protocol, it is useful to think of three layers (this is somewhat arbitrary but bear with me for now), which I dubbed gemini, gemini frame and K197 control respectively:
- The gemini protocol is the lowest layer and is responsible to send arbitrary bit sequences on the wire
- The gemini frame sits on top of gemini and is responsible for sending packing/unpacking byte sequences into frames which are sent/received using the lower layer
- The K197 control protocol encodes commands (to the K197) or measurement results (from the K197) as byte sequences that are sent or received using the gemini frame protocol

The above model is useful to understand the protocol and the library class hierarchy, obviously the original designers of the protocol might have had a completely different model in mind.

The K197 voltmener was designed at the time where hardware support for serial communication was rarely integrated in generic I/O peripherals. Rather than using a dedicated IC (e.g. a UART), they designed a full duplex protocol which can be efficiently bit banged and is very tolerant of any delay from either side. In addition to the input and output I/O pins, the only other HW resource required is an edge sensitive interrupt. 

In fact, the two lower layers (gemini and gemini frame) could be useful even with modern micros when running out of serial ports.

The gemini protocol in a nutshell
---------------------------------------------------

Let's call the two gemini peers Bob and Alice. When nothing need to be transmitted, Bob and Alice will keep their output pins LOW, while monitoring for a positive edge on their input pins (idle state). When Bob wants to send a bit to Alice, it will first set the outup pin high, then it will set the output pin according to the bit value and wait for an acknowledgement from Alice (up to a maximum time, then it will time-out).

When Alice detects a positive edge, it will wait for a predetermined setup time and then read the value of the input pin. As soon as she has got the bit value, it will acknowledge generating a positive edge on its own output pin.  

The genius in this protocol is that the acknowledgment can also be used to send a bit in the opposite direction. Immediately after Alice sets the output pin high to ackbnowledge the reception, she has two choices: if she does not have any data to send, it will return the output pin to the idle state (LOW). If she has data to send, she will set the output pin according to the bit value and wait for an acknowledgement from Bob (which is also including a bit to alice and so forth).

But wait a minute, doesn't this result in an endless sequence of transmissions? Indeed, my first attempts of aping the protocol resulted in a never ending back and forth transmission... the way it works is that the peer that initiates the communication always expects an acknowledgment but once the last bit is sent, it will not acknowledge the last bit received (if for some reason the peer was still sending data, it will time-out).

When a timout is detected, the higher layer (gemini frame) is informed.

Note that this creates a race condition if both Alice and Bob initiate a transmission at the same time. While this could be handled in a number of ways, the K197 appears to solve it by not allowing the IEE488 board to initiate the communication "out of the blue". The K197 is the one initiating the communication, the IEEE488 board can only send a bit when acknowledging a bit from the K197.

When sending a 0 bit, a short pulse is generated on the output pin. The duration of the pulse should be significantly shorter than the setup time, otherwise it could be misunderstood for a 1 level. However it should be long enough to be detected by the other peer. Looking at the datasheet for the 6821 Peripheral Interface Adapter used in the K197, the minimum duration is around 1 us (microsecond). However the pulse lenght observed is between 15 and 30 us, while the setup time is around 200 us. Sending a complete frame takes around 40 ms.

The example replicate the timing observed with the IEEE card, but it can be changed: the pulse duration can be reduced to a few us without apparent issues. the setup time can also be reduced but this hasn't been tested with the K197.

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

In both cases, the K197 will monitor the first bit sent by the K197 card.
- If the first bit received is zero, it will interprete it as if the IEEE488 board does not have any data to send. If even the K197 doesn't have a new measurement to send (e-g- depending on the trigger mode) the frame will only include 1 bit
- If the first bit is one, it will understand that the K197 board has commands to send and it. In such a case, the K197 will send a larger frame (possibly empty), so that the IEE board can send the command back in the acknowledge bit.

For the coding of the bits please refer to the protocol specificvation [link to be provided]

Limitations

Not all aspects and fields have been reverse engineered at this point. In particular, calibration is missing completely and retrieving stored data is not working.  

Calculating the mesurement result
---------------------------------------------------
The K197 does not send the decimal value in the desired unit directly to the IEEE board. Instead, it will send what appears to be a 21 bit raw count value, proportional but different from the displayed value (disregarding the decimal point):

When the display show "000000", the raw count is also 0x0. When the display show "400000" which is the highest number possible (REL mode), the raw count would be 0x200000 or 2097152 decimal (this is an extrapolation, the highest value that can be sent is actually 0x1FFFFF).

In principle any value x in between can be calculated with the formula x = raw_count * 400000 / 2097152. In practice care should be taken that the formula does npot result in a loss of precision, in particular when integer or single point math must be used (as is the case with the AVR).

The decimal point can be inferred from the range (which is provided as an integer from 1 to 7). The sign is included in a separate bit.

The library includes a numkber of functions to access the value as different formats. This includes a logarithmic format with separate integers for characteristic and mantissa. It is also possible to access the value as a double, albeit on the AVR this is the same as float and has just enough precision to hold the measurement value.

The careful reader will notice that the raw count has a higher resolution than what the voltmeter is displaying. The interesting part is that this value is changing in a way that suggests the voltmeter is measufing with a higher resolution than what is displayed. In order to investigate this, the library implements a set of "Extended Resolution" functions that can provide a value with 2 additional significant digits. 

Does this mean that now we have a 7 1/2 digit voltmeter? No, most definitely no. The voltmeter is almost certainly not designed to have the precision required for more than 220000 counts. In particular the voltage reference is not good enough, and there may be other limiting factors (e.g. noise level). And yet, while the additional counts are not available over the IEEE488 card, they are used by the analog output option when configured in the X1000 mode. The IEEE488 manual states explicitly that the X1000 mode extends the resolution of the Model 197 beyond the 5 1/2 digits of the display. It goes on claiming that the extra resolution allows for a more continuos output when high resolution is required. This suggests that there could be cases where having an extended resolution could be beneficial.   



 




