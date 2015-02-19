#include <assert.h>
#include <string.h> /* memset() */

#include <stdlib.h> /* malloc(), free() */

#include "whuuid.h"

#if WHUUID_CONFIG_KEEP_METRICS
#  include <stdio.h> /* fprintf(), FILE */
#endif

const whuuid_t whuuid_t_empty = {
{0,0,0,0,
 0,0,0,0,
 0,0,0,0,
 0,0,0,0}/*bytes*/
};


static void whuuid_noop_cleanup( whuuid_rng * self )
{
    /* does nothing */
}
/**
   An almost-empty-initialized whuuid_rng object which uses
   whuuid_rand_uuint() as its random data source. It has no resources
   associated with it.
*/
const whuuid_rng whuuid_rng_empty = {
NULL/*rand*/,
whuuid_noop_cleanup/*cleanup*/,
NULL/*impl*/,
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}/*distribution*/
};

const whuuid_rng whuuid_rng_lcrng = {
whuuid_lc_rand/*rand*/,
whuuid_noop_cleanup/*cleanup*/,
NULL/*impl*/,
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}/*distribution*/
};

const whuuid_rng whuuid_rng_FILE = {
whuuid_FILE_rand/*rand*/,
whuuid_FILE_cleanup/*cleanup*/,
NULL/*impl*/,
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}/*distribution*/
};

/** BITS2CHAR(X) expects (X<=15). Returns the hex-code char for it
    ('0'..'f'), or 0 if X is out of range. */
#define BITS2CHAR(X) ( ((X)<=0x09) ? ('0'+(X)) : (((X)<=0xF) ? ('a'+((X)-10)) : 0))


void whuuid_FILE_cleanup( whuuid_rng * self )
{
    if( self && self->impl )
    {
        fclose( (FILE*)self->impl );
        self->impl = 0;
    }
}

int whuuid_FILE_rand( whuuid_rng * st, unsigned int * tgt )
{
    if( st && st->impl )
    {
        unsigned int d = 0;
        if( 1 != fread( &d, sizeof(d), 1, (FILE*)st->impl ) )
        {
            return -1;
        }
        *tgt = d;
        return 0;
    }
    return -1;
}

#include <time.h>
int whuuid_lc_rand( whuuid_rng * st, unsigned int  * tgt )
{
    typedef unsigned long NumType;
    NumType num = (NumType)st->impl;
    if( ! st || ! tgt ) return -1;
#define RNG(SEED) (NumType)( (NumType)((NumType)(SEED) * (NumType)1103515245) + 12345)
    /* ^^^^^ This RNG Works very well for this use case (comparable
       with /dev/urandom on my box). Algo found in Angband sources. */
    if( ! num )
    {
        void * x;
        num = (NumType) st;
        /* Generate a unique seed. */
        x = malloc( (num % 13)+9 );
        free(x);
        num = (NumType)(RNG(x) ^ num) >> 6
            /*
              The bitshift part is to work around the problem that the
              left-most byte of generated UUIDs always have the same
              starting sequences.
             */
            ;
    }
    else
    {
        num = RNG(num);
    }
#undef RNG
    st->impl = (void *)num;
    *tgt = num;
    return 0;
}

int whuuid_to_string( whuuid_t const * src, char * dest )
{
    unsigned int i = 0;
    int part = 1;
    int span = 0;
    char byte = 0;
    char nibble = 0;
    if( ! src || ! dest ) return -1;
    for( i = 0; i < whuuid_length_bytes; )
    {
        int x;
        if( 1 == part ) span = 8;
        else if( (part>1) && (part<5) ) span = 4;
        else if( 5 == part ) span = 12;
        for( x = 0; x < (span/2); ++x )
        {
            byte = src->bytes[i++];
            nibble = (byte >> 4) & 0x0F;
            *(dest++) = BITS2CHAR(nibble);
            nibble = (byte & 0x0F);
            *(dest++) = BITS2CHAR(nibble);
        }
        if( part < 5 )
        {
            *(dest++) = '-';
            ++part;
        }
        else break;
    }
    return 0;
}

int whuuid_fill( whuuid_t * dest, unsigned char const * src )
{
    if( ! dest || ! src ) return -1;
    else
    {
        memcpy( dest, src, whuuid_length_bytes );
        return 0;
    }
}

int whuuid_fill_rand( whuuid_t * dest, whuuid_rng * st )
{
    unsigned int i = 0, x = 0;
    unsigned char * c = 0;
    unsigned int r;
    unsigned char nibble;
    int rc = 0;
    if( ! st || ! dest ) return -1;
    if( ! dest ) return -1;
    for( ; i < whuuid_length_bytes; )
    {
        rc = st->rand(st, &r);
        if( rc ) break;
        c = (unsigned char *)&r;
        for( x = sizeof(r); (x > 0) && (i < whuuid_length_bytes); --x, ++i, ++c )
        {
            dest->bytes[i] = *c;
#if WHUUID_CONFIG_KEEP_METRICS
            nibble = (*c >> 4) & 0x0F;
            ++st->distribution[nibble];
            nibble = (*c & 0x0F);
            ++st->distribution[nibble];
#endif
        }
    }
    return rc;
}

short whuuid_compare( whuuid_t const * lhs, whuuid_t const * rhs )
{
    if( ! lhs ) return rhs ? -1 : 0;
    else if( ! rhs ) return 1;
    else if( lhs == rhs ) return 0;
    else
    {
#if 0
        unsigned int i = 0;
        unsigned char const * l = lhs->bytes;
        unsigned char const * r = rhs->bytes;
        unsigned char bl = 0, br = 0; /* current byte of lhs/rhs*/
        unsigned char nl = 0, nr = 0;/* 4-bit part of bl/br*/
        for( ; i < whuuid_length_bytes; ++i )
        {
            bl = l[i];
            br = r[i];
            nl = (bl >> 4);
            nr = (br >> 4);
            if( nl < nr ) return -1;
            else if( nl > nr ) return 1;
            nl = (bl & 0x0F);
            nr = (br & 0x0F);
            if( nl < nr ) return -1;
            else if( nl > nr ) return 1;
        }
        return 0;
#else
        return memcmp( lhs->bytes, rhs->bytes, whuuid_length_bytes );
#endif
    }
}

int whuuid_dump_distribution( whuuid_rng const * st, short full, FILE * dest )
{
#if ! WHUUID_CONFIG_KEEP_METRICS
    fprintf("WHUUID_CONFIG_KEEP_METRICS is false, so whuuid_dump_distribution() cannot work!\n");
    return -1;
#else
    unsigned short i = 0;
    double total = 0;
    unsigned long int max = 0, min = st->distribution[0];
    unsigned long int x = 0;
    char minL = 0, maxL = 0;
    if( full )
    {
        fprintf(dest,"Random number distribution:\nDigit:\tCount:\n");
    }
    for( ; i < 16; ++i )
    {
        x = st->distribution[i];
        total += x;
        if( max < x )
        {
            max = x;
            maxL = BITS2CHAR(i);
        }
        else if( min >= x )
        {
            min = x;
            minL = BITS2CHAR(i);
        }
    }
    if( full )
    {
        for( i = 0; i < 16; ++i )
        {
            x = st->distribution[i];
            fprintf(dest,"%c\t%lu (%0.6f%%)\n",
                    BITS2CHAR(i),
                    x, (x/ total) *100 );
        }
    }
    fprintf(dest,"Least Hits: '%c' (%lu)\nMost Hits: '%c' (%lu)\n",
           minL, min, maxL, max );
    if( max == min )
    {
        fprintf(dest,"Least==Most == best possible random distribution!\n" );
    }
    else
    {
        fprintf(dest,"Max-Min diff = %lu (%0.4f%% of Max)\n", max - min, ((max - min)/(double)max)*100 );
    }
    fprintf(dest,"Total random 4-bit UUID digits: %0.0f\n\n",total);
    return 0;
#endif
}

#undef BITS2CHAR
