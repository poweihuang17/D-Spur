
#define BAUD 38400
//#define C 0.000002
// 100 rad/s*s
//#define P 31415.926535 / 16

/*
 * PORTA: 29    28    27    26    25    24    23    22
 *        EN1  STEP1  EN2   STEP2 EN3   STEP3 EN4   STEP4
 *
 * PORTC: 30    31    32    33    34    35    36    37
 *        DIR1   [     MS1    ]    DIR2   [     MS2    ]
 *
 * PORTL: 42    43    44    45    46    47    48    49
 *        DIR3   [     MS3    ]    DIR4   [     MS4    ]
 *
 * TIMER3: 2    3     5
 *
 *
 */
int i;
double C = 0.000002;
double P = 31415.926535 / 16;

byte rcv_count;
byte rcv_buffer[10];

volatile unsigned long timer_count[4] = {0, 0, 0, 0};
volatile unsigned long step_count[4] = {0, 0, 0, 0}; 
int mod = 1;
int wait_us[4] = {2000, 2000, 2000, 2000};
//int wait_us[4] = {1000, 1000, 1000, 1000};
double precise_wait_us[4] = {2000.0, 2000.0, 2000.0, 2000.0};
//int wait_us[4] = {2000, 2000, 2000, 2000};
int target_wait_us[4] = {-1, -1, -1, -1};
//int target_wait_us[4] = {1000, 1000, 1000, 1000};
//int target_wait_us[4] = {2000, 2000, 2000, 2000};
//int target_wait_us[4] = {500, 500, 500, 500};
//bool accelerating[4] = {true, true, true, true};
bool accelerating[4] = {false, false, false, false};

byte dir_upper = B01111111;  // set DIR1=LOW MS1 DIR2=HIGH MS2 
byte dir_lower = B01111111;  // set DIR3=LOW MS3 DIR4=HIGH MS4 
//byte dir_upper = B01011101;  // set DIR1=LOW MS1 DIR2=HIGH MS2 
//byte dir_lower = B01011101;  // set DIR3=LOW MS3 DIR4=HIGH MS4
volatile byte step_byte = B00000000;

union u_tag {
   byte b[2];
   int ival;
} u;

void setup() 
{
  //Serial.begin(9600);
  //Serial3.begin(BT_BAUD);
  unsigned int baud_setting = (F_CPU / 16 / BAUD) - 1;
  // Set the baud rate
  UBRR0H = (unsigned char)(baud_setting >> 8);
  UBRR0L = (unsigned char)baud_setting;
  // Enable receiver and transmitter, no interrupt
  UCSR0B |= _BV(RXEN0) | _BV(TXEN0);
  UCSR0B |= _BV(RXCIE0);
  UCSR0B &= ~_BV(TXCIE0);   
  // Set frame format: No parity check, 8 Data bits, 1 stop bit
  UCSR0C |= _BV(UCSZ01) | _BV(UCSZ00);
  
  // set pin directions to OUTPUT.
  DDRA = B11111111;
  DDRC = B11111111;
  DDRL = B11111111;
  
  // init all ENx,STEPx to LOW.
  PORTA = B00000000;
  
  // set DIR1,DIR2. set MS1.MS2 to (HIGH,HIGH,HIGH)
  PORTC = dir_upper;
  
  // set DIR3,DIR4. set MS3.MS4 to (HIGH,HIGH,HIGH)
  PORTL = dir_lower;
  
  // wheel 1 : Timer1
  // wheel 2 : Timer3
  // wheel 3 : Timer4
  // wheel 4 : Timer5
  
  // set timer interrupt of timer1 (16-bit)
  TCCR1A = 0x00;
  TCCR1B &= ~_BV(CS12);  // no prescaling. timer3 freq = 12 MHz.
  TCCR1B &= ~_BV(CS11);  
  TCCR1B |=  _BV(CS10);  
  TIMSK1 |= _BV(TOIE1);  // enable timer overflow interrupt
  TCNT1 = 65536 - 12 * wait_us[0];
  
  // set timer interrupt of timer3 (16-bit)
  TCCR3A = 0x00;
  TCCR3B &= ~_BV(CS12);  // no prescaling. timer3 freq = 12 MHz.
  TCCR3B &= ~_BV(CS11);  
  TCCR3B |=  _BV(CS10);  
  TIMSK3 |= _BV(TOIE3);  // enable timer overflow interrupt
  TCNT3 = 65536 - 12 * wait_us[1];
  
  // set timer interrupt of timer4 (16-bit)
  TCCR4A = 0x00;
  TCCR4B &= ~_BV(CS12);  // no prescaling. timer3 freq = 12 MHz.
  TCCR4B &= ~_BV(CS11);  
  TCCR4B |=  _BV(CS10);  
  TIMSK4 |= _BV(TOIE4);  // enable timer overflow interrupt
  TCNT4 = 65536 - 12 * wait_us[2];
  
  // set timer interrupt of timer5 (16-bit)
  TCCR5A = 0x00;
  TCCR5B &= ~_BV(CS12);  // no prescaling. timer3 freq = 12 MHz.
  TCCR5B &= ~_BV(CS11);  
  TCCR5B |=  _BV(CS10);  
  TIMSK5 |= _BV(TOIE5);  // enable timer overflow interrupt
  TCNT5 = 65536 - 12 * wait_us[3];
}

void loop() 
{ 
}

// Interrupt Service Routine for motor control.
ISR (TIMER1_OVF_vect) {
  timer_count[0]++;
  if(timer_count[0] % 1 == 0){
    step_count[0]++;
    
    if(target_wait_us[0] != -1) {
      // Not in stand-by mode.
      // set output pin port register for STEPx
      PORTA |= (1 << 6);
      delayMicroseconds(2);  
      PORTA &= B10111111;
      
      // ramp up/down wait_us
      if(wait_us[0] != target_wait_us[0] && (step_count[0] % 1 == 0)){
        if (accelerating[0]){
          // get next wait_us with recurrence relation.
          precise_wait_us[0] = P / (C * precise_wait_us[0] + P / precise_wait_us[0]);
          
          // if go over target, make precise_wait_us = wait_us = target_us.
          wait_us[0] = precise_wait_us[0];
          if (target_wait_us[0] >= wait_us[0])
            precise_wait_us[0] = target_wait_us[0];
        } else { 
          // get next wait_us with recurrence relation.
          precise_wait_us[0] = P / (-C * precise_wait_us[0] + P / precise_wait_us[0]);
          
          // if go over target, make precise_wait_us = wait_us = target_us.
          wait_us[0] = (int) precise_wait_us[0];
          if (wait_us[0] >= target_wait_us[0])
            precise_wait_us[0] = target_wait_us[0];
        }
      }    
    } else {
      // stand-by. set wait_us init val.
      wait_us[0] = 2000;
    }
  }
  
  // reset timer
  TCNT1 = 65536 - 12 * wait_us[0];
}
// Interrupt Service Routine for motor control.
ISR (TIMER3_OVF_vect) {
  timer_count[1]++;
  
  if(timer_count[1] % 1 == 0){
    step_count[1]++;
    
    if(target_wait_us[1] != -1) {
      // Not in stand-by mode.
      
      // set output pin port register for STEPx
      PORTA |= (1 << 4);
      delayMicroseconds(2);  
      PORTA &= B11101111;
      
      // ramp up/down wait_us
      if(wait_us[1] != target_wait_us[1] && (step_count[1] % 1 == 0)){
        if (accelerating[1]){
          // get next wait_us with recurrence relation.
          precise_wait_us[1] = P / (C * precise_wait_us[1] + P / precise_wait_us[1]);
          
          // if go over target, make precise_wait_us = wait_us = target_us.
          wait_us[1] = (int) precise_wait_us[1];
          if (target_wait_us[1] >= wait_us[1])
            precise_wait_us[1] = target_wait_us[1];
        } else { 
          // get next wait_us with recurrence relation.
          precise_wait_us[1] = P / (-C * precise_wait_us[1] + P / precise_wait_us[1]);
          
          // if go over target, make precise_wait_us = wait_us = target_us.
          wait_us[1] = (int) precise_wait_us[1];
          if (wait_us[1] >= target_wait_us[1])
            precise_wait_us[1] = target_wait_us[1];
        }
      }    
    } else {
      // stand-by. set wait_us init val.
      wait_us[1] = 2000;
    }
  }
  
  // reset timer
  TCNT3 = 65536 - 12 * wait_us[1];
}
// Interrupt Service Routine for motor control.
ISR (TIMER4_OVF_vect) {
  timer_count[2]++;
  
  if(timer_count[2] % 1 == 0){
    step_count[2]++;
    
    if(target_wait_us[2] != -1) {
      // Not in stand-by mode.
      
      // set output pin port register for STEPx
      PORTA |= (1 << 2);
      delayMicroseconds(2);  
      PORTA &= B11111011;
      
      // ramp up/down wait_us
      if(wait_us[2] != target_wait_us[2] && (step_count[2] % 1 == 0)){
        if (accelerating[2]){
          // get next wait_us with recurrence relation.
          precise_wait_us[2] = P / (C * precise_wait_us[2] + P / precise_wait_us[2]);
          
          // if go over target, make precise_wait_us = wait_us = target_us.
          wait_us[2] = (int) precise_wait_us[2];
          if (target_wait_us[2] >= wait_us[2])
            precise_wait_us[2] = target_wait_us[2];
        } else { 
          // get next wait_us with recurrence relation.
          precise_wait_us[2] = P / (-C * precise_wait_us[2] + P / precise_wait_us[2]);
          
          // if go over target, make precise_wait_us = wait_us = target_us.
          wait_us[2] = (int) precise_wait_us[2];
          if (wait_us[2] >= target_wait_us[2])
            precise_wait_us[2] = target_wait_us[2];
        }
      }    
    } else {
      // stand-by. set wait_us init val.
      wait_us[2] = 2000;
    }
  }
  
  // reset timer
  TCNT4 = 65536 - 12 * wait_us[2];
}
ISR (TIMER5_OVF_vect) {
  timer_count[3]++;
  
  if(timer_count[3] % 1 == 0){
    step_count[3]++;
    
    if(target_wait_us[3] != -1) {
      // Not in stand-by mode.
      
      // set output pin port register for STEPx
      PORTA |= 1;
      delayMicroseconds(2);  
      PORTA &= B11111110;
      
      // ramp up/down wait_us
      if(wait_us[3] != target_wait_us[3] && (step_count[3] % 1 == 0)){
        if (accelerating[3]){
          // get next wait_us with recurrence relation.
          precise_wait_us[3] = P / (C * precise_wait_us[3] + P / precise_wait_us[3]);
          
          // if go over target, make precise_wait_us = wait_us = target_us.
          wait_us[3] = (int) precise_wait_us[3];
          if (target_wait_us[3] >= wait_us[3])
            precise_wait_us[3] = target_wait_us[3];
        } else { 
          // get next wait_us with recurrence relation.
          precise_wait_us[3] = P / (-C * precise_wait_us[3] + P / precise_wait_us[3]);
          
          // if go over target, make precise_wait_us = wait_us = target_us.
          wait_us[3] = (int) precise_wait_us[3];
          if (wait_us[3] >= target_wait_us[3])
            precise_wait_us[3] = target_wait_us[3];
        }
      }    
    } else {
      // stand-by. set wait_us init val.
      wait_us[3] = 2000;
    }
  }
  // reset timer
  TCNT5 = 65536 - 12 * wait_us[3];
}

// Interrrupt Service Routine for receiving data from Serial
/*
 *  protocol : 18 bytes
 *    (int)  target_wait_us[0] : 4 bytes
 *    (int)  target_wait_us[1] : 4 bytes
 *    (int)  target_wait_us[2] : 4 bytes
 *    (int)  target_wait_us[3] : 4 bytes
 *    (byte) dir_upper      : 1 byte
 *    (byte) dir_lower      : 1 byte
 */
ISR (USART0_RX_vect){
  rcv_buffer[rcv_count] = UDR0;
  rcv_count++;
  
  // transmission complete
  if(rcv_count == 10){
    for (i = 0; i< 4; i++){
      //u.b[3] = rcv_buffer[0+4*i];
      //u.b[2] = rcv_buffer[1+4*i];
      //u.b[1] = rcv_buffer[2+4*i];
      //u.b[0] = rcv_buffer[3+4*i];
      u.b[1] = rcv_buffer[0+2*i];
      u.b[0] = rcv_buffer[1+2*i];
      target_wait_us[i] = u.ival;
      if(target_wait_us[i] != -1){
        accelerating[i] = (target_wait_us[i] < wait_us[i]);
      }
    }
    // check gear shifting
    if (target_wait_us[0] != -1 &&
        (dir_upper & 0x70) != (rcv_buffer[8] & 0x70))
      wait_us[0] = target_wait_us[0];
    if (target_wait_us[1] != -1 &&
        (dir_upper & 0x07) != (rcv_buffer[8] & 0x07))
      wait_us[1] = target_wait_us[1];
    if (target_wait_us[2] != -1 &&
        (dir_lower & 0x70) != (rcv_buffer[9] & 0x70))
      wait_us[2] = target_wait_us[2];
    if (target_wait_us[3] != -1 &&
        (dir_lower & 0x07) != (rcv_buffer[9] & 0x07))
      wait_us[3] = target_wait_us[3];
    
    dir_upper = rcv_buffer[8];
    dir_lower = rcv_buffer[9];
    
    // set output pin port register for DIRx & MSx
    PORTC = dir_upper;
    PORTL = dir_lower;
    
    rcv_count = 0;
  }
  
}

