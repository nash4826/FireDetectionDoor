#define U_C unsigned char
#define U_I unsigned int
/* 온도센서 SHT11(Sample Code 참고) */
/* H/W => 1:GND, 2:DATA, 3:SHT_SCK, 4:VCC */
#define DATA_OUT DDRE .0
#define DATA_IN PINE .0
#define SHT_SCK PORTE .1

#define noACK 0
#define ACK 1
//                           adr     cmd     r/w
#define STATUS_REG_W 0x06 // 000     0011     0
#define STATUS_REG_R 0x07 // 000     0011     1
#define MEASURE_TEMP 0x03 // 000     0001     1
#define MEASURE_HUMI 0x05 // 000     0010     1
#define RESET 0x1e        // 000     1111     0

/**************************************************/

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

const float C1 = -4.0;
const float C2 = 0.0405;
const float C3 = -0.0000028;
const float T1 = 0.01;
const float T2 = 0.00008;
