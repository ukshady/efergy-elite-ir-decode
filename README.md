# efergy-elite-ir-decode
Decodes frames from the Efergy Elite IR (CA)

## Usage

rtl_fm -f 433500000 -p -s 200000 -r 96000 -g 19 | ./elite-ir-decode -

## Format

* FSK
* Carrier (~7ms) approx. 19.2 kHz
* Preamble
** -ve level with 2 spikes (?) (~4ms)
** +ve level for (~1ms)
* Data (12 bytes)
** +ve level about the same as carrier +ve level = 0 (?)
** +ve level about double carrier +ve level = 1 (?)
* End message
** -ve level for about (~1ms)

