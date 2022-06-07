// CodeVision
#include <mega128.h>
#include <delay.h>
#include <stdio.h>
#include <math.h>
#include <lcd.h>

#define U_C unsigned char
#define U_I unsigned int

#asm
  .equ __lcd_port = 0x15 // Port C LCD 출력
#endasm

/* 온도센서 SHT11(Sample Code 참고) */
/* H/W => 1:GND, 2:DATA, 3:SHT_SCK, 4:VCC */
#define DATA_OUT DDRE.0
#define DATA_IN PINE.0
#define SHT_SCK PORTE.1

#define noACK 0
#define ACK 1
//                           adr     cmd     r/w
#define STATUS_REG_W 0x06 // 000     0011     0
#define STATUS_REG_R 0x07 // 000     0011     1
#define MEASURE_TEMP 0x03 // 000     0001     1
#define MEASURE_HUMI 0x05 // 000     0010     1
#define RESET 0x1e        // 000     1111     0

/**************************************************/

const float C1 = -4.0;
const float C2 = 0.0405;
const float C3 = -0.0000028;
const float T1 = 0.01;
const float T2 = 0.00008;

U_C str[30]; // LCD Buffer
U_C TEMP_cksum, HUMI_cksum;
U_I TEMP_val, HUMI_val;

float dew_point;

/* 온도센서 온습도 계산 */
U_C SHT11_ByteWR(U_C value);
U_C SHT11_ByteRD(U_C ack);
void SHT11_Start(void);
void SHT11_Reset(void);
U_C SHT11_HUMI(void);
U_C SHT11_TEMP(void);
void calc_STH11(void);

/* Main */
void main(void)
{
  int n = 1;
  U_C error;

#asm("cli")
  DDRE = 0x02; //온도
  DDRB = 0xFF; //모터
  DDRD = 0xFF; //출력
  DDRF = 0x00; //부저
  delay_us(150);
  lcd_init(16);

  lcd_clear();

  // 초기 화면 2초간 표시

  sprintf(str, "Engineer");
  lcd_gotoxy(0, 0);
  lcd_puts(str);

  sprintf(str, "by 4 Team");
  lcd_gotoxy(0, 1);
  lcd_puts(str);

  delay_ms(2000);

  /*******************/

  while (n)
  {
    SHT11_Reset();
    error = 0;
    error += SHT11_HUMI();
    error += SHT11_TEMP();

    if (error != 0)
      SHT11_Reset();
    else
      calc_STH11();

    /* 온습도 LCD 출력 */
    lcd_clear();

    sprintf(str, "by 4 Team");
    lcd_gotoxy(0, 1);
    lcd_puts(str);
    sprintf(str, "T=%2d[C] H=%2d[%%]", TEMP_val, HUMI_val);
    lcd_gotoxy(0, 0);
    lcd_puts(str);
    delay_ms(2000);

    /* 일정 온도 이상일때(Detecting TEMP Value -> 60) */
    if (TEMP_val > 60)
    {
      PORTB = 0xFF;  //모터 회전
      delay_ms(550); // 모터 딜레이
      PORTB = 0x00;  // 모터 정지

      /*
      While 의 무한 반복으로 인한 계속된 회전을 막고자 n = 0 할당(한번 회전)
      초기 n = 1 이었기 때문에 While(true)로 움직인다.
      그러다가 n = 0 할당해줌으로써 다음 반복을 막고 종료한다.
      */
      n = 0;
      PORTD = 0xFF; // LED 출력
      DDRF = 0xFF;  //부저 출력
    }
  }

  //화재시 lcd 화면 반전
  lcd_clear();
  sprintf(str, "Warning");
  lcd_gotoxy(4, 0);
  lcd_puts(str);
  sprintf(str, "Get Out");
  lcd_gotoxy(4, 1);
  lcd_puts(str);
  delay_ms(2000);
}

/********************************************/

/* 온습도 Function */
U_C SHT11_ByteWR(U_C value)
{
  U_C i, error = 0;
  for (i = 0x80; i > 0; i >>= 1)
  {
    if (i & value)
      DATA_OUT = 0;
    else
      DATA_OUT = 1;

    delay_us(2);
    SHT_SCK = 1;
    delay_us(6);
    SHT_SCK = 0;
    delay_us(3);
  }
  DATA_OUT = 0;
  SHT_SCK = 1;
  delay_us(3);
  error = DATA_IN;
  delay_us(2);
  SHT_SCK = 0;

  return error;
}

U_C SHT11_ByteRD(U_C ack)
{
  U_C i, val = 0;
  DATA_OUT = 0;
  for (i = 0x80; i > 0; i >>= 1)
  {
    SHT_SCK = 1;
    delay_us(3);

    if (DATA_IN)
      val |= i;

    SHT_SCK = 0;
    delay_us(3);
  }
  DATA_OUT = ack;

  SHT_SCK = 1;
  delay_us(6);
  SHT_SCK = 0;
  delay_us(3);
  DATA_OUT = 0;

  return val;
}

void SHT11_Start(void)
{
  DATA_OUT = 0;
  SHT_SCK = 0;
  delay_us(3);
  SHT_SCK = 1;
  delay_us(3);

  DATA_OUT = 1;
  delay_us(3);

  SHT_SCK = 0;
  delay_us(6);
  SHT_SCK = 1;
  delay_us(3);

  DATA_OUT = 0;
  delay_us(3);

  SHT_SCK = 0;
}

void SHT11_Reset(void)
{
  U_C i;
  DATA_OUT = 0;
  SHT_SCK = 0;
  delay_us(3);

  for (i = 0; i < 9; i++)
  {
    SHT_SCK = 1;
    delay_us(3);
    SHT_SCK = 0;
    delay_us(3);
  }
  SHT11_Start();
}

U_C SHT11_HUMI(void)
{
  U_C error = 0;
  long i;
  error += SHT11_ByteWR(MEASURE_HUMI);

  for (i = 0; i < 400000; i++)
  {
    delay_us(5);
    if (!DATA_IN)
      break;
  }

  if (DATA_IN)
    error++;

  HUMI_val = SHT11_ByteRD(ACK);
  HUMI_val <<= 8;
  HUMI_val += SHT11_ByteRD(ACK);
  HUMI_cksum = SHT11_ByteRD(noACK);

  return error;
}

U_C SHT11_TEMP(void)
{
  U_C error = 0;
  long i;
  SHT11_Start();
  error += SHT11_ByteWR(MEASURE_TEMP);

  for (i = 0; i < 400000; i++)
  {
    delay_us(5);
    if (!DATA_IN)
      break;

    if (DATA_IN)
      error++;

    TEMP_val = SHT11_ByteRD(ACK);
    TEMP_val <<= 8;
    TEMP_val += SHT11_ByteRD(ACK);
    TEMP_cksum = SHT11_ByteRD(noACK);

    return error;
  }
}
void calc_STH11(void)
{
  float rh_lin, rh_true, t_C;
  float TEMP_f, HUMI_f, logEx;

  TEMP_f = (float)TEMP_val;
  HUMI_f = (float)HUMI_val;

  t_C = TEMP_f * 0.01 - 40;
  rh_lin = C3 * HUMI_f * HUMI_f + C2 * HUMI_f + C1;
  rh_true = (t_C - 25) * (T1 + T2 * HUMI_f) + rh_lin;

  if (rh_true > 100)
    rh_true = 100;

  if (rh_true < 0.1)
    rh_true = 0.1;

  TEMP_val = (U_I)t_C;
  HUMI_val = (U_I)rh_true;

  logEx = 0.66077 + 7.5 * t_C / (237.3 + t_C) + (log10(t_C) - 2);

  dew_point = (logEx - 0.66077) * 237.3 / (0.66077 + 7.5 - logEx);
}