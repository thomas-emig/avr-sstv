// library header
#include "libad9850.h"

// global variables
dds_t   dfifo[DFIFO_SIZE];               // fifo variables
uint8_t dfifo_head = 0;
uint8_t dfifo_tail = 0;
volatile uint8_t dfifo_full = 0;
volatile uint8_t dfifo_empty = 1;
volatile uint8_t timer_running = 0;      // lock variable for timer 1
volatile uint8_t spi_wordcnt = 0;        // currently transmitted word on spi
volatile uint8_t spi_data[5];            // spi data array

/* -- SPI --------------------------------------------------------------------------- */
void spi_init(void){
    // enable SPI master LSB first, set clock polarity, clock phase and clock rate = fosc/2
    SPCR = (1 << SPE) | (1 << DORD) | (1 << MSTR) | (0 << SPR0) | (1 << SPIE);
    SPSR |= (1 << SPI2X);
}

void spi_send_command(uint32_t freq, uint8_t phase){
    spi_data[0] = (uint8_t)  freq;
    spi_data[1] = (uint8_t) (freq >>  8);
    spi_data[2] = (uint8_t) (freq >> 16);
    spi_data[3] = (uint8_t) (freq >> 24);
    spi_data[4] = (phase << 3) & 0b11111000;

    SPDR = spi_data[0];          // start transmission
    spi_wordcnt = 4;             // transmit 4 more bytes
}

/* -- AD9850 ------------------------------------------------------------------------ */
void ad9850_init(void){
    // reset the chip, generate W_CLK and FQ_UD pulse,finally initialize spi module
    volatile uint8_t i = 0;

    // set all pins used for SPI comm to output mode
    DDRB  |= (1 << DDB1) | (1 << DDB2) | (1 << DDB3) | (1 << DDB4) | (1 << DDB5); // set PB 1-5 as output
    PORTB &= ~((1 << PORTB1) | (1 << PORTB2) | (1 << PORTB3) | (1 << PORTB4) | (1 << PORTB5)); // set all pins low

    // generate reset pulse,
    PORTB |= (1 << PORTB4);  // reset = 1
    for(i=0;i<255;i++);      // wait some time
    PORTB &= ~(1 << PORTB4); // reset = 0
    for(i=0;i<255;i++);      // wait some time

    // generate W_CLK pulse
    PORTB |= (1 << PORTB2);  // w_clk = 1
    PORTB &= ~(1 << PORTB2); // w_clk = 0

    // generate FQ_UD pulse
    PORTB |= (1 << PORTB1);  // fq_ud = 1
    PORTB &= ~(1 << PORTB1); // fq_ud = 0

    spi_init();
}

/* -- timer ------------------------------------------------------------------------- */
void init_timer1(void){
    // clear timer1 on compare match A (CTC mode)
    TIMSK1 = (1 << OCIE1A);                    // enable timer1 compare A interrupt 
    TCCR1B = 0b01 << WGM12;                    // set CTC mode
}

void start_timer1(uint16_t max){
    OCR1A   = max;                             // set MAX value
    TCCR1B |= 0b011 << CS10;                   // set prescaler to 64 - also starts timer (a value of 0b101 would result in a 1024 prescaler)
    timer_running = 1;
    PORTB |= 1 << PORTB2;                      // activate tx amp
}

void stop_timer1(void){
    TCCR1B &= ~(0b111 << CS10);                // clear timer1 input frequency selection
    timer_running = 0;
    PORTB &= ~(1 << PORTB2);                   // deactivate tx amp
}

/* -- DDS FIFO ---------------------------------------------------------------------- */
void dfifo_push(dds_t new_element){    // push a new element onto fifo - blocking call
    while(dfifo_full);                 // wait until space in fifo available
    dfifo[dfifo_head] = new_element;   // write new element to fifo

    dfifo_head++;                      // increase pointer (eventually wrap around)
    if(dfifo_head == DFIFO_SIZE){
        dfifo_head = 0;
    }

    if(dfifo_head == dfifo_tail){      // fifo is now full
        dfifo_full = 1;
    }

    dfifo_empty = 0;                   // if fifo was empty change status
}

dds_t dfifo_pop(void){                 // return an element from fifo
    dds_t element = {
        .freq  = 0x00000000,
        .phase = 0x00,
        .time  = 0x0000
    };

    if(!dfifo_empty){                   // only pop element if fifo is not empty
        element = dfifo[dfifo_tail];    // get last element from fifo

        dfifo_tail++;                   // increase pointer (eventually wrap around)
        if(dfifo_tail == DFIFO_SIZE){
            dfifo_tail = 0;
        }

        if(dfifo_head == dfifo_tail){   // fifo is now empty
            dfifo_empty = 1;
        }

        dfifo_full = 0;                 // if fifo was full change status

    }
    return element;
}

void dds_enqueue(uint32_t frequency, uint8_t phase, uint16_t time){ // set a new frequency change point
    dds_t element = {
        .freq = frequency,
        .phase = phase,          
        .time = time
    };

    if(timer_running){                  // if timer already working just push element onto fifo
        dfifo_push(element);
    }else{                              // if timer not running directly set timer
        start_timer1(element.time);
        spi_send_command(element.freq, element.phase);
    }
}

/* -- interrupt --------------------------------------------------------------------- */
ISR(SPI_STC_vect){ // executes on SPI transfer complete
    volatile uint8_t __attribute__((unused)) buf = SPSR; // perform a dummy read to clear interrupt flag
    buf = SPDR;

    if(spi_wordcnt != 0){        // load next byte into spi data register if necessary
         SPDR = spi_data[5 - spi_wordcnt];
         spi_wordcnt--;
    }else{ // last word has been transmitted
        // generate FQ_UD pulse
        PORTB |= (1 << PORTB1); // PB1 on
        PORTB &= ~(1 << PORTB1); // PB1 off
    }
}

ISR(TIMER1_COMPA_vect){ // timer1 match A interrupt
    // no need to clear interrupt flag - is cleared by hardware
    dds_t element = {
        .freq  = 0x00000000,
        .phase = 0x00,
        .time = 0x0000
    };

    if(!dfifo_empty){                // if there is data in the fifo set new frequency change
        element = dfifo_pop();
        OCR1A   = element.time;
        spi_send_command(element.freq, element.phase);
    }else{                           // stop timer if there is nothing more to do
        stop_timer1();
    }
}