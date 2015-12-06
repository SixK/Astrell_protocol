// Astrell Temperature sensor - based on work on the following page
// http://arduino.cc/forum/index.php/topic,142871.msg1106336.html#msg1106336
// Author : SixK

// __           ___       ___    ___
//   |         |  |      |  |   |  |
//   |_________|  |______|  |___|  |
//
//   |  Sync      |    1    |  0   |
//   |  9705us    | 4399us  | 2471s

// Defines
#define DataBits0 36                                      // Number of data0 bits to expect
#define DataBits1 36                                      // Number of data1 bits to expect
#define allDataBits 36                                    // Number of data sum 0+1 bits to expect
// isrFlags bit numbers
#define F_HAVE_DATA 1                                     // 0=Nothing in read buffer, 1=Data in read buffer
#define F_GOOD_DATA 2                                     // 0=Unverified data, 1=Verified (2 consecutive matching reads)
#define F_CARRY_BIT 3                                     // Bit used to carry over bit shift from one long to the other
#define F_STATE 7                                         // 0=Sync mode, 1=Data mode

#define DEBUG_TIMINGS 0

// calculated timings :
// Synchro : 9705us
// 1 : 4399us
// 0 : 2471us

// Constants
const unsigned long sync_MIN = 9660;                      // Minimum Sync time in micro seconds
const unsigned long sync_MAX = 9830;

const unsigned long bit1_MIN = 4220;
const unsigned long bit1_MAX = 4540;

const unsigned long bit0_MIN = 2270;
const unsigned long bit0_MAX = 2570;

const unsigned long glitch_Length = 300;                  // Anything below this value is a glitch and will be ignored.

// Interrupt variables
volatile unsigned long fall_Time = 0;                              // Placeholder for microsecond time when last falling edge occured.
volatile unsigned long rise_Time = 0;                              // Placeholder for microsecond time when last rising edge occured.
volatile byte bit_Count = 0;                                       // Bit counter for received bits.
volatile unsigned long build_Buffer[] = {0,0};                     // Placeholder last data packet being received.
volatile unsigned long read_Buffer[] = {0,0};             // Placeholder last full data packet read.
volatile unsigned long tm_Buffer[36]; 
volatile byte isrFlags = 0;                               // Various flag bits
volatile unsigned long Time;

void PinChangeISR0(){                                     // Pin 2 (Interrupt 0) service routine
  Time = micros();                          // Get current time
  if (digitalRead(3) == LOW) {
// Falling edge
    if (Time > (rise_Time + glitch_Length)) {
// Not a glitch
      Time = micros() - fall_Time;                        // Subtract last falling edge to get pulse time.
      if (bitRead(build_Buffer[1],31) == 1)
        bitSet(isrFlags, F_CARRY_BIT);
      else
        bitClear(isrFlags, F_CARRY_BIT);

      if (bitRead(isrFlags, F_STATE) == 1) {
// Looking for Data
        if ((Time > bit0_MIN) && (Time < bit0_MAX)) {
// 0 bit
#if DEBUG_TIMINGS
          tm_Buffer[bit_Count] = Time;
#endif
//          Serial.print("0");
// Serial.print(Time);
// Serial.print('-');
          build_Buffer[1] = build_Buffer[1] << 1;
          build_Buffer[0] = build_Buffer[0] << 1;
          if (bitRead(isrFlags,F_CARRY_BIT) == 1)
            bitSet(build_Buffer[0],0);
          bit_Count++;
        }
        else if ((Time > bit1_MIN) && (Time < bit1_MAX)) {
// 1 bit
//          Serial.print("1");
// Serial.print(Time);
// Serial.print('-');
#if DEBUG_TIMINGS
          tm_Buffer[bit_Count] = Time;
#endif
          build_Buffer[1] = build_Buffer[1] << 1;
          bitSet(build_Buffer[1],0);
          build_Buffer[0] = build_Buffer[0] << 1;
          if (bitRead(isrFlags,F_CARRY_BIT) == 1)
            bitSet(build_Buffer[0],0);
          bit_Count++;
        }
        else {

// Not a 0 or 1 bit so restart data build and check if it's a sync?

          bit_Count = 0;
          build_Buffer[0] = 0;
          build_Buffer[1] = 0;
          bitClear(isrFlags, F_GOOD_DATA);                // Signal data reads dont' match
          bitClear(isrFlags, F_STATE);                    // Set looking for Sync mode
          if ((Time > sync_MIN) && (Time < sync_MAX)) {
            // Sync length okay
                      // Serial.println("S");
                     // Serial.println(Time);

            bitSet(isrFlags, F_STATE);                    // Set data mode
          }
        }
        if (bit_Count >= allDataBits) {
// All bits arrived
          bitClear(isrFlags, F_GOOD_DATA);                // Assume data reads don't match
          if (build_Buffer[0] == read_Buffer[0]) {
            if (build_Buffer[1] == read_Buffer[1]) 
              bitSet(isrFlags, F_GOOD_DATA);              // Set data reads match
          }
          read_Buffer[0] = build_Buffer[0];
          read_Buffer[1] = build_Buffer[1];
          bitSet(isrFlags, F_HAVE_DATA);                  // Set data available
          bitClear(isrFlags, F_STATE);                    // Set looking for Sync mode
// digitalWrite(13,HIGH); // Used for debugging
          build_Buffer[0] = 0;
          build_Buffer[1] = 0;
          bit_Count = 0;
        }
      }
      else {
// Looking for sync
        if ((Time > sync_MIN) && (Time < sync_MAX)) {
// Sync length okay
//Serial.print("-");
          build_Buffer[0] = 0;
          build_Buffer[1] = 0;
          bit_Count = 0;
          bitSet(isrFlags, F_STATE);                      // Set data mode

//Serial.print(Time);
//Serial.print('-');
#if DEBUG_TIMINGS
          tm_Buffer[bit_Count] = Time;
#endif
// digitalWrite(13,LOW); // Used for debugging
        }
      }
      fall_Time = micros();                               // Store fall time
    }
  }
  else {
// Rising edge
    if (Time > (fall_Time + glitch_Length)) {
      // Not a glitch
      rise_Time = Time;                                   // Store rise time
    }
  }
}


void setup() {
pinMode(13,OUTPUT); // Used for debugging
  Serial.begin(9600);
  while (!Serial) {};
  pinMode(3,INPUT);
  Serial.println(F("ISR Pin 3 Configured For Input."));
  attachInterrupt(0,PinChangeISR0,CHANGE);
  Serial.println(F("Pin 3 ISR Function Attached. Here we go."));
}

void loop() {
  unsigned long myData0 = 0;
  unsigned long myData1 = 0;
  if (bitRead(isrFlags,F_GOOD_DATA) == 1) 
{
    byte H=0;
    int temp=0;
    detachInterrupt(0);
    // We have at least 2 consecutive matching reads
    myData0 = read_Buffer[0]; // Read the data spread over 2x 32 variables
    myData1 = read_Buffer[1];
    bitClear(isrFlags,F_HAVE_DATA); // Flag we have read the data

#if DEBUG_TIMINGS
    for (int x=0; x<36; x++)
    {
        Serial.println(tm_Buffer[x]);
    }
#endif

    Serial.print("****");
    Serial.print(myData0, BIN);
    Serial.print("****");
    Serial.print(myData1, BIN);
    Serial.print("****");

//    Serial.print(" - Battery=");
//    H = (myData1 >> 27) & 0x1;   // Get Battery
//    Serial.print(H);
    
    Serial.print(" Channel=");
    // H = ((myData1 >> 30) & 0x3);        // Get Channel
    H = ((myData1 >> 20) & 0x3);        // Get Channel
    Serial.print(H+1);
    
    
    Serial.print(" Temperature=");
    temp = (myData1 >> 8) &0x0FFF; // Get MMMM
    // temp = intSwap(temp,12);    
    int Temperature = temp;
    Serial.print(Temperature/10.0,1);   

    Serial.println(" ");

/*
    Serial.print("C Humidity=");
    H = (myData1 >> 8) & 0xF;        // unités
    byte ML = (myData1 >> 4) & 0xF;  // Dizaines
    
    H = byteSwap(H,4);
    ML = byteSwap(ML,4);
    
    Serial.print(ML*10 + H);
    Serial.println("%");
*/    
    attachInterrupt(0,PinChangeISR0,CHANGE);
  }
  delay(100);
}


byte byteSwap (byte number, byte bits)
{
   int reversed = 0;
   for ( byte b=0 ; b < bits ; b++ ) reversed = ( reversed << 1 ) | ( 0x1 & ( number >> b ) );
   
   return reversed;
}

int intSwap (int number, byte bits)
{
   int reversed = 0;
   for ( byte b=0 ; b < bits ; b++ ) reversed = ( reversed << 1 ) | ( 0x0001 & ( number >> b ) );
   
   return reversed;
}


/* a tester
Conversion LSB to MSB
MSB |= LSB;

// Dallas iButton test vector.
uint8_t serno[] = { 0x02, 0x1c, 0xb8, 0x01, 0, 0, 0, 0xa2 };
int
checkcrc(void)
{
uint8_t crc = 0, i;
for (i = 0; i < sizeof serno / sizeof serno[0]; i++)
crc = _crc_ibutton_update(crc, serno[i]);
return crc; // must be 0
}
*/
