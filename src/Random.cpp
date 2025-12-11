#include "Random.h"
#include <stdlib.h>
#include <time.h>

void Random::init() {
	srand(time(NULL));
}

float Random::randFloat(){
	return (float)((double)rand() / (double)RAND_MAX);
}

float Random::randFloat(float range) {
	return (randFloat() * 2.0f * range) - range;
}

int Random::randint(){
	return rand();
}