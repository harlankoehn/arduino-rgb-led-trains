#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <stdlib.h>
#include <MemoryFree.h>

#define PIN 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(300, PIN, NEO_RGB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

class Speed {
  public:
    byte moveOnMultiple;
    byte moveForwardBy;
    byte skipOnMultiple;
};

class Egg {
  public:
    int pixel;
    uint32_t color;
};


int loopCount = 0;
Speed* speeds = new Speed[37];
bool layEggsOn = true;
int eggCount = 10;
Egg* eggs = new Egg[eggCount];
bool stopLightOn = false;
int nextRedLightOnMultiple = 3;
int nextRedLightOffMultiple = 6;

void addEgg(int pixel, uint32_t color) {
  for(int e = 0; e < eggCount; e++) {
    Egg &eg = eggs[e];
    if(eg.pixel == -1) {
      eg.pixel = pixel;
      eg.color = color;
      return;
    }
  }
}

int getRandomInt(int min, int max) {
   //don't use randomSeed unless you want every number to look about the same
  //randomSeed(analogRead(0));
  return random(min, max);
}

int getReducedSpeed(int curSpeed, int front, int travelDir) {
  if(curSpeed == 0) {
    return 0;
  }
  int ret = curSpeed;
  if((front > 130 && travelDir == 1) || (front < 165 && travelDir == -1)) { 
    if(curSpeed > 13) { 
      ret-=2;
    } else {
       ret--; 
    }
  } else if((front > 110 && travelDir == 1) || (front < 180 && travelDir == -1)) {
    if(curSpeed > 23) {
      ret-=2;
    } else if(curSpeed > 12) {
      ret--;   
    } else {
      if((loopCount % 3) == 0) { 
        ret--;
      } 
    }
  }
  /*if(front < 158 && travelDir == -1) { 
    if(curSpeed > 28) { 
      ret-=3; 
    } else { 
      ret--; 
      }
  } else if(front < 180 && travelDir == -1) {
    if(curSpeed > 28) {
      ret-=2;   
    } else {
         if((loopCount % 10) == 0) { 
            ret = curSpeed - 1; 
        } 
    }
  }*/ 
  return ret;
}

class Train {
  public:
    uint32_t color;
    int hideLength;
    byte carLength;
    byte carGap;
    byte numOfCars;
    int travelDir; //1 or -1
    byte startSpeed;
    byte currentSpeed;
    int start;
    int startBlink;
    int stopBlink;
    bool blinkOn;
    int front;
    int trainEnd;
    int trainLength;
    uint32_t eggColor;
    void move();
    void init();
    void calcTrain();
};

void Train::move() {
  if(stopLightOn == true) {
    if(front < 150 && travelDir == 1) {
      currentSpeed = getReducedSpeed(currentSpeed, front, travelDir);
    } else if(front > 150 && travelDir == -1) {
      currentSpeed = getReducedSpeed(currentSpeed, front, travelDir);
    }
  }
  if(stopLightOn == false || (front > 149 && travelDir == 1) || (front < 149 && travelDir == -1)) {
    if(currentSpeed < startSpeed) {
      if(currentSpeed == 0 && startSpeed > 11) {
        currentSpeed = 9;
      } else {
        if((loopCount % 5) == 0) {
          currentSpeed++;
        }
      }
    }
  }
  if(travelDir == 1) {
    if((loopCount % speeds[currentSpeed].moveOnMultiple) == 0 && (loopCount % speeds[currentSpeed].skipOnMultiple) != 0) {
      front+= speeds[currentSpeed].moveForwardBy;
      calcTrain();
      int resetPoint = strip.numPixels() + hideLength -1;
      if(trainEnd > resetPoint) {
        front = start;
        calcTrain();
      }
    }    
  } else {
    if((loopCount % speeds[currentSpeed].moveOnMultiple) == 0 && (loopCount % speeds[currentSpeed].skipOnMultiple) != 0) {
      front-= speeds[currentSpeed].moveForwardBy;
      calcTrain();
      int resetPoint = (hideLength * -1);
      if(trainEnd < resetPoint) {
        front = start;
        calcTrain();
      }
    }
  }
}

void Train::init() {
    front = start;
    currentSpeed = startSpeed;
    startBlink = getRandomInt(30, 60);
    stopBlink = getRandomInt(500, 800);
    blinkOn = false;
    calcTrain();
}

void Train::calcTrain() {
  trainLength = (numOfCars * (carLength + carGap)) - carGap;
  trainEnd = travelDir > 0 ? (front - trainLength) + 1 : (front + trainLength) - 1;
}



const int trainCount = 5;
Train* trains = new Train[trainCount];

void setup() {  
  Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(255);

  nextRedLightOnMultiple = getRandomInt(100, 1000);
  nextRedLightOffMultiple = getRandomInt(nextRedLightOnMultiple + 500, nextRedLightOnMultiple + 1500);

  fillOutSpeeds();

  /* each new train you define takes 30 bytes of SRAM. Will be even more as more new features/fields are added */
  
  trains[0].color = strip.Color(0, 0, 255);
  trains[0].carLength = 6;
  trains[0].carGap = 1;
  trains[0].numOfCars = 8;
  trains[0].travelDir = 1; //1 or -1
  trains[0].startSpeed = 9;
  trains[0].start = 0;
  trains[0].hideLength = 0;
  trains[0].init();


  trains[1].color = strip.Color(255, 0, 0);
  trains[1].carLength = 5;
  trains[1].carGap = 1;
  trains[1].numOfCars = 7;
  trains[1].travelDir = 1; //1 or -1
  trains[1].startSpeed = 11;
  trains[1].start = 0;
  trains[1].hideLength = 900;
  trains[1].init();


  trains[2].color = strip.Color(43, 94, 56);
  trains[2].carLength = 4;
  trains[2].carGap = 1;
  trains[2].numOfCars = 5;
  trains[2].travelDir = 1; //1 or -1
  trains[2].startSpeed = 13;
  trains[2].start = 0;
  trains[2].hideLength = 450;
  trains[2].init();


  trains[3].color = strip.Color(240, 232, 10);
  trains[3].carLength = 3;
  trains[3].carGap = 1;
  trains[3].numOfCars = 4;
  trains[3].travelDir = 1; //1 or -1
  trains[3].startSpeed = 16;
  trains[3].start = 0;
  trains[3].hideLength = 1100;
  trains[3].init(); 



  //trains[4].color = strip.Color(76, 5, 190);
  trains[4].color = strip.Color(0, 0, 255);
  trains[4].carLength = 6;
  trains[4].carGap = 1;
  trains[4].numOfCars = 8;
  trains[4].travelDir = 1; //1 or -1
  trains[4].startSpeed = 8;
  trains[4].start = 0;
  trains[4].hideLength = 15;
  trains[4].init();
 

  trains[5].color = strip.Color(120, 240, 11);
  trains[5].carLength = 1;
  trains[5].carGap = 7;
  trains[5].numOfCars = 9;
  trains[5].travelDir = 1; //1 or -1
  trains[5].startSpeed = 8;
  trains[5].start = -200;
  trains[5].hideLength = 300;
  trains[5].init();

/*

  trains[6].color = strip.Color(50, 0, 30);
  trains[6].carLength = 2;
  trains[6].carGap = 8;
  trains[6].numOfCars = 8;
  trains[6].travelDir = -1; //1 or -1
  trains[6].startSpeed = 3;
  trains[6].start = 350;
  trains[6].hideLength = 450;
  trains[6].ledStrip = strip;
  trains[6].init();
  */
           
}

void loop() {
  
    //turn all of them off right to start with
    for(int i = 0; i < 300; i++) {
      strip.setPixelColor(i, 0);
    }

  if(stopLightOn == false) {
    if(loopCount > 300 && (loopCount % nextRedLightOnMultiple) == 0) {
      if(putOnStopLight() == true) {
        nextRedLightOffMultiple = getRandomInt(nextRedLightOnMultiple + 500, nextRedLightOnMultiple + 800);
        stopLightOn = true;
        Serial.print("freeMemory()=");
        Serial.println(freeMemory());
      }
    }
  }

  if(stopLightOn == true) {
    if((loopCount % nextRedLightOffMultiple) == 0) {
      stopLightOn = false;
      nextRedLightOnMultiple = getRandomInt(100, 1000);
    } else {    
      strip.setPixelColor(150, strip.Color(0,255,0));
    }
  }

  for(int n = 0; n < trainCount; n++) {
    Train &tr = trains[n];
    if(
        (tr.front >= 0 && tr.front < strip.numPixels())
        ||
       (tr.trainEnd >= 0 && tr.trainEnd < strip.numPixels())
      ) {
        if(tr.travelDir == 1) {
          for(int i = tr.front; i >= tr.trainEnd; i-=(tr.carGap + tr.carLength)) {
            int carEnd = i - tr.carLength;
            for(int x = i; x > carEnd; x--) {
              if(x >= 0 && x < strip.numPixels()) {
                if(x == tr.front) {
                  strip.setPixelColor(x, strip.Color(255,255,255));
                } else {
                  strip.setPixelColor(x, tr.color);
                }
              }
            }
          }
          //if(tr.egg > 0 && (tr.trainEnd > tr.egg)) {
          //  strip.setPixelColor(tr.egg, tr.eggColor);
          //}
        } else {
          for(int i = tr.front; i <= tr.trainEnd; i+=(tr.carGap + tr.carLength)) {
            int carEnd = i + tr.carLength;
            for(int x = i; x < carEnd; x++) {
              if(x >= 0 && x < strip.numPixels()) {
                if(x == tr.front) {
                  strip.setPixelColor(x, strip.Color(255,255,255));
                } else {
                  strip.setPixelColor(x, tr.color);
              }
            }
          }
        }
       }
       if(layEggsOn == true) {
        if(stopLightOn == false) {
         if((loopCount % tr.stopBlink) == 0 && tr.trainEnd > 10 && tr.trainEnd < strip.numPixels() - 10 && tr.trainEnd != 150) {
          addEgg(tr.trainEnd, tr.color);
         }
        }
         for(int e = 0; e < eggCount; e++) {
          Egg &eg = eggs[e];
          if(eg.pixel != -1) {
            if(eg.pixel < tr.front && eg.pixel > tr.trainEnd && tr.travelDir == 1) {
              eg.pixel = -1;
            } else if(eg.pixel > tr.front && eg.pixel < tr.trainEnd && tr.travelDir == -1) {
              eg.pixel = -1;
            }
            if(eg.pixel > -1 && eg.pixel < strip.numPixels()) {
              strip.setPixelColor(eg.pixel, eg.color);
            }
          }
         }
       }
    
       if(tr.currentSpeed == 0) {
         if(tr.blinkOn == false) {
            if((loopCount % tr.startBlink) == 0) {
              tr.blinkOn = true;
            }
         } else {
           if((loopCount % tr.stopBlink) == 0) {
             tr.blinkOn = false;
           }
         }
          if(tr.blinkOn == true) {
            strip.setPixelColor(tr.trainEnd, strip.Color(0,255,0));
          }         
       } else {
         tr.blinkOn = false;
          if(tailOn(tr.trainEnd) == true) {
            strip.setPixelColor(tr.trainEnd, strip.Color(0,255,0));
          }
       }
      }      
    tr.move();
     //} else if(tr.stepOn == 2 && ((loopCount % 2) == 0)) {
     //   tr.move();
     //} else if(tr.stepOn == 3 && ((loopCount % 3) == 0)) {
     //   tr.move();
     //}
  }
  strip.show();
  //delay(500);
  loopCount++;
  if(loopCount > 32765) {
    loopCount = 0;
  }
}

bool tailOn(int num) {
  if(num > 20 && num < 40) {
    return true;
  } else if(num > 80 && num < 100) {
    return true;
  } else if(num > 140 && num < 160) {
    return true;
  } else if(num > 200 && num < 220) {
    return true;
  } else if(num > 260 && num < 280) {
    return true;    
  } else {
    return false;  
  }
}

bool putOnStopLight() {
  bool ret = true;
  for(int n = 0; n < trainCount; n++) {
    Train &tr = trains[n];
    if(tr.front < 150 && tr.front > 130 && tr.travelDir == 1) {
      ret = false;
    } else if(tr.front > 150 && tr.trainEnd < 150 && tr.travelDir == 1) {
      ret = false;
    }
    if(tr.front > 150 && tr.front < 130 && tr.travelDir == -1) {
      ret = false;
    } else if(tr.front < 150 && tr.trainEnd > 150 && tr.travelDir == -1) {
      ret = false;
    }    
  }
  return ret;  
}



void fillOutSpeeds() {
  speeds[0].moveOnMultiple = 1;
  speeds[0].moveForwardBy = 0;
  speeds[0].skipOnMultiple = 0;

  speeds[1].moveOnMultiple = 25;
  speeds[1].moveForwardBy = 1;
  speeds[1].skipOnMultiple = 0;  

  speeds[2].moveOnMultiple = 20;
  speeds[2].moveForwardBy = 1;
  speeds[2].skipOnMultiple = 0;  

  speeds[3].moveOnMultiple = 15;
  speeds[3].moveForwardBy = 1;
  speeds[3].skipOnMultiple = 0;  

  speeds[4].moveOnMultiple = 10;
  speeds[4].moveForwardBy = 1;
  speeds[4].skipOnMultiple = 0;  

  speeds[5].moveOnMultiple = 8;
  speeds[5].moveForwardBy = 1;
  speeds[5].skipOnMultiple = 0;  

  speeds[6].moveOnMultiple = 6;
  speeds[6].moveForwardBy = 1;
  speeds[6].skipOnMultiple = 0;  

  speeds[7].moveOnMultiple = 4;
  speeds[7].moveForwardBy = 1;
  speeds[7].skipOnMultiple = 0;  

  speeds[8].moveOnMultiple = 3;
  speeds[8].moveForwardBy = 1;
  speeds[8].skipOnMultiple = 0;  

  speeds[9].moveOnMultiple = 2;
  speeds[9].moveForwardBy = 1;
  speeds[9].skipOnMultiple = 0;  

  speeds[10].moveOnMultiple = 3;
  speeds[10].moveForwardBy = 2;
  speeds[10].skipOnMultiple = 0;    

  speeds[11].moveOnMultiple = 1;
  speeds[11].moveForwardBy = 1;
  speeds[11].skipOnMultiple = 0;    
  
  speeds[12].moveOnMultiple = 1;
  speeds[12].moveForwardBy = 2;
  speeds[12].skipOnMultiple = 3; 

  speeds[13].moveOnMultiple = 1;
  speeds[13].moveForwardBy = 2;
  speeds[13].skipOnMultiple = 4;
  
  speeds[14].moveOnMultiple = 1;
  speeds[14].moveForwardBy = 2;
  speeds[14].skipOnMultiple = 5;

  speeds[15].moveOnMultiple = 1;
  speeds[15].moveForwardBy = 2;
  speeds[15].skipOnMultiple = 6;
    
  speeds[16].moveOnMultiple = 1;
  speeds[16].moveForwardBy = 2;
  speeds[16].skipOnMultiple = 7; 

  speeds[17].moveOnMultiple = 1;
  speeds[17].moveForwardBy = 2;
  speeds[17].skipOnMultiple = 8;

  speeds[18].moveOnMultiple = 1;
  speeds[18].moveForwardBy = 2;
  speeds[18].skipOnMultiple = 9;

  speeds[19].moveOnMultiple = 1;
  speeds[19].moveForwardBy = 2;
  speeds[19].skipOnMultiple = 10;

  speeds[20].moveOnMultiple = 1;
  speeds[20].moveForwardBy = 2;
  speeds[20].skipOnMultiple = 0;

  speeds[21].moveOnMultiple = 1;
  speeds[21].moveForwardBy = 3;
  speeds[21].skipOnMultiple = 4;

  speeds[22].moveOnMultiple = 1;
  speeds[22].moveForwardBy = 3;
  speeds[22].skipOnMultiple = 5;

  speeds[23].moveOnMultiple = 1;
  speeds[23].moveForwardBy = 3;
  speeds[23].skipOnMultiple = 6;    

  speeds[24].moveOnMultiple = 1;
  speeds[24].moveForwardBy = 3;
  speeds[24].skipOnMultiple = 7;

  speeds[25].moveOnMultiple = 1;
  speeds[25].moveForwardBy = 3;
  speeds[25].skipOnMultiple = 8;  
  
  speeds[26].moveOnMultiple = 1;
  speeds[26].moveForwardBy = 3;
  speeds[26].skipOnMultiple = 9;

  speeds[27].moveOnMultiple = 1;
  speeds[27].moveForwardBy = 3;
  speeds[27].skipOnMultiple = 10;  

  speeds[28].moveOnMultiple = 1;
  speeds[28].moveForwardBy = 3;
  speeds[28].skipOnMultiple = 0;

  speeds[29].moveOnMultiple = 1;
  speeds[29].moveForwardBy = 4;
  speeds[29].skipOnMultiple = 5;

  speeds[30].moveOnMultiple = 1;
  speeds[30].moveForwardBy = 4;
  speeds[30].skipOnMultiple = 6;
  
  speeds[31].moveOnMultiple = 1;
  speeds[31].moveForwardBy = 4;
  speeds[31].skipOnMultiple = 6;  

  speeds[32].moveOnMultiple = 1;
  speeds[32].moveForwardBy = 4;
  speeds[32].skipOnMultiple = 7;

  speeds[33].moveOnMultiple = 1;
  speeds[33].moveForwardBy = 4;
  speeds[33].skipOnMultiple = 8;    

  speeds[34].moveOnMultiple = 1;
  speeds[34].moveForwardBy = 4;
  speeds[34].skipOnMultiple = 9;

  speeds[35].moveOnMultiple = 1;
  speeds[35].moveForwardBy = 4;
  speeds[35].skipOnMultiple = 10;

  speeds[36].moveOnMultiple = 1;
  speeds[36].moveForwardBy = 5;
  speeds[36].skipOnMultiple = 5;

  speeds[37].moveOnMultiple = 1;
  speeds[37].moveForwardBy = 5;
  speeds[37].skipOnMultiple = 3;
}
