#include "MKL25z4.h"
//******************************************************************************************************
// File: PIT timer.c	by Professor Aamodt
//******************************************************************************************************

#define MASK(x) (1UL << (x))
#define DEBUG_PORT PTD

extern volatile uint32_t count;
extern volatile uint32_t switchPress;
extern volatile uint32_t ADC_in;


void Init_PIT(uint32_t period) {
// Enable clock to PIT module
SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
// Enable module, freeze timers in debug mode
PIT->MCR &= ~PIT_MCR_MDIS_MASK;
PIT->MCR |= PIT_MCR_FRZ_MASK;
// Initialize PIT0 to count down from argument
PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(period);
// No chaining
PIT->CHANNEL[0].TCTRL &= PIT_TCTRL_CHN_MASK;
// Generate interrupts
PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TIE_MASK;
/* Enable Interrupts */
NVIC_SetPriority(PIT_IRQn, 64); // 0, 64, 128 or 192
NVIC_ClearPendingIRQ(PIT_IRQn);
NVIC_EnableIRQ(PIT_IRQn);
}

void Start_PIT(void) {
// Enable counter
PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TEN_MASK;
}

void Stop_PIT(void) {
// Enable counter
PIT->CHANNEL[0].TCTRL &= ~PIT_TCTRL_TEN_MASK;
}

void PIT_IRQHandler() {
//clear pending IRQ
NVIC_ClearPendingIRQ(PIT_IRQn);
// check to see which channel triggered interrupt
if (PIT->CHANNEL[0].TFLG & PIT_TFLG_TIF_MASK) {
// clear status flag for timer channel 0
PIT->CHANNEL[0].TFLG &= PIT_TFLG_TIF_MASK;
// Do ISR work
// your code goes here
//DEBUG_PORT->PSOR=MASK(0);

//DEBUG_PORT->PCOR=MASK(0);
} else if (PIT->CHANNEL[1].TFLG & PIT_TFLG_TIF_MASK) {
// clear status flag for timer channel 1
PIT->CHANNEL[1].TFLG &= PIT_TFLG_TIF_MASK;
}
}
