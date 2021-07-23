/**************************************** Background Difference Thresholds ********************/

#define threshA   1.00       //threshold level 1 (minumum temperature difference to identify a human occupied pixel)
#define threshB   0.75       //threshold level 1 (minumum temperature difference to identify a human occupied pixel)

/**************************************** Height of the Sensor ********************/

#define height  300         // sensor mounted height in centimeters
#define humanHeight 160     // typical value for human height in centimeters

/*************************************** Upper Temperature Threshold **************************/

#define upperTemp   35.0

#define delayPerCycle 10   // delay between readings in main loop in seconds. Please use at least 3 seconds
