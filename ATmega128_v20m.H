/* -------------------------------------------------------------------------- */
/* ATmega128_v20m_8bit_LCD.H                                                 */
/* I/O Address Definition & User Function for ATmega128                     */
/* Modified for 8-bit direct GPIO LCD control                               */
/* -------------------------------------------------------------------------- */

// --- 기존 메모리 맵 LCD 정의 제거 ---
// #define LCD_DATABUS      *((volatile unsigned char *)0x2000)
// #define LCD_CONTROL      *((volatile unsigned char *)0x2100)

// --- 새로운 GPIO 기반 LCD 정의 ---
#define LCD_DATA_PORT    PORTC // 데이터 버스를 PORTC로 사용
#define LCD_DATA_DDR     DDRC
#define LCD_CTRL_PORT    PORTA // 제어 신호를 PORTA로 사용
#define LCD_CTRL_DDR     DDRA
#define LCD_RS_PIN       PA0   // Register Select 핀
#define LCD_E_PIN        PA1   // Enable 핀
// R/W 핀은 GND에 연결된 것으로 가정 (항상 쓰기 모드)

#define RTC_SECOND      *((unsigned char *)0x2200)     // DS12C887 RTC
#define RTC_SEC_ALM     *((unsigned char *)0x2201)
#define RTC_MINUTE      *((unsigned char *)0x2202)
#define RTC_MIN_ALM     *((unsigned char *)0x2203)
#define RTC_HOUR        *((unsigned char *)0x2204)
#define RTC_HOUR_ALM    *((unsigned char *)0x2205)
#define RTC_WEEKDAY     *((unsigned char *)0x2206)
#define RTC_DATE        *((unsigned char *)0x2207)
#define RTC_MONTH       *((unsigned char *)0x2208)
#define RTC_YEAR        *((unsigned char *)0x2209)
#define RTC_A           *((unsigned char *)0x220A)
#define RTC_B           *((unsigned char *)0x220B)
#define RTC_C           *((unsigned char *)0x220C)
#define RTC_D           *((unsigned char *)0x220D)
#define RTC_FLAG        *((unsigned char *)0x220E)
#define RTC_Value       *((unsigned int  *)0x220F)

#define KEY_PULLUP   0x0F          // 항상 1 로 유지할 비트
#define LED_OFF()    (PORTB = KEY_PULLUP)
#define LED_GREEN()  (PORTB = KEY_PULLUP | (1<<PB7))
#define LED_RED()    (PORTB = KEY_PULLUP | (1<<PB4))

#define PPI_PORTA       *((unsigned char *)0x2300)     // 8255-1 address
#define PPI_PORTB       *((unsigned char *)0x2301)
#define PPI_PORTC       *((unsigned char *)0x2302)
#define PPI_CW          *((unsigned char *)0x2303)

void MCU_initialize(void)    /* initialize ATmege128 MCU */
{
     MCUCR  = 0x00;      // SRE=0  → 외부 메모리 인터페이스 OFF
     XMCRA  = 0x00;
     XMCRB  = 0x00;

     // PORTB 설정: LED (PB7, PB4 출력), Key_input (PB3-0 입력)
     DDRB  = (1 << DDB7) | (1 << DDB4); // PB7, PB4를 출력으로 설정. PB3-0은 입력(0)으로 유지.
                                        // 기존 0xF0 (PB7-4 출력)에서 더 명시적으로 변경.
                                        // Key_input 함수는 PB3-0을 입력으로 사용합니다.
     PORTB = 0x00;                      // PORTB 초기값 LOW, 입력핀 풀업 없음.

     // PORTD 설정: 초음파 ECHO (PD0 입력), DC 모터 (PD2, PD3 출력)
     DDRD  = (0 << DDD0) | (1 << DDD2) | (1 << DDD3); // PD0 입력, PD2, PD3 출력. 나머지 핀은 입력.
     PORTD = 0x00;                      // PORTD 초기값 LOW. PD0 풀업은 main.c에서 필요시 설정.

     // PORTE 설정: 서보 모터 (PE3 출력), 초음파 TRIG (PE4 출력)
     DDRE  = (1 << DDE3) | (1 << DDE4); // PE3, PE4 출력. 나머지 핀은 입력.
     PORTE = 0x00;                      // PORTE 초기값 LOW.

     // PORTF 설정: ADC 등 (기본적으로 입력)
     DDRF  = 0x00;                      // PORTF 전체 입력.
     PORTF = 0x00;                      // PORTF 풀업 없음.

     // PORTG 설정: 부저 (PG3 출력)
     DDRG  = (1 << DDG3);               // PG3 출력. 나머지 핀은 입력.
     PORTG = 0x00;                      // PORTG 초기값 LOW.

     // LCD 데이터 포트(PORTC) 및 제어 포트(PORTA)의 DDR 설정은
     // LCD_initialize() 함수 내에서 수행됩니다.
}

void Delay_us(unsigned int time_us)        /* time delay for us */
{
     register unsigned int i;

     for(i = 0; i < time_us; i++) {     // 4 cycle +
        asm (" PUSH  R0 ");             // 2 cycle +
        asm (" POP   R0 ");             // 2 cycle +
        asm (" PUSH  R0 ");             // 2 cycle +
        asm (" POP   R0 ");             // 2 cycle +
        asm (" PUSH  R0 ");             // 2 cycle +
        asm (" POP   R0 ");             // 2 cycle = 16 cycle = 1 us for 16MHz
     }
}

void Delay_ms(unsigned int time_ms)        /* time delay for ms */
{
     register unsigned int i;

     for(i = 0; i < time_ms; i++) {
        Delay_us(250);
        Delay_us(250);
        Delay_us(250);
        Delay_us(250);
     }
}

/* LCD 제어 함수 (8비트 직접 GPIO 제어 방식으로 수정) */
void LCD_pulse_enable(void)
{
    LCD_CTRL_PORT |= (1 << LCD_E_PIN);   // E = 1 (Enable HIGH)
    Delay_us(1);                         // Enable 펄스 폭 유지를 위한 짧은 지연 (최소 450ns)
    LCD_CTRL_PORT &= ~(1 << LCD_E_PIN);  // E = 0 (Enable LOW)
}

void LCD_command(unsigned char command)    /* write a command(instruction) to text LCD */
{
     LCD_CTRL_PORT &= ~(1 << LCD_RS_PIN); // RS = 0 (명령 모드)
     // R/W 핀은 GND에 연결되어 있으므로 별도 제어 없음 (항상 쓰기 모드)

     LCD_DATA_PORT = command;             // 데이터 포트에 명령어 전송
     LCD_pulse_enable();                  // Enable 펄스 발생

     // 명령어 실행 시간 대기
     // Clear Display (0x01)나 Return Home (0x02) 명령어는 더 긴 지연이 필요 (약 1.52ms 이상)
     if (command == 0x01 || command == 0x02) {
         Delay_ms(2); // 충분한 시간 대기
     } else {
         Delay_us(40); // 일반 명령어 대기 시간 (약 37us 이상)
     }
}

void LCD_data(unsigned char data)        /* display a character on text LCD */
{
     LCD_CTRL_PORT |= (1 << LCD_RS_PIN);  // RS = 1 (데이터 모드)
     // R/W 핀은 GND에 연결되어 있으므로 별도 제어 없음 (항상 쓰기 모드)

     LCD_DATA_PORT = data;                // 데이터 포트에 데이터 전송
     LCD_pulse_enable();                  // Enable 펄스 발생

     Delay_us(40); // 데이터 쓰기 후 대기 시간 (약 37us 이상)
}

void LCD_string(char command, char *string) /* display a string on LCD */
{
     LCD_command(command);            // 문자열 시작 위치 설정 (예: 0x80은 첫째 줄 시작)
     while(*string != '\0') {         // 문자열 끝까지 반복
        LCD_data(*string);            // 한 문자씩 LCD에 표시
        string++;                     // 다음 문자 포인터로 이동
     }
}

void LCD_initialize(void)            /* initialize text LCD module */
{
    // LCD 데이터 포트 및 제어 포트 핀을 출력으로 설정
    LCD_DATA_DDR = 0xFF; // PORTC 전체를 출력으로
    LCD_CTRL_DDR |= (1 << LCD_RS_PIN) | (1 << LCD_E_PIN); // PA0(RS), PA1(E)을 출력으로

    Delay_ms(20); // 전원 인가 후 LCD 안정화 시간 (15ms 이상)

    // 8비트 모드 초기화 시퀀스
    LCD_command(0x30); // Function Set: 8-bit (이 명령을 여러 번 보낼 수 있음)
    Delay_ms(5);       // 4.1ms 이상 대기
    LCD_command(0x30);
    Delay_us(100);     // 100us 이상 대기
    LCD_command(0x30);
    Delay_us(100);

    // 이제부터 Busy Flag 확인 가능 상태가 되지만, R/W를 GND에 연결했으므로 딜레이로 대체
    LCD_command(0x38); // Function Set: 8-bit, 2 Lines, 5x7 Dots
    LCD_command(0x0C); // Display ON/OFF Control: Display ON, Cursor OFF, Blink OFF
                       // (0x0E: Cursor ON, 0x0F: Cursor ON & Blink ON)
    LCD_command(0x01); // Clear Display
    Delay_ms(2);       // Clear Display 명령어는 실행 시간이 김
    LCD_command(0x06); // Entry Mode Set: Increment cursor, No display shift
}

/* 아래 함수들은 수정된 LCD_data()를 사용하므로 별도 수정 없이 동작할 것입니다. */
void LCD_1d1(float number)
{
     unsigned int i, j;

     j=(int)(number*10. +0.5);
     i = j / 10;
     LCD_data(i + '0');
     LCD_data('.');
     i = j % 10;
     LCD_data(i + '0');
}

void LCD_2d(unsigned int number)
{
     unsigned int i;
     i = number / 10;
     if(i == 0) LCD_data(' '); 
     else LCD_data(i + '0');

     i = number % 10;
     LCD_data(i + '0');
}

void LCD_2hex(unsigned int number)
{
     unsigned int i;

    i = (number >> 4) & 0x0F;
     if(i <= 9) LCD_data(i + '0');
     else       LCD_data(i - 10 + 'A');

     i = number & 0x0F;
     if(i <= 9) LCD_data(i + '0');
     else       LCD_data(i - 10 + 'A');
}

void LCD_2d1(float number)            /* display XX.X number on LCD */
{
     int d1, d2, d3; // 변수 이름 i 대신 d1, d2, d3 사용
     d1 = (int)number / 10;
     d2 = (int)number % 10;
     d3 = (int)((number - (int)number) * 10.0 + 0.5); 

     if (d1 == 0) LCD_data(' '); else LCD_data(d1 + '0');
     LCD_data(d2 + '0');
     LCD_data('.');
     LCD_data(d3 + '0');
}

void LCD_1d2(unsigned int number) 
{
     unsigned int i;
     i = number / 100;
     LCD_data(i + '0');
     LCD_data('.');
     number %= 100;
     i = number / 10;
     LCD_data(i + '0');
     i = number % 10;
     LCD_data(i + '0');
}

void LCD_3d(unsigned int number)
{
     unsigned int i,flag;
     flag = 0;
     i = number / 100;
     if(i == 0 && number < 100) LCD_data(' '); 
     else { LCD_data(i + '0'); flag = 1; }

     number %= 100;
     i = number / 10;
     if(i == 0 && flag == 0 && number < 10) LCD_data(' '); 
     else { LCD_data(i + '0'); flag =1; } // flag=1 추가

     i = number % 10;
     LCD_data(i + '0');
}

void LCD_4d(unsigned int number)
{
     unsigned int i,flag;
     flag = 0;
     i = number / 1000;
     if(i == 0) LCD_data(' '); else { LCD_data(i + '0'); flag = 1; }

     number %= 1000;
     i = number / 100;
     if(i == 0 && flag == 0) LCD_data(' '); else { LCD_data(i + '0'); flag = 1; }

     number %= 100;
     i = number / 10;
     if(i == 0 && flag == 0) LCD_data(' '); else { LCD_data(i + '0'); flag=1;} // flag=1 추가

     i = number % 10;
     LCD_data(i + '0');
}

void LCD_s4d(int number) 
{
     unsigned int temp_num;
     unsigned int i, flag = 0;

     if(number < 0) {
        LCD_data('-');
        temp_num = -number;
     } else {
        LCD_data('+'); // 0도 +로 표시됩니다. 원치 않으면 (number == 0) LCD_data(' '); 등으로 수정
        temp_num = number;
     }

     i = temp_num / 1000;
     if(i == 0) LCD_data(' '); else { LCD_data(i + '0'); flag = 1; }

     temp_num %= 1000;
     i = temp_num / 100;
     if(i == 0 && flag == 0) LCD_data(' '); else { LCD_data(i + '0'); flag = 1; }

     temp_num %= 100;
     i = temp_num / 10;
     if(i == 0 && flag == 0) LCD_data(' '); else { LCD_data(i + '0'); flag=1;} // flag=1 추가

     i = temp_num % 10;
     LCD_data(i + '0');
}


void LCD_4hex(unsigned int number)
{
     unsigned int i;

     i = number >>12;
     if(i <= 9) LCD_data(i + '0'); else LCD_data(i - 10 + 'A');

     i = (number >> 8) & 0x000F;
     if(i <= 9) LCD_data(i + '0'); else LCD_data(i - 10 + 'A');

     i = (number >> 4) & 0x000F;
     if(i <= 9) LCD_data(i + '0'); else LCD_data(i - 10 + 'A');

     i = number & 0x000F;
     if(i <= 9) LCD_data(i + '0'); else LCD_data(i - 10 + 'A');
}

void LCD_4d1(float number)     
{
     unsigned int i,j,flag;
     flag = 0;

     j = (int)(number*10. + 0.5); 
     i = j/10000;
     if(i == 0) LCD_data(' '); else { LCD_data(i + '0'); flag = 1; }

     j %= 10000;
     i = j / 1000;
     if(i == 0 && flag == 0) LCD_data(' '); else { LCD_data(i + '0'); flag = 1; }

     j %= 1000;
     i = j / 100;
     if(i == 0 && flag == 0) LCD_data(' '); else { LCD_data(i + '0'); flag = 1; }

     j %= 100;
     i = j / 10;
     LCD_data(i + '0'); 

     LCD_data('.');

     i = j % 10;
     LCD_data(i + '0'); 
}

void LCD_6d(unsigned long number) 
{
     unsigned int i,flag;

     flag = 0;
     i = number / 100000;
     if(i == 0) LCD_data(' '); else { LCD_data(i + '0'); flag = 1; }

     number %= 100000;
     i = number / 10000;
     if(i == 0 && flag == 0) LCD_data(' '); else { LCD_data(i + '0'); flag = 1; }

     number %= 10000;
     i = number / 1000;
     if(i == 0 && flag == 0) LCD_data(' '); else { LCD_data(i + '0'); flag = 1; }

     number %= 1000;
     i = number / 100;
     if(i == 0 && flag == 0) LCD_data(' '); else { LCD_data(i + '0'); flag = 1; }

     number %= 100;
     i = number / 10;
     if(i == 0 && flag == 0) LCD_data(' '); else { LCD_data(i + '0'); flag=1;} // flag=1 추가

     i = number % 10;
     LCD_data(i + '0');
}

void Beep(void)                   
{
  PORTG |= (1 << PG3); // PG3으로 명시적 변경 (기존 _BV(3)과 동일)              
  Delay_ms(50);                   
  PORTG &= ~(1 << PG3); // PG3으로 명시적 변경             
}

unsigned char Key_input(void)            
{
  register unsigned char key;
  // PORTB의 하위 4비트(PB0-PB3)를 입력으로 사용한다고 가정합니다.
  // MCU_initialize()에서 DDRB는 (1<<DDB7)|(1<<DDB4)로 설정되어 PB0-PB3은 입력입니다.
  // 외부 풀업 또는 풀다운 저항이 필요합니다. 또는 내부 풀업을 활성화해야 합니다.
  // 예: PORTB |= 0x0F; // PB0-PB3 내부 풀업 활성화 (main 시작 시 한 번)

  key = PINB & 0x0F;                
  // 스위치가 눌리지 않았을 때(active LOW, 내부 풀업 사용 시 HIGH)와 눌렸을 때의 값을 정확히 정의해야 합니다.
  // 아래 로직은 스위치가 눌리면 해당 비트가 0이 되고, 안 눌리면 1이라고 가정 (내부 풀업 사용 시)
  if(key == 0x0F) return 0x0F;      // 모든 키가 안 눌렸으면 (모두 HIGH) 0x0F 반환

  // 키가 하나라도 눌렸으면 (하나라도 LOW가 되면) 잠시 대기 (채터링 방지)
  Delay_ms(20);                     // 채터링 시간 (조정 가능)
  key = PINB & 0x0F;                // 다시 읽음

  // 키가 완전히 떨어질 때까지 대기 (모든 키가 다시 HIGH가 될 때까지)
  while((PINB & 0x0F) != 0x0F);
  Delay_ms(20);                     // 키가 떨어진 후 안정화 시간

  return key; // 눌린 키의 상태 반환 (눌린 키는 0, 안 눌린 키는 1)
}
