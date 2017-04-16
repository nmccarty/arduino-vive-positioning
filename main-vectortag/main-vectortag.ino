#include <elapsedMillis.h>

#include <CurieTime.h>

// Used in determining noise floor
float stdTolerance = 2;

bool spiked[6];
int noiseMeans[6];
int noiseStds[6];
long lastSpike[6];

struct pulse {
  long start;
  long end;
  int maxHeight;
};

#define PULSE_HISTORY 3
#define MIN_PULSE_WIDTH 30
pulse pulseStack[6][PULSE_HISTORY];
int currentPulse[6];

void setup() {
  digitalWrite(13, 1);
  Serial.begin(115200);
  while(!Serial);
  digitalWrite(13, 0);

  Serial.println("You have 5 seconds to get your hands out of the way before calibration, you fuck.");
  delay(5000);

  analogReadResolution(12);

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
  }

  delay(2000);

  
}

void loop() {
  while(true){
   int activePins = 1;
   for(int i = 0; i < activePins; i++){
    long ttime = micros();
    int pin = i;
    int spike = detectSpike(0,noiseMeans[pin],noiseStds[pin]);
    int cpulse = currentPulse[pin];
    pulse ppulse = pulseStack[pin][cpulse];
    if(!spiked[pin] && (spike > 0)) {
      currentPulse[pin] = (currentPulse[pin] + 1) % PULSE_HISTORY;
      pulseStack[pin][cpulse] = {ttime,0,spike};
      spiked[pin] =  true;
    } else if(spiked[pin] && !(spike > 0)){
      int spikeValue = spike;
      if(spikeValue < ppulse.maxHeight){
        spikeValue = ppulse.maxHeight;
      }
      pulseStack[pin][cpulse] = {ppulse.start, ttime, spikeValue};
      if((pulseStack[pin][cpulse].end - pulseStack[pin][cpulse].start) > MIN_PULSE_WIDTH){
        spiked[pin]=false;
        printPulse(pulseStack[pin][cpulse]);
      } else {
        pulseStack[pin][cpulse].start=ttime;
      }
       
    } else if (spiked[pin]){
      if(spike > ppulse.maxHeight){
        pulseStack[pin][cpulse].maxHeight = spike;
      }
    }
    }
    
   }
}
  
}

int* measureNoise(int pin,int size){
    int* samples = (int*) malloc(size*sizeof(int));
    for(int i=0; i<size; i++){
      samples[i] = analogRead(pin);
      delayMicroseconds(20);
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

bool validPulseGroup(pin){
  int cpulse = currentPulse[pin];
  pulse p1 = pulseStack[pin][(cpulse - 2) % PULSE_HISTORY];
  pulse p2 = pulseStack[pin][(cpulse - 1) % PULSE_HISTORY];
  pulse p3 = pulseStack[pin][(cpulse - 0) % PULSE_HISTORY];

  long p1time = p1.end - p1.start;
  long p2time = p2.end - p2.start;
  long p3time = p3.end - p3.start;

  return (p1time > p2time) && (p3time > p2time);
}

