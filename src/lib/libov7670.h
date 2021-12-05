#ifndef _LIBOV7670
#define _LIBOV7670

// integer type definitions
#include <stdint.h>
// boolean type definitions
#include <stdbool.h>
// register definitions
#include <avr/io.h>
// interrupt support
#include <avr/interrupt.h>
// twi definitions
#include <util/twi.h>

// ov7670 register names
#include "ov7670_reg.h"

// TWI definitions
#define SLA_W 0x42
#define SLA_R 0x43
// pin definitions
#define VSYNC 0   // PC0
#define RCK   1   // PC1
#define WR    2   // PC2
#define OE    3   // PC3
#define RRST  0   // PB0
#define HREF  1   // PD0
#define WRST  0   // PD1
//#define D0    2   // PB2
#define D1    3   // PC3
#define D2    2   // PD2
#define D3    3   // PD3
#define D4    4   // PD4
#define D5    5   // PD5
#define D6    6   // PD6
#define D7    7   // PD7

// data types
typedef struct {
    uint8_t addr;   // register addr
    uint8_t data;   // register data
    bool    write;  // read or write access
    bool    active; // is the twi module active
    bool    phase1; // read mode: first or second transmission - write mode: addr or data byte sent
} register_comm_t;

typedef struct {
    uint8_t addr;
    uint8_t data;
} reg_t;

// function prototypes
void twi_init(void);
void cam_set_register(uint8_t addr, uint8_t data);
uint8_t cam_read_register(uint8_t addr);
void cam_init(void);
void cam_take_image(void);
uint8_t cam_get_byte(void);

#endif