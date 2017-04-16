#include <BMI160.h>
#include <CurieIMU.h>
#include <MadgwickAHRS.h>

#include <elapsedMillis.h>

#include <CurieTime.h>

// Used in determining noise floor
float stdTolerance = 1;

bool spiked[6];
int noiseMeans[6];
int noiseStds[6];
long lastSpike[6];

long lastPulseTime[6];

bool triggered = false;

int buttonPin=7;

long microsPrevious;

struct pulse {
  long start;
  long end;
  int maxHeight;
};

#define PULSE_HISTORY 6
#define MIN_PULSE_WIDTH 20
pulse pulseStack[6][PULSE_HISTORY];
int currentPulse[6];


Madgwick filter;

float prevPulse1[2];
float prevPulse2[2];

long microsPerReading = 1000000 / 25;


void setup() {
  digitalWrite(13, 1);
  Serial.begin(115200);
  while(!Serial);
  digitalWrite(13, 0);

  Serial.println("You have 5 seconds to get your hands out of the way before calibration, you fuck.");
  delay(5000);

  analogReadResolution(10);

  int numNoiseSamples= 100;

  Serial.println("Measuring noise floors");
  int* noiseSamples[6];
  for(int i=0; i < 6; i++){
    noiseSamples[i] = measureNoise(i, numNoiseSamples);
    noiseMeans[i] = mean(noiseSamples[i], numNoiseSamples);
    noiseStds[i] = stdDev(noiseSamples[i], numNoiseSamples);
    Serial.print("Noise floor for pin ");
    Serial.print(i);
    Serial.print(" : ");
    Serial.print(noiseMeans[i]);
    Serial.print(" : ");
    Serial.println(noiseStds[i]);
    free(noiseSamples[i]);
  }
  Serial.flush();
  
  for(int i=0; i<6; i++){
    spiked[i] = false;
    currentPulse[i] = 0;
    lastPulseTime[i] = 0;
  }

  pinMode(buttonPin,INPUT);

  CurieIMU.begin();
  CurieIMU.setGyroRate(25);
  CurieIMU.setAccelerometerRate(25);
  filter.begin(25);

  // Set the accelerometer range to 2G
  CurieIMU.setAccelerometerRange(2);
  // Set the gyroscope range to 250 degrees/second
  CurieIMU.setGyroRange(1000);

  delay(2000);

  microsPrevious = micros();

  
}

void loop() {
  while(true){


  int aix, aiy, aiz;
  int gix, giy, giz;
  float ax, ay, az;
  float gx, gy, gz;
  float roll, pitch, heading;
  unsigned long microsNow;


 
 /*if (microsNow - microsPrevious >= microsPerReading) {

    // read raw data from CurieIMU
    CurieIMU.readMotionSensor(aix, aiy, aiz, gix, giy, giz);

    // convert from raw data to gravity and degrees/second units
    ax = convertRawAcceleration(aix);
    ay = convertRawAcceleration(aiy);
    az = convertRawAcceleration(aiz);
    gx = convertRawGyro(gix);
    gy = convertRawGyro(giy);
    gz = convertRawGyro(giz);

    // update the filter, which computes orientation
    filter.updateIMU(gx, gy, gz, ax, ay, az);

    // print the heading, pitch and roll
    roll = filter.getRoll();
    pitch = filter.getPitch();
    heading = filter.getYaw();
    Serial.print("Orientation: ");
    Serial.print(heading);
    Serial.print(" ");
    Serial.print(pitch);
    Serial.print(" ");
    Serial.println(roll);

    // increment previous time, so we keep proper pace
    microsPrevious = microsPrevious + microsPerReading;
  } */








   bool trigger = digitalRead(buttonPin) == HIGH;
   if(!triggered && trigger){
    Serial.println("trigger");
    triggered = true;
   } else if(triggered && !trigger) {
    triggered=false;
   }
    
   int activePins = 2;
   for(int i = 0; i < activePins; i++){
    long ttime = micros();
    int pin = i;
    int spike = detectSpike(0,noiseMeans[pin],noiseStds[pin]);
    int cpulse = currentPulse[pin];
    pulse ppulse = pulseStack[pin][cpulse];
    if(!spiked[pin] && (spike > 0)) {
      cpulse = (currentPulse[pin] + 1) % PULSE_HISTORY;
      currentPulse[pin] = cpulse;
      pulseStack[pin][cpulse] = {ttime,0,spike};
      spiked[pin] =  true;
    } else if(spiked[pin] && !(spike > 0)){
      int spikeValue = spike;
      if(spikeValue < ppulse.maxHeight){
        spikeValue = ppulse.maxHeight;
      }
      pulseStack[pin][cpulse] = {ppulse.start, ttime, spikeValue};
      if((pulseStack[pin][cpulse].end - pulseStack[pin][cpulse].start) >= MIN_PULSE_WIDTH){
        spiked[pin]=false;
      } else {
        pulseStack[pin][cpulse].start=ttime;
        pulseStack[pin][cpulse].end=ttime;
      }
       
    } else if (spiked[pin]){
      if(spike > ppulse.maxHeight){
        pulseStack[pin][cpulse].maxHeight = spike;
      }
    }
    }

    if(validPulseGroup(0)){
      pulse pulse1 = pulseStack[0][currentPulse[0]];
      pulse pulse2 = pulseStack[1][currentPulse[1]];

      if((pulse1.end > lastPulseTime[0])){

        //if(((pulse1.end - lastPulseTime[0] )< 80000)){
          float pgangle1 = pulseGroupAngle(0);
          float pgangle2 = pulseGroupAngle(1);

          prevPulse1[1] = prevPulse1[0];
          prevPulse1[0] = pgangle1;

          if(pgangle1 > 0){
            Serial.print("x ");
            Serial.println(pgangle1,20); 
          } else {
            Serial.print("y ");
            Serial.println(pgangle1,20);
          }

          /*const float 

          float l = 10; //inches 
          float phi = filter.getHeading() * 0.0174533;
          float th1 = (90-abs(pangle1)) * 0.0174533;
          float th2 =*/
          


        
        //}
        lastPulseTime[0] = pulse1.end;
        lastPulseTime[1] = pulse2.end;
      }
    }
    
   }
}
  


int* measureNoise(int pin,int size){
    int* samples = (int*) malloc(size*sizeof(int));
    for(int i=0; i<size; i++){
      samples[i] = analogRead(pin);
      delayMicroseconds(59);
    }

    return samples;
}

int mean(int* samples, int size){
  float sum = 0;
  for(int i=0; i < size; i++){
    sum += samples[i];
  }
  return (int)(sum/size);
}

int stdDev(int* samples, int size){
  float mean = 0;
  for(int i=0; i<size; i++){
    mean += samples[i];
  }
  mean = mean/size;
  float sum = 0;
  for(int i=0;i<size;i++){
    float tmp = samples[i] - mean;
    tmp = tmp * tmp;
    sum += tmp;
  }
  sum = sum / size;
  sum = sqrt(sum);
  Serial.println(sum);
  return (int) sum;
  
}

int detectSpike(int pin, int noise, int std){
  int value = analogRead(pin);
  if (value > (noise + std * stdTolerance)){
    return value;
  } else {
    return 0;
  }
}

void printPulse(pulse p){
  Serial.print(p.start);
  Serial.print(",");
  Serial.print(p.end - p.start);
  Serial.print(",");
  Serial.println(p.maxHeight);
}

bool validPulseGroup(int pin){
  int cpulse = currentPulse[pin];
  pulse p1 = pulseStack[pin][(cpulse - 2) % PULSE_HISTORY];
  pulse p2 = pulseStack[pin][(cpulse - 1) % PULSE_HISTORY];
  pulse p3 = pulseStack[pin][(cpulse - 0) % PULSE_HISTORY];

  long p1time = p1.end - p1.start;
  long p2time = p2.end - p2.start;
  long p3time = p3.end - p3.start;

  int tdiff = abs(((p1time - p3time) *100)/((p1time+p3time)/2));

  return (p1time > p2time) && (p3time > p2time) && (p3.end > p1.end) && (tdiff <= 10);
}

float pulseGroupAngle(int pin){
  int cpulse = currentPulse[pin];
  pulse p1 = pulseStack[pin][(cpulse - 2) % PULSE_HISTORY];
  pulse p2 = pulseStack[pin][(cpulse - 1) % PULSE_HISTORY];
  pulse p3 = pulseStack[pin][(cpulse - 0) % PULSE_HISTORY];

  long p2start = p2.start - p1.start;
  long p3start = p3.start - p1.start;

  float angle = ((float) p2start) / ((float) p3start);
  
  angle = angle - 0.5;
  angle = angle * 180;

  return angle;
  
}

float convertRawAcceleration(int aRaw) {
  // since we are using 2G range
  // -2g maps to a raw value of -32768
  // +2g maps to a raw value of 32767
  
  float a = (aRaw * 2.0) / 32768.0;
  return a;
}

float convertRawGyro(int gRaw) {
  // since we are using 250 degrees/seconds range
  // -250 maps to a raw value of -32768
  // +250 maps to a raw value of 32767
  
  float g = (gRaw * 1000.0) / 32768.0;
  return g;
}


