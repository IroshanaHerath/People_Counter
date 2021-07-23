#include <Wire.h>
#include "Configurations.h"

const char addr = 0x69;         //I2c Address of sensor
const char numPixels = 64;
const char sampleDelay = 10;

#define refLength 50

byte pixelTempL, neighbourItems[4], blobSize[numPixels];

float celsius, 
      thermalMatrix[numPixels], 
      avgPixel,                 
      diffMatrix[numPixels],     
      backData[numPixels],      
      backY                     
      ;
      
boolean presence[numPixels], finalPresence[numPixels], markedArray[numPixels];
      
int temperatureTherm;
float celsiusTherm, avgTherm;
double pixelLength;
int rawTemperature, peopleCount, countArray[numPixels];

void setup() {
  
  Wire.begin();
  Serial.begin(115200);
  delay(5000);
  Serial.println("Initializing...");

  if(height > 540)  pixelLength = 50; 
  else if(height < 48)  pixelLength = 6.25;
  else  pixelLength = 2*tan(3.75*PI/180)*(height - humanHeight);
  

  Serial.println("\tPixel Length for the height set in cofig.h = " + String(pixelLength) + "cm");

  readPixels(20);
  calcPixelAverage();
  initBack();
  
  Serial.println("\nInitial Background Temperature Matrix:\n");
  printMatrix(backData);
  
  calcDifference();
  
  Serial.println("Now program started. Leave 2-3 mins to update all matrices & for stable readings");
  
}

void loop(){

  Serial.flush();
  
  readPixels(10);
  Serial.flush();
  
  backgroundSubstraction();
  Serial.flush();
  
  nearestNeighbour();
  Serial.flush();
  
  peopleCount = getPeopleCount(presence);

  Serial.println("\nFinal Human available Pixel Count = " + String(peopleCount));

  unsigned int blobCount = getBlobs();
  
  Serial.println("\nNumber of human-sized objects within the sensing grid = " + String(blobCount) + "\n");

  Serial.print("Blob Sizes =\t");

  for(byte i = 0; i < blobCount; i++) Serial.print(String(blobSize[i]) + "\t");

  int blb = peopleCount * pixelLength/refLength;
  Serial.println("People count using Pixel Grid & configurations set = " + String(blb));

  float total = 0;

  for(byte i = 0 ; i < blobCount; i++) total += blobSize[i]*pixelLength/refLength;

  //Serial.println("Blobs using Blob Pixels = " + String(total));

  Serial.println();
  
  delay(delayPerCycle*1000);
  
}

void readAll(byte samples)
{
  clearMatrix();
  avgTherm = 0;

  for (byte i = 0; i<samples; i++)
  {
    pixelTempL = 0x80;
    for(byte pixel = 0; pixel < numPixels; pixel++)
    {
      Wire.beginTransmission(addr);
      Wire.write(pixelTempL);
      Wire.endTransmission();
      Wire.requestFrom(addr, 2);
      byte lowerLevel = Wire.read();
      byte upperLevel = Wire.read();
  
      rawTemperature = ((upperLevel << 8) | lowerLevel);
  
      if (rawTemperature > 2047)
      {
        rawTemperature -= 4096;
      }
      
      if(rawTemperature * 0.25 < 50.00) celsius = rawTemperature * 0.25;
      
      pixelTempL = pixelTempL + 2;
      
      thermalMatrix[pixel] += celsius;
    }
    
    Wire.beginTransmission(addr);
    Wire.write(0x0E);
    Wire.endTransmission();
    Wire.requestFrom(addr, 2);
    byte upperLevelTherm = Wire.read();
    byte lowerLevelTherm = Wire.read();
  
    temperatureTherm = ((lowerLevelTherm << 8) | upperLevelTherm);
    
    if(temperatureTherm * 0.0625 < 50.00) celsiusTherm = temperatureTherm * 0.0625;
    
  
    avgTherm += celsiusTherm;
    
    
    delay(sampleDelay);
  }

  for(byte pixel = 0; pixel < numPixels; pixel++)
  {
    thermalMatrix[pixel] /= samples;
  }
  
  avgTherm /= samples;

  Serial.println("\nTemperature Matrix:\n");
  printMatrix(thermalMatrix);
  Serial.println("\nThermister Temperature = " + String(avgTherm) + "\n");
  
}

float getCurrentTemp(byte samples)  // ca
{
  avgTherm = 0;
  for(byte i = 0; i < samples; i++)
  {
    Wire.beginTransmission(addr);
    Wire.write(0x0E);
    Wire.endTransmission();
    Wire.requestFrom(addr, 2);
    byte upperLevelTherm = Wire.read();
    byte lowerLevelTherm = Wire.read();
  
    temperatureTherm = ((lowerLevelTherm << 8) | upperLevelTherm);
    
    if(temperatureTherm * 0.0625 < 50.00) celsiusTherm = temperatureTherm * 0.0625;
    
    avgTherm += celsiusTherm;
    
    delay(sampleDelay);
  }
  avgTherm /= samples;
  
  Serial.println("\nThermister Temperature = " + String(avgTherm));
  Serial.println();

  return avgTherm;
  
}

void readPixels(byte samples)  // read only the thermal matrix
{
  
  clearMatrix();

  for (byte i = 0; i<samples; i++)
  {
    pixelTempL = 0x80;
    for(byte pixel = 0; pixel < numPixels; pixel++)
    {
      Wire.beginTransmission(addr);
      Wire.write(pixelTempL);
      Wire.endTransmission();
      Wire.requestFrom(addr, 2);
      byte lowerLevel = Wire.read();
      byte upperLevel = Wire.read();
  
      rawTemperature = ((upperLevel << 8) | lowerLevel);
  
      if (rawTemperature > 2047)
      {
        rawTemperature -= 4096;
      }
      
      if(rawTemperature * 0.25 < 50.00)  celsius = rawTemperature * 0.25;
      
      pixelTempL = pixelTempL + 2;
      
      thermalMatrix[pixel] += celsius;
    }
    
    delay(sampleDelay);
    
  }

  for(byte pixel = 0; pixel < numPixels; pixel++)
  {
    thermalMatrix[pixel] /= samples;
  }

  updateBack();

  Serial.println("\nTemperature Matrix:\n");
  printMatrix(thermalMatrix);
  
}

void clearMatrix()
{
  
  for(byte i =0; i < numPixels; i++)
  {
      thermalMatrix[i] = 0;
  }
  
}

void calcPixelAverage()
{
  avgPixel = 0;
  for(byte i =0; i < numPixels; i++)
  {
      avgPixel += thermalMatrix[i];
  }
  avgPixel /= numPixels;

  Serial.println("\nAverage Pixel Temperature = " + String(avgPixel) + "\n");
  
}

void calcDifference()
{

  for(byte i =0; i < numPixels; i++)
  {
      diffMatrix[i] = thermalMatrix[i] - avgPixel;
      if(diffMatrix[i] < threshA) presence[i] = false;
      else presence[i] = true;
  }

  Serial.println("\nDifference Matrix:\n");
  printMatrix(diffMatrix);

  Serial.println("\nInitial Occupancy Matrix:\n");
  printMatrix(presence);
  
}

void initBack()
{
  
  for(byte i = 0; i < numPixels; i++)
  {
    backData[i] = thermalMatrix[i];  
  }
  
}

void updateBack()
{
  byte count = 0;
  backY = 0;
  for(byte i = 0; i < numPixels; i++)
  {
      if(!presence[i])
      {
        backY += thermalMatrix[i];
        count++;  
      }
  }
  Serial.println("Back Y = " + String(backY));
  if(count != 0)  backY /= count;

  Serial.println("Back Y = " + String(backY) + " Count = " + String(count));
  
  for(byte i = 0; i < numPixels; i++)
  {
      if(!presence[i]) backData[i] = backData[i]*127.00/128 + thermalMatrix[i]*1.00/128;
      else  backData[i] = backData[i]*127.00/128 + backY*1.00/128;   
  }

  Serial.println("\nBackground Temperature Matrix:\n");
  printMatrix(backData);
}

void backgroundSubstraction()
{

  
  for(byte i =0; i < numPixels; i++)
  {
      diffMatrix[i] = thermalMatrix[i] - backData[i];

      ////////////////////////////////
      //diffMatrix[i] = random(50,120)/100.00;
      
      if(presence[i] && diffMatrix[i] < threshB) presence[i] = false;
      else if(presence[i] && diffMatrix[i] >= threshB && thermalMatrix[i] < upperTemp) presence[i] = true;
      else if(!presence[i] && diffMatrix[i] < threshA) presence[i] = false;
      else if(!presence[i] && diffMatrix[i] >= threshA && thermalMatrix[i] < upperTemp) presence[i] = true;

  }

  Serial.println("\nMatrix of Temperature Difference :\n");
  printMatrix(diffMatrix);

  Serial.println("\nOccupancy Matrix before Nearest Neighbour Algorithm :\n");
  printMatrix(presence);
  
}

void nearestNeighbour()
{
  int count = 0;
  
  for(byte i =0; i < numPixels; i++)
  {
    count = 0;
    
    finalPresence[i] = presence[i];
    
    if(presence[i])
    {
      if((i > 7) && presence[i-8]) count++; 
      if((i < 56) && presence[i+8]) count++;
      if((i % 8 != 0) && presence[i-1]) count++;
      if((i % 8 != 7) && presence[i+1]) count++;

      if(count < 2)   finalPresence[i] = false;
      else            finalPresence[i] = true;
    }
  }

  for(byte i =0; i < numPixels; i++)
  {
      presence[i] = finalPresence[i];
  }

  Serial.println("\nOccupancy Matrix after Nearest Neighbour Algorithm :\n");
  printMatrix(presence);
  
}

int getPeopleCount(boolean image[])
{
  int count = 0;
  
  for(byte i = 0; i < numPixels; i++)
  {
    if(image[i])  count++;  
  }  

  return count;
}

void printMatrix(float matrix[]) 
{ 
  //Serial.println("\nGrid-EYE:\n");
  
  for(byte i =0; i < numPixels; i++)
  {
      Serial.print(String(matrix[i]) + " ");
      if((i+1)%8 == 0)
      {
        Serial.println();
      }
  }
  
}

void printMatrix(boolean matrix[])  
{
  //Serial.println("\nGrid-EYE:\n");
  
  for(byte i =0; i < numPixels; i++)
  {
      Serial.print(String(matrix[i]) + " ");
      if((i+1)%8 == 0)
      {
        Serial.println();
      }
  }
  
}

void initMarkedArray()
{
  
  for(byte i = 0; i < numPixels; i++)
  {
    markedArray[i] = false;  
  }
  
}


int getBlobs()
{
  int blobs = 0;
  byte count = 0;

  initMarkedArray();
  
  for(byte i = 0; i < numPixels; i++)
  {
    if(!markedArray[i] && presence[i])
    {

      if(!((i % 8 != 0) && markedArray[i-1]) && !((i > 7) && markedArray[i-8]) && !(((i % 8 != 7) && presence[i+1]) && ((i % 8 != 7) && (i > 7) && presence[i-7]) ))
      {
          blobs++;
          count = 0;
      }
      
      markedArray[i] = true;
      count++;
      if((i % 8 != 7) && presence[i+1])
      {
        markedArray[i+1] = true;
        count++;
      }
      if((i < 56) && presence[i+8])
      {
        markedArray[i+8] = true;
        count++;
      }
      if((i < 56) && (i % 8 != 7) && presence[i+9])
      {
        markedArray[i+9] = true;
        count++;
      }

      if(blobs > 0) blobSize[blobs-1] = count;
      
    }
  }
  
  return blobs;
  
}

void initCount()
{
  
  for(int i = 0 ; i < numPixels ; i++)
  {
      countArray[i] = 0;
  }
  
}

int getCount()
{
  int maximum = 0, index = 0;
  for(int i = 0 ; i < numPixels ; i++)
  {
      if(maximum < countArray[i])
      {
         maximum = countArray[i];
         index = i;
      }
  }

  return index;
}






