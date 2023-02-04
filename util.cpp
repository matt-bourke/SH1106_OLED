#ifndef UTIL_C
#define UTIL_C
#include <sine_lut.cpp>

static void clamp(uint8_t &value, uint8_t lowerBound, uint8_t upperBound) {
    if (value < lowerBound) {
        value = lowerBound;
    } else if (value > upperBound) {
        value = upperBound;
    }
}


static int sign(int value) {
    return ((value > 0) - (value < 0));
}

template <typename T>
static void swap(T &a, T &b) {
    b = a + b;
    a = b - a;
    b = b - a;
}

static float getSineAngle(int angle) {
    if (angle >= 0 && angle <= 90) { 
        return pgm_read_float(&sine_lut[angle]);
    }

    if (angle > 90 && angle < 180) {
        return pgm_read_float(&sine_lut[180 - angle]);
    }

    if (angle >= 180 && angle <= 270) {
        return -pgm_read_float(&sine_lut[angle - 180]);
    }

    if (angle > 270 && angle <= 360) {
        return -pgm_read_float(&sine_lut[360 - angle]);
    }
}


static float getCosineAngle(int angle) {
    if (angle >= 0 && angle <= 90) { 
        return pgm_read_float(&sine_lut[90 - angle]);
    }

    if (angle > 90 && angle < 180) {
        return -pgm_read_float(&sine_lut[angle - 90]);
    }

    if (angle >= 180 && angle <= 270) {
        return -pgm_read_float(&sine_lut[270 - angle]);
    }

    if (angle > 270 && angle <= 360) {
        return pgm_read_float(&sine_lut[angle - 270]);
    }
}

static uint8_t getClampedRadius(uint8_t width, uint8_t height, uint8_t radius) {
    uint8_t maxRadius = min(width, height) / 2;
    return min(radius, maxRadius);
}

#endif