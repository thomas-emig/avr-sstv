// integer type definitions
#include <stdint.h>
// register definitions
#include <avr/io.h>
// ov7670 camera library
#include "libov7670.h"
// ad9850 library
#include "libad9850.h"
// morse code library
#include "libmorse.h"

#define FREQUENCY 0                   // set tx frequency in kHz here
#define MORSE_WPM 20                  // morse speed in words per minute
#define WAIT_TIME 0x00FFFFFF          // wait time between pictures - not an exact time measure

#define F_AD9850  FREQUENCY*34360     // frequency value as ad9850 data word
#define AD_800HZ  27488               // some ad9850 frequency values
#define AD_1100HZ 37796
#define AD_1200HZ 41232
#define AD_1300HZ 44668
#define AD_1500HZ 51540
#define AD_1900HZ 65284
#define T_DIV 4                       // parameters how to calculate time value from actual time
#define T_OFFSET 0
#define T_MORSE (1200000/MORSE_WPM)   // (T_morse = 1200000 / WPM) in us

char callsign[] = "DN9AAA";
char message[]  = "sent with avr-sstv by ";

uint16_t pixeldata[72] = {
    0b1000011111110001, // A
    0b1000100000011001,
    0b1001011011111001,
    0b1001011011111001,
    0b1001011011110001,
    0b1001100000011001,
    0b1000111111111001,
    0b1000011111111001,
    0b1000000000000001,
    0b1000111100000001, // V
    0b1001000111100001,
    0b1001111000110001,
    0b1000111111011001,
    0b1000001111011001,
    0b1000111000111001,
    0b1001000111110001,
    0b1001111111100001,
    0b1000111100000001,
    0b1000000000000001,
    0b1000111111110001, // R
    0b1001000000011001,
    0b1001011011111001,
    0b1001011011111001,
    0b1001011011110001,
    0b1001100100011001,
    0b1000111111111001,
    0b1000011011111001,
    0b1000000000000001,
    0b1000000110000001, // -
    0b1000001011000001,
    0b1000001011000001,
    0b1000001011000001,
    0b1000001011000001,
    0b1000001111000001,
    0b1000000111000001,
    0b1000000000000001,
    0b1000011100110001, // S
    0b1000100111011001,
    0b1001011011011001,
    0b1001011011011001,
    0b1001011011011001,
    0b1001011100111001,
    0b1001111111111001,
    0b1000111011110001,
    0b1000000000000001,
    0b1000011100110001, // S
    0b1000100111011001,
    0b1001011011011001,
    0b1001011011011001,
    0b1001011011011001,
    0b1001011100111001,
    0b1001111111111001,
    0b1000111011110001,
    0b1000000000000001,
    0b1000110000000001, // T
    0b1001011000000001,
    0b1001011111110001,
    0b1001000000011001,
    0b1001011111111001,
    0b1001011111111001,
    0b1001111000000001,
    0b1000111000000001,
    0b1000000000000001,
    0b1000111100000001, // V
    0b1001000111100001,
    0b1001111000110001,
    0b1000111111011001,
    0b1000001111011001,
    0b1000111000111001,
    0b1001000111110001,
    0b1001111111100001,
    0b1000111100000001,
};

reg_t cam_init_seq[] = { // ov7670 camera init sequence
    {OV7670_CLKRC,              0x01}, {OV7670_COM7,                 0x01}, {OV7670_COM3,                     0x00},
    {OV7670_COM14,              0x00}, {OV7670_SCALING_XSC,          0x3A}, {OV7670_SCALING_YSC,              0x35},
    {OV7670_SCALING_DCWCTR,     0x11}, {OV7670_SCALING_PCLK_DIV,     0xF0}, {OV7670_SCALING_PCLK_DELAY,       0x02},  // output format and scaling options
    {OV7670_MVFP,         0b00000000},                                                                                // mirror bits
    {OV7670_COM8,         0b11100111}, {OV7670_NALG,           0b00000000}, {OV7670_AEW,                      0x75},  // settings for AGC, AEC and AWB
    {OV7670_AEB,                0x63}, {OV7670_VPT,                  0xD4}, {OV7670_COM4,               0b00000000},
    {OV7670_COM9,         0b01100000}, {OV7670_ABLC1,          0b00000100}, {OV7670_THL_ST,                   0x80},
    {OV7670_THL_DLT,            0x04}, {OV7670_AWBCTR0,        0b00001101}, {OV7670_COM16,              0b00111000},
    {OV7670_EOT,          0b00000000}                                                                                 // end of sequence
};

uint32_t freq_conv[16] = {  // frequency conversion table
    51540, 53372, 55205, 57037,
    58870, 60702, 62535, 64367,
    66200, 68032, 69865, 71697,
    73530, 75362, 77195, 79027
}; // frequency range 1500-2300Hz

uint16_t pixel_counter = 0;   // reading pixel #
uint8_t  line_type     = 0;   // 0: green line, 1: blue line, 2: red line
uint8_t  line_data_blue[320]; // buffer for the blue line data

volatile uint32_t tx_freq = F_AD9850;           // frequency for transmission of sstv - AD9850 value
volatile uint8_t upper_sideband = 0;            // 0 -> lower sideband, 1 -> upper sideband

/* -- functions --------------------------------------------------------------------- */
void dds_enqueue_ssb (uint32_t bb_freq, uint32_t time){
    uint32_t frequency;

    if(upper_sideband){
        frequency = tx_freq + bb_freq;
    }else{
        frequency = tx_freq - bb_freq;
    }
    if(bb_freq == 0){
        frequency = 0;
    }

    dds_enqueue(frequency, 0, (uint16_t)(time/T_DIV)-T_OFFSET);
}

// sstv sends green -> blue -> red
// bayer pattern:
//   bgbg...
//   grgr...
uint8_t get_sstv_pixel(void){
    uint8_t buffer = 0;

    switch(line_type){
        case 0:                // reading green line
            line_data_blue[pixel_counter] = cam_get_byte(); // blue value
            buffer = cam_get_byte();                        // green value
            break;
        case 1:                // reading blue line
            buffer = line_data_blue[pixel_counter];
            break;
        case 2:                // reading red line
            cam_get_byte();                                 // green value
            buffer = cam_get_byte();                        // red value
            break;
    }

    pixel_counter++;
    if(pixel_counter == 320){
        pixel_counter = 0;
        line_type++;
        if(line_type == 3){
            line_type = 0;
        }
    }

    return buffer;
}

void init(void){
    volatile uint32_t k = 0;

    pixel_counter = 0;
    line_type     = 0;

    // determine if upper or lower sideband modulation is needed
    if(tx_freq > 373597383){ // tx frequency above 10MHz -> upper sideband
        upper_sideband = 1;
    }else{ // tx frequency below 10MHz -> lower sideband
        if(tx_freq == 0){ // if tx frequency is 0: transmit baseband signals -> equivalent to upper sideband
            upper_sideband = 1;
        }else{
            upper_sideband = 0;
        }
    }

    cam_init();
    twi_init();

    init_timer1();
    ad9850_init();

    sei();

    // set AD9850 frequency and phase to 0
    dds_enqueue_ssb(0, 1024/4);

    cam_set_register(OV7670_COM7, 0x80);  // reset cam
    for(k=0;k<0x0000FFFF;k++);            // wait at least 1ms

    // init cam registers
    k = 0;
    while(cam_init_seq[k].addr != OV7670_EOT){
        cam_set_register(cam_init_seq[k].addr, cam_init_seq[k].data);
        k++;
    }
}

/* -- sstv encoder ------------------------------------------------------------------ */
void transmit_vis_header(uint8_t vis_mode){
    uint8_t i = 0;
    uint8_t parity = 0;

    dds_enqueue_ssb(AD_1900HZ, 150000);    // leader tone
    dds_enqueue_ssb(AD_1900HZ, 150000);
    dds_enqueue_ssb(AD_1200HZ,  10000);    // break
    dds_enqueue_ssb(AD_1900HZ, 150000);    // leader tone
    dds_enqueue_ssb(AD_1900HZ, 150000);
    dds_enqueue_ssb(AD_1200HZ,  30000);    // start bit    
    for(i=0;i<7;i++){                      // seven data bits LSB first
        if((vis_mode & 0x01) == 0x01){
            dds_enqueue_ssb(AD_1100HZ, 30000);
            parity++;
        }else{
            dds_enqueue_ssb(AD_1300HZ, 30000);
        }
        vis_mode >>= 1;
    }
    if((parity & 0x01) == 0x01){           // parity bit
        dds_enqueue_ssb(AD_1100HZ, 30000);
    }else{
        dds_enqueue_ssb(AD_1300HZ, 30000);
    }
    dds_enqueue_ssb(AD_1200HZ, 30000);     // stop bit
    dds_enqueue_ssb(0, 30000);             // pause between vis header and picture
}

void sstv_send_line(uint8_t header, uint8_t line_ctr){
    uint16_t i = 0;
    uint8_t  k = 0;
    uint8_t  buf = 0;

    dds_enqueue_ssb(AD_1200HZ, 4862);    // sync pulse
    dds_enqueue_ssb(AD_1500HZ,  572);    // sync porch
    for(k=0;k<3;k++){                    // green, blue and red scan
        for(i=0;i<320;i++){    
            if(header){
                if(i>247){
                    buf = ((pixeldata[i-248] << line_ctr) & 0x8000) ? 0x00 : 0xFF;
                }else{
                    buf = 0xFF;
                }
            }else{
                buf = get_sstv_pixel();
            }
            dds_enqueue_ssb(freq_conv[(buf & 0xF0) >> 4], 458);
        }
        dds_enqueue_ssb(AD_1500HZ, 572); // separator pulse
    }
}

/* -- morse implementations -------------------------------------------------------- */
void morse_dit(void){
    dds_enqueue_ssb(AD_800HZ, T_MORSE);
    dds_enqueue_ssb(0,        T_MORSE);
}

void morse_dah(void){
    dds_enqueue_ssb(AD_800HZ, 3*T_MORSE);
    dds_enqueue_ssb(0,          T_MORSE);
}

void morse_wordspace(void){
    dds_enqueue_ssb(0, 4*T_MORSE);
}

void morse_letterspace(void){
    dds_enqueue_ssb(0, 3*T_MORSE);
}

/* -- main -------------------------------------------------------------------------- */
int main(void){
    volatile uint32_t k = 0;

    init();

    while(1){

        for(k=0;k<WAIT_TIME;k++); // wait some long time

        cam_take_image();
        pixel_counter = 0;
        line_type     = 0;

        // read some dummy bytes
        for(k=0;k<9;k++){
            cam_get_byte();
        }

        // send call sign
        morse_str(callsign);
        dds_enqueue_ssb(0, 4*T_MORSE); // word space

        // start transmission of sstv
        transmit_vis_header(44); // 44 is martin1 header

        for(k=0;k<16;k++){ // send 16 line header
            sstv_send_line(1, k);
        }

        for(k=0;k<240;k++){ // send 240 lines of the picture
            sstv_send_line(0, 0);
        }
        dds_enqueue_ssb(0, 1024/4); // pause

        // send message and callsign
        morse_str(message);
        morse_str(callsign);
        dds_enqueue_ssb(0, 4*T_MORSE); // word space
    }

    while(1);
    return 0;
}