#include "../header/sth11.h"
#include <mega128.h>
#include <stdio.h>
#include <lcd.h>

#asm
  .equ __lcd_port = 0x15 // Port C LCD 출력
#endasm

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