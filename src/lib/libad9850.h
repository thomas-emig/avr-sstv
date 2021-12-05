#ifndef _LIBAD9850
#define _LIBAD9850

// integer type definitions
#include <stdint.h>
// register definitions
#include <avr/io.h>
// interrupt support
#include <avr/interrupt.h>

// FIFO definitions
#define DFIFO_SIZE 128

// type definition
typedef struct {
    uint32_t freq;  // frequency for the dds chip
    uint8_t  phase; // phase value of ad9850
    uint16_t time;  // wait time until next frequency change in 64us steps
} dds_t;

// function prototypes
void spi_init(void);
void spi_send_command(uint32_t freq, uint8_t phase);
void ad9850_init(void);
void init_timer1(void);
void start_timer1(uint16_t max);
void stop_timer1(void);
void dfifo_push(dds_t new_element);
dds_t dfifo_pop(void);
void dds_enqueue(uint32_t frequency, uint8_t phase, uint16_t time);

#endif