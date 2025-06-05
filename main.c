#include <avr/io.h>
#include <avr/interrupt.h>

#include "ATmega128_v20m.H"

#define F_CPU 16000000UL

volatile uint16_t Echo_count = 0;

// --- 초음파 인터럽트: 에코 타이머 기록 ---
ISR(INT0_vect) {
	Echo_count = TCNT1;
}

// --- 초음파 거리 측정 함수 ---
int UltraSonic(char ch) {
    int range;
    switch(ch) {
        case 0:
        PORTE |= (1 << PORTE4); // TRIG HIGH
        EIMSK |= (1 << INT0); // INT0 활성화 (비트 위치 명시)
        break;
        default:
        break;
    }

    Delay_us(12);
    PORTE &= ~(1 << PORTE4); // TRIG LOW

    TCNT1 = 0x0000;
    EIFR |= (1 << INTF0); // INT0 플래그 클리어
    Delay_ms(50);
    EIMSK &= ~(1 << INT0); // INT0 비활성화

    // 거리 계산 공식 수정 (58us/cm 기준)
    if (Echo_count > 0) {
        range = Echo_count / 116; // 0.5us 단위로 변환 후 58us/cm로 나누기
    } else {
        range = 0;
    }
    
    if (range > 400) range = 400; // 최대 측정 거리 제한
    return range;
}

// --- 서보 초기화 (Timer3, PORTE3 = OC3A) ---
void init_servo() {
    DDRE |= (1 << PORTE3);
    TCCR3A = 0x82;
    TCCR3B = 0x1C; // 분주 256으로 수정 (20ms 주기를 위해)
    ICR3 = 1249;   // 16MHz/256 = 62.5kHz, 20ms = 1250-1 = 1249
    OCR3A = 94;    // 1.5ms (중앙 위치)
}

// --- 서보 각도 설정 (0~180도) ---
void set_servo_angle(uint8_t angle) {
    OCR3A = 31 + ((uint16_t)angle * 63) / 180; // 0.5ms~2.5ms 범위
}

// --- DC 모터 전진 (PORTD2, PORTD3) ---
void motor_forward(void) {
	PORTD |= (1 << PORTD2);
	PORTD &= ~(1 << PORTD3);
}


 // --- 메인 함수 ---
int main(void) {
    MCU_initialize(); // 기본 초기화
    
    // 키 입력을 위한 내부 풀업 활성화 (필요시)
    PORTB |= 0x0F; // PB0-PB3 내부 풀업 활성화
    
    // LCD 초기화
	LCD_initialize(); // LCD 초기화
	init_servo(); // 서보 초기화

	LCD_string(0x80, "Smart Sorter");
	LCD_string(0xC0, "Init Complete");

	// 타이머1: 초음파용
	TCCR1A = 0x00;
	TCCR1B = 0x02; // clk/8 → 0.5us 단위
	TCNT1 = 0x0000;

	// 외부 인터럽트 INT0: 하강 에지 -> 상승 에지로 변경 필요
	EICRA = 0x03; // INT0를 상승 에지로 설정 (ECHO 신호 감지용)
	EIMSK = 0x00;
	sei(); // 전역 인터럽트 허용

	// 포트 설정
	DDRE |= (1 << PORTE4); // TRIG
	// ECHO 핀(PD0)은 이미 MCU_initialize()에서 입력으로 설정됨
	DDRD |= (1 << PORTD2) | (1 << PORTD3); // 모터
	DDRG |= (1 << PORTG3); // 부저
	PORTB = 0x00;

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

