# include <MKL25Z4.h>
// *************************************************************************//
// Filename: Init_ADC.c
// Author: Cadyn Maddocks //
// Version: 03/16/22 //
// Processor: NXP MKL25Z4 //
// Compiler: Keil uVision5 //
// Library: CMSIS core and device startup //
// Hardware: NXP Freedom board connected to a 16x2 LCD display //
// Software note: This program is a "bare metal" application since it //
// doesn't use an operating system. //
// Operation: Initializes the Analog to Digital Converter //
// contains the ADC interrupt function, which takes the ADC input, outputs //
// it using the DAC, and save the BPM for averaging and display. //
// It also saves pre and following ADCinputs and displays to the DAC from Memory//
// *************************************************************************//

#define ADC_POS (20)
uint32_t Flag = 0;
uint32_t countTime =0;
uint32_t numBeats=0;
extern uint32_t savedDP1;
extern uint32_t savedDP2;
extern volatile uint32_t dacMem;
extern volatile uint32_t Save;
extern uint32_t saveBPM[5];
extern uint32_t data_Points[6];
extern uint32_t temp_dac_memory[1024];
extern uint32_t dac_memory1[1024];
extern uint32_t dac_memory2[1024];
extern volatile uint32_t dataPoints;
extern volatile uint32_t mode;
extern uint32_t ADCsampleRate;
uint32_t first_sample = 0;
uint32_t max;
extern volatile uint32_t firstPoint;
uint32_t counter = 0;
uint32_t counterOut = 0;
// *************************************************************************//
//	ADC Initialization Function													Cadyn Maddocks			//
// *************************************************************************//
void Init_ADC(void) {
	SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
	SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK; // incase it isn't already on
	
	// Set analog pins
	PORTE->PCR[ADC_POS] &= ~PORT_PCR_MUX_MASK;
	PORTE->PCR[ADC_POS] |= ~PORT_PCR_MUX(0);
	
	// ADC Settings
	SIM_SOPT7 |= SIM_SOPT7_ADC0ALTTRGEN(1);
	SIM_SOPT7 |= SIM_SOPT7_ADC0PRETRGSEL(0);
	SIM_SOPT7 |= SIM_SOPT7_ADC0TRGSEL(4);
	
	ADC0_CFG1 |= ADC_CFG1_ADLPC(0); 
	ADC0_CFG1	|= ADC_CFG1_ADLSMP(0);
	ADC0_CFG1	|= ADC_CFG1_ADIV(0);
	ADC0_CFG1	|= ADC_CFG1_MODE(1);
	ADC0_CFG1	|= ADC_CFG1_ADICLK(1);
	ADC0_SC2  |= ADC_SC2_ADTRG(1);
	ADC0_SC2	|= ADC_SC2_ACFE(0);
	ADC0_SC2	|= ADC_SC2_DMAEN(0);
	ADC0_SC2	|= ADC_SC2_REFSEL(0);
	ADC0_SC3  |= ADC_SC3_ADCO(0); 
	ADC0_SC3	|= ADC_SC3_AVGE(1);
	ADC0_SC3	|= ADC_SC3_AVGS(0);
	ADC0_SC1A |= ADC_SC1_DIFF(0);
	ADC0_SC1A &= ~ADC_SC1_ADCH_MASK;
	


}
// *************************************************************************//
//	ADC Interrupt Initialization Function								Cadyn Maddocks			//
// *************************************************************************//
void Init_ADC_Interrupt(void) {
	ADC0_SC1A |= ADC_SC1_AIEN(1);
		
	NVIC_SetPriority(ADC0_IRQn, 128);
	NVIC_ClearPendingIRQ(ADC0_IRQn);
	NVIC_EnableIRQ(ADC0_IRQn);
}

// *************************************************************************//
//	ADC_IRQHandler																			Cadyn Maddocks			//
// *************************************************************************//
void ADC0_IRQHandler(void) {
	NVIC_ClearPendingIRQ(ADC0_IRQn);
	
	uint32_t ADC_in;
	//Get the ADC input
	ADC_in = ADC0_RA;
	
		// Reset the ADC interrupt
	ADC0_SC1A |= ADC_SC1_AIEN(0);
	ADC0_SC1A |= ADC_SC1_AIEN(1);
	
	// Output with DAC
	if(mode != 6) {
	DAC0->DAT[0].DATL = DAC_DATL_DATA0(ADC_in);
	DAC0->DAT[0].DATH = DAC_DATH_DATA1(ADC_in>>8);
	}
	if(mode==6) {
		if(dacMem==1) {
			DAC0->DAT[0].DATL = DAC_DATL_DATA0(dac_memory1[counterOut]);
			DAC0->DAT[0].DATH = DAC_DATH_DATA1(dac_memory1[counterOut]>>8);
			counterOut++;
			if (counterOut > savedDP1) {
				counterOut = 0;
			}
		}
		if(dacMem==2) {
			DAC0->DAT[0].DATL = DAC_DATL_DATA0(dac_memory2[counterOut]);
			DAC0->DAT[0].DATH = DAC_DATH_DATA1(dac_memory2[counterOut]>>8);
			counterOut++;
			if (counterOut > savedDP2) {
				counterOut = 0;
			}
		}
	}
	
	//Save Prec
	if(Save==0) {
		temp_dac_memory[firstPoint] = ADC_in;
		firstPoint++;
	if(firstPoint>1023) {
		firstPoint = 0;
	}
}
	// Save New
	if((mode==3)&&(Save==1)) {
		if(dacMem==1) {
			savedDP1=data_Points[dataPoints];
			dac_memory1[counter] = ADC_in;
		} else if (dacMem==2) {
			savedDP2=data_Points[dataPoints];
			dac_memory2[counter] = ADC_in;
		}
		counter++;
		if(counter>data_Points[dataPoints]) {
			counter = 0;
			Save = 0;
		}
	}
	// first sample idea from Taylor Smith
	if(first_sample==0) {
		max=ADC_in;
		first_sample++;
	} else {
		if(ADC_in>max) {
		max=ADC_in;
		}
	}

	uint32_t high_threshold;
	uint32_t low_threshold;
	
		
	high_threshold = 0.8*(max);
	low_threshold = 0.7*(max);
	
	
	
	// Determine the period
	countTime++;
	if((ADC_in>high_threshold)&&(Flag==0)) {
		saveBPM[numBeats]=60000/countTime;
		countTime=0;
		numBeats++;
		if (numBeats==5) {
			numBeats=0;
		}
		Flag=1;
	}
	if ((ADC_in<low_threshold)&(Flag==1)) {
		Flag=0;
	}
}
