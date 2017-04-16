#include <elapsedMillis.h>

#include <CurieTime.h>

// Used in determining noise floor
float stdTolerance = 1.5;

void setup() {
  digitalWrite(13, 1);
  Serial.begin(9600);
  while(!Serial);
  digitalWrite(13, 0);

  analogReadResolution(12);

  int numNoiseSamples= 200;

  Serial.println("Measuring noise floors");
  int* noiseSamples[6];
  int noiseMeans[6];
  int noiseStds[6];
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
  }

  
}

void loop() {

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
  float m = mean(samples,size);
  float sum = 0;
  for(int i=0;i<size;i++){
    float tmp = samples[i] - m;
    tmp = tmp * tmp;
  }
  sum = sum / size;
  return (int) sqrt(sum);
  
}

bool detectSpike(int pin, int noise, int std){
  int value = analogRead(pin);
  return value > (noise + std * stdTolerance);
}

