/*  A multi-threaded program that takes 16-bit integer input 
    from multiple channels and formulates a final input by
    adding samples form all channels with optional low-pass
    and/or amplification values.

    IMPORTANT NOTE:
    The alpha computation is first performed on the sample before amplification.
    The final sample value should be rounded up to an integer value
    If integer overflow, the final outut value is 65535 (max 16bit int value)
*/

#include <stdio.h>
#include <stdlib.h>



int main(int argc, char *argv[]) {
    
}