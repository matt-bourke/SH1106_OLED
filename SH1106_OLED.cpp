#include "SH1106_OLED.h"

/*!
    @brief  Instantiates a SH1106_OLED screen object.
    @param  width   Width of display in pixels
    @param  height  Height of display in pixels
    @param  address I2C address of SH1106 device
*/
SH1106_OLED::SH1106_OLED(uint8_t width, uint8_t height, uint8_t address) : width(width), height(height), address(address), bufferSize(width * height / 8) {}


/*!
    @brief  Initialises the SH1106 OLED screen display.
            Commands set according to datasheet - https://www.pololu.com/file/0J1813/SH1106.pdf
*/
bool SH1106_OLED::init() {
    buffer = (uint8_t *)malloc(bufferSize);
    memset(buffer, 0x00, bufferSize);

    Wire.begin();
    Wire.setClock(400000);
    delay(100);

    sendCommand(0xAE); // Turn display off
    sendDualCommand(0xD5, 0x80); // Set display clock divide ratio
    sendDualCommand(0xA8, 0x3F); // Set multiplex ratio
    sendDualCommand(0xD3, 0x00); // Set display offset
    sendCommand(0x40); // Set display start line
    sendDualCommand(0xAD, 0x8B); // Set charge pump
    sendCommand(0xA1); // Set segment re-map
    sendCommand(0xC8); // Set COM output scan direction
    sendDualCommand(0xDA, 0x12); // Set COM pins hardware config
    sendDualCommand(0x81, 0xFF); // Set contrast
    sendDualCommand(0xD9, 0x1F); // Set pre-charge period
    sendDualCommand(0xDB, 0x40); // Set VCOMH deselect level
    sendCommand(0x33); // Set VPP
    sendCommand(0xA6); // Set normal/inverse display
    sendCommand(0xA4); // Set all display on

    delay(100);
    display();
    sendCommand(0xAF); // Set all display on

    setFontSize(4);

    return true;
}


/*!
    @brief  Sends the display buffer to the SH1106 OLED screen module.
*/
void SH1106_OLED::display() {
    for(int i = 0; i < 8; i++) {
        uint8_t cmd[] = {
            0x00,
            (uint8_t)(0xB0 + i),
            (uint8_t)(0x10),
            (uint8_t)(0x02)
        };

        Wire.beginTransmission(address);
        Wire.write(cmd, 4); 
        Wire.endTransmission();

        Wire.beginTransmission(address);
        Wire.write(0x40);
        uint8_t bytesWritten = 1;
        for(int j = 0; j < 128; j++) {
            Wire.write(buffer[j + (i * 128)]);
            bytesWritten++;
            if (bytesWritten == WIRE_MAX) {
                Wire.endTransmission(false);
                Wire.beginTransmission(address);
                Wire.write(0x40);
                bytesWritten = 1;
            }
        }
        Wire.endTransmission(true);
    }
}


/*!
    @brief  Returns the value of the pixel at specified x, y position.
    @param  x   x coordinate of pixel
    @param  y   y coordinate of pixel
    @returns Boolean true if on, and false if off
*/
bool SH1106_OLED::getPixel(uint8_t x, uint8_t y) {
    uint16_t bufferIndex = x + ((y / 8) * width);
    return (buffer[bufferIndex] >> y) & 0x01;
}


/*!
    @brief  Sets the pixel at the specified x, y position to be on.
    @param  x   x coordinate of pixel
    @param  y   y coordinate of pixel
*/
void SH1106_OLED::setPixel(uint8_t x, uint8_t y) {
    uint16_t bufferIndex = x + ((y / 8) * width);
    buffer[bufferIndex] |= (0x01 << (y & 0x07));
}


/*!
    @brief  Unsets the pixel at the specified x, y position.
    @param  x   x coordinate of pixel
    @param  y   y coordinate of pixel
*/
void SH1106_OLED::clearPixel(uint8_t x, uint8_t y) {
    uint16_t bufferIndex = x + ((y / 8) * width);
    buffer[bufferIndex] &= ~(0x01 << (y & 0x07));
}


/*!
    @brief  Inverts the value of the pixel at specified x, y position.
    @param  x   x coordinate of pixel
    @param  y   y coordinate of pixel
*/
void SH1106_OLED::invertPixel(uint8_t x, uint8_t y) {
    uint16_t bufferIndex = x + ((y / 8) * width);
    buffer[bufferIndex] ^= (0x01 << (y & 0x07));
}


/*!
    @brief  Clears the screen buffer by setting all values to 0.
*/
void SH1106_OLED::clear() {
    memset(buffer, 0x00, bufferSize);
}


/*!
    @brief  Inverts all values of the screen buffer.
*/
void SH1106_OLED::invert() {
    for(int i = 0; i < bufferSize; i++) {
        buffer[i] = ~buffer[i];
    }
}


/*!
    @brief  Sets font pointer based on width size. Defaults to size 4 if specified size not supported.
    @param  size    Desired font size
*/
void SH1106_OLED::setFontSize(uint8_t size) {
    fontSize = size;
    if (size == 5) {
        fontSet = *alphabet_5x5_monospace;
    } else {
        fontSet = *alphabet_5x4_monospace;
    }
}


/*!
    @brief  Write a message string starting at specified x, y position. Uses current font.
    @param  msg     Message to write to screen buffer
    @param  x       x coordinate corresponding to top left position of text start
    @param  y       y coordinate corresponding to top left position of text start
*/
void SH1106_OLED::print(String msg, uint8_t x, uint8_t y) {
    int fontWidthInc = (fontSize + 1);
    int messageLength = msg.length() * fontWidthInc - 1;
    uint16_t *message = (uint16_t *)malloc(messageLength * 2);
    memset(message, 0, messageLength * 2);
    msg.toUpperCase();

    for (int i = 0; i < msg.length(); i++) {
        char s = msg[i];
        uint8_t letterIndex = (uint8_t)s - (uint8_t)(' ');
        for(int j = 0; j < fontSize; j++) {
            uint8_t b = pgm_read_byte(&fontSet[letterIndex*fontSize] + j);
            message[(i*fontWidthInc) + j] = (uint16_t)b;
        }
    }

    int bufferIndex = x + ((y / 8) * width);
    for (int i = 0; i < messageLength; i++) {
        buffer[bufferIndex + i] |= (*(message + i) << (y & 0x07)) & 0xFF;
        if ((y / 8) < (y + fontSize) / 8) {
            buffer[bufferIndex + i + width] = ((*(message + i) << (y & 0x07)) >> 8) & 0xFF;
        }
    }

    delete(message);
}


/*!
    @brief  Draws a bitmap of specified width and height to the screen buffer.
    @param  bitmap          Pointer to uint8_t specifying beginning address of bitmap array
    @param  x               x coordinate corresponding to top left position of bitmap start
    @param  y               y coordinate corresponding to top left position of bitmap start
    @param  bitmapWidth     Width of bitmap
    @param  bitmapHeight    Height of bitmap
*/
void SH1106_OLED::drawBitmap(uint8_t *bitmap, uint8_t x, uint8_t y, uint8_t bitMapWidth, uint8_t bitMapHeight) {
    if (bitMapWidth * bitMapHeight == 0) {
        return;
    }

    uint8_t verticalOffset = y & 0x07;
    uint8_t pageCount = (bitMapHeight / 8) + (bitMapHeight % 8 != 0);
    uint16_t bufferIndex = x + (y / 8) * width;

    for (uint8_t j = 0; j < pageCount; j++) {
        for (uint8_t i = 0; i < bitMapWidth; i++) {
            uint8_t byteToWrite = pgm_read_byte(bitmap + i + (j * bitMapWidth));
            buffer[bufferIndex + i + (j * width)] |= byteToWrite << verticalOffset;
            if (verticalOffset) {
                buffer[bufferIndex + i + ((j + 1) * width)] |= byteToWrite >> (8 - verticalOffset);
            }
        }
    }
}


/*!
    @brief  Draws horizontal line from x1 to x2 at vertical position y.
    @param  x1  Starting x coordinate of line
    @param  x2  Ending x coordinate of line
    @param  y   Vertical position of line
*/
void SH1106_OLED::drawHLine(uint8_t x1, uint8_t x2, uint8_t y) {
    clamp(x1, 0, width - 1);
    clamp(x2, 0, width - 1);
    clamp(y, 0, height - 1);

    uint8_t xMin = x1;
    uint8_t distance = x2 - x1;
    if (x2 < x1) {
        xMin = x2;
        distance = x1 - x2;
    }

    int minBufferIndex = (y / 8) * width + xMin;

    if (distance == 0) {
        buffer[minBufferIndex] |= 0x01 << (y & 0x07);
        return;
    }

    for (int i = 0; i <= distance; i++) {
        buffer[minBufferIndex + i] |= 0x01 << (y & 0x07);
    }
}


/*!
    @brief  Draws vertical line from y1 to y2 at horizontal position x.
    @param  y1  Starting y coordinate of line
    @param  y2  Ending y coordinate of line
    @param  x   Horizontal position of line
*/
void SH1106_OLED::drawVLine(uint8_t y1, uint8_t y2, uint8_t x) {
    clamp(y1, 0, height - 1);
    clamp(y2, 0, height - 1);
    clamp(x, 0, width - 1);

    uint8_t yMin = y1;
    uint8_t distance = y2 - y1;
    if (y2 < y1) {
        yMin = y2;
        distance = y1 - y2;
    }

    uint8_t yMax = yMin + distance;
    uint8_t byteDistance = (yMax / 8) - (yMin / 8);
    int minBufferIndex = (yMin / 8) * width + x;

    if (distance == 0) {
        buffer[minBufferIndex] |= 0x01 << (y1 & 0x07);
        return;
    }

    uint8_t firstByte = 0xFF;
    if (byteDistance == 0 && distance < 8) {
        firstByte = 0x01;
        for (int i = 0; i < distance; i++) {
            firstByte <<= 1;
            firstByte ^= 0x01;
        }
    }

    buffer[minBufferIndex] |= firstByte << (yMin & 0x07);
    
    for (int i = 1; i < byteDistance; i++) {
        buffer[minBufferIndex + (i * width)] = 0xFF;
    }

    if (byteDistance > 0) {
        int maxBufferIndex = (yMax / 8) * width + x;
        buffer[maxBufferIndex] |= 0xFF >> (7 - (yMax & 0x07));
    }
}


/*!
    @brief  Draws a line from position x1, y1 to position x2, y2.
    @param  x1  x coordinate of line starting point
    @param  y1  y coordinate of line starting point
    @param  x2  x coordinate of line ending point
    @param  y2  y coordinate of line ending point
*/
void SH1106_OLED::drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    if (y1 == y2) {
        drawHLine(x1, x2, y1);
        return;
    }

    if (x1 == x2) {
        drawVLine(y1, y2, x1);
        return;
    }

    int8_t xDistance = x2 - x1;
    int8_t yDistance = y2 - y1;
    float gradient = yDistance / (float)xDistance;

    if (gradient <= 1 && gradient >= -1) {
        for (int8_t i = 0; i <= abs(xDistance); i++) {
            int8_t xIncrement = i * sign(xDistance);
            uint8_t yValue = (gradient * xIncrement) + y1 + 0.5;
            setPixel(xIncrement + x1, yValue);
        }
    } else {
        for (int8_t i = 0; i <= abs(yDistance); i++) {
            int8_t yIncrement = i * sign(yDistance);
            uint8_t xValue = (yIncrement / gradient) + x1 + 0.5;
            setPixel(xValue, yIncrement + y1);
        }
    }
}


/*!
    @brief  Draws a rectange at position x, y with specified width and height.
    @param  x   Horizontal position of top left corner of rectangle
    @param  y   Vertical position of top left corner of rectangle
    @param  w   Width of rectangle in pixels
    @param  h   Height of rectangle in pixels
*/
void SH1106_OLED::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    drawHLine(x, x + w, y);
    drawHLine(x, x + w, y + h);
    drawVLine(y, y + h, x);
    drawVLine(y, y + h, x + w);
}


/*!
    @brief  Draws a filled rectange at position x, y with specified width and height.
    @param  x   Horizontal position of top left corner of rectangle
    @param  y   Vertical position of top left corner of rectangle
    @param  w   Width of rectangle in pixels
    @param  h   Height of rectangle in pixels
*/
void SH1106_OLED::drawRectFill(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    for (int i = 0; i <= h; i++) {
        drawHLine(x, x + w, y + i);
    }
}


/*!
    @brief  Draws a rectange with rounded corners at position x, y with specified width and height and corner radius.
    @param  x   Horizontal position of top left corner of rectangle
    @param  y   Vertical position of top left corner of rectangle
    @param  w   Width of rectangle in pixels
    @param  h   Height of rectangle in pixels
    @param  r   Radius of rounded corners
*/
void SH1106_OLED::drawRoundedRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r) {
    r = getClampedRadius(w, h, r);

    drawHLine(x + r, x + w - r, y);
    drawHLine(x + r, x + w - r, y + h);
    drawVLine(y + r, y + h - r, x);
    drawVLine(y + r, y + h - r, x + w);

    drawArc(x + r, y + r, r, TOP_LEFT);
    drawArc(x + w - r, y + r, r, TOP_RIGHT);
    drawArc(x + r, y + h - r, r, BOTTOM_LEFT);
    drawArc(x + w - r, y + h - r, r, BOTTOM_RIGHT);
}


/*!
    @brief  Draws a filled rectange with rounded corners at position x, y with specified width and height and corner radius.
    @param  x   Horizontal position of top left corner of rectangle
    @param  y   Vertical position of top left corner of rectangle
    @param  w   Width of rectangle in pixels
    @param  h   Height of rectangle in pixels
    @param  r   Radius of rounded corners
*/
void SH1106_OLED::drawRoundedRectFill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r) {
    r = getClampedRadius(w, h, r);

    for (int i = 0; i <= r; i++) {
        drawHLine(x + r, x + w - r, y + i);
        drawHLine(x + r, x + w - r, y + h - i);
    }

    for (int i = 0; i <= (h - 2 * r); i++) {
        drawHLine(x, x + w, y + r + i);
    }

    drawArcFill(x + r, y + r, r, TOP_LEFT);
    drawArcFill(x + w - r, y + r, r, TOP_RIGHT);
    drawArcFill(x + r, y + h - r, r, BOTTOM_LEFT);
    drawArcFill(x + w - r, y + h - r, r, BOTTOM_RIGHT);
}


/*!
    @brief  Draws a circle with centre at xCentre, yCentre position, with specified radius.
            Shoutout to Adafruit for the sick circle algorithm
    @param  xCentre     Centre x coordinate of circle
    @param  yCentre     Centre y coordinate of circle
    @param  radius      Radius of circle
*/
void SH1106_OLED::drawCircle(uint8_t xCentre, uint8_t yCentre, uint8_t radius) {
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;

    setPixel(xCentre, yCentre + radius);
    setPixel(xCentre, yCentre - radius);
    setPixel(xCentre + radius, yCentre);
    setPixel(xCentre - radius, yCentre);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        setPixel(xCentre + x, yCentre + y);
        setPixel(xCentre - x, yCentre + y);
        setPixel(xCentre + x, yCentre - y);
        setPixel(xCentre - x, yCentre - y);
        setPixel(xCentre + y, yCentre + x);
        setPixel(xCentre - y, yCentre + x);
        setPixel(xCentre + y, yCentre - x);
        setPixel(xCentre - y, yCentre - x);
    }
}


/*!
    @brief  Draws a filled circle with centre at xCentre, yCentre position, with specified radius.
    @param  xCentre     Centre x coordinate of circle
    @param  yCentre     Centre y coordinate of circle
    @param  radius      Radius of circle
*/
void SH1106_OLED::drawCircleFill(uint8_t xCentre, uint8_t yCentre, uint8_t radius) {
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;

    drawHLine(xCentre - radius, xCentre + radius, yCentre);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        drawHLine(xCentre - x, xCentre + x, yCentre + y);
        drawHLine(xCentre - x, xCentre + x, yCentre - y);
        drawHLine(xCentre - y, xCentre + y, yCentre + x);
        drawHLine(xCentre - y, xCentre + y, yCentre - x);
    }
}


/*!
    @brief  Draws a corner arc with centre at xCentre, yCentre position, with specified radius.
    @param  xCentre     Centre x coordinate of arc
    @param  yCentre     Centre y coordinate of arc
    @param  radius      Radius of arc
    @param  corner      Corner corresponding to arc orientation
*/
void SH1106_OLED::drawArc(uint8_t xCentre, uint8_t yCentre, uint8_t radius, Corner corner) {
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;

    if (corner == BOTTOM_LEFT || corner == BOTTOM_RIGHT) setPixel(xCentre, yCentre + radius);
    if (corner == TOP_LEFT || corner == TOP_RIGHT) setPixel(xCentre, yCentre - radius);
    if (corner == TOP_RIGHT || corner == BOTTOM_RIGHT) setPixel(xCentre + radius, yCentre);
    if (corner == TOP_LEFT || corner == BOTTOM_LEFT) setPixel(xCentre - radius, yCentre);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        if (corner == TOP_LEFT) {
            setPixel(xCentre - y, yCentre - x);
            setPixel(xCentre - x, yCentre - y);
        }

        if (corner == TOP_RIGHT) {
            setPixel(xCentre + x, yCentre - y);
            setPixel(xCentre + y, yCentre - x);
        }

        if (corner == BOTTOM_RIGHT) {
            setPixel(xCentre + x, yCentre + y);
            setPixel(xCentre + y, yCentre + x);
        }

        if (corner == BOTTOM_LEFT) {
            setPixel(xCentre - y, yCentre + x);
            setPixel(xCentre - x, yCentre + y);
        }
    }
}


/*!
    @brief  Draws a filled corner arc with centre at xCentre, yCentre position, with specified radius.
    @param  xCentre     Centre x coordinate of arc
    @param  yCentre     Centre y coordinate of arc
    @param  radius      Radius of arc
    @param  corner      Corner corresponding to arc orientation
*/
void SH1106_OLED::drawArcFill(uint8_t xCentre, uint8_t yCentre, uint8_t radius, Corner corner) {
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;

    if (corner == BOTTOM_LEFT || corner == BOTTOM_RIGHT) drawVLine(yCentre, yCentre + radius, xCentre);
    if (corner == TOP_LEFT || corner == TOP_RIGHT) drawVLine(yCentre - radius, yCentre, xCentre);
    if (corner == TOP_RIGHT || corner == BOTTOM_RIGHT) drawHLine(xCentre, xCentre + radius, yCentre);
    if (corner == TOP_LEFT || corner == BOTTOM_LEFT) drawHLine(xCentre, xCentre - radius, yCentre);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        if (corner == TOP_LEFT) {
            drawHLine(xCentre, xCentre - x, yCentre - y);
            drawHLine(xCentre, xCentre - y, yCentre - x);
        }

        if (corner == TOP_RIGHT) {
            drawHLine(xCentre, xCentre + x, yCentre - y);
            drawHLine(xCentre, xCentre + y, yCentre - x);
        }

        if (corner == BOTTOM_RIGHT) {
            drawHLine(xCentre, xCentre + x, yCentre + y);
            drawHLine(xCentre, xCentre + y, yCentre + x);
        }

        if (corner == BOTTOM_LEFT) {
            drawHLine(xCentre, xCentre - y, yCentre + x);
            drawHLine(xCentre, xCentre - x, yCentre + y);
        }
    }
}


/*!
    @brief  Draws an arc with centre at xCentre, yCentre position, with specified radius between start and end angles. *Unoptimised*
    @param  xCentre     Centre x coordinate of arc
    @param  yCentre     Centre y coordinate of arc
    @param  radius      Radius of arc
    @param  startAngle  Angle corresponding to start of arc
    @param  endAngle    Angle corresponding to end of arc
*/
void SH1106_OLED::drawArcRaw(uint8_t xCentre, uint8_t yCentre, uint8_t radius, uint16_t startAngle, uint16_t endAngle) {
    uint8_t prevX = xCentre;
    uint8_t prevY = yCentre;
    float angleIncrement = 180 / (radius * PI);

    float angle = startAngle + angleIncrement;
    while (angle < endAngle) { // lets assume for now startAngle < endAngle
        uint8_t x = radius * getCosineAngle(angle);
        uint8_t y = radius * getSineAngle(angle);

        if (x != prevX || y != prevY) {
            setPixel(x + xCentre, y + yCentre);
        }

        prevX = x;
        prevY = y;
        angle += angleIncrement;
    }
}


/*!
    @brief  Draws triangle specified by x, y positions of corners
    @param  x1  x coordinate of first corner
    @param  y1  y coordinate of first corner
    @param  x2  x coordinate of second corner
    @param  x2  x coordinate of second corner
    @param  y3  y coordinate of third corner
    @param  y3  y coordinate of third corner
*/
void SH1106_OLED::drawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3) {
    drawLine(x1, y1, x2, y2);
    drawLine(x2, y2, x3, y3);
    drawLine(x3, y3, x1, y1);
}


/*!
    @brief  Draws filled triangle specified by x, y positions of corners
    @param  x1  x coordinate of first corner
    @param  y1  y coordinate of first corner
    @param  x2  x coordinate of second corner
    @param  x2  x coordinate of second corner
    @param  y3  y coordinate of third corner
    @param  y3  y coordinate of third corner
*/
void SH1106_OLED::drawTriangleFill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3) {
    if (y2 < y1) {
        swap(y1, y2);
        swap(x1, x2);
    }

    if (y3 < y2) {
        swap(y2, y3);
        swap(x2, x3);
    }

    if (y2 < y1) {
        swap(y1, y2);
        swap(x1, x2);
    }

    uint8_t xMin;
    uint8_t xMax;
    if (y1 == y3) {
        xMin = x1;
        xMax = x3;
        if (x2 < xMin) {
            xMin = x2;
        }

        if (x3 < xMin) {
            xMin = x3;
        }

        if (x1 > xMax) {
            xMax = x1;
        }

        if (x2 > xMax) {
            xMax = x2;
        }

        drawHLine(xMin, xMax, y1);
        return;
    }

    int8_t xDistance12 = x2 - x1, xDistance13 = x3 - x1, xDistance23 = x3 - x2; 
    int8_t yDistance12 = y2 - y1, yDistance13 = y3 - y1, yDistance23 = y3 - y2;
    int32_t runAmount12 = 0;
    int32_t runAmount13 = 0;
    uint8_t yStop = y2 - 1;
    if (y2 == y3) {
        yStop++;
    }

    for (uint8_t y = y1; y <= yStop; y++) {
        xMin = x1 + runAmount12 / yDistance12;
        xMax = x1 + runAmount13 / yDistance13;
        runAmount12 += xDistance12;
        runAmount13 += xDistance13;
        drawHLine(xMin, xMax, y);
    }

    runAmount13 = (int32_t)xDistance13 * (yStop - y1 + 1);
    int32_t runAmount23 = (int32_t)xDistance23 * (yStop - y2 + 1);
    for (uint8_t y = yStop + 1; y < y3; y++) {
        xMin = x2 + runAmount23 / yDistance23;
        xMax = x1 + runAmount13 / yDistance13;
        runAmount13 += xDistance13;
        runAmount23 += xDistance23;
        drawHLine(xMin, xMax, y);
    }
}


/*!
    @brief  Draws battery icon with variable charge level to top right corner of screen.
    @param  percentage  Percentage charge of battery (0-100)
*/
void SH1106_OLED::displayBattery(uint8_t percentage) {
    uint8_t batteryWidth = 12;
    uint8_t batteryBitmap[batteryWidth];

    for (int i = 0; i < batteryWidth; i++) {
        batteryBitmap[i] = pgm_read_byte(batteryCase + i);
    }

    if (percentage > 5) {
        batteryBitmap[2] |= pgm_read_byte(batteryLowCell);
        batteryBitmap[3] |= pgm_read_byte(batteryLowCell + 1);
    }

    if (percentage > 35) {
        batteryBitmap[5] |= batteryMidCell;
    }

    if (percentage > 70) {
        batteryBitmap[7] |= pgm_read_byte(batteryHighCell);
        batteryBitmap[8] |= pgm_read_byte(batteryHighCell + 1);
    }

    int bufferIndex = width - batteryWidth;
    for (int i = 0; i < batteryWidth; i++) {
        buffer[bufferIndex + i] = batteryBitmap[i];
    }
}


/*!
    @brief  Sends single command to SH1106 OLED screen.
    @param  command     Byte value for command according to SH1106 datasheet
*/
void SH1106_OLED::sendCommand(uint8_t command) {
    Wire.beginTransmission(address);
    uint8_t buf[2] = { 0x00, command };
    Wire.write(buf, 2);
    Wire.endTransmission(true);
}


/*!
    @brief  Sends a dual command/data packet to SH1106 OLED screen.
    @param  command     Byte value for command according to SH1106 datasheet
    @param  data        Data value for specified command
*/
void SH1106_OLED::sendDualCommand(uint8_t command, uint8_t data) {
    Wire.beginTransmission(address);
    uint8_t cmdBuf[2] = { 0x02, command };
    uint8_t dataBuf[2] = { 0x00, data };
    Wire.write(cmdBuf, 2);
    Wire.write(dataBuf, 2);
    Wire.endTransmission(true);
}
