Astrell_Protocol
====================

Here is source code to decode Astrell Temperature sensor using an arduino and RF433Mhz Receiver. 
This may work with some other various device clones (maybe Innovalley devices), but has only been tested on Astrell model.  

This code is based on work on this page :  
http://arduino.cc/forum/index.php/topic,142871.msg1106336.html#msg1106336

** Todo **  
Improve timings to be more accurate and catch signals from bigger distance

**Byte type is defined by time between 2 rising edges :**  
Synchro byte : 9705us  
1 : 4399us  
0 : 2471us  


**Here are Data samples:**  

| 10101111111111000001110100000011111 | Chan3 | 23.2°c | Manual send |  
| 10101111111110100001110101100011111 | Chan2 | 23.5   | Manual send |  
| 10101111111110000001110100100011111 | Chan1 | 23.3   | Manual send |  
| 10101111111100000001111010100011110 | Chan1 | 24.5   | Auto send   |  
| 10101111111110000001111011000011110 | Chan1 |        | Manual send |  



**Here is how 36 Bytes datagram is composed :**  

| Header        | Manuel/Auto | Channel   | Temperature (12bits MSB) | Fin        |  
|---------------|-------------|-----------|--------------------------|------------|  
| 101011111111  | 0           | 00        | 000011101011             | 00011111   |  
| Random id ? | Auto        | Channel 1 | 235 --> 23.5°C           | CRC ? |  

**How to get Channel :**  

| bytes   | Meaning        |
|----------|----------------------|
| 00 | Channel 1 |
| 01 | Channel 2 |
| 10 | Channel 3 |


** Extra info :**  

| byte | Meaning |  
|--------|-------------|
| 0       | Automatic send    |
| 1       | Manual send     |


**Reverse engineering done with :**  
- Arduino leonardo  
- RF433Mhz Receiver  
- PC with sound card and Audacity (Low cost Oscilloscope)  
- Male 2 Male Jack 3.5 cable  
- 1 wire tight between RF433Mhz Receiver data pin and left/right Jack 3.5 channel  
- 1 wire tight between Arduino ground and Jack 3.5 ground


SixK
