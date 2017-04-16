#include <elapsedMillis.h>

#include <CurieTime.h>

// Used in determining noise floor
float stdTolerance = 2;
int minPulseLength = 10;

bool spiked[6];
int noiseMeans[6];
int noiseStds[6];
long lastSpike[6];

void setup() {
  digitalWrite(13, 1);
  Serial.begin(2000000);
  while(!Serial);
  digitalWrite(13, 0);

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
  }

  delay(2000);

  
}

void loop() {
  while(true){
    bool spike = detectSpike(0,noiseMeans[0],noiseStds[0]);
    if(!spiked[0] && spike){
      lastSpike[0]=micros();
      spiked[0] = true;
    }
    if(spiked[0] && !spike){
      long s = micros();
      spiked[0] = false;
      Serial.println(lastSpike[0]);
      Serial.println(s - lastSpike[0]);
      Serial.println();
      Serial.flush();
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

bool detectSpike(int pin, int noise, int std){
  int value = analogRead(pin);
  return value > (noise + std * stdTolerance);
}


