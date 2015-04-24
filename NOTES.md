# Last 2 data bytes vs meter

|  9  | 10  | Meter reading (kWh) |
| --- | --- | ------------------- |
| 1   | 16  | 0.224               |
| 2   | 16  | 0.239               |
| 3   | 15  | 0.359               |
| 4   | 15  | 0.479               |
| 5   | 15  | 0.599               |
| 6   | 15  | 0.719               |
| 7   | 15  | 0.839               |
| 8   | 15  | 0.959               |
| 11  | 15  | 1.319               |
| 12  | 15  | 1.439               |

First 8 bytes (for me) are always: 9 186 218 64 0 0 0 0

Bytes 11 and 12 are a CRC16 XMODEM over the first 10 bytes

