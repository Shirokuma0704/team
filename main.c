#include <avr/io.h>
#include <avr/interrupt.h>

#include "ATmega128_v20m.H"

#define F_CPU 16000000UL

volatile uint16_t echo_start = 0, Echo_count = 0;
volatile uint8_t  echo_state = 0;          // 0:대기, 1:High 측정중

// --- 초음파 인터럽트: 에코 타이머 기록 ---
ISR(INT0_vect)
{
    if(echo_state == 0) {                  // ① 상승 에지
        echo_start = TCNT1;
        /* 다음 인터럽트를 하강 에지로 변경 */
        EICRA &= ~((1<<ISC01)|(1<<ISC00)); // CL
        EICRA |=  (1<<ISC01);              // ISC01=1, ISC00=0 : FALL
        echo_state = 1;
    } 
    else {                                 // ② 하강 에지
        Echo_count = TCNT1 - echo_start;   // High 구간 길이
        /* 다시 상승 에지 대기 */
        EICRA |=  (1<<ISC00);              // ISC01=1, ISC00=1 : RISE
        echo_state = 0;
        EIMSK  &= ~(1<<INT0);              // 필요하면 여기서 중단
    }
}
// --- 초음파 거리 측정 함수 ---
int UltraSonic(char ch) {
	int range;
	switch(ch) {
		case 0:
		PORTE |= 0x10; // TRIG HIGH (PORTE4)
		EIMSK |= 0x01; // INT0 활성화
		break;
		default:
		break;
	}

	Delay_us(12);
	PORTE &= ~0x10; // TRIG LOW

	TCNT1 = 0;
	EIFR  |= (1<<INTF0);   // 플래그 클리어
	EICRA |= (1<<ISC01)|(1<<ISC00); // RISING
	EIMSK |= (1<<INT0);    // INT0 Enable
	
	Echo_count -= 4400;
	if (Echo_count < 180) Echo_count = 180;
	if (Echo_count > 36000L) Echo_count = 36000L;
	range = Echo_count / 116; // cm 변환
	return range;
}

// --- 서보 초기화 (Timer3, PORTE3 = OC3A) ---
void init_servo() {
	DDRE |= (1 << PORTE3);
	TCCR3A = 0x82; // 비반전 모드, OCR3A
	TCCR3B = 0x1A; // 분주 8, WGM = 고속 PWM, TOP = ICR3
	ICR3 = 19999; // 20ms
	OCR3A = 1500; // 90도
}

// --- 서보 각도 설정 (0~180도) ---
void set_servo_angle(uint8_t angle) {
	OCR3A = 500 + ((uint16_t)angle * 2000) / 180;
}

// --- DC 모터 전진 (PORTD2, PORTD3) ---
void motor_forward(void) {
	PORTD |= (1 << PORTD2);
	PORTD &= ~(1 << PORTD3);
}


// --- 메인 함수 ---
int main(void) {
	MCU_initialize(); // 기본 초기화
	LCD_initialize(); // LCD 초기화
	init_servo(); // 서보 초기화

	LCD_string(0x80, "Smart Sorter");
	LCD_string(0xC0, "Init Complete");

	// 타이머1: 초음파용
	TCCR1A = 0x00;
	TCCR1B = 0x02; // clk/8 → 0.5us 단위
	TCNT1 = 0x0000;

	// 외부 인터럽트 INT0: 하강 에지
	EICRA = 0x0A;
	EIMSK = 0x00;
	sei(); // 전역 인터럽트 허용

	// 포트 설정
	DDRE |= (1 << PORTE4); // TRIG
	DDRD |= (1 << PORTD2) | (1 << PORTD3); // 모터
	DDRG |= (1 << PORTG3); // 부저
	DDRB |= (1<<DDB4) | (1<<DDB7);   // LED 출력
	PORTB  |= 0x0F;                    // PB0~PB3 Pull-up ON  (LED 비트는 아직 0)

	motor_forward(); // 모터 시작

	while (1) {
		int distance = UltraSonic(0);

		LCD_string(0x80, "US_Range: cm");
		LCD_command(0x80 + 10);
		LCD_4d(distance);

		if (distance >= 11 && distance <= 350) {
			PORTB = 0x80; // LED 표시
			set_servo_angle(45); // 분류 위치1
			} else if (distance >= 2 && distance <= 10) {
			PORTB = 0x10; // 경고 표시
			Beep(); // 부저
			set_servo_angle(90); // 분류 위치2
			Delay_ms(1500); // 잠시 정지
			} else {
			set_servo_angle(135); // 기본 위치
			PORTB = 0x00;
		}

		Delay_ms(500);
	}
}


