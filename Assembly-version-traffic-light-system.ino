/*
 * The following is an interrupt-driven program that simulates the traffic light using 2 iternal interrupts instead of a delay function, 
 * and 2 external interrupts to simulate the pedestrian crosses using inline assembly
 
  * red1 is the red LED of traffic1 is attached to pin13 (i.e., bit7 in DRB)
  * amber1 is the amber LED of traffic1 is attached to pin12 (i.e., bit6 in DRB)
  * green1 is the green LED of traffic1 is attached to pin11 (i.e., bit5 in DRB)
  * red2 is the red LED of traffic2 is attached to pin10 (i.e., bit4 in DRB)
  * amber2 is the amber LED of traffic2 is attached to pin50 (i.e., bit3 in DRB)
  * green2 is the green LED of traffic2 is attached to pin51 (i.e., bit2 in DRB)
  * 
  * p1Green is the green pedestrian light of traffic1 is attached to pin37 (i.e., bit0 in DRC)
  * p1Red is the red pedestrian light of traffic1 is attached to pin36 (i.e., bit1 in DRC)
  * p2Green is the green pedestrian light of traffic2 is attached to pin35 (i.e., bit2 in DRC)
  * p2Red is the red pedestrian light of traffic2 is attached to pin34 (i.e., bit3 in DRC)
*/


const uint16_t t_load = 0;  //the variable that will be used to reload the timers
const uint16_t t1_comp = 62500; //this is to have a delay of 4 seconds, because (16000000*2)/1024 = 62500
const uint16_t t3_comp = 46875; //this is to have a delay of 3 seconds, because (16000000*3)/1024 = 46875

/*the 4 variables below represent the states of the traffic lights using the values that should we written to PortB to move to a specific state */
volatile const byte red1Green2 = 0x84;  //the value that should be written to portB to switch the red light of traffic1 and the green light of traffic2 on 
volatile const byte red2Green1 = 0x30;  //the value that should be written to portB to switch the red light of traffic2 and the green light of traffic1 on 
volatile const byte red1Amber1Amber2 = 0xC8;  //the value that should be written to portB to switch red1, amber1 and amber2 on 
volatile const byte red2Amber2Amber1 = 0x58;  //the value that should be written to portB to switch red2, amber2 and amber1 on 

/*the 3 variables below represent the states of the pedestrian lights using the values that should we written to PortC to move to a specific state */
volatile const byte pRed1pRed2 = 0x0A;  //the value that should be written to portC to switch all pedestrians red lights on
volatile const byte pGreen1 = 0x09;  //the value that should be written to portC to switch the green light of traffic1 pedestrians on and the red light of traffic2 pedestrians on
volatile const byte pGreen2 = 0x06;  //the value that should be written to portC to switch the green light of traffic2 pedestrians on and the red light of traffic1 pedestrians on

/*the two variables below can either be 0 or 1, and they are used to check if pedestrians wanted to cross a traffic but could not be allowed */
volatile byte p1RequiresCross = 0x00; //checks if pedestrians wanted to cross one of the tarffics but could not be allowed
volatile byte p2RequiresCross = 0x00; //checks if pedestrians wanted to cross one of the tarffics but could not be allowed


volatile byte first_step = 0x01; //this variable is used to initialise the traffic lights immidiatley when the program is run, because otherwise the traffic light simulation will only start when an interrupt occurs


/* the variable below represents the next phase of the traffic light to move to, 1 means that the next phase is "re1 and green2 on", 
 *  and 2 means that the next phase is "red2 and green1 on"
 */
volatile byte step_number = 0x01;  


/*the variable below is used to check if timer1 counter value is less than 31250, is yes it is set to 1, otherwise it is set to 0,  
 * and this allows knowing if a red light has been on for less than 2 seconds before allowing the pedestrians to cross
 */
volatile byte timer1_lt_2s = 0x00; 


#define traffic1Button 2  //the first pedestrian button is connected to digital pin 2 
#define traffic2Button 3  //the second pedestrian button is connected to digital pin 3


volatile byte traffic_lights_state; //this variable will be used to read the bit values of DRB to know the state of the traffic lights
volatile byte ped_lights_state;    //this variable will be used to read the bit values of DRC to know the state of the pedestrian lights


void setup() {

  //setting the pins connected to the LEDs as output
  asm volatile (  
  "ldi r18, 0xFF \n"  
  "out 4, r18      ;setting the pins associated with all bits of DDRB as output \n" 

  "ldi r18, 0x0F \n"  
  "out 7, r18      ;setting the pins associated with bits 0, 1, 2, and 3 of DDRC as output \n" 

  ::: "r18");  


  /*-------------------- Setting the external interrupt -----------------------*/
  
  pinMode(traffic1Button, INPUT_PULLUP);

  pinMode(traffic2Button, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(traffic1Button), allowPed1ToCross, LOW);
  
  attachInterrupt(digitalPinToInterrupt(traffic2Button), allowPed2ToCross, LOW);

  noInterrupts(); //disable all the interrupts

  

  /*-------------------- Setting the timer 1 interrupt -----------------------*/
  
  TCCR1A = 0 ; //reset the timer counter of register A to 0

  //using the prescaler as 1024, i.e., writing 101 to CS12, CS11, and CS10 bits or TCCR1B
  TCCR1B |= (1 << CS12); 
  TCCR1B &= ~(1 << CS11);
  TCCR1B |= (1 << CS10);

  
  TCNT1 = t_load; //load the timer with 0

  OCR1A = t1_comp; //store the match value into the output compare register

  TIMSK1 = (1 << OCIE1A); //enable the compare match interrupt by setting the OCIE1A bit to 1



  /*-------------------- Setting the timer 3 -----------------------*/
  
  TCCR3A = 0 ; //reset the timer counter of register A to 0

  //using the prescaler as 256, i.e., writing 100 to CS12, CS11, and CS10 bits or TCCR3B
  TCCR3B |= (1 << CS12); 
  TCCR3B &= ~(1 << CS11);
  TCCR3B |= (1 << CS10);

  
  TCNT3 = t_load; //load the timer with 0

  OCR3A = t_load; //store 0 into the output compare register

  TIMSK3 &= ~(1 << OCIE3A); //disable the compare match interrupt by setting the OCIE3A bit to 0

  interrupts(); //enable global interrupts

  Serial.begin(9600);
  
}


void loop() {

  
  //the loop function only has an effect when the program has just started, in that case it moves to the initial state of the traffic light 
  
  asm volatile(

    "jmp initialise%=       \n"
    
    "pRedOn%=: \n"
      "lds r18, pRed1pRed2 \n"
      "out 8, r18 \n"
      "ret \n"

    "initialise%=: \n"
      "lds r16, first_step \n"
      "ldi r17, 0x01 \n"
      "cpse r16, r17             ;if first_step is not equal to 1, exit the funtion \n"
      "jmp end%= \n"
      "lds r18, red1Green2 \n"
      "out 5, r18                ;if first_step is equal to 1, switch red1 and green2 on \n"
      "lds r17, step_number \n"
      "inc r17                   ;increment step_number to move to the next phases in the traffic light system \n"
      "sts step_number, r17  \n"
      "rcall pRedOn%=            ;initially, the pedestrians red lights are switched on \n" 
      
      "end%=: \n"

      ::: "r16", "r17", "r18");


      if(first_step == 1)
    {
      printState();
      first_step --; //set first_step to 0 so that this initialisation is done only once
    }

}



/******************************* interrupt service routine which will run when timer1 reaches 4 seconds **************************************************/

ISR(TIMER1_COMPA_vect) {

  asm volatile(
    ";switch the amber lights on, but leave the red lights on if they were on \n"
    "sbic 0x05, 7                    ;if bit 7 of DRB is set (i.e., if red1 is on, switch red1 amber1 and amber2 on \n"
    "lds r18, red1Amber1Amber2 \n"
    "sbic 0x05, 4 \n"
    "lds r18, red2Amber2Amber1       ;if bit 4 of DRB is set (i.e., if red2 is on, switch red2 amber2 and amber1 on\n"
    "out 5, r18 \n"
    :::"r18");

  //enabling timer3
  TCNT3 = t_load;  //reload timer3 with 0
  OCR3A= t3_comp;  //set the value of the output compare register, which will make a timer3 interrupt occur after 3 seconds
  TIMSK3 |= (1 << OCIE3A); //enable the compare match interrupt by setting the OCIE3A bit to 1

  printState();
  
}



/******************************* interrupt service routine which will run when timer3 reaches 3 seconds **************************************************/

ISR(TIMER3_COMPA_vect) {

  asm volatile(
    
    "rjmp nextPhase%=  ;move to the next phase of traffic light (i.e., either switch red1 and green2 on OR red2 and green1 on) depending on the value of step_number \n"

    "turnRed1Green2On%=:  \n"
      "lds r18, red1Green2   \n"
      "out 5, r18               ;switch the red light of traffic1 and the green light of traffic2 on \n"
      "lds r17, step_number \n"
      "inc r17 \n"
      "sts (step_number), r17   ;increment step_number to move to a new phase the next time a timer3 interrupt occurs\n"
      "ret \n"
                             

    "turnRed2Green1On%=:  \n"
      "lds r18, red2Green1 \n"
      "out 5, r18               ;switch the red light of traffic2 and the green light of traffic1 on\n"
      "lds r17, step_number \n"
      "dec r17 \n"
      "sts (step_number), r17   ;decrement step_number to move to a new phase the next time a timer3 interrupt occurs \n"
      "ret \n"
      

    "pRedOn%=: \n"

      "lds r16, pRed1pRed2 \n"
      "jmp flashP1Green%= \n"

      "delay%=: \n"  
        "ldi r22, 70 \n"      
        "loop2%=: \n"     
        "ldi r20, 60 \n"         
        "loop%=:"          
        "ldi r19, 60 \n"            
        "delay_1us%=: \n"         
        "dec r19 \n"
        "brne delay_1us%= \n"        
        "dec r20 \n"     
        "brne loop%= \n"       
        "dec r22 \n"  
        "brne loop2%= \n" 
        "ret \n"  

      "flash%=: \n"
        "out 8, r21               ;switch p2Green off \n"
        "rcall delay%=            ;a short delay to make the flashing appear \n"  
        "out 8, r18               ;switch p2Green on\n"
        "rcall delay%=            ;a short delay to make the flashing appear \n"  
        "out 8, r21               ;switch p2Green off \n"
        "rcall delay%=            ;a short delay to make the flashing appear \n"  
        "out 8, r18               ;switch p2Green on\n"
        "rcall delay%=            ;a short delay to make the flashing appear \n"
        "ret \n"
        
      "flashP1Green%=: \n"
        "ldi r21, 0x08 \n"
        "in r17, 8 \n"
        "lds r18, pGreen1 \n"
        "cpse r17, r18 \n"
        "jmp flashP2Green%= \n"
        "rcall flash%= \n"  
        "jmp exit%= \n"

      "flashP2Green%=: \n"
        "ldi r21, 0x02 \n"
        "lds r18, pGreen2 \n"
        "cpse r17, r18 \n"
        "jmp exit%= \n"
        "rcall flash%= \n"  
        
      "exit%=: \n"
        "out 8, r16               ;switch the pedestrians red lights on \n"
        "ret \n"
              

    /*after moving to a new phase of the traffic light, we check if the pedestrians wanted to cross a traffic but they were not allowed to do that, and now they can*/
    "checkPedestrians1%=: \n"
      "ldi r18, 0x01                   ;storing the value 1 in r18 to be able to check if the value of p1RequiresCross is the same as this value \n"
      "ldi r19, 0x00                   ;storing the value 0 in r19 to be able to reset the value of p1RequiresCross to this value if it was 1 \n"
      "sbis 0x05, 7                    ;if bit 7 of DRB is set (i.e., if red1 is on), check is p1Requirescross is 1, if yes, switch p1Green on and reset p1Requirescross to 0 \n"
      "jmp checkPedestrians2%=         ;if bit 7 of DRB is not set, check is the pedestrians wanted to cross traffic2 \n"
      "lds r16, p1RequiresCross \n"
      "cpse r16, r18 \n"
      "jmp checkPedestrians2%=         ;if bit 7 of DRB is set but p1RequiresCross is not equal to 1, check is the pedestrians wanted to cross traffic2 \n" 
      "lds r17, pGreen1 \n"
      "out 8, r17 \n"
      "sts p1RequiresCross, r19 \n"
      
      "ldi r18, 0x01 \n"
      "ldi r19, 0x00 \n"
      
    "checkPedestrians2%=: \n"
      "sbis 0x05, 4                    ;if bit 4 of DRB is set (i.e., if red2 is on), check is p2Requirescross is 1, if yes, switch p2Green on and reset p2Requirescross to 0 \n"
      "jmp end%=                       ;if bit 4 of DRB is not set, exit \n"
      "lds r16, p2RequiresCross \n"
      "cpse r16, r18 \n"
      "jmp end%=                       ;if bit 4 of DRB is set but p2RequiresCross is not equal to 1, exit \n"
      "lds r17, pGreen2 \n"
      "out 8, r17 \n"
      "sts p2RequiresCross, r19 \n"
      
      "end%=: \n"
      "ret \n"

    
    "nextPhase%=:  \n"

      "rcall pRedOn%=               ;every time when switching to a new phase of the traffic light, the pedestrians red lights must be switched on\n" 

      "lds r16, step_number \n"
      "ldi r20, 0x01 \n"
      "cpse r16, r20                ;if step_number is not equal to 1 (i.e., step_number = 2), the next phase must be red2 and green1 on \n"
      "rcall turnRed2Green1On%= \n"

      "ldi r20, 0x02 \n"
      "cpse r16, r20                ;if step_number is not equal to 2 (i.e., step_number = 1), the next phase must be red1 and green2 on \n"
      "rcall turnRed1Green2On%= \n"

      "rcall checkPedestrians1%=    ;check if pedestrians wanted to cross traffic1. And from checkPedestrians1 we also check if pedestrians wanted to cross traffic2\n"

      ::: "r16", "r17", "r18", "r19", "r20", "r21", "r22");

   
  //mask timer3 to prevent its interrupt from being serviced before timer1 interrupt occurs
  TIMSK3 &= ~(1 << OCIE3A); //disable the compare match interrupt by setting the OCIE3A bit to 0

  //reload timer1 with 0 which will cause a timer1 interrupt to occur after 4 seconds
  TCNT1 = t_load;  

  printState();
  
}



/******************************* interrupt service routine which will run when traffic1 button is pressed **************************************************/

void allowPed1ToCross(){
  /* Two conditions are required to let the pedestrians cross traffic1:
  1) red1 must be switched on (i.e., red1On = true)
  2) red1 must have been on for 2 seconds or less (i.e., TCNT1 has counted to a number less than or equal to 31250)
  */ 
  if(TCNT1 <= 31250)
  {
    asm volatile(
      "ldi r16, 0x01 \n"
      "sts timer1_lt_2s, r16 \n"
      :::"r16");
  }
    asm volatile (
    "lds r16, pGreen1 \n"
    "sbis 0x05, 7                  ;if bit7 of DRB is not set (i.e., if red1 is off), set p1RequiresCross to 1 and exit \n"
    "jmp setP1RequiresCross%= \n"
    "sbic 0x05, 6                  ;if bit7 and bit6 of DRB are set (i.e., if red1 and amber1 are on), set p1RequiresCross to 1 and exit \n"
    "jmp setP1RequiresCross%= \n"
    "lds r18, timer1_lt_2s \n"
    "ldi r19, 0x01 \n"
    "cpse r18, r19                 ;if timer1 counter value is greater than 2 seconds, set p1RequiresCross to 1 and exit \n"
    "jmp setP1RequiresCross%= \n"
    "out 8, r16                    ;if bit7 of DRB is set and bit6 of DRB is not set (red1 is on and amber1 is off) and timer1 counter value is less than 2 seconds, switch p1Green on\n"
    "ldi r17, 0x00 \n"
    "sts p1RequiresCross, r17      ;if p1Green was switched on, reset p1RequiresCross to 0 \n"
    "dec r18 \n"
    "sts timer1_lt_2s, r18         ;if timer1_lt_2s was 1 (i.e., timer1 counter value was less than 2 seconds) and p1Green was switched on, reset timer1_lt_2s to 0 \n"
    
    "jmp end%= \n"
    
    "setP1RequiresCross%=: \n"
      "ldi r17, 0x01 \n"
      "sts p1RequiresCross, r17      ;set p1RequiresCross to 1 because p1Green could not be switched on\n"

    "end%=: \n"
    
    ::: "r16", "r17", "r18", "r19");

    printState();
}


/******************************* interrupt service routine which will run when traffic2 button is pressed **************************************************/

void allowPed2ToCross(){
  /* Two conditions are required to let the pedestrians cross traffic2:
  1) red2 must be switched on (i.e., red2On = true)
  2) red2 must have been on for 2 seconds or less (i.e., TCNT1 has counted to a number less than or equal to 31250)
  */

  if(TCNT1 <= 31250)
  {
    asm volatile(
      "ldi r16, 0x01 \n"
      "sts timer1_lt_2s, r16 \n"
      :::"r16");
  }
  
  asm volatile (
    "lds r16, pGreen2 \n"
    "sbis 0x05, 4                    ;if bit4 of DRB is not set (i.e., if red2 is off), set p2RequiresCross to 1 and exit \n"
    "jmp setP2RequiresCross%= \n"
    "sbic 0x05, 3                    ;if bit4 and bit3 of DRB are set (i.e., if red2 and amber2 are on), set p2RequiresCross to 1 and exit \n"
    "jmp setP2RequiresCross%= \n"
    "lds r18, timer1_lt_2s \n"
    "ldi r19, 0x01 \n"
    "cpse r18, r19                 ;if timer1 counter value is greater than 2 seconds, set p2RequiresCross to 1 and exit \n"
    "jmp setP2RequiresCross%= \n"
    "out 8, r16                      ;if bit4 of DRB is set and bit3 of DRB is not set (red2 is on and amber2 is off) and timer1 counter value is less than 2 seconds, switch p2Green on \n"
    "ldi r17, 0x00 \n"
    "sts p2RequiresCross, r17        ;if p2Green was switched on, reset p2RequiresCross to 0 \n"
    "dec r18 \n"
    "sts timer1_lt_2s, r18         ;if timer1_lt_2s was 1 (i.e., timer1 counter value was less than 2 seconds) and p2Green was switched on, reset timer1_lt_2s to 0 \n"
    "jmp end%= \n"
    
    "setP2RequiresCross%=: \n"
      "ldi r17, 0x01 \n"
      "sts p2RequiresCross, r17      ;set p2RequiresCross to 1 because p2Green could not be switched on \n"

    "end%=: \n"
    ::: "r16", "r17", "r18", "r19");

    printState();
}


// the following function prints the state of the lights to the serial monitor console
void printState(){
  
   asm volatile(
    "in r16, 5                       ;read the value in DRB into register 16 \n"
    "sts traffic_lights_state, r16   ;store the value in register 16 (i.e., the state of the traffic lights) into traffic_lights_state \n"
    :::"r16");

    // traffic_lights_state = red1Green2 means that traffic1 is in the RED phase and traffic2 is in the GREEN phase 
    if(traffic_lights_state == red1Green2){
      Serial.println("Traffic1: ON OFF OFF. Traffic2: OFF OFF ON");
      printPedState();
      return;
    }

    // traffic_lights_state = red1Amber1Amber2 means that traffic1 is in the RED AMBER phase and traffic2 is in the AMBER phase 
    if(traffic_lights_state == red1Amber1Amber2){
      Serial.println("Traffic1: ON ON OFF. Traffic2: OFF ON OFF");
      printPedState();
      return;
    }

    // traffic_lights_state = red2Amber2Amber1 means that traffic1 is in the AMBER phase and traffic2 is in the RED AMBER phase
    if(traffic_lights_state == red2Amber2Amber1){
      Serial.println("Traffic1: OFF ON OFF. Traffic2: ON ON OFF");
      printPedState();
      return;
    }

    // traffic_lights_state = red2Green1 means that traffic1 is in the GREEN phase and traffic2 is in the RED phase 
    if(traffic_lights_state == red2Green1){
      Serial.println("Traffic1: OFF OFF ON. Traffic2: ON OFF OFF");
      printPedState();
      return;
    }   
}


// the following function prints the state of the pedestrian crossing to the serial monitor console
void printPedState(){

  asm volatile(
    "in r16, 8                     ;read the value in DRC into register 16 \n"
    "sts ped_lights_state, r16     ;store the value in register 16 (i.e., the state of the pedestrian crossing) into ped_lights_state \n"
    :::"r16");

  // ped_lights_state = pGreen1 means that the green light of the first pedestrian crossing is on and the red light of the second pedestrian crossing is off 
  if(ped_lights_state == pGreen1){
      Serial.println("Pedestrians1: OFF ON. Pedestrians2: ON OFF");
      Serial.println();
      return;
  }

  // ped_lights_state = pGreen2 means that the green light of the second pedestrian crossing is on and the red light of the first pedestrian crossing is off
  if(ped_lights_state == pGreen2){
      Serial.println("Pedestrians1: ON OFF. Pedestrians2: OFF ON");
      Serial.println();
      return;
  }

  // ped_lights_state = pRed1pRed2 means that the red lights of both pedestrian crossing are on
  if(ped_lights_state == pRed1pRed2){
      Serial.println("Pedestrians1: ON OFF. Pedestrians2: ON OFF");
      Serial.println();
      return;
  }
}
 
