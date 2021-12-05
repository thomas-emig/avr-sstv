// library header
#include "libmorse.h"

// global variables
// morsecode table: 11 -> dah, 00 -> dit, 10 -> word space, 01 -> end
static const uint16_t morsecode[] = {// ascii 32-95
                        0b1001000000000000, 0b1100110011110100, 0b0011000011000100, 0b0100000000000000, // space ! " n
                        0b0100000000000000, 0b0100000000000000, 0b0100000000000000, 0b0011111111000100, // n n n '
                        0b1100111100110100, 0b1100111100110100, 0b0100000000000000, 0b0011001100010000, // ( ) n +
                        0b1111000011110100, 0b1100000000110100, 0b0011001100110100, 0b1100001100010000, // , - . /
                        0b1111111111010000, 0b0011111111010000, 0b0000111111010000, 0b0000001111010000, // 0 1 2 3
                        0b0000000011010000, 0b0000000000010000, 0b1100000000010000, 0b1111000000010000, // 4 5 6 7
                        0b1111110000010000, 0b1111111100010000,                                         // 8 9
                        0b1111110000000100, 0b1100110011000100, 0b0100000000000000,                     // : ; n
                        0b1100000011010000, 0b0100000000000000, 0b0000111100000100, 0b0011110011000100, // = n ? @
                        0b0011010000000000, 0b1100000001000000, 0b1100110001000000, 0b1100000100000000, // A B C D
                        0b0001000000000000, 0b0000110001000000, 0b1111000100000000, 0b0000000001000000, // E F G H
                        0b0000010000000000, 0b0011111101000000, 0b1100110100000000, 0b0011000001000000, // I J K L
                        0b1111010000000000, 0b1100010000000000, 0b1111110100000000, 0b0011110001000000, // M N O P
                        0b1111001101000000, 0b0011000100000000, 0b0000000100000000, 0b1101000000000000, // Q R S T
                        0b0000110100000000, 0b0000001101000000, 0b0011110100000000, 0b1100001101000000, // U V W X
                        0b1100111101000000, 0b1111000001000000,                                         // Y Z
                        0b0100000000000000, 0b0100000000000000, 0b0100000000000000, 0b0000111100110100  // n n n _
};

// functions
char to_uppercase(char input){             // convert ascii char to uppercase
    if((input > 96) && (input < 123)){
        return input - 32;
    }else{
        return input;
    }
}

void morse_char(char input){
    uint16_t morsechar = 0b0100000000000000;
    uint8_t  buffer    = 0;
    uint8_t  i         = 0;

    input = to_uppercase(input);

    if((input > 31) && (input < 96)){
        morsechar = morsecode[input - 32];
    }

    for(i=14; i!=0; i-=2){
        buffer = (uint8_t)((morsechar >> i) & 3);
        if(buffer == 0b00){                     // dit
            morse_dit();
        }
        if (buffer == 0b11){                    // dah
            morse_dah();
        }
        if (buffer == 0b10){                    // word space
            morse_wordspace();
        }
        if (buffer == 0b01){                    // end
            morse_letterspace();
            break;
        }
    }
}

void morse_str(char* str){
    uint8_t counter;

    counter = 0;
    while(str[counter] != 0){
        morse_char(str[counter]);
        counter++;
    }
}