# include <MKL25Z4.h>
// *************************************************************************//
// Filename: Main Project File.c
// Author: Cadyn Maddocks //
// Date: 03/15/22 //
// Processor: NXP MKL25Z4 //
// Compiler: Keil uVision5 //
// Library: CMSIS core and device startup //
// also needs lcd_lib_4bit.c // debug_signals.c // switches.c //
// Hardware: NXP Freedom board connected to a 16x2 LCD display //
// Software note: This program is a "bare metal" application since it //
// doesn't use an operating system. //
// Operation:	Cycles through modes left and right, cycles through setting up and down middle for save	//
// Also contains the BPM averaging function and the Save preceeding function
// RESULTS: Menu works perfectly, save preceeding is still non-functional  //
// *************************************************************************//

// Main Functions
void display_string(char string[]);
void display_decimal(uint32_t value);
uint32_t Avg_BPM(void);
void Save_Prec_DAC_Memory(void);
// Outside Functions
void Init_ADC(void);
void Init_ADC_Interrupt(void);
void ADC0_IRQHandler(void);
void init_debug_signals(void);
void Init_PIT(uint32_t period);
void Start_PIT(void);
void Stop_PIT(void);
void PIT_IRQHandler(void);
void Init_DAC(void);
void LCD_send_data(uint32_t data);
void LCD_init(void);
void delayMs(uint32_t n);
void LCD_command(uint32_t command);  
void init_switch(void);
void PORTA_IRQHandler(void);
// Globals
volatile uint32_t changeFlag = 0;
volatile uint32_t count = 0;
volatile uint32_t mode = 1;
volatile uint32_t dacMem = 1;
volatile uint32_t ADCsampleRate = 5;
volatile uint32_t dataPoints = 2;
volatile uint32_t Save = 0;
uint32_t firstPoint = 0;
uint32_t lastPoint;
uint32_t savedDP1;
uint32_t savedDP2;
uint32_t saveBPM[5] = {0,0,0,0,0};
//extra values for rap-around
uint32_t data_Points[6] = {128,128,256,512,1024,1024};
uint32_t sample_Rate[7] = {50,50,100,200,500,1000,1000};
//Arrays for DAC Memory
uint32_t temp_dac_memory[1024];
uint32_t dac_memory1[1024];
uint32_t dac_memory2[1024];

// *************************************************************************//
//	Main																								Cadyn Maddocks			//
// *************************************************************************//
int main() {
	//start clocks and initialize
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
	SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
	SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;

// Initialize everything
	Init_PIT(24000);
	Start_PIT();
	Init_DAC();
	Init_ADC();
	Init_ADC_Interrupt();
	__enable_irq();
	init_debug_signals();
	LCD_init();
	init_switch();
	LCD_command(0x01);
	
//Display the Period to LCD as a decimal
while(1){
	delayMs(50);
switch(mode) {
	case 1:
		//EKG Mode
		LCD_command(0x80);
		display_string("EKG Mode");
		LCD_command(0xC0);
		display_string("BPM:");
		display_decimal((sample_Rate[ADCsampleRate]*Avg_BPM())/1000);
	break;
	case 2:
		//Sample and store preceding
		LCD_command(0x80);
		display_string("Smpl Pre");
		LCD_command(0xC0);
		if(Save==0) {
		display_string("Saved");
			if(dacMem==1) {
			display_string("DM1");
			}
			if(dacMem==2) {
			display_string("DM2");
		}
	} else {
		display_string(" Saving ");
		Save_Prec_DAC_Memory();
	}
	break;
	case 3:
		//Sample and store new
		LCD_command(0x80);
		display_string("Smpl New");
		LCD_command(0xC0);
	if(Save==0) {
		display_string("Saved");
		if(dacMem==1) {
			display_string("DM1");
		}
		if(dacMem==2) {
			display_string("DM2");
		}
	} else {
		display_string(" Saving ");
	}
		
	break;
	case 4:
		//Set ADC sample rate
		LCD_command(0x80);
		display_string("SmplRate");
		LCD_command(0xC0);
		display_string("SR: ");
		display_decimal(sample_Rate[ADCsampleRate]);
		if(changeFlag==1) {
		Stop_PIT();
		Init_PIT((1000*24000)/sample_Rate[ADCsampleRate]);
		Start_PIT();
			changeFlag=0;
		}
	break;
	case 5:
		//Set Number of Data Points
		LCD_command(0x80);
		display_string("DataPnts");
		LCD_command(0xC0);
		display_string("DP: ");
		display_decimal(data_Points[dataPoints]);
	break;
	case 6:
		// Create output waveform using data stored in DAC Memory
		LCD_command(0x80);
		display_string("FromMem ");
		LCD_command(0xC0);
		display_string("        ");
	break;
	case 7:
		// Load a waveform into DAC Memory using serial port
		LCD_command(0x80);
		display_string("LDtoMem ");
		LCD_command(0xC0);
		display_string("        ");
	break;
}
}
}

// *************************************************************************//
//	Display String																			Cadyn Maddocks			//
// *************************************************************************//
void display_string(char string[]) {
	int i =0;
	while (string[i] != 0){
		LCD_send_data(string[i]);
		i++;
	}
}
// *************************************************************************//
//	Display Decimal																			Cadyn Maddocks			//
// *************************************************************************//
void display_decimal(uint32_t value) {
	if(value>=1000) {
		LCD_send_data((value/1000)%10+48);
		LCD_send_data((value/100)%10+48);
		LCD_send_data((value/10)%10+48);
		LCD_send_data((value/1)%10+48);
	} else if(value>=100) {
		display_string(" ");
		LCD_send_data((value/100)%10+48);
		LCD_send_data((value/10)%10+48);
		LCD_send_data((value/1)%10+48);
	} else if(value>=10) {
		display_string("  ");
		LCD_send_data((value/10)%10+48);
		LCD_send_data((value/1)%10+48);
	} else	{
		display_string("   ");
		LCD_send_data((value/1)%10+48);
	}
}

// *************************************************************************//
//	Average the BPM Array																Cadyn Maddocks			//
// *************************************************************************//
uint32_t Avg_BPM(void) {
	uint32_t avgBPM;
	avgBPM=(saveBPM[0]+saveBPM[1]+saveBPM[2]+saveBPM[3]+saveBPM[4])/5;
	return avgBPM;
}

// *************************************************************************//
//	Saves Temp DAC Memory																Cadyn Maddocks			//
// *************************************************************************//
void Save_Prec_DAC_Memory(void) {
	lastPoint = firstPoint + 1024 - data_Points[dataPoints] + 1;
	uint32_t counter = 0;
	while(counter<data_Points[dataPoints]) {
		if(dacMem==1) {
			dac_memory1[counter]= temp_dac_memory[lastPoint%1024];
		}
		if(dacMem==2) {
			dac_memory2[counter]= temp_dac_memory[lastPoint%1024];
	}
		counter++;
		lastPoint++;
	} 
	Save=0;
}
