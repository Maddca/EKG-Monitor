//***************************************************************************
//  file:  switches.c
//***************************************************************************
#include <MKL25Z4.H>

#define MASK(x) (1UL << (x))

// Debug status bits
#define DBG_ISR_POS (0)
#define DBG_MAIN_POS (1)

#define DEBUG_PORT PTD
// Switches is on port D for interrupt support
#define SW_Pos1 (1)
#define SW_Pos2 (2)
#define SW_Pos3 (4)
#define SW_Pos4 (5)
#define SW_Pos5 (12)
// Function prototypes
extern void init_switch(void);

// Shared variables
extern volatile unsigned count;
extern volatile uint32_t mode;
extern volatile uint32_t dacMem;
extern volatile uint32_t ADCsampleRate;
extern volatile uint32_t dataPoints;
extern volatile uint32_t Save;
extern volatile uint32_t changeFlag;
//***************************************************************************
//	Initialize Switches															Professor Aadmodt			 //
//***************************************************************************
void init_switch(void) {
	SIM->SCGC5 |=  SIM_SCGC5_PORTA_MASK; /* enable clock for port A (was D) */

	
	// Clear
	PORTA->PCR[SW_Pos1] &= PORT_PCR_MUX(0);
	PORTA->PCR[SW_Pos2] &= PORT_PCR_MUX(0);
	PORTA->PCR[SW_Pos3] &= PORT_PCR_MUX(0);
	PORTA->PCR[SW_Pos4] &= PORT_PCR_MUX(0);
	PORTA->PCR[SW_Pos5] &= PORT_PCR_MUX(0);
	/* Select GPIO and enable pull-up resistors and interrupts 
		on falling edges for pins connected to switches */
	PORTA->PCR[SW_Pos1] |= PORT_PCR_MUX(1) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_IRQC(0x0a);
	PORTA->PCR[SW_Pos2] |= PORT_PCR_MUX(1) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_IRQC(0x0a);
	PORTA->PCR[SW_Pos3] |= PORT_PCR_MUX(1) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_IRQC(0x0a);
	PORTA->PCR[SW_Pos4] |= PORT_PCR_MUX(1) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_IRQC(0x0a);
	PORTA->PCR[SW_Pos5] |= PORT_PCR_MUX(1) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_IRQC(0x0a);
	
	/* Set port A switch bit to inputs */
	PTA->PDDR &= ~MASK(SW_Pos1);
	PTA->PDDR &= ~MASK(SW_Pos2);
	PTA->PDDR &= ~MASK(SW_Pos3);
	PTA->PDDR &= ~MASK(SW_Pos4);
	PTA->PDDR &= ~MASK(SW_Pos5);
	
	/* Enable Interrupts */
	NVIC_SetPriority(PORTA_IRQn, 64); // 0, 64, 128 or 192
	NVIC_ClearPendingIRQ(PORTA_IRQn); 
	NVIC_EnableIRQ(PORTA_IRQn);
}

void PORTA_IRQHandler(void) {  
	DEBUG_PORT->PSOR = MASK(DBG_ISR_POS);
	// clear pending interrupts
	NVIC_ClearPendingIRQ(PORTA_IRQn);
	if ((PORTA->ISFR & MASK(SW_Pos1))) {
		mode--;
		if(mode==0) {
			mode=7;
		} else if (mode==8){
			mode=1;
		}
	} else if ((PORTA->ISFR & MASK(SW_Pos5))) {
		mode++;
		if(mode==0) {
			mode=7;
		} else if (mode==8){
			mode=1;
		}
	} else if ((PORTA->ISFR & MASK(SW_Pos2))) {
		if ((mode == 2) || (mode == 3) || (mode == 6) || (mode==7)) {
			dacMem++;
				if(dacMem==3) {
					dacMem=1;
			}
		} else if (mode == 4) {
				ADCsampleRate++;
				changeFlag=1;
				if(ADCsampleRate==6) {
					ADCsampleRate=1;
			}
		} else if (mode == 5) {
			dataPoints++;
			if(dataPoints==5) {
				dataPoints=1;
		}
	}	
}		else if ((PORTA->ISFR & MASK(SW_Pos4))) {
	if ((mode == 2) || (mode == 3) || (mode == 6) || (mode==7)) {
			dacMem--;
			if(dacMem==0) {
				dacMem=2;
			}	
		} else if (mode == 4) {
				ADCsampleRate--;
				changeFlag=1;
				if(ADCsampleRate==0) {
					ADCsampleRate=5;
				}
			} else if (mode == 5) {
			dataPoints--;
				if(dataPoints==0) {
					dataPoints=4;
				}
	}
}	else if ((PORTA->ISFR & MASK(SW_Pos3))) {
		if(Save==0) {
			Save=1;
		}
		} 
	// clear status flags 
	PORTA->ISFR = 0xffffffff;
	DEBUG_PORT->PCOR = MASK(DBG_ISR_POS);
}
 
