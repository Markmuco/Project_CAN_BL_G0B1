#ifndef _TIME
#define _TIME

#include "stdbool.h"
#include "stdint.h"

#define min2ms(a) (uint32_t) (a *1000*60)
#define sec2ms(a) (uint32_t) (a *1000)

// Maximum number of timers
#define MAX_TIMERS     (16) // Must be a multiple of 8!

// Define invalid timer
#define NO_TIMER       (250)

// Public function prototypes
bool timer_open(void);
uint8_t timer_get(void);
bool timer_free(uint8_t *timerid);
bool timer_start(uint8_t timerid, uint32_t value, void *p_fxn);
bool timer_stop(uint8_t timerid);
bool timer_resume(uint8_t timerid);
bool timer_elapsed(uint8_t timerid);
bool delay_us(uint32_t time_us);
bool delay_ms(uint32_t time_ms);
uint32_t timer_read(uint8_t timerid);
void timer_callback(void);

  
#endif




