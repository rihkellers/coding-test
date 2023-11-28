/*
 * CodingTest1.c
 *
 * Created: 11/27/2023 2:38:00 PM
 * Author : user
 */ 

#define F_CPU 16000000L
#define BUFFER_LENGTH 512
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

char input_buffer[BUFFER_LENGTH];
char input_char;
uint16_t read_spot = 0;
int outputPins = 0b00000000;
int pin_a = 0b00000000;
int pin_a_ms = 0;
int pin_b = 0b00000000;
int pin_b_ms = 0;
int test_a = 0;

void usart0_init(void)
{
	UCSR0B |= (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
	UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);

	uint16_t baudRate = 0;
	#if defined(BAUDRATE)
		baudRate = F_CPU/(uint16_t)BAUDRATE/16-1;
	#else
		baudRate = F_CPU/9600/16-1;
	#endif

	//USART0 Baud Rate Register
	UBRR0 = baudRate;
}

void pinSetA_init(void)
{
	#if defined(PINA_0)
	// Can be <= 7, but task required 4 alternatives
	if ((int)PINA_0 >= 0 && (int)PINA_0 <= 3) {
		outputPins ^= (0b00000001 << (int)PINA_0);
		pin_a |= (0b00000001 << (int)PINA_0);
	} else {
		outputPins ^= 0b00000001;
		pin_a |= 0b00000001;
	}
	#endif
	#if defined(PINA_1)
	if ((int)PINA_1 >= 0 && (int)PINA_1 <= 3) {
		outputPins ^= (0b00000001 << (int)PINA_1);
		pin_b |= (0b00000001 << (int)PINA_1);
	} else {
		outputPins ^= 0b00000010;
		pin_b |= 0b00000010;
	}
	#endif
	DDRA = outputPins;
	//DDRA = 0b11111111;
}

void usart0_send(char data)
{
	while (!(UCSR0A & (1 << UDRE0)));

	UDR0 = data;
}

void usart0_sendString(char *str)
{
	for(size_t i = 0; i < strlen(str); i++)
	{
		usart0_send(str[i]);
	}
}

void parse_command(void)
{

	char *str1, *token;
	char *saveptr1;
	bool mode_set = false;
	bool mode_echo = false;
	int* duration = 0;
	int length = 0;
	int j;
	char copy[BUFFER_LENGTH];
	strcpy(copy, input_buffer);

	usart0_sendString("\r\n");
	for (j = 1, str1 = input_buffer; ; j++, str1 = NULL) {
		token = strtok_r(str1, " ,", &saveptr1);
		if (token == NULL) break;
		//usart0_sendString(token);
		//usart0_sendString("\r\n");
		if (j == 1) {
			if (strcmp(token, "set-led") == 0) {
				usart0_sendString("set-led test\r\n");
				mode_set = true;
			} else if (strcmp(token, "echo") == 0) {
				mode_echo = true;
			} else {
				usart0_sendString("ERROR\r\n");
				break;
			}
		} else if (j == 2) {
			if (mode_set) {
				if (atoi(token) == 0) {
					PORTA |= pin_a;
					duration = &pin_a_ms;
				} else if (atoi(token) == 1) {
					PORTA |= pin_b;
					duration = &pin_b_ms;
				} else {
					usart0_sendString("ERROR\r\n");
					break;
				}
			} else if (mode_echo) {
				char *temp;
				long value = strtol(token,&temp,10);
				if (temp != token && *temp == '\0') {
					if (value >= 0 && value <= 300) {
						length = value;
					} else {
						usart0_sendString("ERROR\r\n");
						break;
					}
				} else {
					usart0_sendString("ERROR\r\n");
					break;
				}
			}
		} else if (j == 3) {
			if (mode_set) {
				char *temp;
				token[strcspn(token, "\r")] = 0;
				long value = strtol(token,&temp,10);
				if (temp != token && *temp == '\0') {
					if (value >= 1 && value < 5000) {
						*duration = value;
						usart0_sendString("OK\r\n");
					} else {
						usart0_sendString("ERROR\r\n");
						break;
					}
				} else {
					usart0_sendString("ERROR\r\n");
					break;
				}
			} else if (mode_echo) {			
				char *dest = strstr(copy, token);
				usart0_sendString(token);
				usart0_sendString("\r\n");
				usart0_sendString(copy);
				usart0_sendString("\r\n");
				usart0_sendString("data: ");
				
				int pos = dest - copy;
				while ( (input_buffer[pos] != '\r') && length != 0 ) {
					usart0_send(input_buffer[pos]);
					length--;
					pos++;
				}
				usart0_sendString("\r\n");
				usart0_sendString("OK\r\n");
				/*int substringLength = strlen(dest) - pos;
				if (substringLength <= 400) {
					usart0_sendString("ERROR\r\n");
					break;
				}
				if (length > substringLength) {
					length = substringLength;
				}
				strncpy(result, input_buffer + pos, length - pos );
				usart0_sendString(result);
				usart0_sendString("\r\n");*/
			}
		}
	}
	
	
	//usart0_sendString("\r\n");
	//usart0_sendString("TEST");
	//usart0_sendString("\r\n");
	
	
	/*
	if (strcmp(pt, "set-led ") == 0) {
		usart0_sendString("set-led\r\n");
	} else if (strcmp(pt, "echo ") == 0) {
		usart0_sendString("echo\r\n");
	} else {
		usart0_sendString("ERROR\r\n");
	}*/
}

ISR(USART0_RX_vect) {
	input_buffer[read_spot] = UDR0;
	read_spot++;
	if (read_spot == BUFFER_LENGTH) read_spot = 0;
	UDR0 = input_buffer[read_spot-1];
	
	if ((input_buffer[read_spot - 1] == '\r') || (input_buffer[read_spot - 2] == '\r')) {
		//PORTA ^= outputPins;
		cli();
		parse_command();
		//usart0_sendString("\n\r");
		//usart0_sendString("OK\n\r");
		memset(input_buffer, 0, sizeof input_buffer);
		read_spot = 0;
		sei();
	}
}

int main(void)
{
	usart0_init();
	pinSetA_init();
	sei();

    while (1) 
    {
		if (pin_a_ms == 0) {
			if (PORTA&pin_a) {
				usart0_sendString("led-off: 0\n\r");
				PORTA^=pin_a;
			}
		} else if (pin_a_ms > 0) {
			pin_a_ms--;
		}

		if (pin_b_ms == 0) {
			if (PORTA&pin_b) {
				usart0_sendString("led-off: 1\n\r");
				PORTA^=pin_b;
			}
		} else if (pin_b_ms > 0) {
			pin_b_ms--;
		}

		_delay_ms(1);	

    }
}

