/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --bpp 1 --size 16 --font C:/Users/Алексей/OneDrive/Документы/SLP/UI.v2/assets/GoodCyr-WideBlack.ttf -o C:/Users/Алексей/OneDrive/Документы/SLP/UI.v2/assets\ui_font_BaseFont16.c --format lvgl -r 0x20-0x7E -r 0x0400-0x04FF --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_BASEFONT16
#define UI_FONT_BASEFONT16 1
#endif

#if UI_FONT_BASEFONT16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xff, 0xff, 0x1f, 0xf0,

    /* U+0022 "\"" */
    0xff, 0xff, 0xf6,

    /* U+0023 "#" */
    0x36, 0x1b, 0xd, 0x9f, 0xff, 0xf9, 0xb0, 0xd9,
    0xff, 0xff, 0x9b, 0xd, 0x86, 0xc0,

    /* U+0024 "$" */
    0xc, 0x6, 0xf, 0xf, 0xef, 0xff, 0x53, 0xe0,
    0xfc, 0x3f, 0x7, 0xc2, 0xff, 0x7f, 0xf3, 0xf0,
    0x60, 0x30,

    /* U+0025 "%" */
    0x70, 0xdf, 0x33, 0x66, 0x6d, 0x8f, 0xe0, 0xec,
    0x3, 0x70, 0x7f, 0x1b, 0x66, 0x6c, 0xcf, 0xb0,
    0xe0,

    /* U+0026 "&" */
    0x1e, 0xf, 0xc7, 0x39, 0xc0, 0x73, 0x8f, 0xf7,
    0xff, 0x8e, 0xe3, 0xb8, 0xef, 0xfd, 0xf7,

    /* U+0027 "'" */
    0xff, 0xe0,

    /* U+0028 "(" */
    0x3b, 0xfd, 0xce, 0x73, 0x9c, 0xe7, 0x39, 0xce,
    0x73, 0xcf, 0x38,

    /* U+0029 ")" */
    0xf3, 0xe3, 0x87, 0x1c, 0x71, 0xc7, 0x1c, 0x71,
    0xc7, 0x1c, 0x73, 0xbe, 0xf0,

    /* U+002A "*" */
    0x33, 0x7e, 0xe5, 0x80,

    /* U+002B "+" */
    0x18, 0x30, 0x67, 0xff, 0xe3, 0x6, 0xc,

    /* U+002C "," */
    0xff, 0xbf, 0x80,

    /* U+002D "-" */
    0xff,

    /* U+002E "." */
    0xff, 0x80,

    /* U+002F "/" */
    0x7, 0x6, 0x6, 0xc, 0xc, 0xc, 0x18, 0x18,
    0x38, 0x30, 0x30, 0x60, 0x60, 0x60,

    /* U+0030 "0" */
    0x3e, 0x3f, 0xbf, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0xe3, 0xff, 0xdf, 0xc7, 0xc0,

    /* U+0031 "1" */
    0x1c, 0x1c, 0xfc, 0xfc, 0x1c, 0x1c, 0x1c, 0x1c,
    0x1c, 0x1c, 0x7f, 0x7f,

    /* U+0032 "2" */
    0x3e, 0x3f, 0xbf, 0xfc, 0x72, 0x38, 0x3c, 0x7c,
    0x7c, 0x78, 0x78, 0x3f, 0xff, 0xf0,

    /* U+0033 "3" */
    0x3e, 0x3f, 0xb8, 0xe4, 0x70, 0x38, 0xf8, 0x7c,
    0x7, 0x3, 0xb1, 0xdf, 0xc7, 0xc0,

    /* U+0034 "4" */
    0x7, 0x81, 0xe0, 0xf8, 0x3e, 0x1f, 0x8e, 0xe3,
    0xb9, 0xce, 0xff, 0xff, 0xf0, 0x38, 0xe,

    /* U+0035 "5" */
    0x7f, 0xbf, 0xdf, 0xce, 0x7, 0xe3, 0xf8, 0x8e,
    0x7, 0x23, 0xf1, 0xdf, 0xc7, 0xc0,

    /* U+0036 "6" */
    0x1e, 0x3f, 0xb8, 0xdc, 0xf, 0xe7, 0xff, 0x8f,
    0xc7, 0xe3, 0xff, 0xdf, 0xc7, 0xc0,

    /* U+0037 "7" */
    0xff, 0xff, 0xff, 0xe0, 0xf0, 0x70, 0x78, 0x38,
    0x3c, 0x1c, 0xe, 0xe, 0x7, 0x0,

    /* U+0038 "8" */
    0x3e, 0x7f, 0xf8, 0xfc, 0x77, 0xf3, 0xfb, 0x8f,
    0xc7, 0xe3, 0xf1, 0xdf, 0xc7, 0xc0,

    /* U+0039 "9" */
    0x3e, 0x3f, 0xbf, 0xfc, 0x7e, 0x3f, 0xfd, 0xfe,
    0x7, 0x3, 0xb3, 0xdf, 0xc7, 0xc0,

    /* U+003A ":" */
    0xff, 0x80, 0x3f, 0xe0,

    /* U+003B ";" */
    0xff, 0x80, 0x3f, 0xef, 0xe0,

    /* U+003C "<" */
    0x2, 0x1c, 0x7b, 0xef, 0xf, 0x87, 0x87, 0x2,

    /* U+003D "=" */
    0xff, 0xf0, 0x3f, 0xfc,

    /* U+003E ">" */
    0x81, 0x83, 0xc3, 0xc3, 0xef, 0x3c, 0x60, 0x80,

    /* U+003F "?" */
    0x3e, 0x3f, 0xb8, 0xe0, 0x70, 0x78, 0xf8, 0x78,
    0x38, 0x0, 0xe, 0x7, 0x3, 0x80,

    /* U+0040 "@" */
    0xf, 0xc1, 0xff, 0x1c, 0x1c, 0xc0, 0x7c, 0x7d,
    0xe7, 0xef, 0x76, 0x7b, 0xb3, 0xdd, 0x9e, 0xec,
    0xf7, 0xfd, 0x9f, 0xc6, 0x0, 0x38, 0x38, 0xff,
    0xc1, 0xf8,

    /* U+0041 "A" */
    0xf, 0x1, 0xf0, 0x1f, 0x81, 0xf8, 0x3b, 0x83,
    0xb8, 0x39, 0xc7, 0x9c, 0x7f, 0xc7, 0xfe, 0xf0,
    0xef, 0xe,

    /* U+0042 "B" */
    0xfe, 0x7f, 0xbf, 0xfc, 0x7e, 0x3f, 0xfb, 0xfd,
    0xc7, 0xe3, 0xff, 0xff, 0xff, 0xc0,

    /* U+0043 "C" */
    0x3c, 0x7f, 0xff, 0xe3, 0xe0, 0xe0, 0xe0, 0xe0,
    0xe0, 0xe3, 0x7f, 0x3e,

    /* U+0044 "D" */
    0xff, 0x3f, 0xef, 0xff, 0x87, 0xe1, 0xf8, 0x7e,
    0x1f, 0x87, 0xe1, 0xff, 0xff, 0xfb, 0xf8,

    /* U+0045 "E" */
    0xff, 0xff, 0xff, 0xe0, 0xe0, 0xfe, 0xfe, 0xe0,
    0xe0, 0xff, 0xff, 0xff,

    /* U+0046 "F" */
    0xff, 0xff, 0xff, 0xe0, 0xe0, 0xff, 0xff, 0xe0,
    0xe0, 0xe0, 0xe0, 0xe0,

    /* U+0047 "G" */
    0x1f, 0x1f, 0xef, 0xff, 0xc6, 0xe0, 0x39, 0xfe,
    0x7f, 0x87, 0xe1, 0xfc, 0xf7, 0xfc, 0xff,

    /* U+0048 "H" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x7f, 0xff, 0xff, 0xff,
    0xc7, 0xe3, 0xf1, 0xf8, 0xfc, 0x70,

    /* U+0049 "I" */
    0xff, 0xff, 0xff, 0xff, 0xf0,

    /* U+004A "J" */
    0x39, 0xce, 0x73, 0x9c, 0xe7, 0x39, 0xce, 0x77,
    0xfb, 0x80,

    /* U+004B "K" */
    0xe3, 0xb9, 0xee, 0xf3, 0xb8, 0xfe, 0x3f, 0xf,
    0xe3, 0xf8, 0xef, 0x39, 0xee, 0x3b, 0x8f,

    /* U+004C "L" */
    0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0,
    0xe0, 0xff, 0xff, 0xff,

    /* U+004D "M" */
    0xe0, 0x3f, 0x83, 0xfc, 0x3f, 0xf1, 0xff, 0xdf,
    0xfe, 0xff, 0xfe, 0xfd, 0xf7, 0xe7, 0x3f, 0x39,
    0xf8, 0xf, 0xc0, 0x70,

    /* U+004E "N" */
    0xe1, 0xfc, 0x7f, 0x1f, 0xe7, 0xfd, 0xff, 0xff,
    0xff, 0xbf, 0xe7, 0xf8, 0xfe, 0x3f, 0x87,

    /* U+004F "O" */
    0x3f, 0x1f, 0xef, 0xff, 0x87, 0xe1, 0xf8, 0x7e,
    0x1f, 0x87, 0xe1, 0xf8, 0x77, 0xf8, 0xfc,

    /* U+0050 "P" */
    0xfe, 0x7f, 0xbf, 0xfc, 0x7e, 0x3f, 0x1f, 0xfd,
    0xfc, 0xe0, 0x70, 0x38, 0x1c, 0x0,

    /* U+0051 "Q" */
    0x3f, 0x1f, 0xef, 0xff, 0x87, 0xe1, 0xf8, 0x7e,
    0x1f, 0x87, 0xe1, 0xf8, 0x77, 0xf8, 0xfc, 0x7,
    0x80, 0xe0, 0x0,

    /* U+0052 "R" */
    0xfe, 0x3f, 0xcf, 0xfb, 0x8e, 0xe3, 0xbf, 0xcf,
    0xe3, 0xbc, 0xe7, 0x39, 0xee, 0x3b, 0x8e,

    /* U+0053 "S" */
    0x3e, 0x3f, 0xb8, 0xde, 0xf, 0xc3, 0xf8, 0xfe,
    0x1f, 0x3, 0xf1, 0xff, 0xc7, 0xc0,

    /* U+0054 "T" */
    0xff, 0xff, 0xff, 0xe3, 0x81, 0xc0, 0xe0, 0x70,
    0x38, 0x1c, 0xe, 0x7, 0x3, 0x80,

    /* U+0055 "U" */
    0xe1, 0xf8, 0x7e, 0x1f, 0x87, 0xe1, 0xf8, 0x7e,
    0x1f, 0x87, 0xe1, 0xff, 0xf7, 0xf8, 0xfc,

    /* U+0056 "V" */
    0xf0, 0xf7, 0xe, 0x71, 0xe7, 0x9e, 0x39, 0xc3,
    0x9c, 0x3b, 0xc1, 0xf8, 0x1f, 0x81, 0xf8, 0xf,
    0x0, 0xf0,

    /* U+0057 "W" */
    0xf1, 0xe3, 0x9c, 0x78, 0xe7, 0x1e, 0x39, 0xcf,
    0x9e, 0x73, 0xe7, 0xe, 0xfd, 0xc3, 0xb7, 0x70,
    0xec, 0xd8, 0x1f, 0x3e, 0x7, 0xcf, 0x81, 0xe3,
    0xe0, 0x78, 0x70,

    /* U+0058 "X" */
    0xf1, 0xef, 0x38, 0xef, 0x1f, 0xc1, 0xf8, 0x3e,
    0x7, 0xc0, 0xfc, 0x3b, 0x8f, 0x79, 0xc7, 0x78,
    0xf0,

    /* U+0059 "Y" */
    0xf1, 0xee, 0x39, 0xef, 0x1d, 0xc3, 0xb8, 0x3e,
    0x7, 0xc0, 0x70, 0xe, 0x1, 0xc0, 0x38, 0x7,
    0x0,

    /* U+005A "Z" */
    0xff, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xf0, 0xf0,
    0x70, 0x78, 0x7f, 0xff, 0xff, 0xf0,

    /* U+005B "[" */
    0xff, 0xf9, 0xce, 0x73, 0x9c, 0xe7, 0x39, 0xce,
    0x73, 0x9f, 0xf8,

    /* U+005C "\\" */
    0x60, 0x60, 0x60, 0x30, 0x30, 0x38, 0x18, 0x18,
    0x1c, 0xc, 0xc, 0x6, 0x6, 0x6,

    /* U+005D "]" */
    0xff, 0xce, 0x73, 0x9c, 0xe7, 0x39, 0xce, 0x73,
    0x9c, 0xff, 0xf8,

    /* U+005E "^" */
    0x30, 0xf1, 0xf6, 0x60,

    /* U+005F "_" */
    0xff, 0xff,

    /* U+0060 "`" */
    0xe7,

    /* U+0061 "a" */
    0x3c, 0x7e, 0x6e, 0xe, 0x7e, 0xfe, 0xee, 0xff,
    0x77,

    /* U+0062 "b" */
    0xe0, 0xe0, 0xe0, 0xe0, 0xfe, 0xff, 0xe7, 0xe7,
    0xe7, 0xe7, 0xe7, 0xff, 0xfe,

    /* U+0063 "c" */
    0x3c, 0xff, 0x97, 0xe, 0x1c, 0x39, 0x3f, 0x3c,

    /* U+0064 "d" */
    0x7, 0x7, 0x7, 0x7, 0x7f, 0xff, 0xe7, 0xe7,
    0xe7, 0xe7, 0xe7, 0xff, 0x7f,

    /* U+0065 "e" */
    0x3c, 0x7e, 0xe7, 0xff, 0xff, 0xe0, 0xe6, 0x7f,
    0x3e,

    /* U+0066 "f" */
    0x3b, 0xdd, 0xff, 0xb9, 0xce, 0x73, 0x9c, 0xe0,

    /* U+0067 "g" */
    0x2, 0x7f, 0xfe, 0xee, 0xee, 0xfe, 0x7c, 0xc0,
    0xfe, 0xff, 0xe7, 0xff, 0x7e,

    /* U+0068 "h" */
    0xe0, 0xe0, 0xe0, 0xe0, 0xfe, 0xff, 0xe7, 0xe7,
    0xe7, 0xe7, 0xe7, 0xe7, 0xe7,

    /* U+0069 "i" */
    0xfc, 0x7f, 0xff, 0xff, 0xf0,

    /* U+006A "j" */
    0x77, 0x7, 0x77, 0x77, 0x77, 0x77, 0xfe, 0xc0,

    /* U+006B "k" */
    0xe0, 0x70, 0x38, 0x1c, 0xe, 0x77, 0x3b, 0xb9,
    0xf8, 0xfc, 0x7f, 0x3b, 0x9c, 0xee, 0x78,

    /* U+006C "l" */
    0xff, 0xff, 0xff, 0xff, 0xfe,

    /* U+006D "m" */
    0xfe, 0xf7, 0xff, 0xf9, 0xcf, 0xce, 0x7e, 0x73,
    0xf3, 0x9f, 0x9c, 0xfc, 0xe7, 0xe7, 0x38,

    /* U+006E "n" */
    0xfe, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7,
    0xe7,

    /* U+006F "o" */
    0x3c, 0x7e, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0x7e,
    0x3c,

    /* U+0070 "p" */
    0xfe, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xff,
    0xfe, 0xe0, 0xe0, 0xe0,

    /* U+0071 "q" */
    0x7f, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xff,
    0x7f, 0x7, 0x7, 0x7,

    /* U+0072 "r" */
    0xff, 0xff, 0x38, 0xe3, 0x8e, 0x38, 0xe0,

    /* U+0073 "s" */
    0x7d, 0xff, 0x97, 0xc7, 0xc7, 0xd3, 0xff, 0x78,

    /* U+0074 "t" */
    0x30, 0xc7, 0x3f, 0xfd, 0xc7, 0x1c, 0x71, 0xe7,
    0xcf,

    /* U+0075 "u" */
    0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xff,
    0x7f,

    /* U+0076 "v" */
    0xf3, 0x9c, 0xe7, 0x39, 0xdc, 0x37, 0xf, 0xc3,
    0xe0, 0x78, 0x1e, 0x0,

    /* U+0077 "w" */
    0xe7, 0x3b, 0x9e, 0xe7, 0x7b, 0x9d, 0xee, 0x7f,
    0xb1, 0xf7, 0xc3, 0xdf, 0xf, 0x3c, 0x3c, 0xe0,

    /* U+0078 "x" */
    0xf3, 0x9c, 0xe3, 0xf0, 0xf8, 0x1e, 0xf, 0x87,
    0x71, 0xce, 0xf3, 0x80,

    /* U+0079 "y" */
    0xf3, 0x9c, 0xe7, 0x39, 0xdc, 0x3f, 0xf, 0x81,
    0xe0, 0x78, 0x1c, 0x7, 0x3, 0x80, 0xc0,

    /* U+007A "z" */
    0xff, 0xfc, 0x71, 0xe3, 0x8e, 0x1c, 0x7f, 0xfe,

    /* U+007B "{" */
    0x1d, 0xf7, 0x1c, 0x71, 0xc7, 0x1c, 0xe1, 0xc7,
    0x1c, 0x71, 0xc7, 0x1f, 0x1c,

    /* U+007C "|" */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0,

    /* U+007D "}" */
    0xe3, 0xe3, 0x8e, 0x38, 0xe3, 0x8f, 0x1c, 0xf3,
    0x8e, 0x38, 0xe3, 0xbc, 0xe0,

    /* U+007E "~" */
    0x77, 0xff, 0x70,

    /* U+0401 "Ё" */
    0x6c, 0x6c, 0x0, 0xff, 0xff, 0xff, 0xe0, 0xe0,
    0xfc, 0xfc, 0xe0, 0xe0, 0xff, 0xff, 0xff,

    /* U+0402 "Ђ" */
    0xff, 0x3f, 0xcf, 0xf0, 0xfe, 0x3f, 0xce, 0x73,
    0x9c, 0xe7, 0x39, 0xce, 0x73, 0x9c, 0xe7, 0x1,
    0xc0, 0xe0, 0x30,

    /* U+0403 "Ѓ" */
    0x3c, 0x38, 0x0, 0xff, 0xff, 0xff, 0xe0, 0xe0,
    0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0,

    /* U+0404 "Є" */
    0x3e, 0x3f, 0xb8, 0xfc, 0xe, 0x7, 0xf3, 0xf9,
    0xc0, 0xe0, 0x71, 0xdf, 0xe7, 0xc0,

    /* U+0405 "Ѕ" */
    0x3e, 0x3f, 0xb8, 0xde, 0xf, 0xc3, 0xf8, 0xfe,
    0x1f, 0x3, 0xf1, 0xff, 0xc7, 0xc0,

    /* U+0406 "І" */
    0xff, 0xff, 0xff, 0xff, 0xf0,

    /* U+0407 "Ї" */
    0xde, 0xc0, 0xe7, 0x39, 0xce, 0x73, 0x9c, 0xe7,
    0x39, 0xc0,

    /* U+0408 "Ј" */
    0x39, 0xce, 0x73, 0x9c, 0xe7, 0x39, 0xce, 0x77,
    0xfb, 0x80,

    /* U+0409 "Љ" */
    0x3f, 0xe0, 0x3f, 0xe0, 0x3f, 0xe0, 0x38, 0xe0,
    0x38, 0xe0, 0x38, 0xfe, 0x38, 0xff, 0x38, 0xe7,
    0x38, 0xe7, 0x78, 0xff, 0xf0, 0xff, 0xe0, 0xfe,

    /* U+040A "Њ" */
    0xe3, 0x81, 0xc7, 0x3, 0x8e, 0x7, 0x1c, 0xf,
    0xff, 0x9f, 0xff, 0xbf, 0xff, 0xf1, 0xc7, 0xe3,
    0x8f, 0xc7, 0xff, 0x8f, 0xf7, 0x1f, 0xc0,

    /* U+040B "Ћ" */
    0xff, 0x3f, 0xcf, 0xf0, 0xfe, 0x3f, 0xce, 0x73,
    0x9c, 0xe7, 0x39, 0xce, 0x73, 0x9c, 0xe7,

    /* U+040C "Ќ" */
    0xe, 0x3, 0x80, 0x0, 0x70, 0xee, 0x3d, 0xcf,
    0x39, 0xc7, 0x78, 0xfe, 0x1f, 0xc3, 0xbc, 0x73,
    0xce, 0x79, 0xc7, 0xf8, 0x78,

    /* U+040E "Ў" */
    0x1f, 0x81, 0xe0, 0x0, 0x78, 0x77, 0x1e, 0xf3,
    0x8e, 0x71, 0xfe, 0x1f, 0x83, 0xf0, 0x3c, 0x7,
    0x80, 0xe0, 0x7c, 0xe, 0x0,

    /* U+040F "Џ" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0xe3, 0xff, 0xff, 0xff, 0xf1, 0xc0, 0xe0,

    /* U+0410 "А" */
    0xf, 0x1, 0xf0, 0x1f, 0x81, 0xf8, 0x3b, 0x83,
    0xb8, 0x39, 0xc7, 0x9c, 0x7f, 0xc7, 0xfe, 0xf0,
    0xef, 0xe,

    /* U+0411 "Б" */
    0xff, 0x7f, 0xbf, 0xdc, 0xe, 0x7, 0xf3, 0xff,
    0xc7, 0xe3, 0xff, 0xff, 0xdf, 0xc0,

    /* U+0412 "В" */
    0xfe, 0x7f, 0xbf, 0xfc, 0x7e, 0x3f, 0xfb, 0xfd,
    0xc7, 0xe3, 0xff, 0xff, 0xff, 0xc0,

    /* U+0413 "Г" */
    0xff, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0,
    0xe0, 0xe0, 0xe0, 0xe0,

    /* U+0414 "Д" */
    0x3f, 0xe1, 0xff, 0xf, 0xf8, 0x71, 0xc3, 0x8e,
    0x1c, 0x70, 0xe3, 0x87, 0x1c, 0x38, 0xe3, 0xc7,
    0x3f, 0xff, 0xff, 0xfe, 0x3, 0xf0, 0x1f, 0x80,
    0xe0,

    /* U+0415 "Е" */
    0xff, 0xff, 0xff, 0xe0, 0xe0, 0xfe, 0xfe, 0xe0,
    0xe0, 0xff, 0xff, 0xff,

    /* U+0416 "Ж" */
    0x71, 0xc7, 0xbc, 0xe3, 0xde, 0x73, 0xe7, 0x39,
    0xc1, 0xdd, 0xc0, 0xff, 0xe0, 0x7f, 0xf0, 0x7b,
    0xbc, 0x3d, 0xce, 0x3c, 0xe7, 0xbe, 0x73, 0xfe,
    0x38, 0xf0,

    /* U+0417 "З" */
    0x3e, 0x3f, 0xb8, 0xe4, 0x70, 0x38, 0xf8, 0x7c,
    0x7, 0x23, 0xf1, 0xdf, 0xc7, 0xc0,

    /* U+0418 "И" */
    0xe1, 0xf8, 0xfe, 0x3f, 0x9f, 0xef, 0xfb, 0xff,
    0xdf, 0xf7, 0xf9, 0xfc, 0x7f, 0x1f, 0x87,

    /* U+0419 "Й" */
    0x3f, 0x7, 0x80, 0x3, 0x87, 0xe3, 0xf8, 0xfe,
    0x7f, 0xbf, 0xef, 0xff, 0x7f, 0xdf, 0xe7, 0xf1,
    0xfc, 0x7e, 0x1c,

    /* U+041A "К" */
    0xe1, 0xdc, 0x7b, 0x9e, 0x73, 0x8e, 0xf1, 0xfc,
    0x3f, 0x87, 0x78, 0xe7, 0x9c, 0xf3, 0x8f, 0xf0,
    0xf0,

    /* U+041B "Л" */
    0x3f, 0xe7, 0xfc, 0xff, 0x9c, 0x73, 0x8e, 0x71,
    0xce, 0x39, 0xc7, 0x38, 0xef, 0x1f, 0xc3, 0xf0,
    0x70,

    /* U+041C "М" */
    0xe0, 0x3f, 0x83, 0xfc, 0x3f, 0xf1, 0xff, 0xdf,
    0xfe, 0xff, 0xfe, 0xfd, 0xf7, 0xe7, 0x3f, 0x39,
    0xf8, 0xf, 0xc0, 0x70,

    /* U+041D "Н" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x7f, 0xff, 0xff, 0xff,
    0xc7, 0xe3, 0xf1, 0xf8, 0xfc, 0x70,

    /* U+041E "О" */
    0x3f, 0x1f, 0xef, 0xff, 0x87, 0xe1, 0xf8, 0x7e,
    0x1f, 0x87, 0xe1, 0xf8, 0x77, 0xf8, 0xfc,

    /* U+041F "П" */
    0xff, 0xff, 0xff, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0xe3, 0xf1, 0xf8, 0xfc, 0x70,

    /* U+0420 "Р" */
    0xfe, 0x7f, 0xbf, 0xfc, 0x7e, 0x3f, 0x1f, 0xfd,
    0xfc, 0xe0, 0x70, 0x38, 0x1c, 0x0,

    /* U+0421 "С" */
    0x3c, 0x7f, 0xff, 0xe3, 0xe0, 0xe0, 0xe0, 0xe0,
    0xe0, 0xe3, 0x7f, 0x3e,

    /* U+0422 "Т" */
    0xff, 0xff, 0xff, 0xe3, 0x81, 0xc0, 0xe0, 0x70,
    0x38, 0x1c, 0xe, 0x7, 0x3, 0x80,

    /* U+0423 "У" */
    0xf0, 0xee, 0x3d, 0xe7, 0x1c, 0xe3, 0xfc, 0x3f,
    0x7, 0xe0, 0x78, 0xf, 0x1, 0xc0, 0xf8, 0x1c,
    0x0,

    /* U+0424 "Ф" */
    0x7, 0x1, 0xff, 0x1f, 0xfd, 0xce, 0x7e, 0x73,
    0xf3, 0x9f, 0x9c, 0xfc, 0xe7, 0x7f, 0xf1, 0xff,
    0x1, 0xc0, 0xe, 0x0,

    /* U+0425 "Х" */
    0xf1, 0xef, 0x38, 0xef, 0x1f, 0xc1, 0xf8, 0x3e,
    0x7, 0xc0, 0xfc, 0x3b, 0x8f, 0x79, 0xc7, 0x78,
    0xf0,

    /* U+0426 "Ц" */
    0xe3, 0x9c, 0x73, 0x8e, 0x71, 0xce, 0x39, 0xc7,
    0x38, 0xe7, 0x1c, 0xe3, 0x9f, 0xff, 0xff, 0xff,
    0xf0, 0xe, 0x1, 0xc0,

    /* U+0427 "Ч" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x7e, 0x3f, 0xfd, 0xfe,
    0x7f, 0x3, 0x81, 0xc0, 0xe0, 0x70,

    /* U+0428 "Ш" */
    0xe3, 0x8f, 0xc7, 0x1f, 0x8e, 0x3f, 0x1c, 0x7e,
    0x38, 0xfc, 0x71, 0xf8, 0xe3, 0xf1, 0xc7, 0xe3,
    0x8f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0,

    /* U+0429 "Щ" */
    0xe3, 0x8e, 0xe3, 0x8e, 0xe3, 0x8e, 0xe3, 0x8e,
    0xe3, 0x8e, 0xe3, 0x8e, 0xe3, 0x8e, 0xe3, 0x8e,
    0xe3, 0x8e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x0, 0x7, 0x0, 0x7,

    /* U+042A "Ъ" */
    0xfc, 0xf, 0xc0, 0xfc, 0x1, 0xc0, 0x1c, 0x1,
    0xfc, 0x1f, 0xe1, 0xc7, 0x1c, 0x71, 0xff, 0x1f,
    0xe1, 0xfc,

    /* U+042B "Ы" */
    0xe0, 0x1f, 0x80, 0x7e, 0x1, 0xf8, 0x7, 0xe0,
    0x1f, 0xf8, 0x7f, 0xf9, 0xf8, 0xe7, 0xe3, 0x9f,
    0xfe, 0x7f, 0xf1, 0xff, 0x87,

    /* U+042C "Ь" */
    0xe0, 0x70, 0x38, 0x1c, 0xe, 0x7, 0xf3, 0xff,
    0xc7, 0xe3, 0xff, 0xff, 0xdf, 0xc0,

    /* U+042D "Э" */
    0x3e, 0x3f, 0xb8, 0xe4, 0x70, 0x38, 0xfc, 0x7e,
    0x7, 0x23, 0xf1, 0xdf, 0xc7, 0xc0,

    /* U+042E "Ю" */
    0xe1, 0xf3, 0x8f, 0xee, 0x7f, 0xf9, 0xc7, 0xe7,
    0x1f, 0xfc, 0x7f, 0xf1, 0xf9, 0xc7, 0xe7, 0x1f,
    0x9f, 0xfe, 0x3f, 0xb8, 0x7c,

    /* U+042F "Я" */
    0x1f, 0xcf, 0xf7, 0xfd, 0xc7, 0x71, 0xcf, 0xf1,
    0xfc, 0xf7, 0x39, 0xde, 0x7f, 0x9f, 0xc7,

    /* U+0430 "а" */
    0x3c, 0x7e, 0x6e, 0xe, 0x7e, 0xfe, 0xee, 0xff,
    0x77,

    /* U+0431 "б" */
    0x0, 0x3e, 0x7f, 0xfc, 0xe0, 0xde, 0xff, 0xe7,
    0xe7, 0xe7, 0xe7, 0xe7, 0x7e, 0x3c,

    /* U+0432 "в" */
    0xfc, 0xfe, 0xee, 0xee, 0xfc, 0xfe, 0xe7, 0xff,
    0xfe,

    /* U+0433 "г" */
    0xff, 0xfe, 0x38, 0xe3, 0x8e, 0x38, 0xe0,

    /* U+0434 "д" */
    0x3f, 0x8f, 0xe3, 0xb8, 0xce, 0x33, 0x8c, 0xe7,
    0x3b, 0xff, 0xff, 0xf8, 0x7e, 0x1f, 0x87,

    /* U+0435 "е" */
    0x3c, 0x7e, 0xe7, 0xff, 0xff, 0xe0, 0xe6, 0x7f,
    0x3e,

    /* U+0436 "ж" */
    0xe7, 0x3f, 0xbb, 0xdd, 0xdc, 0x7f, 0xc3, 0xfe,
    0x3b, 0xb9, 0xdd, 0xdc, 0xe7, 0xe7, 0x38,

    /* U+0437 "з" */
    0x7d, 0xfd, 0x39, 0xf3, 0xe1, 0xd3, 0xff, 0x7c,

    /* U+0438 "и" */
    0xe7, 0xef, 0xef, 0xef, 0xff, 0xf7, 0xf7, 0xf7,
    0xe7,

    /* U+0439 "й" */
    0x3e, 0x3c, 0x0, 0xe7, 0xef, 0xef, 0xef, 0xff,
    0xf7, 0xf7, 0xf7, 0xe7,

    /* U+043A "к" */
    0xe3, 0xf3, 0xfb, 0x9f, 0xcf, 0xe7, 0x73, 0xbd,
    0xcf, 0xe3, 0x80,

    /* U+043B "л" */
    0x3f, 0x9f, 0xce, 0xe7, 0x73, 0x3b, 0x9d, 0xcf,
    0xe7, 0xe3, 0x80,

    /* U+043C "м" */
    0xe1, 0xfc, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xfe,
    0xdf, 0xb7, 0xe1, 0xc0,

    /* U+043D "н" */
    0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0xff, 0xe7, 0xe7,
    0xe7,

    /* U+043E "о" */
    0x3c, 0x7e, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0x7e,
    0x3c,

    /* U+043F "п" */
    0xff, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7,
    0xe7,

    /* U+0440 "р" */
    0xfe, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xff,
    0xfe, 0xe0, 0xe0, 0xe0,

    /* U+0441 "с" */
    0x3c, 0xff, 0x97, 0xe, 0x1c, 0x39, 0x3f, 0x3c,

    /* U+0442 "т" */
    0xff, 0xff, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38,
    0x38,

    /* U+0443 "у" */
    0xf3, 0x9c, 0xe7, 0x38, 0xfc, 0x3f, 0xf, 0x81,
    0xe0, 0x78, 0x1c, 0x7, 0x7, 0x81, 0xc0,

    /* U+0444 "ф" */
    0x7, 0x0, 0x38, 0x1, 0xc0, 0xff, 0xef, 0xff,
    0x73, 0x9f, 0x9c, 0xfc, 0xe7, 0xe7, 0x3f, 0x39,
    0xff, 0xfe, 0xff, 0xe0, 0x70, 0x3, 0x80, 0x1c,
    0x0,

    /* U+0445 "х" */
    0xf3, 0x9c, 0xe3, 0xf0, 0xf8, 0x1e, 0xf, 0x87,
    0x71, 0xde, 0xf3, 0x80,

    /* U+0446 "ц" */
    0xe7, 0x73, 0xb9, 0xdc, 0xee, 0x77, 0x3b, 0x9d,
    0xff, 0xff, 0x81, 0xc0, 0xe0, 0x70,

    /* U+0447 "ч" */
    0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0x7f, 0x7, 0x7,
    0x7,

    /* U+0448 "ш" */
    0xe7, 0x3f, 0x39, 0xf9, 0xcf, 0xce, 0x7e, 0x73,
    0xf3, 0x9f, 0x9c, 0xff, 0xff, 0xff, 0xf8,

    /* U+0449 "щ" */
    0xe7, 0x3b, 0x9c, 0xee, 0x73, 0xb9, 0xce, 0xe7,
    0x3b, 0x9c, 0xee, 0x73, 0xbf, 0xff, 0xff, 0xfc,
    0x0, 0x70, 0x1, 0xc0, 0x7,

    /* U+044A "ъ" */
    0xf8, 0x3e, 0x3, 0x80, 0xfe, 0x3f, 0xce, 0x73,
    0x9c, 0xff, 0x3f, 0x80,

    /* U+044B "ы" */
    0xe0, 0x7e, 0x7, 0xe0, 0x7f, 0xe7, 0xff, 0x7e,
    0x77, 0xe7, 0x7f, 0xf7, 0xfe, 0x70,

    /* U+044C "ь" */
    0xe0, 0xe0, 0xe0, 0xfe, 0xff, 0xe7, 0xe7, 0xff,
    0xfe,

    /* U+044D "э" */
    0x79, 0xf9, 0x39, 0xf3, 0xe1, 0xf3, 0xfe, 0x78,

    /* U+044E "ю" */
    0xe3, 0xce, 0xfe, 0xee, 0x7f, 0xe7, 0xfe, 0x7e,
    0xe7, 0xee, 0x7e, 0x7e, 0xe3, 0xc0,

    /* U+044F "я" */
    0x3f, 0xbf, 0xdc, 0xee, 0x77, 0xf9, 0xfc, 0xef,
    0xf7, 0xf3, 0x80,

    /* U+0451 "ё" */
    0x3c, 0x3c, 0x0, 0x3c, 0x7e, 0xe7, 0xff, 0xff,
    0xe0, 0xe6, 0x7f, 0x3e,

    /* U+0452 "ђ" */
    0xfc, 0x7e, 0x1c, 0xf, 0xe7, 0xfb, 0x9d, 0xce,
    0xe7, 0x73, 0xb9, 0xdc, 0xee, 0x70, 0x38, 0x3c,
    0xc,

    /* U+0453 "ѓ" */
    0x39, 0xc0, 0x3f, 0xff, 0x8e, 0x38, 0xe3, 0x8e,
    0x38,

    /* U+0454 "є" */
    0x3c, 0xff, 0x97, 0xcf, 0x9c, 0x39, 0xbf, 0x3c,

    /* U+0455 "ѕ" */
    0x7d, 0xff, 0x97, 0xc7, 0xc7, 0xd3, 0xff, 0x78,

    /* U+0456 "і" */
    0xfc, 0x7f, 0xff, 0xff, 0xf0,

    /* U+0457 "ї" */
    0xde, 0xc0, 0xe7, 0x39, 0xce, 0x73, 0x9c, 0xe0,

    /* U+0458 "ј" */
    0x77, 0x7, 0x77, 0x77, 0x77, 0x77, 0xfe, 0xc0,

    /* U+0459 "љ" */
    0x3f, 0x80, 0xfe, 0x3, 0xb8, 0xe, 0xfe, 0x33,
    0xfd, 0xce, 0x77, 0x39, 0xfc, 0xff, 0xe3, 0xf8,

    /* U+045A "њ" */
    0xe7, 0x7, 0x38, 0x39, 0xc1, 0xce, 0xf, 0xff,
    0x7f, 0xff, 0x9c, 0xfc, 0xff, 0xe7, 0xf0,

    /* U+045B "ћ" */
    0xfc, 0x7e, 0x1c, 0xf, 0xe7, 0xfb, 0x9d, 0xce,
    0xe7, 0x73, 0xb9, 0xdc, 0xee, 0x70,

    /* U+045C "ќ" */
    0x1e, 0xe, 0x0, 0x1c, 0x7e, 0x7f, 0x73, 0xf9,
    0xfc, 0xee, 0x77, 0xb9, 0xfc, 0x70,

    /* U+045E "ў" */
    0x3f, 0x7, 0x80, 0x3, 0xce, 0x73, 0x9c, 0xe3,
    0xf0, 0xfc, 0x3e, 0x7, 0x81, 0xe0, 0x70, 0x1c,
    0x1e, 0x7, 0x0,

    /* U+045F "џ" */
    0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xff,
    0xff, 0x1c, 0x1c, 0x1c,

    /* U+0490 "Ґ" */
    0xe, 0x1f, 0xff, 0xff, 0xfc, 0x38, 0x70, 0xe1,
    0xc3, 0x87, 0xe, 0x1c, 0x0,

    /* U+0491 "ґ" */
    0x1c, 0x7f, 0xff, 0xe3, 0x8e, 0x38, 0xe3, 0x8e,
    0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 54, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 78, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 6, .adv_w = 121, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 9, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 23, .adv_w = 165, .box_w = 9, .box_h = 16, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 41, .adv_w = 186, .box_w = 11, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 58, .adv_w = 165, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 63, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 75, .adv_w = 88, .box_w = 5, .box_h = 17, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 86, .adv_w = 88, .box_w = 6, .box_h = 17, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 99, .adv_w = 90, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 9},
    {.bitmap_index = 103, .adv_w = 133, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 110, .adv_w = 65, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 113, .adv_w = 87, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 114, .adv_w = 65, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 100, .box_w = 8, .box_h = 14, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 130, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 165, .box_w = 8, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 170, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 165, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 213, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 241, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 255, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 269, .adv_w = 65, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 273, .adv_w = 65, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 278, .adv_w = 133, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 286, .adv_w = 133, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 290, .adv_w = 133, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 298, .adv_w = 154, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 312, .adv_w = 230, .box_w = 13, .box_h = 16, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 338, .adv_w = 186, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 176, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 156, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 382, .adv_w = 186, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 397, .adv_w = 157, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 409, .adv_w = 157, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 421, .adv_w = 179, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 436, .adv_w = 188, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 450, .adv_w = 92, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 455, .adv_w = 95, .box_w = 5, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 465, .adv_w = 184, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 480, .adv_w = 145, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 492, .adv_w = 239, .box_w = 13, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 512, .adv_w = 188, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 527, .adv_w = 179, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 542, .adv_w = 173, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 179, .box_w = 10, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 575, .adv_w = 176, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 590, .adv_w = 154, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 604, .adv_w = 154, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 618, .adv_w = 186, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 633, .adv_w = 190, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 651, .adv_w = 283, .box_w = 18, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 678, .adv_w = 182, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 695, .adv_w = 182, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 712, .adv_w = 164, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 726, .adv_w = 86, .box_w = 5, .box_h = 17, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 737, .adv_w = 103, .box_w = 8, .box_h = 14, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 751, .adv_w = 88, .box_w = 5, .box_h = 17, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 762, .adv_w = 133, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 766, .adv_w = 134, .box_w = 8, .box_h = 2, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 768, .adv_w = 83, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 10},
    {.bitmap_index = 769, .adv_w = 140, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 778, .adv_w = 152, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 791, .adv_w = 129, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 799, .adv_w = 152, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 812, .adv_w = 138, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 821, .adv_w = 94, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 829, .adv_w = 141, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 842, .adv_w = 154, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 855, .adv_w = 79, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 860, .adv_w = 81, .box_w = 4, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 868, .adv_w = 157, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 883, .adv_w = 80, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 888, .adv_w = 224, .box_w = 13, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 903, .adv_w = 154, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 912, .adv_w = 148, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 921, .adv_w = 152, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 933, .adv_w = 152, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 945, .adv_w = 111, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 952, .adv_w = 123, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 960, .adv_w = 100, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 969, .adv_w = 154, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 978, .adv_w = 154, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 990, .adv_w = 217, .box_w = 14, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1006, .adv_w = 154, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1018, .adv_w = 152, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1033, .adv_w = 130, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1041, .adv_w = 89, .box_w = 6, .box_h = 17, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1054, .adv_w = 91, .box_w = 3, .box_h = 17, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1061, .adv_w = 89, .box_w = 6, .box_h = 17, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1074, .adv_w = 144, .box_w = 7, .box_h = 3, .ofs_x = 1, .ofs_y = 10},
    {.bitmap_index = 1077, .adv_w = 157, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1092, .adv_w = 179, .box_w = 10, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1111, .adv_w = 138, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1126, .adv_w = 161, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1140, .adv_w = 154, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1154, .adv_w = 92, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1159, .adv_w = 92, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1169, .adv_w = 95, .box_w = 5, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1179, .adv_w = 272, .box_w = 16, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1203, .adv_w = 270, .box_w = 15, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1226, .adv_w = 179, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1241, .adv_w = 190, .box_w = 11, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1262, .adv_w = 182, .box_w = 11, .box_h = 15, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1283, .adv_w = 186, .box_w = 9, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1299, .adv_w = 186, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1317, .adv_w = 171, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1331, .adv_w = 177, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1345, .adv_w = 138, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1357, .adv_w = 199, .box_w = 13, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1382, .adv_w = 157, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1394, .adv_w = 268, .box_w = 17, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1420, .adv_w = 159, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1434, .adv_w = 191, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1449, .adv_w = 191, .box_w = 10, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1468, .adv_w = 190, .box_w = 11, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1485, .adv_w = 190, .box_w = 11, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1502, .adv_w = 239, .box_w = 13, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1522, .adv_w = 188, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1536, .adv_w = 179, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1551, .adv_w = 186, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1565, .adv_w = 173, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1579, .adv_w = 156, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1591, .adv_w = 154, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1605, .adv_w = 182, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1622, .adv_w = 221, .box_w = 13, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1642, .adv_w = 182, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1659, .adv_w = 191, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1679, .adv_w = 174, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1693, .adv_w = 271, .box_w = 15, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1716, .adv_w = 278, .box_w = 16, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1744, .adv_w = 200, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1762, .adv_w = 253, .box_w = 14, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1783, .adv_w = 169, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1797, .adv_w = 161, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1811, .adv_w = 256, .box_w = 14, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1832, .adv_w = 179, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1847, .adv_w = 140, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1856, .adv_w = 148, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1870, .adv_w = 142, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1879, .adv_w = 114, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1886, .adv_w = 163, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1901, .adv_w = 138, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1910, .adv_w = 222, .box_w = 13, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1925, .adv_w = 135, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1933, .adv_w = 160, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1942, .adv_w = 160, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1954, .adv_w = 153, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1965, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1976, .adv_w = 192, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1988, .adv_w = 158, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1997, .adv_w = 148, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2006, .adv_w = 155, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2015, .adv_w = 152, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2027, .adv_w = 129, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2035, .adv_w = 137, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2044, .adv_w = 152, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2059, .adv_w = 218, .box_w = 13, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2084, .adv_w = 154, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2096, .adv_w = 159, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2110, .adv_w = 151, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2119, .adv_w = 228, .box_w = 13, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2134, .adv_w = 234, .box_w = 14, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2155, .adv_w = 169, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2167, .adv_w = 214, .box_w = 12, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2181, .adv_w = 142, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2190, .adv_w = 134, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2198, .adv_w = 213, .box_w = 12, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2212, .adv_w = 151, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2223, .adv_w = 138, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2235, .adv_w = 154, .box_w = 9, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2252, .adv_w = 114, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2261, .adv_w = 129, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2269, .adv_w = 123, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2277, .adv_w = 79, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2282, .adv_w = 79, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2290, .adv_w = 81, .box_w = 4, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2298, .adv_w = 228, .box_w = 14, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2314, .adv_w = 225, .box_w = 13, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2329, .adv_w = 154, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2343, .adv_w = 153, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2357, .adv_w = 152, .box_w = 10, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2376, .adv_w = 156, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 2388, .adv_w = 138, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2401, .adv_w = 114, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_4[] = {
    0x0, 0x1, 0x32, 0x33
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1025, .range_length = 12, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1038, .range_length = 66, .glyph_id_start = 108,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1105, .range_length = 12, .glyph_id_start = 174,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 1118, .range_length = 52, .glyph_id_start = 186,
        .unicode_list = unicode_list_4, .glyph_id_ofs_list = NULL, .list_length = 4, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 0, 1, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 2, 3, 2,
    4, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    5, 0, 6, 7, 8, 9, 10, 11,
    12, 0, 0, 0, 13, 14, 0, 0,
    9, 15, 9, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 0, 25, 0, 0,
    0, 0, 26, 27, 0, 0, 27, 28,
    29, 30, 0, 0, 31, 0, 30, 30,
    27, 27, 0, 32, 0, 0, 0, 33,
    34, 35, 33, 36, 0, 0, 0, 0,
    10, 30, 37, 8, 17, 0, 0, 0,
    38, 38, 30, 39, 40, 0, 6, 41,
    7, 37, 42, 10, 39, 7, 0, 0,
    39, 0, 0, 0, 9, 0, 15, 8,
    18, 40, 43, 22, 42, 0, 0, 42,
    38, 0, 38, 9, 9, 0, 26, 27,
    0, 32, 44, 27, 45, 0, 0, 0,
    45, 0, 0, 0, 27, 0, 27, 0,
    46, 33, 27, 35, 44, 0, 0, 44,
    47, 0, 47, 27, 27, 0, 27, 30,
    28, 0, 0, 0, 0, 0, 47, 47,
    30, 45, 33, 0, 37, 28
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 0, 1, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 2, 3, 2,
    4, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    5, 0, 6, 0, 7, 0, 0, 0,
    7, 0, 0, 8, 0, 0, 0, 0,
    7, 0, 7, 0, 9, 10, 11, 5,
    12, 13, 14, 15, 0, 16, 0, 0,
    0, 0, 17, 0, 18, 18, 18, 19,
    20, 0, 0, 21, 0, 0, 22, 22,
    18, 22, 18, 22, 23, 24, 25, 26,
    27, 28, 26, 29, 0, 0, 0, 0,
    0, 0, 0, 7, 9, 0, 0, 8,
    30, 0, 10, 0, 31, 0, 6, 0,
    0, 0, 30, 0, 32, 33, 0, 0,
    0, 30, 0, 0, 7, 0, 0, 7,
    10, 31, 34, 13, 0, 35, 0, 0,
    10, 0, 0, 33, 0, 30, 17, 0,
    22, 22, 36, 18, 37, 38, 22, 22,
    22, 36, 22, 22, 18, 22, 22, 18,
    39, 26, 18, 28, 22, 40, 22, 22,
    39, 22, 22, 38, 22, 41, 18, 0,
    22, 18, 23, 0, 0, 21, 36, 22,
    0, 22, 26, 22, 0, 22
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, -24, -19, -31, 0, -21, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -10, -10, 0, -11, 0, 0, -8, 0,
    0, 0, 0, 0, 0, -9, 0, 0,
    0, -3, 0, -15, 0, -9, 0, 0,
    -12, -24, 0, -15, 0, -25, 0, -4,
    0, 0, -24, -4, -18, 0, -25, 0,
    -19, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -17, -13, 0, 0, 0, -21,
    0, 0, -7, -22, 0, 0, 0, -15,
    -14, 0, -19, -15, 0, 0, -9, -7,
    0, 0, -7, -12, 0, -7, -8, -16,
    -6, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -6, 0, 0,
    -19, 0, -6, 0, 0, -4, 0, 0,
    -3, 0, 0, 0, -21, 0, -31, 0,
    -18, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -15, -11, 0, -15, 0,
    -7, -8, 0, -7, 0, 0, -4, 0,
    -8, 0, 0, 0, 0, 0, -14, 0,
    -6, 0, 0, 0, 0, -46, 0, -23,
    0, -17, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -6, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -9, -5, 0, 0, 0, 0, -10,
    0, -4, 0, 0, -9, -21, 0, -7,
    0, -17, 0, -5, 15, 0, -14, -8,
    -15, 0, -24, 0, -9, -1, -3, -3,
    0, 6, 0, 0, -8, -5, -13, -8,
    0, 0, 0, -18, 0, 0, -6, -14,
    5, 0, -4, -14, -18, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, -5,
    0, 0, 0, -13, 0, 0, 0, 0,
    -5, 0, 0, 0, 0, -5, 0, -7,
    -7, -8, 0, 0, 0, 0, 0, 0,
    -5, 0, 0, -4, 0, 0, 0, 0,
    0, -10, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -4, 0,
    -5, -4, 0, 0, 0, 0, 0, 0,
    -9, 0, 0, 0, 0, -9, -17, 0,
    0, -4, 0, -5, -7, -5, 0, 0,
    0, -3, 0, -6, -7, -12, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -9, -6,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -16, 0, -19, 0, -14,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -10, -11, -2, -9, 0, -9,
    -8, -3, 0, -2, 0, 0, 0, -12,
    0, 0, 0, 0, 0, -19, 0, -11,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -3, 0, -4, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -6, 0, 0, 0, 0, -10, 0,
    0, 0, -11, 13, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -9, 0, 0,
    0, 0, 0, 0, -7, -10, -9, 0,
    0, 0, 0, 0, 0, -10, 0, 7,
    0, -6, -21, -24, 0, -26, 0, -6,
    0, -22, 0, -3, 12, 0, -21, -5,
    -20, 0, -28, 0, -24, 0, -5, -4,
    0, 0, 0, 0, -6, 0, -13, -10,
    0, 1, 1, -22, 0, 0, -9, -19,
    0, 0, -4, -23, -22, 0, 0, -22,
    0, -20, 0, -15, 0, 0, 0, 0,
    0, 0, -7, -10, -5, 0, 0, -5,
    0, 0, 0, -4, 0, 0, 0, 0,
    0, 0, 0, -11, -7, -3, 0, 0,
    0, -17, 0, -4, 0, 0, -10, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    -3, 0, -6, 0, -10, 0, 0, 0,
    -6, 0, 0, 0, 0, 0, 0, 0,
    -1, -2, 0, 0, 0, -8, 0, 0,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, 0, 0, 0, -8, 0,
    0, 0, 0, 0, 0, 0, -6, 0,
    0, 0, -24, -12, -20, 0, -14, -3,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -19, -20, -3, -19, 0, -11, -15,
    -4, -9, -7, -7, -6, -11, -13, 0,
    0, 0, -12, 0, -19, -10, -20, -10,
    -12, -19, 0, -4, 0, -5, 0, -8,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 0, 0, -7, 0, 0,
    0, 0, 0, 0, -25, -9, -22, 0,
    -17, -7, 0, -8, 0, 0, 0, 0,
    0, 0, 0, -18, -23, -5, -22, 0,
    -14, -23, -8, -13, -10, -9, -10, -14,
    -10, 0, 0, 0, -7, 0, -22, -19,
    -19, -14, -17, -21, 0, -18, -7, -18,
    0, -15, -6, -5, -6, 0, 0, 0,
    0, 0, 0, 0, -17, -17, -4, -17,
    -1, -11, -12, -5, -11, -9, -9, -8,
    -13, -7, 0, 0, 0, -8, 0, -20,
    -12, -16, -13, -15, -19, 0, 0, -8,
    0, 0, 0, -7, 5, -5, 0, 0,
    0, 0, 0, 0, 0, 0, -5, -2,
    0, 0, 0, 0, 0, 0, -12, -7,
    0, 0, 0, 0, 0, 0, -10, 0,
    0, 0, 0, -15, -19, 0, 0, -25,
    -16, -22, 0, -24, -12, -4, -6, 0,
    0, 0, 0, 0, 0, 1, -29, -27,
    -12, -23, 0, -21, -25, -13, -21, -15,
    -15, -10, -19, -14, 0, 0, 0, -12,
    0, -25, -15, -26, -17, -17, -26, 0,
    0, -6, 0, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -6, -4, 0, 0, 0, 0, 0, 0,
    -5, 0, 0, 0, 0, -10, -10, 0,
    -29, 0, 0, 0, -29, 0, -9, 24,
    -7, -21, -10, -19, 0, -24, 0, -30,
    0, -4, 0, 0, 0, 0, 0, -5,
    -4, -12, -9, 0, 0, 0, -27, 0,
    0, -10, -18, 0, 0, 0, -12, -11,
    0, -10, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, -7, -4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -7,
    -7, 0, -9, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -3, -2, -4, -1, 0,
    0, 0, 0, 0, 0, 0, -2, 0,
    0, 0, 0, 0, -7, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -11, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -2, -5, 0, -9, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -7, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -4, 0, 0, 0, 0,
    -13, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -13, 0, -3, 0, 0, -3,
    0, -17, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -6, -3, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -14, 0, -2, 0, 0,
    -6, 0, -13, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -4, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -10, 0, -3, 0,
    0, -4, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -33, -22, -26,
    0, -19, -6, -7, 0, 0, 0, 0,
    0, 0, 0, 8, -21, -23, -3, -19,
    0, -16, -18, -6, -10, -9, -9, -9,
    -14, -19, 0, 0, 0, -11, 0, -28,
    -14, -23, -10, -15, -21, -21, 0, 0,
    0, -16, 0, 0, 0, 0, -19, 0,
    -12, -4, -27, 0, -20, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -12, -6,
    -4, -2, -1, -25, 0, 0, 0, -12,
    0, -2, -3, -9, -8, -1, 0, 0,
    0, 0, 0, 0, -6, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    0, 0, 0, 5, 0, 0, 0, -6,
    0, 0, 0, 0, -12, -14, 0, 0,
    -28, -20, -31, 0, -26, -13, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -27,
    -23, -7, -20, 0, -17, -19, -5, -17,
    -13, -10, -13, -17, -19, 0, 0, -6,
    -14, 0, -29, -19, -24, -19, -17, -21,
    0, 0, 0, 0, 0, -3, 0, 0,
    0, -8, 0, 0, -7, -9, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -4, -4, -8, -5, -1, 0, -4,
    -4, 0, -5, 0, 0, 0, -5, 0,
    0, 0, 0, 0, 0, -9, 0, -1,
    22, 0, -7, 0, -5, 0, -8, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -6, 0, 0, 0, 16, -8,
    0, 0, -3, -3, 14, 0, 0, -10,
    -9, 0, 0, 0, 0, -10, -9, -9,
    0, 0, 0, -10, 0, -4, -10, -13,
    -7, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -7,
    -13, -4, -6, 0, -2, -8, 0, 0,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 13, 0,
    0, -9, -10, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, -13, -3,
    -15, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -13, 0, 0, 0, 0, -4, -22, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -7,
    -6, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -13, -11, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 47,
    .right_class_cnt     = 41,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 5,
    .bpp = 1,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_BaseFont16 = {
#else
lv_font_t ui_font_BaseFont16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 18,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_BASEFONT16*/

