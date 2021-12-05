#include "libov7670.h"

// global variables
volatile register_comm_t reg = {0x00, 0x00, false, false, true};

/* -- TWI functions ----------------------------------------------------------------- */
void twi_init(void){
    DDRC &= ~((1 << DDC5) | (1 << DDC4));   // enable pull up resisitors
    PORTC |= (1 << PORTC5) | (1 << PORTC4);
    TWSR  =  0x00;
    TWBR  =  0x10;
}

void cam_set_register(uint8_t addr, uint8_t data){
    while(reg.active);              // wait for twi operation to finish
    reg.addr = addr;
    reg.data = data;
    reg.write = true;
    reg.phase1 = true;

    reg.active = true;
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);              // send start condition
}

uint8_t cam_read_register(uint8_t addr){ // doesnt work because cam sends NACK after SLA_R
    while(reg.active);              // wait for twi operation to finish
    reg.addr = addr;
    reg.write = false;
    reg.phase1 = true;

    reg.active = true;
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);              // send start condition
    while(reg.active);              // wait for twi operation to finish
    return reg.data;
}

/* -- ov7670 functions -------------------------------------------------------------- */
void cam_init(void){
    DDRC &= ~(1 << VSYNC);                        // PC0 VSYNC                 input
    DDRC |= (1 << RCK)|(1 << WR)| (1 << OE);      // PC1 RCK;  PC2 WR; PC3 OE  output
    DDRB |= 1 << RRST;                            // PB0 RRST                  output
    DDRD &= 0b00000011;                           // PD2-PD7 D2-D7             input
    DDRC &= ~(1 << D1);                           // PC3 D1                    input
    //DDRB &= ~(1 << D0);                           // PB2 D0                    input
    DDRD &= ~(1 << HREF);                         // PB1 HREF                  input
    DDRD |= (1 << WRST);                          // PB4 WRST                  output

    PORTC &= ~((1 << RCK)|(1 << WR)|(1 << OE));   // RCK low, WR low, OE low
    PORTB |= 1 << RRST;                           // RRST high
    PORTD |= 1 << WRST;                           // WRST high

    PORTC &= (1 << VSYNC) ;                       // disable pull-up resistors on VSYNC
    PORTD &= 0b00000011;                          // disable pull-up resistors on D2-D7
    PORTC &= ~(1 << D1);                          // disable pull-up resistors on D1
    //PORTB &= ~(1 << D0);                          // disable pull-up resistors on D0
    PORTD &= ~(1 << HREF);                        // disable pull-up resistors on HREF
}

void cam_take_image(void){
    volatile uint16_t counter;

    cli();

    // generate read reset
    PORTB &= ~(1 << RRST);               // RRST low
    PORTC |= 1 << RCK;                   // RCK high
    for(counter=0;counter<50;counter++);
    PORTC &= ~(1 << RCK);                // RCK low
    PORTB |= 1 << RRST;                  // RRST high

    if(!(PINC & (1 << VSYNC))){          // wait while VSYNC low
        while(!(PINC & (1 << VSYNC)));
    }
    PORTC |= 1 << WR;                    // WR high
    PORTD &= ~(1 << WRST);               // WRST low
    while(PINC & (1 << VSYNC));          // wait while VSYNC high
    PORTD |= 1 << WRST;                  // WRST high
    // count 480 HREF pulses
    for(counter=0;counter<480;counter++){
        while(!(PIND & (1 << HREF)));    // wait while HREF low
        while(PIND & (1 << HREF));       // wait while HREF high
    }
    PORTC &= ~(1 << WR);                 // WR low

    sei();
}

uint8_t cam_get_byte(void){
    volatile uint8_t counter;
    uint8_t buf = 0;
    PORTC |= 1 << RCK;                 // RCK high
    for(counter=0;counter<50;counter++);
    PORTC &= ~(1 << RCK);              // RCK low
    //buf = (PIND & 0b11111100) | (((PINC & 0b00001000)|(PINB & 0b00000100)) >> 2); // get 8 bit input data
    buf = (PIND & 0b11111100) | ((PINC & 0b00001000) >> 2);                         // get 7 bit input data
    return buf;
}

/* -- interrupt TWI operation ------------------------------------------------------- */
ISR(TWI_vect){ // TWI Interface
    if(TWSR == TW_START){ // start condition sent
        if(reg.phase1 || reg.write){
            TWDR = SLA_W;      // send SLA_W
        }else{
            TWDR = SLA_R;      // send SLA_R
        }
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE); // send
    }
    if(TWSR == TW_MT_SLA_ACK){ // slave ack after write addr received
        TWDR = reg.addr;   // send addr
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE); // send
    }
    if(TWSR == TW_MT_DATA_ACK){ // slave ack after data received
        if(reg.phase1 && reg.write){
            reg.phase1 = false;
            TWDR = reg.data; // send data
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE); // send
        }else{
            if(reg.write){
                TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE);     // send stop condition
                reg.active = false;
            }else{
                reg.phase1 = false;
                TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE); // send stop followed by start condition
            }
        }
    }
    if(TWSR == TW_MR_SLA_ACK){ // slave ack after read addr received
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE); // receive data and send NACK
    }
    if(TWSR == TW_MR_DATA_NACK){ // data byte received
        reg.data = TWDR;
        TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE);     // send stop condition
        reg.active = false;
    }

    // error handling
    if(TWSR == TW_MT_DATA_NACK){ // slave nack after data received - error
        TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE); // send stop followed by start condition
        reg.phase1 = true;
    }
    if(TWSR == TW_MT_SLA_NACK){ // error after write addr transmitted
        TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE); // send stop followed by start condition
        reg.phase1 = true;
    }
    if(TWSR == TW_MR_SLA_NACK){ // error after read addr transmitted
        TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE);     // send stop condition
        reg.active = false;
    }
}