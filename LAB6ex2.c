#include <avr/io.h>
#include <avr/interrupt.h>

volatile unsigned char TimerFlag = 0; //TimerISR() sets this to a 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; //Start count from here, down to 0. Default to 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1 ms ticks

void TimerOn()
{
	//AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B; //bit3 = 0: CTC mode (clear timer on compare)
	//bit2bit1bit0 = 011: pre-scaler /64
	// 00001011: 0x0B
	// so, 8MHz clock or 8,000,000 /64 =125,000 ticks/s
	// Thus, TCNT1 register will count as 125,000 ticks/s
	//AVR output compare register OCR1A.
	OCR1A = 125;   // Timer interrupt will be generated when TCNT1 == OCR1A
	// We want a 1 ms tick. 0.001s *125,000 ticks/s = 125
	// so when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	//AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	// Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |=0x80; // 0x80: 1000000

}

void TimerOff()
{
	TCCR1B = 0x00; // bit3bitbit0 -000: Timer off
}

void TimerISR()
{
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect)
{
	//CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) //results in a more efficient compare
	{
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}
//Set TimerISR() to tick every M ms
void TimerSet(unsigned long M)
{
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

enum States {Init, Shift, Stop} state;
//unsigned char tempB = 0x00;
	
int main()
{
	DDRB = 0xFF; PORTB =0x01;//Set PORTB to output PORTB =0x00; // Init PORTB to 0s
	DDRA = 0x00; PORTA = 0xFF;//SET PORTA to input button pull up mode 
	TimerSet(200);
	TimerOn();
	state = Init;
	while (1)
	{
		Tick();
		while(!TimerFlag);
		TimerFlag = 0;
	}
	
	return 0;
		
}

 void Tick() 
{
	unsigned char A0 = ~PINA & 0x01;
	switch(state) // transitions
	{
		case Init: 
			state = Shift;
			break;
		case Shift: 
			if(!A0) state = Shift;
			else state = Stop;
			break;
		case Stop: 
			if (!A0) state = Stop;
			else state = Init;
			break;
	}
	
	switch(state) //actions
	{
		case Init:
			PORTB = 0x01;
			break;
		case Shift:
			if(PORTB < 0x04)
			PORTB = PORTB << 1;
			else
			PORTB = 0x01;
		case Stop:
			break;
	}
		
}
