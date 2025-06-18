/*******************************************************************************

  SENHA TOC-TOC

  Senha de acesso feita atrav�s de batidas na porta (toc-toc).

  Usa piezo como sensor (colado na porta)

  Utiliza um rel� na sa�da para ligar uma fechadura el�trica

  Autor: Claudio L�rios

  Data: 17/11/2016

  Uso: Primeiro dever� ser gravado uma senha de toc-tocs.
  Aperte o bot�o sw_prog moment�neamente. O led prog se acender� indicando
  modo de programa��o de senha.
  Bata na porta (ou local que est� colado o sensor piezo) com tocs que n�o ul-
  trapassem o total de 20 e que com tempo entre tocs menores que 3,6 segundos.
  Ap�s inserir a senha de tocs espere o led apagar.
  Para testar, bata na porta com o mesmo n�mero de tocs e na mesma sequ�ncia,
  imitando o mais fiel poss�vel a senha gravada. Se correta ir� acionar a sa�da
  do rel�.
  O modo de opera��o da sa�da poder� ser escolhido entre Pulso ou Reten��o, co-
  mentando o #define SAIDA_PULSO abaixo.
  O disco piezoel�trico (diametro de 20 mm) usado como sensor de toque deve ser
  colado no local que receber� os toc-tocs. Poder� usar cola quente (silicone).


  Uso did�tico.

  Este arquivo � parte integrante do blog Larios.Tecnologia.ws

********************************************************************************
Escolha do usu�rio:
Descomente o abaixo para modo pulso na sa�da do rele (recompile o arquivo para
 obter o novo arquivo hex).                                                 */

       #define SAIDA_PULSO


/*****************************************************************************/
#include <12F675.h>
#device adc=8
#use delay(clock=4000000)
#fuses NOWDT,INTRC_IO, NOCPD, NOPROTECT, NOMCLR, NOPUT, NOBROWNOUT


#byte gpio   = 0x5
#byte trisio = 0x85
#byte tmr1h  = 0x0f
#byte tmr1l  = 0x0e
#byte option_reg  = 0x81
#byte wpu  = 0x95
#bit  wpu_0 = 0x95.0
#bit  tmr1_on = 0x10.0

#bit  piezo   = 0x5.2 //pino 5  -entrada/saida do disco piezo
#bit  tris_p  = 0x85.2//pino 5  - sentido do pino 5 como entrada ou sa�da
#bit  sw_gr   = 0x5.0 //pino 7  - bot�o para gravar senha
#bit  led_gr  = 0x5.4 //pino 3  - led indicador de modo de grava��o de senha
#bit  rele    = 0x5.5 //pino 2  - rele que ligar� a fechadura el�trica
#bit  led_op   = 0x5.1 //pino 6 - led monitor de opera��o

//vari�veis globais
unsigned int8 valor[20];//buffer de tocs
unsigned int8 tmr1u=0;//quantidade de vezes que houve interrup��o do timer 1
unsigned int8 qtoc=0;//indica qual toc est� sendo apurado
unsigned int1 f_tmax=0;//flag indicador de overflow dos contadores de tempo (3,6 seg)

//defines
#define N_TOC_MAX 20  // n�mero de toques m�ximo
#define TOLERANCIA 5 // diferen�a suport�vel entre senha gravada e a de acesso
#define TEMPO_RELE_LIGADO 5000//Tempo que o rele fica ligado -5 seg (modo pulso)


/*******************************************************************************
             ROTINA GERADORA DE SOM NO DISCO PIEZO
*******************************************************************************/
void som(){//gera som para indicar sucesso na combina��o de toques
   unsigned int8 a;
   setup_adc_ports(NO_ANALOGS);//desliga port do adc
   tris_p=0;//torna pino como sa�da

    for(a=0;a<100;a++){
         piezo=1;
         delay_us(637);//784 hz sol
         piezo=0;
         delay_us(634);
    }
       delay_ms(100);
      for(a=0;a<100;a++){
         piezo=1;
         delay_us(758);//659 hz  mi
         piezo=0;
         delay_us(755);
      }
      delay_ms(100);
       for(a=0;a<100;a++){
         piezo=1;
         delay_us(1136);//440 hz la
         piezo=0;
         delay_us(1134);
      }
       delay_ms(100);

   //===============================================
   tris_p=1;//pino volta a ser entrada
   setup_adc_ports(sAN2|VSS_VDD);//volta a usar o adc do pino 5
   delay_ms(100);//delay
}

/*******************************************************************************
                      LE ADC - TESTA SW PROG
*******************************************************************************/
void le_adc_sw(){

 while(read_adc()<4){
     if(!sw_gr)led_gr=1;//testa interruptor de grava��o
     if(f_tmax){led_op=0;break;}//se tempo m�ximo entre toc-toc foi alcan�ado sai desta rotina
     delay_us(50);
   }//aguarda toc-toc
   delay_ms(30);//espera finalizar o toc-toc atual
   if(!qtoc && !f_tmax){//se houve um toc
      tmr1l=0;//reseta para uma nova contagem entre tocs
      tmr1u=0;
      tmr1h=0;
   }
}

/*******************************************************************************
                 ROTINA DE INTERRUP��O DO TIMER 1
*******************************************************************************/

#int_timer1
void isr_timer1(){
   tmr1u++; //incrementa este contador a cada 524288 useg
   if(tmr1u>=7){
      f_tmax=1;//indica tempo m�ximo no intervalo (3,6seg)
   }
}

/*******************************************************************************
             ROTINA   MAIN
*******************************************************************************/

void main() {


   setup_adc_ports(sAN2|VSS_VDD);//ajusta port para anal�gico no pino 5
   setup_adc(ADC_CLOCK_INTERNAL);//clock do adc
   setup_counters(RTCC_INTERNAL,RTCC_DIV_1);//n�o usado
   setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);//timer 1 div/8 interno
   setup_comparator(NC_NC_NC_NC);//desliga comparadores
   setup_vref(FALSE);//desliga vref
   set_adc_channel(2);//ajusta canal para o conversor ad para o pino 5
   gpio=0; //desliga todas as sa�das
   trisio=0b001101;//saidas e entradas
   option_reg=0;//ativa pull up geral
   wpu_0=1;//liga pullup do pino 7
   set_timer1(0);
   enable_interrupts(INT_TIMER1);//liga interrup��o do timer 1
   enable_interrupts(GLOBAL);//liga geral das interrup��es
   tmr1_on=1;//liga timer 1
   f_tmax=0;//reseta flag de tempo m�ximo
   qtoc=0;//zera contador

/*******************************************************************************
            LA�O DE REPETI��O
*******************************************************************************/

   for(;;){
   unsigned int8 tempo,a,z,b;
   signed int16 x,y;
   le_adc_sw();//testa por algum toc-toc ou estouro de tempo (testa sw de programa��o)

   if(!f_tmax){//foi um  toc-toc
      led_op=1;//led de opera��o ligado
      if(qtoc>1) {
         tempo=tmr1h;//salva valor do timer1h
         tempo=tempo>>3;//desloca 3 casas a direita
         tmr1u=tmr1u<<5;//desloca 5 casas para esquerda
         tmr1u=tmr1u&0b11100000;//mant�m apenas os 3 digitos superiores
         tempo=tempo|tmr1u;//concatena
         b=qtoc-2;//elimina contagem morta
         valor[b]=tempo; //salva no array
      }//if(qtoc>1)

      qtoc++;//incrementa n�mero de tocs
      if(qtoc>N_TOC_MAX)qtoc=0;//limita n�mero de toc-tocs
      tmr1l=0;
      tmr1u=0;
      tmr1h=0;
   }//if(!f_tmax)
   else{//foi pelo tempo maximo alcan�ado

     if(qtoc>2){//somente se tiver tido algum toc-toc salvo
       qtoc-=3;////elimina contagem morta
       if(led_gr){//salvar na eeprom

        write_eeprom(127,qtoc);   //salva numero de toc-tocs
        for(a=0;a<=qtoc;a++){ //ok
        write_eeprom(a,valor[a]);//salva na eeprom
        }
         led_gr=0;//sai do modo de programa��o
         qtoc=0;
       }
       else{ //rotina que fara a compara��o das batidas

          z=0;
          for(a=0;a<qtoc;a++){//estava a<=qtoc
           x=read_eeprom(a);
           y=valor[a];
           if((y<=x+TOLERANCIA) && (y>=x-TOLERANCIA))z++;
          }


         if((a==z)&& (read_eeprom(127)==a)){//compara se toc foram aceitos e o n�mero correto de tocs

          #ifdef SAIDA_PULSO
          //modo pulso
          rele=1;//ativa rele
          som();
          delay_ms(TEMPO_RELE_LIGADO);
          rele=0;//desativa rele
          #else
          //modo reten��o
          rele=!rele; //inverte a sa�da
          som();
          #endif
         }
     }//else compara
    }//if(qtoc)
        tmr1u=0;
        tmr1l=0;
        tmr1h=0;
        f_tmax=0;
        qtoc=0;//reseta
   }//else

 }//for(;;)
}//main()
