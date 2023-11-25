# Gemini protocol Layer
Description: The framing layer divides the continuous bitstream into frames and sub-frames to facilitate data transmission. Frames may contain optional synchronization sequences to aid in frame detection.

## how the gemini protocol works
The gemini layer is responsible for exchanging bits between two peers. Normally one peer is configured to initiate the communication and is the one controlling when the communication starts and ends. 

When the initiator want to send a bit to the other side, it will set the outup pin high, then set it according to the bit value and wait for an acknowledgement ((up to a maximum time, then it will time-out)). When the other peer detects a positive edge, it will wait for a predetermined setup time, read the value of the input pin and acknowledge generating a positive edge on its own output pin. The acknowledgment is also used to send a bit in the opposite direction in the same way.

When sending a 0 bit, a short pulse is generated on the output pin. The duration of the pulse should be significantly shorter than the setup time, otherwise it could be misunderstood for a 1 level. However it should be long enough to be detected by the other peer (for the K197, it should be at least 1 microsecond long to be detected reliably).

The K197 is always the initiator.

# Gemini Frame Layer

Description: The framing layer divides the continuous bitstream into frames and sub-frames to facilitate data transmission. Frames may contain optional synchronization sequences to aid in frame detection.

## Frame Structure

A frame consists of the following components:

| Component                   | Description                                          |
|-----------------------------|------------------------------------------------------|
| Synchronization Sequence    | Optional, composed of zero bits (0x00)              |
| Sub-Frames                  | Optional, contains data and start bit (described below)       |
| Synchronization Sequence    | Optional, composed of zero bits (0x00)              |

All components are optional, but obviously at least one component must be present for a frame to exist

Timeout (t_frame): if no data is sent within a t_frame time interval (frame timout), the next bit will be considered as the start of a new frame


## Sub-Frame Structure

A sub-frame contains the following components:

| Component            | Description                        |
|----------------------|------------------------------------|
| Start Bit (1)        | Indicates the start of a sub-frame |
| Data (8 bits)        | Contains 8 bits of data            |

## Frame Synchronization

- The synchronization sequence (if used) consists of zero bits and is placed at the beginning of the frame.
- Its purpose is to aid in frame detection and synchronization. It is also used when the number of sub-frames in the two directions are different, due to the fact that the number of bits sent in each direction must be the same (for each bit sent in one direction there is a corrresponding bit sent in the opposite direction). 

The K197 add a synchronization sequence at the beginning of the communication, the IEEE488 card may add synchronization sequences at the end of the communication 

## Data Encoding in Sub-Frames

- Data bytes are encoded in sub-frames using MSB (Most Significant Bit) first and LSB (Least Significant Bit) last.

## Example Frame

Here's an example of a frame structure with a synchronization sequence:

| Component                   | Value (Hex) |
|-----------------------------|-------------|
| Synchronization Sequence    | 0x00 0x00   |
| Sub-Frame 1                | 0x81 [Data] |
| Sub-Frame 2                | 0x42 [Data] |
| ...                         | ...         |
| Sub-Frame N                | 0xF5 [Data] |

## Notes

- For generic use (anything other than contro0lling the K197 voltmeter), the use of synchronization sequences can vary depending on the application requirements.
- t_frame is a timeout value that determines when a new frame starts if no bits are sent within it.
- Make sure to adjust the frame and sub-frame structure according to the specific needs of your application.

--------------------------------------------------------------------------------
# K197 control protocol layer - Sending Measurement Results

Description: This section outlines how measurement results and commands are encoded and transmitted. 

## Retrieving measurement results from the K197

Each measurement consists of 4 bytes, where each byte contains data from bit7 (MSB) to bit0 (LSB). When a new measurement rsult is available, the K197 will attempt to send it to the IEEE 488 bord using the gemini framing layer.

When the K197 is measuring continuosly, it will send about 3 measurements every second. When the K197 is waiting for a trigger, an empty frame is sent instead (this is done so the IEEE 488 card can trigger the measurement and/or sned any other command to the K197). This emtpty frame is sent more frequently than 3 times a second, in this way the IEEE 488 can can trigger a measurement more quickly (see next session).

The format of the measurement data is described here: https://github.com/alx2009/K197Control/blob/main/K197control_measurements.md

## Sending Commands to the K197

Commands are always sent in a 5 bytes structure, where each byte contains data from bit7 (MSB) to bit0 (LSB). The structure allow buffering different command in the same 5 byte structre before they are sent to the K197. In this way the voltmeter can be completely reconfigured and triggered sending a single frame. 

When a command structure is ready to be sent (execute command on the IEEE 488 bus), the IEEE 488 card must wait until a new frame is started by the K197. The frame with the command then can be sent. It must be sent without any initial synchronization sequence.

The format of the control data is described here: https://github.com/alx2009/K197Control/blob/main/K197control_commands.md

