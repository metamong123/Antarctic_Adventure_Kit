#include "mbed.h"
#include "Adafruit_SSD1306.h"

enum State { // 상태 enum
    SHRINK,
    UP,
    DOWN,
    STRETCH
};

enum State state; // 열거형 변수 state 선언


//객체 선언
Ticker led_blink;            //Blink Ticker
InterruptIn btn(D2, PullUp); //Switch Interrupt
Timeout _tmo;                //Debouncing Timer

DigitalOut led(D7, 0);       //LED
PwmOut rcServo(D6);          //Servo
PwmOut buzzer(D9);           //Buzzer
I2C i2c(I2C_SDA, I2C_SCL);   // SDA, SCL
Adafruit_SSD1306_I2c myOled(i2c, D4, 0x78, 32, 128); //OLED


//전역변수 선언
volatile float inc = 1; //서보모터 각도 증분
volatile bool _updated = true, inter = false;
//_updated: state 업데이트 확인, state가 바뀔 때마다 true가 됨
//inter: UP, DOWN 중에 스위치가 눌리면 true가 됨
volatile int _ready = 1; //디바운싱


//콜백 함수 정의
void blink(){ //Blink
    led =! led;
    if (led) buzzer.write(0.5); //Buzzer On
    else buzzer.write(0);       //Buzzer Off
}

void decide() { //Debouncing
    _ready = 1;
    _tmo.detach();
}

void sw_callback() {
    if(_ready) {
        _ready = 0;
        _tmo.attach_us(callback(&decide), 200000); //Debouncing
        _updated = true;
        switch(state) {
        case SHRINK:  // SHRINK -> UP(slow)
            inc = 1;
            state = UP;
            break;
        case UP:      // UP -> DOWN(fast)
            inter = true;
            state = DOWN;
            break;
        case DOWN:    // DOWN -> DOWN(fast)
            inter = true;
            state = DOWN;
            break;
        case STRETCH: // STRETCH -> DOWN(slow)
            inc = -1;
            state = DOWN;
            break;
        }
    }
}


//함수 원형
void turn(PwmOut &rc, float deg); //Servo
template <class T>
T map(T x, T in_min, T in_max, T out_min, T out_max);


//메인 함수
int main() {
    state = SHRINK;         // 열거형 값 초기화

    float ang=0.;           // 서보모터 초기화
    rcServo.period_ms(20);
    turn(rcServo, 0);
    wait(0.1);

    btn.fall(&sw_callback); // 스위치 콜백함수 지정

    i2c.frequency(400000);  //OLED 초기화
    myOled.begin();
    
    buzzer.write(0);        //부저 초기화
    buzzer.period_us(900);
    
    while(1) {
        switch(state) {
        case SHRINK:  // SHRINK 상태
            if (_updated) {
                led_blink.detach(); //Blink Off
                buzzer.write(0);    //Buzzer Off
                _updated = false;
            }

            myOled.clearDisplay();  //OLED
            myOled.printf("SHRINK\r");
            myOled.display();

            led = 0;                //LED Off
            break;

        case UP:      // UP 상태
            if (_updated) {
                led_blink.attach(&blink, 0.1); //Blink, Buzzer On (100ms)
                _updated = false;
            }

            myOled.clearDisplay();             //OLED
            myOled.printf("GOING UP\r");
            myOled.display();

            turn(rcServo, ang);                //각도 증가(20ms에 1도씩)
            wait_ms(20);
            ang += inc;

            if (ang > 180.f) {                 //0도 -> 180도
                state = STRETCH;               //상태 전환
                _updated = true;
            }
            break;

        case DOWN:    // DOWN 상태
            if (_updated) {
                led_blink.attach(&blink, 0.1); //Blink, Buzzer On (100ms)
                _updated = false;
            }

            if (inter) {                       //UP, DOWN 중 스위치가 눌린 경우
                turn(rcServo, 0);              //-> 0도
                wait(0.2);
                state = SHRINK;                //상태 전환
                inter = false;
                _updated = true;
                ang = 0;
                break;
            }

            myOled.clearDisplay();             //OLED
            myOled.printf("GOING DOWN\r");
            myOled.display();
            
            turn(rcServo, ang);                //각도 감소(20ms에 -1도씩)
            wait_ms(20);
            ang += inc;

            if (ang < 0.f) {                   //180도 -> 0도
                state = SHRINK;                //상태 전환
                _updated = true;
            }
            break;

        case STRETCH: // STRETCH 상태
            if (_updated) {
                led_blink.detach(); //Blink Off
                buzzer.write(0);    //Buzzer Off
                _updated = false;
            }

            myOled.clearDisplay();  //OLED
            myOled.printf("STRETCH\r");
            myOled.display();

            led = 1;                //LED on
            break;
        }
    }
}


//함수 정의
void turn(PwmOut &rc, float deg) {
    uint16_t pulseW = map<float>(deg, 0., 180., 600., 2400.);
    rc.pulsewidth_us(pulseW);
}

template <class T>
T map(T x, T in_min, T in_max, T out_min, T out_max){
    return (x - in_min)*(out_max - out_min)/(in_max - in_min) + out_min;
}
