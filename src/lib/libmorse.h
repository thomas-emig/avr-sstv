#ifndef _LIBMORSE
#define _LIBMORSE

// integer type definitions
#include <stdint.h>
// register definitions
#include <avr/io.h>

// function prototypes
char to_uppercase(char input);
void morse_char(char input);
void morse_str(char* str);

// external functions
extern void morse_dit(void);
extern void morse_dah(void);
extern void morse_wordspace(void);
extern void morse_letterspace(void);

#endif