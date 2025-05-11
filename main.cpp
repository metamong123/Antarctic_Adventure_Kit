#include "mbed.h"
#include "MPU9250_SPI.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include "game.h"

//#define CALIBRATION_MODE
#define OLED_I2C_ADR 0x78
#define OLED_HEIGHT 64
#define OLED_WIDTH  128

// 객체 선언
MPU9250_SPI mpu(D11, D12, D13, D2, D3);                                      // MPU 객체 (mosi, miso, sclk, cs, intr)
I2C i2c(I2C_SDA, I2C_SCL);                                                   // I2C 객체 
Adafruit_SSD1306_I2c myOled(i2c, D4, OLED_I2C_ADR, OLED_HEIGHT, OLED_WIDTH); // OLED 객체
Serial pc(USBTX, USBRX, 115200);                                             // USB Serial 객체
Ticker hole_selector;                                                        // 구멍 생성 위치 선택 Ticker 객체

// 함수 원형
void calibrationProcess();  // MPU Calibration 함수, CALIBRATION_MODE가 정의되었을 때만 실행
void applyCalbratedValue(); // MPU Calibration 적용 함수
void selectHole();          // 구멍 생성 위치 선택 Ticker 콜백 함수

// 객체 선언
Hole hole1(0); // 구멍 객체, 한 화면에 최대 3개의 구멍이 생성됨 (x, dx, dc)
Hole hole2(1);
Hole hole3(2);
Penguin penguin(D8);        // 펭귄 객체, 버튼 핀 번호

// 전역변수 초기화
int hole_idx = 0; // hole selector에서 선택된 구멍의 위치
bool change_hole_flag = false;
float dt;

// 메인 함수
int main(){
    srand(time(NULL));
    hole_selector.attach(&selectHole, 0.7);
    
    mpu.setup();
    mpu.setMagneticDeclination(8.5);
    mpu.setSampleRate(SR_100HZ);
    mpu.setGyroRange(GYRO_RANGE_2000DPS);
    mpu.setAccelRange(ACCEL_RANGE_16G);
    mpu.setDlpfBandwidth( DLPF_BANDWIDTH_184HZ);
    mpu.enableDataReadyInterrupt();

    #ifdef CALIBRATION_MODE
    calibrationProcess(); // CALIBRATION_MODE일 때만 실행
    #endif
    applyCalbratedValue();
    
    i2c.frequency(400000);
    myOled.begin();
    myOled.clearDisplay();

    while(true){
        if (!penguin.get_game_over()) {                 // 게임 오버가 아닌 상태
            if (penguin.game_start_flag) {              // 게임 시작 초기화
                hole_selector.attach(&selectHole, 0.7); // 구멍 선정 Ticker 비활성화
                myOled.clearDisplay();                  // 화면 초기화
                change_hole_flag = false;
                penguin.game_start_flag = false;
            }
            
            if (mpu.isDataReady()){ // MPU 값 받아오기
                mpu.update(MADGWICK); // MAGDWICK
                if (mpu.getPitch()*RAD_TO_DEG <= -10) { penguin.move_right(); }    // 센서를 오른쪽으로 기울이면 펭귄이 오른쪽으로 이동
                else if (mpu.getPitch()*RAD_TO_DEG >= 10) { penguin.move_left(); } // 센서를 왼쪽으로 기울이면 펭귄이 왼쪽으로 이동
            }
            
            if (change_hole_flag){  // hole_selector로 새로 선정된 구멍 위치로 변경
                if (!hole1.onoff) { // 비활성화 상태일 때 변경
                    hole1.set(hole_idx);
                    change_hole_flag = false;
                } else if (!hole2.onoff) {
                    hole2.set(hole_idx);
                    change_hole_flag = false;
                } else if (!hole3.onoff) {
                    hole3.set(hole_idx);
                    change_hole_flag = false;
                }
            }
            
            myOled.drawBitmap(0, 0, ground, OLED_WIDTH, OLED_HEIGHT, WHITE); // 배경 그리기
            hole1.rect(myOled);                              // 구멍 그리기
            hole2.rect(myOled);
            hole3.rect(myOled);
            penguin.draw_penguin(myOled);                    // 펭귄 그리기
            myOled.display();
            
            penguin.decision(hole1, hole2, hole3);           // 게임 오버 판정 (구멍에 빠졌거나 물개와 부딪혔는지 확인)
            wait(0.07);
            
        } else { // 게임 오버인 상태
            if (penguin.game_over_flag) {
                hole1.off(); // 구멍 비활성화
                hole2.off();
                hole3.off();
                myOled.fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT, BLACK); // OLED 화면 초기화
                hole_selector.detach();                // 구멍 선정 Ticker 비활성화
                penguin.game_over_flag = false;
            }
            myOled.setTextCursor(0, 0);                // 게임오버 화면 출력
            myOled.setTextSize(2);
            myOled.printf("\nGame Over!");
            myOled.setTextSize(1);
            myOled.printf("\nPress Button to Start\n");
            myOled.display();
            wait(0.1);
        }
    }
}

void calibrationProcess(){  // MPU Calibration 설정 함수
  mpu.calibrateGyro();   
  mpu.calibrateMag();
  pc.printf("Calibration Completed !!!!!!!!\n");  
    while(true);   //stop here
}

void applyCalbratedValue(){ // MPU 보정 적용 함수
  Vect3 gBias ={ -2.580,    0.136  ,  -0.574};
  mpu.setGyroBias(gBias);
  Vect3 mBias ={-16665.123,   58.476  ,  -1.751};
  mpu.setMagBias(mBias);
  Vect3 mScale={  0.356,   11.881  ,   9.384};
  mpu.setMagScale(mScale);
}

void selectHole() {        // Ticker hole_selector 콜백 함수
    hole_idx = rand() % 3; // 구멍 발생 위치 3개 중 한 개 선택
    change_hole_flag = true;
}
