/*
    Methods of placing functions and data at specific addresses
    https://www.keil.com/support/man/docs/armlink/armlink_chunk1880490665.htm

    Syntax of a scatter file
    https://www.keil.com/support/man/docs/armlink/armlink_pge1362075656353.htm
*/

#include <stdint.h>
#include "version.in"

//#define FIXED_ADDR  __attribute__((at(0x8001B00)))
////#define FIXED_ADDR  __attribute__((section("version")))

//#define kMagicWord 0xB001

//struct version_t {
//    uint16_t magic;
//    uint16_t major;
//    uint16_t minor;
//    uint16_t hash;
//};

//const struct version_t version FIXED_ADDR = {
//    kMagicWord, 1, 0, kCommitHash
//};
