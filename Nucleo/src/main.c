/**
  ******************************************************************************
  * @file    main.c
  * @author  Arkadiusz Merta
  * @version V0.1
  * @date    10-April-2016
  * @brief   FibocomG510-based GSM extension from MikroTar.pl
  * This code is based on:
  * forbot.pl        : http://forbot.pl/blog/artykuly/programowanie/stm32-praktyce-1-platforma-srodowisko-id2733
  * STM32F4 Discovery: http://stm32f4-discovery.com/2014/04/library-04-connect-stm32f429-discovery-to-computer-with-usart/
  ******************************************************************************
*/
#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>

//**************************
//Number of WD (watchdog) ticks before sensing out-of-data
#define WD_RETRIES 200
//Delay (in ms) between watchdog ticks
#define WD_DELAY 1
#undef DEBUG

const char retOK[]    = {"OK"};
const char retERROR[] = {"ERROR"};

/******************************************
 * Delay function: wait timeMs for further processing
 * timeMs: number of miliseconds to wait
 */
volatile uint32_t timer_ms = 0;
void SysTick_Handler(){
    if (timer_ms) {
        timer_ms--;
    }
}

void delay(int timeMs){
    timer_ms = timeMs;
    while(timer_ms > 0){};
}

/***************************
 * Override for printf on USART2
 */
int __io_putchar(int c)
{
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	USART_SendData(USART2, c);
    return c;
}

/***************************
 * Send a string of characters to USART1. Please do remember
 * about the trailing #13#10
 * USARTx: USART port
 * txt: input string
 */
void send(USART_TypeDef* USARTx, const char* txt)
{
    while( *txt ){
    	while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
    	USART_SendData(USARTx, *txt++);
    }

}

/***************************
 * Arduino IDE-like, loop function
 */
void sendATcommand(
		USART_TypeDef* USARTx,
		const char* cmd,
		const char* pars,
		const char* postEcho,
		char* bufRet,
		int bufRetSize ) {
	printf("-----------------\r\n");
	//Send AT command to G510; AT commands must finish with \r\n
	printf("Sending command: %s\r\n", cmd);

	send(USARTx, cmd );
    if( pars )send(USARTx, pars);

	//Initialize variables
	unsigned int watchdog = WD_RETRIES;
	int bufPos = 0;
	const int bufSize = 1000;
	char bufNow[bufSize];
	char bufString[bufSize];

	while(1){
		if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE)==SET){
			char c = USART_ReceiveData(USART1);
			if( c == '\n')continue;
			bufNow[ bufPos++ ] = c;

		}else{
			//Nothing received, let's wait a while
			watchdog -= 1;
			delay( WD_DELAY );
		}
		if( watchdog == 0 || ( bufPos + 1 == bufSize ) ){
			break;
		}
	}
	char *beg = bufNow;
	char *buf = bufNow;
	int len = 0;
	for(int i=0; i < bufPos; i++, len++){
		char c = *buf++;
#ifdef DEBUG
		if(isalnum(c)){
			printf("-%c{%d}",c,(int)c);
		}else
			printf("-{%d}",(int)c);
#endif
		if( c == '\r' ){
			//End of line or buffer overrun, print output
			strncpy(bufString, beg, len);
			bufString[len]='\0';
#ifdef DEBUG
			printf("\r\nReceived [%d]: %s\r\n", len, bufString);
#endif
			if( !strncmp(bufString, retOK, strlen(retOK)) ){
				printf("-->>Received OK\r\n");
			}else
			if( !strncmp(bufString, retERROR, strlen(retERROR)) ){
				printf("-->>Received ERROR\r\n");
			}
			else
			if( !strncmp(bufString, cmd, strlen(cmd)-2) ){
				printf("-->>Received echo\r\n");
			}else if( len>4 && !strncmp(bufString, cmd+2, strlen(cmd)-4) ){

				if( bufRet ){
					int aLen = strlen(cmd)-4+1;
					printf("-->>Received length: %d\r\n", aLen);
					strncpy(bufRet, bufString+aLen, len - aLen );
					printf("-->>Received result: %s|\r\n", bufRet);
				}

			}
			len = 0;
			beg = buf;
		}

	}

}

/*****************************************
 * Arduino IDE-like setup function. Prepares
 * SysTick, USART1 and USART2
 */
void setup() {
	//Configure tick timer (for delay function)
	SysTick_Config(SystemCoreClock / 1000);

	// Enable clocks for GPIOA and USART1
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	//Set alternate functions for PA9 and PA10
   	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

	//Initialize PA9 and PA10 with alternate functions
	GPIO_InitTypeDef gpio;
	gpio.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_9;
	gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpio);

	//Set UART1 parameters - G510 uses auto-config; here: 9600 is default
	USART_InitTypeDef usartInit;
	USART_StructInit( &usartInit );

	//Init and enable USART1
    USART_Init(USART1, &usartInit);
    USART_Cmd(USART1, ENABLE);

    //USART2 on PA2/PA3
    //Enable clock for USART2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

	//Configure PA2 and PA3
	gpio.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3;
	gpio.GPIO_Mode = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_PP;
	gpio.GPIO_PuPd = GPIO_PuPd_UP;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpio);

	//Set UART2 parameters - G510 uses auto-config; here: 9600 is default
	USART_StructInit( &usartInit );
	USART_Init(USART2, &usartInit);
	USART_Cmd(USART2, ENABLE);
	printf("================\r\n");
	printf("Setup completed.\r\n");

}
/*****************************************
 * Main function
 */
int main(void)
{
	static int _bufSize=200;
	char bufRet[_bufSize];
	setup();
	for(;;){

		sendATcommand(USART1, "AT\r\n",      NULL, NULL, bufRet, _bufSize);
		sendATcommand(USART1, "AT+CGMI\r\n", NULL, NULL, bufRet, _bufSize);
		sendATcommand(USART1, "AT+CGMM\r\n", NULL, NULL, bufRet, _bufSize);
		sendATcommand(USART1, "AT+CGMR\r\n", NULL, NULL, bufRet, _bufSize);
		sendATcommand(USART1, "AT+CGSN\r\n", NULL, NULL, bufRet, _bufSize);

		delay(10000);

	}
}
