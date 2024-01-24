
/* This program is a new version of the trafficLight4 program, which uses pushbuttons instead of the serial monitor, to switch the pedestrians green light on */

//the constants representing the pin numbers to which thetraffic lights are attached
#define RED1 13  
#define AMBER1 12  
#define GREEN1 11  
#define RED2 10  
#define AMBER2 9    
#define GREEN2 8  
#define P1RED 7  
#define P1GREEN 6  
#define P2RED 5  
#define P2GREEN 4  

#define BUTTON1 3
#define BUTTON2 2


/*The following constants represent some delay values (in milliseconds). 
 * TG is the time a green light stays on, TA is the time an amber light stays on, and TFlash is the time for which the pedestrian green light will be flashing before going off */
const int TG= 5000, TA= 2000, TFLASH= 800; 
 
unsigned long tRed, t0, t; /*These variable are stored as "unsigned long" because they will be used to represent time.
                            * They are decalred here so that their values can be changed without allocating new memory locations every time */

byte prevInput;  //stores the pin number linked to a pedestrian button if that button was pressed while the program was checking if the other button was pressed 


bool pCrossing= false;  //this boolean variable becomes true only when the pedestrians are crossing traffic 1 or 2 (i.e., a pedestrians green light is on)

int pGreen, pRed; /*these two integers will be used to store the numbers of the pins to which the pedestrains LEDs of the traffic whose red light is on are attached,
                   because the status of these two LEDs can change, as opposed to the status of the pedestrians LEDs corresponding to the traffic whose green light is on */
                  
char state[100];  //this array of characters will be used to store the status of the traffic light system, which will be printed to the serial monitor 

                         //they are decalred here instead of in a function, so that their values can be changed without allocating new memory locations evry time



//###################################################################################################################################################

//the setup function to set up the hardware
void setup() {
  //writing output to digital pins 13 to 4
for(int i=13; i>1; i--){
    //2 and 3 are the numbers of the pins to which the pedestrian buttons are attached, so these pins are configured as INPUT_PULLUP
    if(i==3 || i==2) { pinMode(i, INPUT_PULLUP); continue;}
    pinMode(i, OUTPUT); //and all the other pins are output pins
}
  
Serial.begin(9600); //beginning the communication between the pc and the arduino
Serial.println("Enter 1 to cross the first traffic and 2 to cross the second traffic");
}


//#######################################################################################################################################################

//the loop function that runs repeatedly
void loop() {
 /*calling the function to switch from a red to green, in one traffic, and green to red in another traffic, 
 while telling it the time the red light has been on for (tRed), and updating tRed to be the value returned by the function */
 
  tRed= redToGreenAndGreenToRed(1, RED1, AMBER1, GREEN1, 2, GREEN2, AMBER2, RED2, tRed); //this function call will change traffic1 from the red phase to the green phase, and the traffic2 from the green phase to the red phase 
  
  tRed= redToGreenAndGreenToRed(2, RED2, AMBER2, GREEN2, 1, GREEN1, AMBER1, RED1, tRed); //this function call will change traffic2 from the red phase to the green phase, and the traffic1 from the green phase to the red phase 
}


//#######################################################################################################################################################

/* The following function is the main function that controls the traffic light system, it switches between red and green phases, passing by the amber and red-amber phases, 
 * by writing to different pin numbers in order to switch the LEDs on and off, the parameters are the number of pins to which each tarffic light is connected, 
 * and the time the red light (red1) has been on, and the number of the traffic to which red1 corresponds.
 * As the names suggest, the parameter "red1" must represent a red LED, "amber1" must represent an amber LED, and "green1" must be a green LED, 
 * and they all must belong to the traffic whose number is represented by the "traffic1" parameter. Also, the same for "traffic2", "green2", "amber2", and "red2".
 * Any change to this order will lead to unexpected results*/
unsigned long redToGreenAndGreenToRed(int traffic1, int red1, int amber1, int green1, int traffic2, int green2, int amber2, int red2, unsigned int tRed){

/*First of all, we turn on the lights attached to red1 and green2, we aslo initialise pGreen and pRed (based on the value of traffic1, i.e., the traffic whose red light is currently on), as well as t and t0*/
initialise(red1, green2, traffic1); 

printState(traffic1, "ON OFF OFF", "ON OFF", traffic2, "OFF OFF ON"); //print the state of both traffics

//while the red light has been on for less than TG, check if the pedestrians want to cross 
while(t + tRed < TG) {
   t = millis() - t0; //update t to be the time the while loop has been running
   if(pressedButton((traffic1==1)? BUTTON1:BUTTON2)){
      delaySwitch(0,LOW, 1, pRed); delaySwitch(0, HIGH, 1, pGreen); pCrossing= true; printState(traffic1, "ON OFF OFF", "OFF ON", traffic2, "OFF OFF ON");
      break; //if a pedestrian presses the button, turn their green light on, and exit the loop
   }
}

/*Check if we exited the loop because a pedestrian pressed the button, i.e, pCrossing is true. 
If so, that means the green light (green2) has been on for t, so we need to wait until it has been on for TG to turn it off */
if(pCrossing){
    //wait for TG-t, and then turn the green light (green2) off and the amber light (amber2) on
    delaySwitch(TG -t, LOW, 1, green2); 
    delaySwitch(0, HIGH, 1, amber2); 
    printState(traffic1, "ON OFF OFF", "ON OFF", traffic2, "OFF ON OFF"); //print the new state of the traffic lights

/*In order to make sure the amber light (amber2) stays on for TA, and the pedestrians green light (which has now been on for TG -t) stays on for TG,
we need to compare TA and t, to know whether to turn the pedestrians green light off first (i.e, t < TA) or turn amber2 off first (i.e., t > TA, or do both at the same time (i.e., t = TA) */

//if t is less than or equal to TA
    if(t <= TA) {
    delayFlashPG(t, pGreen, pRed);
    delaySwitch(0, HIGH, 1, amber1);  //immediately turn the amber light (amber1) on after turning the pedestrians green light off
    printState(traffic1, "ON ON OFF", "ON OFF", traffic2, "OFF ON OFF");
    
    delaySwitch(TA-t, LOW, 1, amber2); //wait until the amber light (amber2) has been on for TA, then turn it off, and turn the red light (red2) on
    delaySwitch(0, HIGH, 1, red2);
    printState(traffic1, "ON ON OFF", "ON OFF", traffic2, "ON OFF OFF");
    
    delaySwitch(t, LOW, 2, red1, amber1);//wait until the amber light (amber1) has been on for TA (because it has been on for TA-t, we need to wait for t), and then turn it off, as well as red1
    delaySwitch(0, HIGH, 1, green1);  //then we turn the green light (green1) on
    printState(traffic1, "OFF OFF ON", "ON OFF", traffic2, "ON OFF OFF");
      
    return t;   //the red light (red2) has been of for t, so we need to return this value, to pass it as an argument to the next function call
   }

//if t is greater than TA
    else {
    delaySwitch(TA, LOW, 1, amber2);  //turn the amber light (amber2) off after TA
    delaySwitch(0, HIGH, 1, red2);
    printState(traffic1, "ON OFF OFF", "OFF ON", traffic2, "ON OFF OFF");

    delayFlashPG(t-TA, pGreen, pRed);   //now turn the pedestrians green light off
    delaySwitch(0, HIGH, 1, amber1); //turn the amber light (amber1) on immediately after turning the pedestrians green light off 
    printState(traffic1, "ON ON OFF", "ON OFF", traffic2, "ON OFF OFF");

    delaySwitch(TA, LOW, 2, red1, amber1); //after amber1 has been on for TA, turn it and red1 off, and turn green1 on
    delaySwitch(0, HIGH, 1, green1);
    printState(traffic1, "OFF OFF ON", "ON OFF", traffic2, "ON OFF OFF");
    
    return t;  //the red light (red2) has been of for t, so we need to return this value, to pass it as an argument to the next function call
    }
}

//if no pedestrian pressed a button, we should resume the normal traffic light system where green and red lights of two different traffics are turned on at the same time
else {
    //At this point, the condition in the first while loop is no more true (i.e., t + tRed = TG), but the green light (green2) has been on for t only, 
    //so to make it last on for TG, we need to wait for tRed, and then turn it off and turn the amber light (amber2) on, and then we can turn the second green light (green1) on
    delaySwitch(tRed, LOW, 1, green2);     
    delaySwitch(0, HIGH, 2, amber1, amber2); 
    printState(traffic1, "ON ON OFF", "ON OFF", traffic2, "OFF ON OFF");

    delaySwitch(TA, LOW, 3, red1, amber1, amber2); //wait for TA before turning the amber lights off, and green1 and red2 on
    delaySwitch(0, HIGH, 2, green1, red2);
    printState(traffic1, "OFF OFF OFF", "ON OFF", traffic2, "ON OFF OFF");
    
    return 0; 
}

}


//##########################################################################################################################################################

void initialise(int red, int green, int traffic) { 
  
//turn on the red and green lights of different traffics and all the pedestrians red lights. This is only needed the very first time this function is invoked, because after that this will be done at the end of the previous call of the redToGreenAndGreenToRed() function
delaySwitch(0, HIGH, 4, red, green, P1RED, P2RED); 

//setting the values of pGreen and pred depending on the traffic number
if(traffic ==1) { pGreen= P1GREEN; pRed= P1RED;}
if(traffic ==2) { pGreen= P2GREEN; pRed= P2RED; }

pCrossing= false; //reseting pCrossing to false

t=0; //reset the value of t to 0
t0= millis(); //t0 is the time for which the program has been running till this point
}


//##########################################################################################################################################################

//the function below is told the state of the lights of each traffic, but it only needs to know the state of one pedestrian light, 
//because while one pedestrian light can be switched from green to red, the second pedestrian light must stay red (i.e., its state will be ON OFF)
void printState(int a, char traffic1[15], char ped1[10], int b, char traffic2[15]){
  /*using sprintf function to format the output string describing the state of the traffic light, and store it in "state"*/
  sprintf(state, "Traffic %d: %s, Pedestrian %d: %s. Traffic %d: %s, Pedestrian %d: ON OFF.", a, traffic1, a, ped1, b, traffic2, b);
  Serial.println(state);
  }

  
//##########################################################################################################################################################
/*
* Since digitalWrite writes to one pin at a time, it would be used several times in the code, we also often need to call the delay function before calling it;
* so to lessen the code, the follwing function "delaySwitch" is given a time to wait for, and one or more pin numbers and the state they should be set to (i.e., LOW or HIGH), as well as the total number of those pins
*/
void delaySwitch(unsigned long theDelay, char state[4], int n, ...){
  delay(theDelay);
  va_list list;      
  va_start(list, n); 
  for (int i=0; i< n; i++) {
    digitalWrite(va_arg(list, int), state); //setting each digital pin in the list to the state (ON or OFF)
  }
  va_end(list);
}


//##########################################################################################################################################################
//The following function will return true when the button connected to the pin nmber passed to it as a peremeter was pressed
boolean pressedButton(int button){
  /*if the button was pressed, or the button was pressed the last time this function was invoked, the returned value is true, and 
  the prevInput is cleared to store a new pin number if the button connected to that pin is pressed */
  boolean result= (digitalRead(button)==LOW); //if the button was pressed, the returned value will be true
  if (prevInput==button ) {result= true; prevInput=0;} 
  if(button==BUTTON1 && digitalRead(BUTTON2)==LOW) prevInput= BUTTON2;
  if(button==BUTTON2 && digitalRead(BUTTON1)==LOW) prevInput= BUTTON1;

  return result;
 }

//##########################################################################################################################################################


//the following function flashes a pedestrians green light then it turns it off and turns the red light on, all of this after waiting for "theDelay"
void delayFlashPG(unsigned long theDelay, int green, int red) {
  //flashing the pedestrians' green light
  delaySwitch(theDelay, LOW, 1, green);
  
  for(int i=0; i<4; i++){
  delaySwitch(TFLASH/4, (i%2 == 0)? HIGH:LOW, 1, green);
  }
  //and then the red light is turned on
  delaySwitch(0, HIGH, 1, red);
}
