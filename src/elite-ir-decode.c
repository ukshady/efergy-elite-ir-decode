#include<stdio.h>
#include<stdint.h>
#include<string.h>

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
    // 1 = preamble found, start decoding
    uint8_t mode;
};

int get_sample ( FILE * f, int16_t * val )
{
    int v1 = fgetc ( f );
    int v2 = fgetc ( f );

    if ( v1 == EOF || v2 == EOF ) return 0;

    *val = ( v1 & 0xff ) | ( ( v2 & 0xff ) << 8 );

    return 1;
}

int calc_checksum ( uint8_t * buf, uint8_t len )
{
    if ( len < 10 )
        return -1;

    int csum = 0;

    for ( int i = 0; i < 10; ++i )
    {
        csum += buf[i];
    }

    return ( csum % 256 );
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

    int16_t s = 0;

    struct msgdata d;
    memset ( &d, 0, sizeof ( d ) );

    while ( get_sample ( f, &s ) )
    {
        if ( s > 0 )
        {
            ++d.n_up;

            // 40+ samples down while in mode 1 = end of message
            if ( d.mode == 1 && d.n_down > 40 )
            {
                d.mode = 0;

                printf ( "\nbyte: %d, bit: %d\n", d.byte, d.bit );
                for ( int bx = 0; bx < d.byte; ++bx )
                {
                    printf ( "%3d(0x%02x) ", d.buf[bx], d.buf[bx] );
                }

                int csum = calc_checksum ( d.buf, d.byte );

                if ( csum < 0 )
                {
                    printf ( "\nChecksum BAD, bad length\n" );
                }
                else if ( csum == d.buf[10] )
                {
                    printf ( "\nChecksum OK\n" );
                }
                else
                {
                    // TODO checksum doesn't work
                    printf ( "\n(FIXME)Checksum BAD, (expected) %d != (buf) %d\n", csum, d.buf[10] );
                }

                // TODO decode kWh

                memset ( &d, 0, sizeof ( d ) );
            }

            d.n_down = 0;
        }
        else if ( s < 0 && d.n_up > 0 )
        {
            // Negative edge
            
            d.last_n_up = d.n_up;
            d.n_up = 0;
        }
        else if ( s < 0 )
        {
            // Negative level
            
            // TODO look for 100+ samples down preceeding 40+ samples up for preamble
            
            if ( d.mode == 0 && d.last_n_up > 40 )
            {
                // Preamble found
                d.mode = 1;
                d.last_n_up = 0;
            }
            else if ( d.mode == 1 && d.n_down == 0 )
            {
                // TODO is this the correct 1/0 mapping?
                if ( d.last_n_up > 3 && d.last_n_up < 10 )
                {
                    ++d.bit;
                    printf ( "0" );
                }
                else if ( d.last_n_up > 10 )
                {
                    d.buf[d.byte] |= ( 1 << d.bit );
                    ++d.bit;
                    printf ( "1" );
                }

                if ( d.bit > 7 )
                {
                    ++d.byte;

                    d.bit = 0;

                    if ( d.byte > 19 )
                    {
                        printf ( "\nError, unknown format (byte>19), ignoring.\n\n" );

                        memset ( &d, 0, sizeof ( d ) );
                    }
                    else
                    {
                        printf ( " [%2d] = %3d(0x%02x)\n", d.byte - 1, d.buf[d.byte-1], d.buf[d.byte-1] );
                    }
                }
            }

            ++d.n_down;
        }
    }

    return 0;
}
