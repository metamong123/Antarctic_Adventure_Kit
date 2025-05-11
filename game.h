#ifndef GAME_H
#define GAME_H

#include "mbed.h"
#include "Adafruit_SSD1306.h"

extern const uint8_t seal_img[];
extern const uint8_t penguin_img[];
extern const uint8_t ground[];

class Hole {
    int16_t rect_x;
    int16_t rect_y;
    int16_t rect_w;
    int16_t rect_dx;
    int16_t dcount;
    uint16_t count;
    bool seal;
    
    const static int hole_arr[3][3];
    
public:
    bool onoff;
    
    Hole(int hole_idx);
    
    int get_x() { return rect_x; }
    int get_y() { return rect_y; }
    int get_w() { return rect_w; }
    bool get_seal() { return seal; }

    void set(int hole_idx);
    void off();
    void rect(Adafruit_SSD1306_I2c& oled);
};

class Penguin {
    InterruptIn btn;
    Timeout tmo;
    
    int16_t penguin_x;
    int16_t penguin_y;
    bool _jump;
    bool game_over;
    
    void jump();
    void down();
    
public:
    bool game_over_flag;
    bool game_start_flag;
    
    Penguin(PinName jump_pin);
    
    bool get_game_over() { return game_over; }

    void move_right();
    void move_left();
    void decision(Hole& hole1, Hole& hole2, Hole& hole3);
    void draw_penguin(Adafruit_SSD1306_I2c& oled);
};
#endif
