#include "../header/sth11.h"
#include <stdio.h>
#include <delay.h>
#include <math.h>

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