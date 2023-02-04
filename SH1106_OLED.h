#ifndef SH1106_OLED_h
#define SH1106_OLED_h

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <Wire.h>
#include <GFX.cpp>
#include <util.cpp>

#define WIRE_MAX 32

enum Corner {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_RIGHT,
    BOTTOM_LEFT
};


class SH1106_OLED {
    public:
        SH1106_OLED(uint8_t width, uint8_t height, uint8_t address);

        bool init();
        void display();
        bool getPixel(uint8_t x, uint8_t y);
        void setPixel(uint8_t x, uint8_t y);
        void clearPixel(uint8_t x, uint8_t y);
        void invertPixel(uint8_t x, uint8_t y);
        void clear();
        void invert();
        void setFontSize(uint8_t size); // gotta think about if I want font size to be a thing
        void print(String msg, uint8_t x, uint8_t y);
        void drawBitmap(uint8_t *bitmap, uint8_t x, uint8_t y, uint8_t bitMapWidth, uint8_t bitMapHeight);
        void drawHLine(uint8_t x1, uint8_t x2, uint8_t y);
        void drawVLine(uint8_t y1, uint8_t y2, uint8_t x);
        void drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
        void drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
        void drawRectFill(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
        void drawRoundedRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r);
        void drawRoundedRectFill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r);
        void drawCircle(uint8_t xCentre, uint8_t yCentre, uint8_t radius);
        void drawCircleFill(uint8_t xCentre, uint8_t yCentre, uint8_t radius);
        void drawArc(uint8_t xCentre, uint8_t yCentre, uint8_t radius, Corner corner);
        void drawArcFill(uint8_t xCentre, uint8_t yCentre, uint8_t radius, Corner corner);
        void drawArcRaw(uint8_t xCentre, uint8_t yCentre, uint8_t radius, uint16_t startAngle, uint16_t endAngle);
        void drawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3);
        void drawTriangleFill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3);
        void displayBattery(uint8_t percentage);

    private:
        void sendCommand(uint8_t command);
        void sendDualCommand(uint8_t command, uint8_t data);

        uint8_t width;
        uint8_t height;
        uint8_t address;
        uint8_t *buffer;
        uint16_t bufferSize;

        uint8_t fontSize;
        const uint8_t *fontSet;
};

#endif