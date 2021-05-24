/*
 * FreqRespiracao.c
 *
 * Aluna: Ana Rita Diniz da Cruz (118110418)
 * Sprint 9
 */ 

#define BAUD 9600
#define MYUNRR F_CPU/16/BAUD-1
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "LedsAnimation.h"
#include "nokia5110.h"

int angle = 0;
int posicao = 0, direcao = 1, periodo;
int FreqResp = 20, FreqCardio = 0, O2 = 0, O2resp = 0, vol = 6, pausa = 300, sensib = 5;
int tempSensib = 250;
unsigned int leitura;
long long int tempo_ms = 0, start_T, final_T, inicio, esfInicial;
float V, Temp=30.0, half_T;

char strModo[12] = "Controlado ";
char strModo1[12] = "Controlado ";
char strModo2[12] = "Ctrl/Assist";
int mude_p1 = 0, mude_p0 = 1;
int modo = 0;
char numero[4] = "   ";

char dado;
char informacao[8] = "        ";
char strErro[8] = " ERRO*  ";
char strPressao[8] = "120x80 ";

int estagio = 0, futuro = 0;
int i = 0, j = 0, p , linha = 0, col;

char counter = 0x00;

int sel = 0;
int limpa;

void ctrlRespiracao();
void convLeitura();
void vitaisLCD();
void atualizaLCD();
void respLCD();
void O2LCD();
void volLCD();
void sensibLCD();
void pausaLCD();

void USART_init(unsigned int ubrr);
int fluxo(char[]);
void analise(int, char[]);

void readTecl();
void guarda (int, int);
void mudaParametro();

int main(void)
{
	DDRB = 0b11111110; // Pinos da porta B como saídas e entradas
	PORTB = 0b11110001; 
	
	DDRC = 0b1111100; // Bits mais significativos da porta C como saída (LCD) e os dois menos como entrada (sensores)

	DDRD = 0b00100000; // Entradas e saídas da porta D
	
	PORTD |= 0b11010111; // Ativação de pull-up
	
	PCICR = 0b00000101; // Interrupção na porta D
	PCMSK2 = 0b11010000; // Interrupção em mudança de D4, D6 ou D7
	PCMSK0 = 0b00000001;
	
	EICRA = 0b00000100; //interrupção externa INT1 na borda de subida
	EIMSK = 0b00000010;
	
	TCCR0A = 0b00000010; // configura modo CTC, comparando com OCR0A
	TCCR0B = 0b00000011; // liga TC0 com prescaler = 64
	OCR0A = 249; // ajusta comparador para 249
	TIMSK0 = 0b00000010; // habilita interrupção
	
	ICR1 = 39999;
	TCCR1A = 0b10100010;
	TCCR1B = 0b00011010;
	
	OCR1A = 2000;
	OCR1B = 2000;
	
	ADMUX = 0b01000000;
	ADCSRA = 0b11101111;
	ADCSRB = 0b00000000;
	DIDR0 = 0b00111110;
	
	TCCR2A = 0x00;   //Timer operando em modo normal
	TCCR2B = 0x07;   //Prescaler 1:1024
	TCNT2  = 100;    //10 ms overflow again
	TIMSK2 = 0x01;   //Habilita interrupção do Timer2
	

	USART_init(MYUNRR);
	
	sei(); // Habilita as interrupções

	nokia_lcd_init();
	nokia_lcd_clear();
	vitaisLCD();

while (1)
{	
	if(PINB == (PINB | 0b00001000)){
		_delay_ms(150);
		PORTB = (PORTB & 0b11110111);
	}
	
	if (tempo_ms%200==0){
		convLeitura();
		atualizaLCD();
	}
	if (tempo_ms>start_T+(2*half_T)+200){
		final_T = tempo_ms;
		int T = final_T - start_T; // período em ms
		FreqCardio = (1.0/T)*60000; // conversão de KHz para bpm
	}
}
}

ISR(TIMER2_OVF_vect)
{
	TCNT2=100;         
	counter++;         
	
	if(counter == 0x05) 
	{                  
		counter = 0x00;  
		readTecl(); 
	} //end if counter
	

}

void USART_init(unsigned int ubrr){
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = 0b10011000;
	UCSR0C = 0b00000110;
}

ISR(USART_RX_vect){
	dado = UDR0;
	analise(fluxo(informacao), informacao);
}

int fluxo(char informacao[]){
	char n[11] = "0123456789";
	estagio = futuro;
	
	switch(estagio){
	
	case 0: // INICIO DO PACOTE
		i = 0;
		if(dado == ';') futuro = 1;
		else futuro = 9;
		break;
	
	case 1://H1
		futuro = 9;
		for(int j = 0;j < 10;j++){
			if(dado == n[j]){
				futuro = 2;
				informacao[i] = dado;
				i++;
			}
		}
	break;
	
	case 2: //H2
		if(dado == 'x'){
			futuro = 5;
			informacao[i] = dado;
			i++;
		}
		else futuro = 9;
		for(int j = 0;j < 10;j++){
			if(dado == n[j]){
				futuro = 3;
				informacao[i] = dado;
				i++;
			}
		}
	break;
	
	case 3: // H3
		if(dado == 'x'){
			futuro = 5;
			informacao[i] = dado;
			i++;
		}
		else futuro = 9;
		for(int j = 0;j < 10;j++){
			if(dado == n[j]){
				futuro = 4;
				informacao[i] = dado;
				i++;
			}
		}
	break;
			
	case 4:// x
		if(dado == 'x'){
			futuro = 5;
			informacao[i] = dado;
			i++;
		}
		else futuro = 9;
	break;
	
	case 5:// L1
		futuro = 9;
		for(int j = 0;j < 10;j++){
			if(dado == n[j]){
				futuro = 6;
				informacao[i] = dado;
				i++;
			}
		}
	break;
	
	case 6:  // L2
		if(dado == ':') futuro = 0;
		else futuro = 9;
		for(int j = 0;j < 10;j++){
			if(dado == n[j]){
				futuro = 7;
				informacao[i] = dado;
				i++;
			}
		}
	break;
	
	case 7: // L3
		if(dado == ':') futuro = 0;
		else futuro = 9;
		for(int j = 0;j < 10;j++){
			if(dado == n[j]){
				futuro = 8;
				informacao[i] = dado;
				i++;
			}
		}
	break;
	
	case 8: // final do pacote
		if(dado == ':') futuro = 0;
		else futuro = 9;
	break;
	
	case 9:  // erro
		if(dado == ';') futuro = 1;
		else futuro = 9;
		i = 0;
	break;
	}
	
	return futuro;
}

void analise(int sequencia, char informacao[]){
	int i;
	if(sequencia == 9){
		for(i=0;i<7;i++){
			strPressao[i] = strErro[i];
			informacao[i] = ' ';
		}
	}
	else if (sequencia == 0){
		for(i=0;i<7;i++){
			strPressao[i] = informacao[i];
			informacao[i] = ' ';
		}
	}
}

void ctrlRespiracao(){
	if (posicao == vol){ // Teste para mudar de direção
		if(!direcao){
			if(mude_p1)
				modo = 1;
			else if (mude_p0)
				modo = 0;
		}
		posicao = 0;
		direcao ^= 1;
	}

	if(direcao)
		angle = 2000 + posicao*250; // Inspiração
	else
		angle = 2000 + (vol*250) - posicao*250; // Expiração
	
	OCR1A = angle;
		
	if(angle == 2000){
		PORTD |= 0b00100000;
		_delay_ms(pausa);
		PORTD &= 0b11011111;
	}
	
	posicao++; // Ação sob o próximo led
}

void convLeitura(){
	
	if(ADMUX == 0b01000000)
		Temp = 10*(V+1.0);
		
	else if(ADMUX == 0b01000001)
		O2 =  20*V;

	if((O2 < 60)||(Temp < 35)||(Temp > 41))
		PORTD |= 0b00100000;
	else
		PORTD &= 0b11011111;
}

ISR(ADC_vect){
	leitura = ADC;
	V = (leitura*5.0)/(1023.0);
}

ISR(PCINT0_vect){
	if(modo == 1){
		if(PINB == (PINB & 0b11111110)) // descida do pino B0
			esfInicial = tempo_ms;
	}
}


ISR(PCINT2_vect)
{	
	int k;
	
	if (PIND == (PIND & 0b11101111)){
		sel ++;
		limpa = 1;
		if (sel == 6) sel = 0; 
	}
	
	if (limpa){
		nokia_lcd_clear();
		nokia_lcd_render();
	}
	limpa = 0;
	
	if (sel != 0){
		if (PIND == (PIND & 0b10111111)) // foi borda de descida de D6?
		{
			mude_p0 = 1;
			mude_p1 = 0;
			for (k = 0; k < 12; k++)
				strModo[k] = strModo1[k];
		}
		if (PIND == (PIND & 0b01111111)) // foi borda de descida de D7?
		{
			mude_p0 = 0;
			mude_p1 = 1;
			for (k = 0; k < 12; k++)
				strModo[k] = strModo2[k];
		}
	}
}

ISR(TIMER0_COMPA_vect)
{
	tempo_ms ++;
	
	periodo = (60000/(2*vol*FreqResp));
	
	if(modo==1){
		if(tempo_ms == esfInicial+tempSensib){
			p = 0;
			PORTB = (PORTB | 0b00001000);
		}
	}
	
	if(modo == 1){
		if(p<(2*vol)){
			if (tempo_ms%periodo==0){
				ctrlRespiracao();
				p++;
			}
		}
	}
	
	if(p==2*vol){
		if (mude_p0)
			modo = 0;
	}
	
	if (modo == 0){
		if (tempo_ms%periodo==0)
			ctrlRespiracao();
	}
	
	if(tempo_ms%150==0){
		ADMUX ^= 0b00000001;
		DIDR0 ^= 0b00000011;
	}
	
}

ISR (INT1_vect)
{
	if (PIND == (PIND | 0b00001000)) //borda de subida?
		start_T = tempo_ms - 15;
	else if (PIND == (PIND & 0b11110111)) //borda de descida?
	{
		final_T = tempo_ms - 15; // final do período
		half_T = final_T - start_T; // período em ms
		FreqCardio = (1.0/half_T)*30000; // conversão de KHz para bpm
	}
}

void vitaisLCD(){

	char bpm[3], o2[2];
	char str2[2], str1[1];
	int Temp_int = Temp;
	int aux = 10*Temp;
	int Temp_frac = aux%10;
	
	nokia_lcd_set_cursor(2,1);
	nokia_lcd_write_string("SINAIS VITAIS", 1);
	
	nokia_lcd_set_cursor(5,10);
	itoa(FreqCardio, bpm, 10);
	nokia_lcd_write_string("    ", 1);
	nokia_lcd_set_cursor(5,10);
	nokia_lcd_write_string(bpm, 1);
	nokia_lcd_set_cursor(45,10);
	nokia_lcd_write_string("bpm", 1);
	
	nokia_lcd_set_cursor(5,19);
	itoa(O2, o2, 10);
	nokia_lcd_write_string("    ", 1);
	nokia_lcd_set_cursor(5,19);
	nokia_lcd_write_string(o2, 1);
	nokia_lcd_set_cursor(45,19);
	nokia_lcd_write_string("%SpO2", 1);
	
	nokia_lcd_set_cursor(5,28);
	nokia_lcd_write_string("    ", 1);
	nokia_lcd_set_cursor(5,28);
	itoa(Temp_int, str2, 10);
	nokia_lcd_write_string(str2, 1);
	nokia_lcd_set_cursor(18,28);
	nokia_lcd_write_string(".",1);
	nokia_lcd_set_cursor(23,28);
	itoa(Temp_frac, str1, 10);
	nokia_lcd_write_string(str1, 1);
	nokia_lcd_set_cursor(45,28);
	nokia_lcd_write_string("°C", 1);
	
	nokia_lcd_set_cursor(1,37);
	nokia_lcd_write_string(strPressao,1); 
	nokia_lcd_set_cursor(45,37);
	nokia_lcd_write_string("mmHg", 1);
	
	nokia_lcd_render();
}

void atualizaLCD(){
	if (sel == 0)
		vitaisLCD();
	else if (sel == 1)
		respLCD();
	else if (sel == 2)
		O2LCD();
	else if (sel == 3)
		volLCD();
	else if (sel == 4)
		pausaLCD();
	else if (sel == 5){
		if(modo == 1)
			sensibLCD();
		else sel = 0;
	}
}

void respLCD(){
	char str2[3], str1[2];
	
	nokia_lcd_set_cursor(10,1);
	nokia_lcd_write_string("PARAMETROS", 1);
	
	nokia_lcd_set_cursor(10,10);
	nokia_lcd_write_string(strModo, 1);
	
	itoa(FreqResp,str2,10);
	nokia_lcd_set_cursor(5,20);
	nokia_lcd_write_string("  ", 1);
	nokia_lcd_set_cursor(5,20);
	nokia_lcd_write_string(str2, 1);
	nokia_lcd_set_cursor(20,20);
	nokia_lcd_write_string("* resp/min", 1);
	
	itoa(O2resp, str2, 10);
	nokia_lcd_set_cursor(5,30);
	nokia_lcd_write_string(str2, 1);
	nokia_lcd_set_cursor(30,30);
	nokia_lcd_write_string("%O2", 1);
	
	itoa(vol, str1, 10);
	nokia_lcd_set_cursor(5,40);
	nokia_lcd_write_string(str1, 1);
	nokia_lcd_set_cursor(30,40);
	nokia_lcd_write_string("vol", 1);
	
	nokia_lcd_render();
}

void O2LCD(){
	char str2[3], str1[2], str3[4];
	
	nokia_lcd_set_cursor(10,1);
	nokia_lcd_write_string("PARAMETROS", 1);
	
	nokia_lcd_set_cursor(10,10);
	nokia_lcd_write_string(strModo, 1);
	
	itoa(FreqResp, str2, 10);
	nokia_lcd_set_cursor(5,20);
	nokia_lcd_write_string(str2, 1);
	nokia_lcd_set_cursor(30,20);
	nokia_lcd_write_string("resp/min", 1);
	
	itoa(O2resp,str3,10);
	nokia_lcd_set_cursor(5,30);
	nokia_lcd_write_string("   ", 1);
	nokia_lcd_set_cursor(5,30);
	nokia_lcd_write_string(str3, 1);
	nokia_lcd_set_cursor(20,30);
	nokia_lcd_write_string("* %O2", 1);
	
	itoa(vol, str1, 10);
	nokia_lcd_set_cursor(5,40);
	nokia_lcd_write_string(str1, 1);
	nokia_lcd_set_cursor(30,40);
	nokia_lcd_write_string("vol", 1);
	
	nokia_lcd_render();
}

void volLCD(){
	char str2[3], str1[2];
	
	nokia_lcd_set_cursor(10,1);
	nokia_lcd_write_string("PARAMETROS", 1);
	
	nokia_lcd_set_cursor(10,10);
	nokia_lcd_write_string(strModo, 1);
	
	itoa(FreqResp, str2, 10);
	nokia_lcd_set_cursor(5,20);
	nokia_lcd_write_string(str2, 1);
	nokia_lcd_set_cursor(30,20);
	nokia_lcd_write_string("resp/min", 1);
	
	itoa(O2resp, str2, 10);
	nokia_lcd_set_cursor(5,30);
	nokia_lcd_write_string(str2, 1);
	nokia_lcd_set_cursor(30,30);
	nokia_lcd_write_string("%O2", 1);
	
	itoa(vol,str1,10);
	nokia_lcd_set_cursor(5,40);
	nokia_lcd_write_string(" ", 1);
	nokia_lcd_set_cursor(5,40);
	nokia_lcd_write_string(str1, 1);
	nokia_lcd_set_cursor(20,40);
	nokia_lcd_write_string("* vol", 1);
	
	nokia_lcd_render();
}

void pausaLCD(){
	char str2[2], str3[4];
	
	nokia_lcd_set_cursor(10,1);
	nokia_lcd_write_string("PARAMETROS", 1);
	
	nokia_lcd_set_cursor(5,10);
	nokia_lcd_write_string("* Pausa Insp.", 1);
	
	itoa(pausa,str3,10);
	nokia_lcd_set_cursor(5,20);
	nokia_lcd_write_string("   ", 1);
	nokia_lcd_set_cursor(5,20);
	nokia_lcd_write_string(str3, 1);
	nokia_lcd_set_cursor(40,20);
	nokia_lcd_write_string("ms", 1);
	
	if (modo == 1){
		nokia_lcd_set_cursor(5,30);
		nokia_lcd_write_string("Sensib.", 1);
	
		itoa(sensib, str2, 10);
		if (sensib<10){
			str2[2] = ' ';
			str2[1] = str2[0];
			str2[0] = '0';
		}
		nokia_lcd_set_cursor(5,40);
		nokia_lcd_write_string("-0.", 1);
		nokia_lcd_set_cursor(23,40);
		nokia_lcd_write_string(str2, 1);
		nokia_lcd_set_cursor(40,40);
		nokia_lcd_write_string("cmHg", 1);
	}
	
	nokia_lcd_render();
}

void sensibLCD(){
	char str2[3], str3[4];
	
	nokia_lcd_set_cursor(10,1);
	nokia_lcd_write_string("PARAMETROS", 1);
	
	nokia_lcd_set_cursor(5,10);
	nokia_lcd_write_string("Pausa Insp.", 1);
	
	itoa(pausa, str3, 10);
	nokia_lcd_set_cursor(5,20);
	nokia_lcd_write_string(str3, 1);
	nokia_lcd_set_cursor(40,20);
	nokia_lcd_write_string("ms", 1);
	
	if (modo == 1){
		nokia_lcd_set_cursor(5,30);
		nokia_lcd_write_string("* Sensib.", 1);
	
		itoa(sensib, str2, 10);
		if (sensib<10){
			str2[2] = ' ';
			str2[1] = str2[0];
			str2[0] = '0';
		}
		nokia_lcd_set_cursor(5,40);
		nokia_lcd_write_string("-0.", 1);
		nokia_lcd_set_cursor(23,40);
		nokia_lcd_write_string("  ", 1);
		nokia_lcd_set_cursor(23,40);
		nokia_lcd_write_string(str2, 1);
		nokia_lcd_set_cursor(40,40);
		nokia_lcd_write_string("cmHg", 1);
	}
	
	nokia_lcd_render();
}

void readTecl(){
		
	if (linha == 0){
		PORTB = (PORTB & 0b01111111);
		PORTB = (PORTB | 0b01110000);
	}
	else if (linha == 1){
		PORTB = (PORTB & 0b10111111);
		PORTB = (PORTB | 0b10110000);
	}
	else if (linha == 2){
		PORTB = (PORTB & 0b11011111);
		PORTB = (PORTB | 0b11010000);
	}
	else if (linha == 3){
		PORTB = (PORTB & 0b11101111);
		PORTB = (PORTB | 0b11100000);
	}
		
	if (PIND == (PIND & 0b11111110)){
		 col = 0;
		 guarda(linha,col);
	}
	else if(PIND == (PIND & 0b11111101)){
		 col = 1;
		 guarda(linha,col);
	}
	else if(PIND == (PIND & 0b11111011)){
		col = 2;
		guarda(linha,col);
	}
	linha++;
	if(linha == 4)
		linha = 0;
}

void guarda (int linha, int col){
	char matriz[4][3] =
	{
		{'1','2','3'},
		{'4','5','6'},
		{'7','8','9'},
		{' ','0',' '}
	};
	 TIMSK2 = 0x00; 
	_delay_ms(20);
	
	if (linha == 3 & col == 0)
		j--;
	
	if (linha == 3 & col == 2)
		mudaParametro();
	
	numero[j] = matriz[linha][col];
	j++;
		
	TIMSK2 = 0x01;
}

void mudaParametro(){
	j = 0;
	
	switch(sel){
	case 1: 
		FreqResp = atoi(numero);
		if (FreqResp<5) FreqResp = 5;
		else if (FreqResp>30) FreqResp = 30;
		break;
	case 2:
		O2resp = atoi(numero);
		OCR1B = 2000 + O2resp*20;
		if (O2resp>100) O2resp = 100;
		break;
	case 3:
		vol = atoi(numero);
		if (vol>8) vol = 8;
		break;
	case 4:
		pausa = atoi(numero);
		if (pausa>500) pausa = 500;
		else if (pausa<300) pausa = 300;
		break;
	case 5:
		sensib = atoi(numero);
		tempSensib = 50 * sensib;
		if (sensib>20) sensib = 20;
		else if (sensib<5) sensib = 5;
		break;	
	}
	for(int i = 0; i<4; i++)
		numero[i] = " ";
}