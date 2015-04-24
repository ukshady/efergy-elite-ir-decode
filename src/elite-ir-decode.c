#include<stdio.h>
#include<stdint.h>
#include<string.h>

enum Mode
{
    None = 0,
    NegPreamble = 1,
    Decode = 2
};

// Data about a message from the Elite IR
struct msgdata
{
    int16_t n_up;       // current number of samples >0
    int16_t last_n_up;  // number of samples >0 since the last sample <0
    int16_t n_down;     // current number of samples <0

    // buffer of current message
    uint8_t buf[20];

    uint8_t byte;   // number of bytes so far
    uint8_t bit;    // current bit to write

    // 0 = preamble not found
    // 1 = found 100+ -ve
    // 2 = preamble found, start decoding
    uint8_t mode;
};

uint16_t crc_table[256];

void init_crc_table()
{
    const uint16_t poly = 0x1021U;

    for(int i = 0; i < 256; ++i)
    {
        uint16_t v = 0;
        uint16_t t = i << 8;
        
        for(int j = 0; j < 8; ++j)
        {
            if(((v ^ t) & 0x8000U) )
            {
                v = ((v << 1) ^ poly);
            }
            else
            {
                v <<= 1;
            }
            
            t <<= 1;
        }
        
        crc_table[i] = v;
    }
}

uint16_t crc16xmodem ( uint8_t * d, int len )
{
    uint16_t crc = 0;
    
    for ( int i = 0; i < len; ++i )
    {
        crc = (uint16_t)( ( crc << 8 ) ^ crc_table[( ( crc >> 8 ) ^ d[i] ) & 0xff] );
    }
    
    return crc;
}

int get_sample ( FILE * f, int16_t * val )
{
    int v1 = fgetc ( f );
    int v2 = fgetc ( f );

    if ( v1 == EOF || v2 == EOF ) return 0;

    *val = ( v1 & 0xff ) | ( ( v2 & 0xff ) << 8 );

    return 1;
}

int main( int argc, char *argv[] )
{
    if ( argc < 2 || strlen ( argv[1] ) == 0 )
    {
        fprintf ( stderr, "Missing input\n" );
        return -1;
    }

    FILE * f = 0;

    if ( argv[1][0] == '-' && strlen ( argv[1] ) == 1 )
    {
        f = stdin;
    }
    else
    {
        f = fopen ( argv[1], "r" );
    }

    if ( !f )
    {
        fprintf ( stderr, "Failed to open input\n" );
        return -1;
    }

    init_crc_table();

    int16_t s = 0;

    struct msgdata d;
    memset ( &d, 0, sizeof ( d ) );

    while ( get_sample ( f, &s ) )
    {
        if ( s > 0 )
        {
            ++d.n_up;
            d.n_down = 0;
        }
        else if ( s < 0 && d.n_up > 0 )
        {
            // Negative edge

            d.last_n_up = d.n_up;
            d.n_up = 0;
            
            ++d.n_down;
            
            if ( d.mode == NegPreamble )
            {
                if ( d.last_n_up > 40 )
                {
                    // Preamble found
                    d.mode = Decode;
                    d.last_n_up = 0;
                }
                else
                {
                    // False alarm, just garbage, clear everything
                    memset ( &d, 0, sizeof ( d ) );
                }
            }
            else if ( d.mode == Decode )
            {
                // MSB
                if ( d.last_n_up > 3 && d.last_n_up < 10 )
                {
                    ++d.bit;
                    printf ( "0" );
                }
                else if ( d.last_n_up > 10 )
                {
                    d.buf[d.byte] |= ( 0x80U >> d.bit );
                    ++d.bit;
                    printf ( "1" );
                }

                if ( d.bit > 7 )
                {
                    ++d.byte;

                    d.bit = 0;

                    if ( d.byte > 12 )
                    {
                        printf ( "\nError, unknown format (byte>12), ignoring.\n\n" );

                        memset ( &d, 0, sizeof ( d ) );
                    }
                    else
                    {
                        printf ( " [%2d] = %3d(0x%02x)\n", d.byte - 1, d.buf[d.byte-1], d.buf[d.byte-1] );
                    }
                }
            }
        }
        else if ( s < 0 )
        {
            // Negative level

            ++d.n_down;

            if ( d.mode == None && d.n_down > 50 )
            {
                d.mode = NegPreamble;
            }
            // 40+ samples down while in decode mode followed by up = end of message
            else if ( d.mode == Decode && d.n_down > 40 )
            {
                if ( d.byte == 12 && d.bit == 0 )
                {
                    for ( int bx = 0; bx < d.byte; ++bx )
                    {
                        printf ( "%3d(0x%02x) ", d.buf[bx], d.buf[bx] );
                    }

                    const uint16_t rcv_csum = d.buf[10] << 8 | d.buf[11];
                    const uint16_t csum = crc16xmodem ( d.buf, 10);

                    if ( csum == rcv_csum )
                    {
                        printf ( "\nCRC OK\n" );

                        // TODO decode kWh
                    }
                    else
                    {
                        printf ( "\nCRC BAD, (expected) %04x != (buf) %04x\n", csum, rcv_csum );
                    }
                }

                memset ( &d, 0, sizeof ( d ) );
            }
        }
    }

    return 0;
}
