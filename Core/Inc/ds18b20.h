uint8_t reset_ds18b20(void);
uint8_t read_bit_ds18b20(void);
uint8_t read_byte_ds18b20(void);
void write_bit_ds18b20(uint8_t bit);
void write_byte_ds18b20(uint8_t data);
uint8_t init_ds18b20(int pin_numb);
void measure_cmd_ds18b20(void);
void read_stratcpad_ds18b20(uint8_t *Data);
uint8_t get_sign_ds18b20(uint16_t sign);
float ds18b20_Convert(uint16_t raw_temp);
void delay_us(int delay_value);
int pin_numb;
uint16_t pin;

uint8_t reset_ds18b20(void)
{
	uint16_t status;
	HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_RESET);
	delay_us(485);
	HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_SET);
	delay_us(65);
	status = HAL_GPIO_ReadPin(GPIOB, pin);
	delay_us(500);
	if (status == 0){
		return 1;
	}
	else {
		return 0;
	}
}

uint8_t read_bit_ds18b20(void)
{
	uint16_t bit;
	HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_RESET);
	delay_us(2);
	HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_SET);
	delay_us(13);
	bit = HAL_GPIO_ReadPin(GPIOB, pin);
	delay_us(45);
	return bit;
}

uint8_t read_byte_ds18b20(void)

{
  uint8_t data = 0;

  for (uint8_t i = 0; i <= 7; i++)

  data += read_bit_ds18b20() << i;

  return data;
}

void write_bit_ds18b20(uint8_t bit)
{
	if (bit == 1){
		HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_RESET);
		delay_us(3);
		HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_SET);
		delay_us(65);
	}
	else
	{
		HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_RESET);
		delay_us(65);
		HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_SET);
		delay_us(3);
	}
}

void write_byte_ds18b20(uint8_t data)
{
	for (uint8_t i = 0; i<8; i++){
		write_bit_ds18b20((data >> i) & 1);
		delay_us(5);
	}
}


uint8_t init_ds18b20(int pin_numb)
{

	if (pin_numb == 1)
	{
		pin = GPIO_PIN_12;
	}
	else if (pin_numb == 2)
	{
		pin = GPIO_PIN_13;
	}
	else if (pin_numb == 3)
	{
		pin = GPIO_PIN_14;
	}
	else if (pin_numb == 4)
	{
		pin = GPIO_PIN_15;
	}
	if (reset_ds18b20() == 0){
		return 0;
	}
	else{
	write_byte_ds18b20(0xCC); // SKIPROM
	write_byte_ds18b20(0x4E); // Write scratchpad
	write_byte_ds18b20(0x64); // 100 deg
	write_byte_ds18b20(0x9E); // -30 deg
	write_byte_ds18b20(0x7F); // 12 bit
	return 1;
	}
}

void measure_cmd_ds18b20(void)
{
	reset_ds18b20();
	write_byte_ds18b20(0xCC); // SKIPROM
	write_byte_ds18b20(0x44); //CONVERT T

}

void read_stratcpad_ds18b20(uint8_t *Data)
{

  uint8_t i;

	reset_ds18b20();

	write_byte_ds18b20(0xCC);// SKIPROM

	write_byte_ds18b20(0xBE);//READ SCRATCHPAD

  for(i=0; i<8; i++)

  {
    Data[i] = read_byte_ds18b20();
  }
}

uint8_t get_sign_ds18b20(uint16_t sign)
{
	 if (sign &(1<<11)) return 1;

	  else return 0;
}

float ds18b20_Convert(uint16_t raw_temp)
{
	float t;

	  t = (float) ((raw_temp & 0x07FF) >> 4); //отборосим знаковые и дробные биты

	  t += (float)(raw_temp & 0x000F) / 16.0f;	  //Прибавим дробную часть

	  return t;
}

void delay_us(int delay_value)
{
	TIM4->ARR = delay_value;
	TIM4->CNT = 0;
	while (TIM4->CNT < delay_value) {}
	TIM4->ARR = 0;

}



