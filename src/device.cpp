#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

uint32_t lastTimeDevice = 0;

void setupDevice() {

}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 1000) {
    lastTimeDevice = now;
    
  }
}
