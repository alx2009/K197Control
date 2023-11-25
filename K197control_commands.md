# Gemini control protocol, commands 
Commands are sent from the IEEE488 to the K197

## Measurement Data Format

A command structure is represented as follows:

| Byte | Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
|------|-------|-------|-------|-------|-------|-------|-------|-------|
| B0   |       |       |       |       |       |       |       |       |
| B1   |       |       |       |       |       |       |       |       |
| B2   |       |       |       |       |       |       |       |       |
| B3   |       |       |       |       |       |       |       |       |
| B4   |       |       |       |       |       |       |       |       |

- Each byte (B0 to B3) represents a part of the command data.
- Bit 7 is the Most Significant Bit (MSB), and Bit 0 is the Least Significant Bit (LSB).

## Encoding and Transmission

Commands are transmitted within sub-frames as part of the data payload. The protocol encodes and transmits each byte of the measurement data in a series of sub-frames, with each sub-frame containing one byte of data.

For example, to transmit a command:

- Byte B0 is transmitted in a sub-frame with the start bit (1) and the 8 data bits representing B0.
- Byte B1 is transmitted in the next sub-frame, and so on.
- This continues until the complete frame with all 5 bytes are transmitted, each byte in its own sub-frame.

## Example

Here's an example of how a measurement result is encoded and transmitted within sub-frames:

| Sub-Frame | Start Bit (1) | Data (8 bits) |
|-----------|---------------|---------------|
| 1         | 1             | B0            |
| 2         | 1             | B1            |
| 3         | 1             | B2            |
| 4         | 1             | B3            |
| 5         | 1             | B4            |

-----------------------------------------------------------------------------------
# Command Data - B0 Encoding

Description: This section explains how Byte B0 of the measurement result is encoded. B0 contains various parameters, including the measurement unit, AC/DC, relative measurement, and measurement range.

## Byte B0 Format

Byte B0 is structured as follows:

| Bit 7-6 (Db) || Bit 5-4 (Relative) || Bit 3-0 (Range) ||
|--------------|--------------------|-----------------||
| Set dB       | dB                 | Set Relative    | Relative          | Set Range        | Range          |

- Bits 7 and 6 (Unit) together represent the measurement unit, as follows:
  - 00: Volt
  - 10: Ampere
  - 01: Ohm
  - 11: Decibel (dB)

- Bit 5 (AC/DC) represents the AC/DC nature of the measurement:
  - 0: DC (Direct Current)
  - 1: AC (Alternating Current)

- Bit 4 (Undefined) is undefined and reserved for future use.

- Bit 3 (Relative) indicates a relative measurement when set to 1.

- Bits 2-0 (Range) indicate the measurement range and can assume values from 1 to 6.

## Encoding of Measurement Unit

The measurement unit in Byte B0 is encoded using bits 7 and 6 (Unit), as described earlier.

## AC/DC Nature

Bit 5 (AC/DC) indicates whether the measurement is AC or DC. When it is set to 0, the measurement is DC; when set to 1, the measurement is AC.

## Relative Measurement

Bit 3 (Relative) indicates a relative measurement when set to 1.

## Measurement Range

Bits 2-0 (Range) specify the measurement range and can assume values from 1 to 6.

## Example

Here's an example of Byte B0 with various parameters set:

| Bit 7-6 (Unit) | Bit 5 (AC/DC) | Bit 4 (Undefined) | Bit 3 (Relative) | Bit 2-0 (Range) |
|----------------|---------------|-------------------|------------------|----------------|
| 0              | 1             | 0                 | 1                | 101            |

In this example:
- The measurement unit is Ampere (01).
- The measurement is AC (Alternating Current).
- Bit 4 is undefined.
- It is a relative measurement.
- The measurement range is 5.

## Notes

- Undefined bits should be ignored (seems to be always set to 1).

# Measurement Result Data - B1 Encoding

Description: This section explains how Byte B1 of the measurement result is encoded. B1 contains information about the sign of the measurement, an overrange flag, and the 5 most significant bits of the display count.

## Byte B1 Format

Byte B1 is structured as follows:

| Bit 7 (Sign) | Bit 6 (Undefined) | Bit 5 (Overrange) | Bit 4-0 (Display Count MSB) |
|--------------|-------------------|-------------------|-----------------------------|
| Sign         | Undefined         | Overrange         | Display Count MSB           |

- Bit 7 (Sign) represents the sign of the measurement:
  - 1: Negative
  - 0: Positive

- Bit 6 (Undefined) is undefined and should be set to 1.

- Bit 5 (Overrange) is the overrange flag, indicating an overrange condition when set to 1.

- Bits 4-0 (Display Count MSB) represent the 5 most significant bits of the display count.

## Sign of the Measurement

Bit 7 (Sign) indicates whether the measurement is positive or negative:
- 1: Negative
- 0: Positive

## Undefined Bit

Bit 6 (Undefined) is undefined and should be set to 1.

## Overrange Flag

Bit 5 (Overrange) serves as an overrange flag. It is set to 1 to indicate an overrange condition.

## Display Count MSB

Bits 4-0 (Display Count MSB) represent the 5 most significant bits of the display count.

## Example

Here's an example of Byte B1 with various parameters set:

| Bit 7 (Sign) | Bit 6 (Undefined) | Bit 5 (Overrange) | Bit 4-0 (Display Count MSB) |
|--------------|-------------------|-------------------|-----------------------------|
| 1            | 1                 | 0                 | 11010                       |

In this example:
- The measurement is negative.
- Bit 6 is undefined and set to 1.
- There is no overrange condition (Bit 5 is 0).
- The 5 most significant bits of the display count are 11010.

# Measurement Result Data - B2 and B3 Encoding

Description: This section explains how Bytes B2 and B3 of the measurement result encode the least significant bits of the display count.

## Byte B2 and B3 Format

Bytes B2 and B3 are structured as follows:

### Byte B2:

| Bit 7-0 (Display count LSB) |
|-----------------------------|
| Display count bits 8-15     |

- Bits 7-0 (Display count LSB) represent bit 8-15 of the display count.

### Byte B3:

| Bit 7-0 (Display count LSB) |
|-----------------------------|
| Display count LSB           |

- Bits 7-0 (Display count LSB) represent the 8 least significant bits of the display count.

## Display count LSB

Bytes B2 and B3 together represent the 16 least significant bits of the display count. They contain the least significant bits of the display count value.

## Example

Here's an example of Bytes B2 and B3 with display count LSB values:

### Byte B2:

| Bit 7-0 (Display count LSB) |
|-----------------------------|
| 11011010                    |

### Byte B3:

| Bit 7-0 (Display count LSB) |
|-----------------------------|
| 00101101                    |

In this example, B2 and B3 together encode the 16 least significant bits of the display count.

# Measurement Result Data - B0 to B3 Encoding and Value Calculation

Description: This section explains how to obtain the measurement value from Bytes B0 to B3 of the measurement result.

## Byte B0 to B3 Format

Bytes B0 to B3 are structured as follows:

### Byte B0:

| Bit 7-6 (Unit) | Bit 5 (AC/DC) | Bit 4 (Undefined) | Bit 3 (Relative) | Bit 2-0 (Range) |
|----------------|---------------|-------------------|------------------|----------------|
| Unit           | AC/DC         | Undefined         | Relative         | Range          |

### Byte B1:

| Bit 7 (Sign) | Bit 6 (Undefined) | Bit 5 (Overrange) | Bit 4-0 (Display Count MSB) |
|--------------|-------------------|-------------------|-----------------------------|
| Sign         | Undefined         | Overrange         | Display Count MSB           |

### Byte B2:

| Bit 7-0 (Display Count LSB) |
|-----------------------------|
| Display Count LSB           |

### Byte B3:

| Bit 7-0 (Display Count LSB) |
|-----------------------------|
| Display Count LSB (big endian)|

## Procedure to Obtain Measurement Value

To calculate the measurement value from Bytes B0 to B3, follow these steps:

1. Extract the measurement unit from Byte B0 using bits 7 and 6. The unit is determined as follows:
   - 00: Volt
   - 10: Ampere
   - 01: Ohm
   - 11: dB

2. Check Bit 5 of Byte B1 (Overrange flag). If it is set (1), an overrange condition is indicated.

3. Extract the sign of the measurement from Bit 7 of Byte B1. If it is set (1), the measurement is negative; otherwise, it is positive.

4. Combine the 16-bit Display Count value from Bytes B2 and B3. These bytes contain the least significant bits of the Display Count.

5. The Display Count represents a 21-bit binary number that encodes an unsigned long integer within the range 0 to 2,097,152 (corresponding to 0 to 400,000 in the voltmeter's display, ignoring sign and decimal point).

6. Apply the measurement unit to the Display Count value based on the unit extracted in step 1. This gives you the raw measurement value.

7. Depending on the range (Bit 2-0 in Byte B0), scale the raw measurement value by dividing or multiplying it by the appropriate power of 10. The exact scaling factor should be determined based on your protocol's specifications.

8. If the overrange flag is set (step 2), handle the overrange condition according to your protocol's specifications.

9. If the sign bit is set (step 3), negate the measurement value obtained in step 7 to account for the negative sign.

10. The result obtained in step 9 represents the final measurement value, scaled to the appropriate unit and range.

## Resolution Note

Please note that the Display Count provides slightly better resolution compared to what is displayed by the voltmeter. However, the extra resolution is not significant because it is smaller than the measurement error according to the voltmeter's specifications (more experimentation is needed to understand if there are special cases where the extra resolution could be significant).

## Example

Here's an example calculation using Bytes B0 to B3:

- Byte B0: 00000101 (Unit = Ohm)
- Byte B1: 01000000 (Positive, No Overrange)
- Byte B2: 11011010 (Display Count LSB)
- Byte B3: 00101101 (Display Count LSB)

Procedure:
- Unit: Ohm (from B0)
- Overrange: No (from B1)
- Sign: Positive (from B1)
- Display Count: 1101101000101101 (from B2 and B3)
- Scaling Factor: Determined by range and protocol specifications

Final Measurement Value: 87021 Ohms (Scaled and considering unit and sign)
