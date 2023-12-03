# K197Control

This is an Arduino library to control a K197 voltmeter with the internal interface intended for the IEEE488 option. The communication is over a two wire bus (one wire for each direction).

The lower layers of the protocol could even be used as a generic two wire communication protocol between two microcontrollers (only one input pin supporting edge interrupt + one output pin is needed on each microcontroller). 

At the moment the library only supports Arduino Uno and other boards with a compatible microcontroller.

## DISCLAIMER: Please note that the purpose of this repository is educational. Any use of the information for any other purpose is under own responsibility.

# Installation and use

Please note the disclaimer above. These instructions explain how to download the library and compile under the Arduino IDE. Any other use is your own responsibility.

From the GitHub repository (https://github.com/alx2009/K197Control), select "Releases", decide what release you want and download the .zip file with the source code for that release. Then follow the instructions on how to import a .zip library here: https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries#importing-a-zip-library

Once the library is installed, the example programs can be found in the Arduino IDE under File > Examples > K197control

- The K197Probe example demonstrates how to probe the interface at low level, displaying raw frame data.
- The K197ControlDataLogger example demonstrates how to log measurements results to Serial
- The K197DataAcquisition example is similar to K197ControlDataLogger but in addition it can send command to the voltmeter, including setting trigger mode and overriding the range.

In case you want to understand how the K197 comunicates with the programs (e.g. to modify the library or create your own), the protocol specification can be found here: https://github.com/alx2009/K197Control/blob/main/K197control_protocol_specification.md 

## Test setup

Please note the disclaimer above. This section documents how the examples have been tested. Any other use is your own responsibility.

The following picture shows the test setup with the pin configuration in the examples. If you change the pin definitions in the example you will need to connect accordingly. Note however that on Arduino Uno only pin 2 or 3 can be defined as INPUT_PIN.

![K197control_uno_composite](https://github.com/alx2009/K197Control/assets/100997527/5ead693a-ab44-4873-83b2-d300976670d0)

- Connect the second terminal in the IEEE connector of the K197 with the Arduino Uno GND pin (black wire in the above picture)
- Connect the third terminal in the IEEE connector of the K197 with the Arduino Uno I/O pin no. 3 (orange wire in the above picture). In the examples pin 3 is defined as OUTPUT_PIN.
- Connect the fourth terminal in the IEEE connector of the K197 with the Arduino Uno I/O pin no. 2 (blue wire in the above picture). In the examples pin 2 is defined as INPUT_PIN.

The first terminal of the IEEE connector is the one that is closest to the power transformer (see picture above).

Important: As shown in the picture, the IEEE card must be removed in order to test this library. Failure to do so may result in damage to the voltmeter and/or the IEEE card and/or the Arduino Uno. 

## Known Limitations

This is a very early release, developed at the same time the protocol was reverse engineered using a IEEE 488 card (mod. 1972A) and one voltmeter (mod. 197A, SW revision level A01). It may or may not work elsewhere, e.g. due to different firmware revision. 

Only a limited amount of tests has been performed. 

The API is not fully consistent and may have to be changed in future revisions. I also expect that there are many bugs lingering around.

Status report and calibration have not been reverse engineering. Reading stored data is not working.

The library does not implement the acknowdlege timeout. This means that if the K197 does not acknowledge a bit the library will hang. In practice this is rare as the K197 seems to be very well behaved.  

# Background information

This library was inspired from discussions in the EEVBlog forum: https://www.eevblog.com/forum/testgear/keithley-197a-owners_-corner/ and https://www.eevblog.com/forum/projects/replacement-display-board-for-keithley-197a/msg4232665/#msg4232665

Some forum members suggested to use the IEEE488 internal interface to control the voltmeter via bluetooth (inside the voltmeter there is a 6 pin connector used by an optional IEEE488 interface board). 
Using the IEEE interface instead of the display interface offers more possibilities to control the voltmeter, for example it is possible to select the range bypassing the front panel switches (albeit this is not possible in Ampere mode. Switching between AC and DC can only be done via fron panel switch). With the help of a forum member who lended me a IEEE488 card, I managed to reverse engineer the protocol used. This library and the associated examples implement this reverse engineered protocol.

The protocol used by the K197 to communicate with the IEEE board uses two wires, one for each direction. As far as I know it is a unique protocol (at least, I never encountered it before). If you know of other applications/instruments using a similar protocol please comment in one of the forum threads linked above.

The K197 voltmeter was designed at the time where hardware support for serial communication was rarely integrated in generic I/O peripherals. Rather than using a dedicated IC (e.g. a UART), the designers implemented an ad-hoc, full duplex protocol using the few HW resources available. In addition to the input and output pins, the only other HW resource required is an edge sensitive interrupt.

To understand the protocol, it is useful to think of three layers (this is somewhat arbitrary but bear with me for now), which I dubbed gemini, gemini frame and K197 control respectively:
- The gemini protocol is the lowest layer and is responsible to send a stream of bits between two peers. For every bit transmitted in one direction there is a bit transmitted in the opposite direction.
- The gemini frame sits on top of gemini and is responsible for packing/unpacking byte sequences into frames which are sent/received using the lower layer
- The K197 control protocol encodes commands (to the K197) or measurement results (from the K197) as byte sequences that are sent or received using the gemini frame protocol

The above model is useful to understand the protocol and the library class hierarchy, obviously the original designers of the protocol might have had a completely different model in mind.

## The gemini protocol in a nutshell

Let's call Bob and Alice the two peers that are comunicating via the gemini protocol. When nothing need to be transmitted, Bob and Alice will keep their output pins LOW, while monitoring for a positive edge on their input pins (idle state). When Bob wants to send a bit to Alice, he will set the outup pin high, then he will set it according to the bit value and wait for an acknowledgement from Alice (up to a maximum time, then it will time-out).

When Alice detects a positive edge, she will wait for a predetermined setup time and then read the value of the input pin. As soon as she has got the bit value, she will acknowledge generating a positive edge on its own output pin.  

But this is not all. The acknowledgment is used to send a bit in the opposite direction using exactly the same logic. Immediately after Alice sets the output pin high to acknowledge the reception, she has two choices: if she does not have any data to send, she will return the output pin to the idle state (LOW). If she has data to send, she will set the output pin according to the bit value and wait for an acknowledgement from Bob (which is also including a bit to alice and so forth).

But wait a minute, doesn't this result in an endless sequence of transmissions? Indeed, my first apish attempts resulted in a never ending back and forth transmission. The way it works is that the peer that initiates the communication is also the one that controls its end (by not replying to the last acknowledgement). In this way, for every bit transmitted in one direction there is a bit transmitted in the opposite direction. 

Note that there is a risk that Alice and Bob initiate a transmission at the same time. While this could be handled in a number of ways, the K197 appears to solve it by not allowing the IEE488 board to initiate the communication "out of the blue". The K197 is always the one initiating the communication, the IEEE488 board can only send a bit when acknowledging a bit from the K197. 

### Timing
When sending a 0 bit, a short pulse is generated on the output pin. The duration of the pulse should be significantly shorter than the setup time, otherwise it could be misunderstood for a 1 level. However it should be long enough to be detected by the other peer. Looking at the datasheet for the 6821 Peripheral Interface Adapter used in the K197, the minimum duration is around 1 us (microsecond). However the pulse lenght observed is between 15 and 30 us, while the setup time is around 200 us. Sending a complete frame takes around 40 ms (milliseconds).

The examples provided with the library replicate the timing observed with the IEEE card, but this can be changed. The pulse duration can be reduced to a few us without apparent issues. The setup time can also be reduced but this hasn't been tested with the K197, yet. 

## The gemini frame protocol

The gemini frame protocol packs and unpacks sequence of bytes in frames. A frame is composed of sub-frames and synchronization sequences. Each sub-frame is composed by 9 bits: a start bit (set to 1) and 8 data bits encoding a byte of data. Any consecutive 0 bits outside a sub-frame are synchronization sequences. Both the sub-frame rapresenting the bytes and the bits within a subframe are sent MSB first.

In principle there are a number of ways to detect a frame boundary:
- detect an initial synchronization sequence with 9 or more consecutive zeroes (this seems to be the case for the K197 but I have not used this method in the library).  
- rely on a frame timeout period. Once the communication is idle for more than the frame timeout period, the next bit will start a new frame (this is the method implemented in the library to detect the frame start).
- use frames of known lenght. Once all the bytes of a frame have been received we know that the frame reception is complete (this is also supported by the library to detect the frame end, so that we can process it as soon as possible)

When sending a frame, if an acknowledgment timeout is detected by the lower layer the frame layer will assume that the peer is unavailable and skip the rest of the frame. If an incomplete frame has been received the entire frame must be ingnored. 

Note that it is possible to send an empty frame (a frame consisting of a single synchronization sequence and no data). In fact, when the K197 is not measuring (e.g. due to the trigger mode) it will send empty frames to allow the IEE488 card to send commands in the opposite direction. 

## The K197 control protocol

As anticipated, the K197 is always the initiator of the communication. When it is triggered, it will attempt to send the measurement result encoded in a frame including an initial synchronization sequence of 16 '0' and  4 sub-frames with 4 bytes of data. When the K197 is not triggered, it will still periodically "poll" the IEEE board sending an empty frame.

In both cases, the K197 will monitor the first bit sent by the K197 card.
- if the acknowledge times out, the K197 assumes there is no IEE488 card connected or the card is busy and cannot answer at this time.
- If the first bit received is zero, it will interprete it as if the IEEE488 board does not have any data to send. If  the K197 doesn't have a new measurement to send (e-g- depending on the trigger mode) the frame will only include one synchronization bit and no data.
- If the first bit is one, it will understand that the K197 board has commands to send to the K197, and the first bit is the start bit of the first sub-frame. In such a case, the K197 will send a larger frame (possibly empty), so that the IEEE board can send the entire command back to the K197.

For the coding of the bits please refer to the protocol specification (https://github.com/alx2009/K197Control/blob/main/K197control_protocol_specification.md)

### Limitations

Not all aspects and fields have been reverse engineered at this point. In particular, calibration is missing completely and retrieving stored data is not working.  

### Calculating the measurement result

The K197 send the measurement result as a 21 bit unsigned binary number which is proportional but different from the measured value. 

Some definitions used in the following: 
- measurement value is the value of the measurement, including sign and decimal point. 
- measurement result is the complete measurement result, including unit, AC mode, overrange indicators and measurement value
- display value is the absolute value of the measurement value, without sign and decimal point 
- binary count (or count in short) is a 21 bit unsigned binary number sent by the K197. The binary count is proportional to the display value (see below). 

When the display value is 0, the binary count is 0x0. When the display value is "400000", the binary count would be 0x200000 or 2097152 decimal (this is an extrapolation, the highest value that can be sent is actually 0x1FFFFF).

In principle any value x in between can be calculated with the formula x = binary_count * 400000 / 2097152. In practice care should be taken in the calculation to avoid loss of precision, in particular when 32 bit integer or floating point math must be used (as is the case with the AVR where double are the same as float).

The decimal point can be inferred from the range (which is provided as an integer from 1 to 7). The sign is included in a separate bit.

The library includes a number of functions to access the measurement value as different data types. This includes a logarithmic format with separate integers for characteristic and mantissa. It is also possible to access the value as a double, albeit on the AVR this is the same as float and has just enough precision to hold the measurement value.

#### Enhanced Resolution

At this point, an alert reader will notice that the binary count has a higher resolution than what the voltmeter is displaying. In order to experiment with this, the library implements a set of "Enhanced Resolution" (ER) functions that can provide a value with 2 additional significant digits. 

Does this mean that we have a 7 1/2 digit voltmeter now? No, most definitely not. The voltmeter is not designed to have the accuracy required for more than 220000 counts. In particular the voltage reference is not good enough, and there may be other limiting factors (e.g. noise level). And yet, while the additional counts are not available digitally, they are used by the analog output option when configured in the X1000 mode. The IEEE488 manual states explicitly that the X1000 mode extends the resolution of the Model 197 beyond the 5 1/2 digits of the display. It goes on claiming that the extra resolution allows for a more continuos output when high resolution is required. This suggests that there could be use cases where having an enhanced resolution could be beneficial, so the library provides the needed support to experiment with this concept.




 




