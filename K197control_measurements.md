# Gemini control protocol, measurement data 
Measurement data is sent from the K197 to the IEEE488 card

## Measurement Data Format

A single measurement result is represented as follows:

| Byte | Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
|------|-------|-------|-------|-------|-------|-------|-------|-------|
| B0   |       |       |       |       |       |       |       |       |
| B1   |       |       |       |       |       |       |       |       |
| B2   |       |       |       |       |       |       |       |       |
| B3   |       |       |       |       |       |       |       |       |

- Each byte (B0 to B3) represents a part of the measurement data.
- Bit 7 is the Most Significant Bit (MSB), and Bit 0 is the Least Significant Bit (LSB).

## Encoding and Transmission

Measurement results are transmitted within sub-frames as part of the data payload. The protocol encodes and transmits each byte of the measurement data in a series of sub-frames, with each sub-frame containing one byte of data.

For example, to transmit a measurement result:

- Byte B0 is transmitted in a sub-frame with the start bit (1) and the 8 data bits representing B0.
- Byte B1 is transmitted in the next sub-frame, and so on.
- This continues until the complete measurement frame is sent, with each of the 4 bytes in its own sub-frame.

## Example

Here's an example of how a measurement result is encoded and transmitted within sub-frames:

| Sub-Frame | Start Bit (1) | Data (8 bits) |
|-----------|---------------|---------------|
| 1         | 1             | B0            |
| 2         | 1             | B1            |
| 3         | 1             | B2            |
| 4         | 1             | B3            |

-----------------------------------------------------------------------------------
# Measurement Result Data - B0 Encoding

Description: This section explains how Byte B0 of the measurement result is encoded. B0 contains various parameters, including the measurement unit, AC/DC, relative measurement, and measurement range.

## Byte B0 Format

Byte B0 is structured as follows:

| Bit 7-6 (Unit) | Bit 5 (AC/DC) | Bit 4 (Undefined) | Bit 3 (Relative) | Bit 2-0 (Range) |
|----------------|---------------|-------------------|------------------|----------------|
| Unit           | AC/DC         | Undefined         | Relative         | Range          |

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

| Bit 7 (Sign) | Bit 6 (Undefined) | Bit 5 (Overrange) | Bit 4-0 (Binary Count MSB) |
|--------------|-------------------|-------------------|-----------------------------|
| Sign         | Undefined         | Overrange         | Binary Count MSB           |

- Bit 7 (Sign) represents the sign of the measurement:
  - 1: Negative
  - 0: Positive

- Bit 6 (Undefined) is undefined and should be set to 1.

- Bit 5 (Overrange) is the overrange flag, indicating an overrange condition when set to 1.

- Bits 4-0 (Display Count MSB) represent the 5 most significant bits of the binary count.

## Sign of the Measurement

Bit 7 (Sign) indicates whether the measurement is positive or negative:
- 1: Negative
- 0: Positive

## Undefined Bit

Bit 6 (Undefined) is undefined and should be set to 1.

## Overrange Flag

Bit 5 (Overrange) serves as an overrange flag. It is set to 1 to indicate an overrange condition.

## Display Count MSB

Bits 4-0 (Display Count MSB) represent the 5 most significant bits of the binary count.

## Example

Here's an example of Byte B1 with various parameters set:

| Bit 7 (Sign) | Bit 6 (Undefined) | Bit 5 (Overrange) | Bit 4-0 (binary Count bits 20-16) |
|--------------|-------------------|-------------------|-----------------------------------|
| 1            | 1                 | 0                 | 11010                             |

In this example:
- The measurement is negative.
- Bit 6 is undefined and set to 1.
- There is no overrange condition (Bit 5 is 0).
- The 5 most significant bits (bits 20-16) of the binary count are 11010.

# Measurement Result Data - B2 and B3 Encoding

Description: This section explains how Bytes B2 and B3 of the measurement result encode the least significant bits of the display count.

## Byte B2 and B3 Format

Bytes B2 and B3 are structured as follows:

### Byte B2:

| Bit 7-0 (Binary count bits 15-8) |
|-----------------------------|
| Display count bits 15-8     |

- Bits 7-0 represent bits 8-15 of the binary count.

### Byte B3:

| Bit 7-0 (Binary count bits 7-0) |
|-----------------------------|
| Binary count bits 7-0           |

- Bits 7-0  represent the 8 least significant bits of the display count.

## Display count LSB

Bytes B2 and B3 together represent the 16 least significant bits of the binary count.

## Example

Here's an example of Bytes B2 and B3 with display count LSB values:

### Byte B2:

| Bit 7-0 (Display count  bits 15-8) |
|-----------------------------|
| 11011010                    |

### Byte B3:

| Bit 7-0 (Display count  bits 7-0) |
|-----------------------------|
| 00101101                    |

In this example, B2 and B3 together encode the 16 least significant bits of the display count, equal to:
- 1101101000101101 in base 2 notation (binary)
- DA2D in base 16 notation (hexadecimal)
- 55853 in base 10 notation (decimal)

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
| Sign         | Undefined         | Overrange         | Binary Count MSB           |

### Byte B2:

| Bit 7-0 (Binary Count bits 15-8) |
|-----------------------------|
| Binary Count bits 15-8           |

### Byte B3:

| Bit 7-0 (Binary Count bits 7-0) |
|-----------------------------|
| Binary Count bits 7-0      |

## Procedure to Obtain Measurement Value

To calculate the measurement value from Bytes B0 to B3, follow these steps:

1. If the overrange flag is set (step 2), the voltmeter is in overrange and you could skip the next steps.

2. Combine the 21 bit Binary Count value from Bytes B1, B2 and B3. The Binary Count represents a 21-bit binary number that encodes an unsigned long integer value within the range 0 to 2,097,152. This range corresponds to 0 to 400,000 in the voltmeter's display, ignoring sign and decimal point.

3. Calculate the display value with the formula display_value = binary_count * 400000 / 2097152 (alternatively use display_value = binary_count * 40000000 / 2097152 to extend the resolution to 2 more digits)

4. Extract the measurement unit from Byte B0 using bits 7 and 6. The unit is determined as follows:
   - 00: Volt
   - 10: Ampere
   - 01: Ohm
   - 11: dB

5. Depending on the range (Bit 2-0 in Byte B0) and measurement unit (step 1), scale the display value by dividing or multiplying it by the appropriate power of 10.

6. Extract the sign of the measurement from Bit 7 of Byte B1. If it is set (1), the measurement is negative; otherwise, it is positive.

7. IF Bit 3 of B0 is set the measurement is relative

## Resolution Note

Please note that the Display Count provides slightly better resolution compared to what is displayed by the voltmeter (see also step 3 above). However, the extra resolution is not significant because it is smaller than the measurement error according to the voltmeter's specifications (more experimentation is needed to understand if there are special cases where the extra resolution could be useful).

## Example

Here's an example calculation using Bytes B0 to B3:

- Byte B0: 00000101 (Unit = Ohm)
- Byte B1: 01000000 (Positive, No Overrange, binary count bits 20-16 = 0)
- Byte B2: 11011010 (Binary Count bits 15-8)
- Byte B3: 00101101 (binary Count bits 7-0)

Procedure:
- Unit: Ohm (from B0)
- Overrange: No (from B1)
- Sign: Positive (from B1)
- Display Count: 1101101000101101 (from B2 and B3)
- Scaling Factor: Determined by range and protocol specifications

Binary count = 55853. display count = 55853 * 400000 / 2097152 = 10653 (rounded to the next integer)

Final Measurement Value: 10653 Ohms (Scaled and considering unit and sign)

If we want to extend the resolution, the display value would be 55853 * 40000000 / 2097152 = 1065311 (rounded to the next integer), corresponding to 10653.11 Ohm when we consider unit and range


