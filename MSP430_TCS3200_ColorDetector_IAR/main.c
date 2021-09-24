
#include "io430.h"
#include "lcdLib.h"
#include <stdint.h>
#include <stdio.h>

// Function prototypes
void InitClocks(void);
void InitTCS3200(void);
void SetUnusedPinsasInputviaPullUpResistors(void);
void InitTimer1(void);
void StartTCS3200(void);
uint16_t ComputeMax(uint16_t const arr[]);
void DecideColorandUpdateLCDContents(const uint16_t  sampleRGB[]);
void ComputeRGBPercentagesbyClear(const uint16_t TCS3200_out[4], uint16_t RGB_out[3]);

// Global variables
char lcd_out_row1[17] = {0};
char lcd_out_row2[17] = {0};
uint16_t TCS3200_out[4] = {0};

volatile uint16_t frequency = 0;
volatile uint8_t TimetoStartTCS3200 = 0;
volatile uint8_t measurement_done = 0;


int main( void )
{
  uint16_t RGB_out[3] = {0};  
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
    
  InitClocks();
  InitTCS3200();
  
  lcdInit();
  lcdClear();
  lcdSetText("Kadir Yetis",0,0);
  lcdSetText("140208013",0,1);
  
  SetUnusedPinsasInputviaPullUpResistors();
  InitTimer1();
  
  while(1)
  {
    __low_power_mode_3();

    if(TimetoStartTCS3200 == 1)
    {
      TimetoStartTCS3200 = 0;
      StartTCS3200();
      __low_power_mode_1();      
    }     
        
    if(measurement_done==1)
    {
      measurement_done = 0;      
      //Compute percentages by Clear filter
      ComputeRGBPercentagesbyClear(TCS3200_out, RGB_out);
   
      sprintf(lcd_out_row1, "%d,%d,%d", RGB_out[0],RGB_out[1],RGB_out[2]);
      DecideColorandUpdateLCDContents( RGB_out );
      lcdClear();
      lcdSetText(lcd_out_row1, 0,0);
      lcdSetText(lcd_out_row2, 0,1);
    }
    
  }// end of endless while loop

  return 0;
}

void InitClocks(void)
{
  DCOCTL = 0;
  BCSCTL1 = CALBC1_1MHZ;
  DCOCTL  = CALDCO_1MHZ;
  BCSCTL3 |= LFXT1S_2;
}

void InitTCS3200(void)
{
  P1DIR |= (BIT2 | BIT3);
  P1DIR &= ~BIT1;
  P1SEL &= ~BIT1;
  P1IES &= ~BIT1;
  P1IFG &= ~BIT1;
  P1IE  &= ~BIT1;  
}

void SetUnusedPinsasInputviaPullUpResistors(void)
{
  // No tilda
  P1SEL &= (BIT1 | BIT2 | BIT3); 
  P1SEL2 &= (BIT1 | BIT2 | BIT3);
  P1DIR &= (BIT1 | BIT2 | BIT3);
  P1REN |= (BIT1 | BIT2 | BIT3); 
  P1OUT |= (BIT1 | BIT2 | BIT3);
  
  P2SEL &= ~(BIT6 | BIT7);
  P2SEL2 &= ~(BIT6 | BIT7);
  P2DIR &= ~(BIT6 | BIT7);
  P2REN |= (BIT6 | BIT7);
  P2OUT |= (BIT6 | BIT7);
  
}

void InitTimer1(void)
{
  TA1CCR0 = 12000;
  TA1CCTL0 |= CCIE;
  TA1CTL |= TASSEL_1 | MC_2 | TACLR ;
}

void StartTCS3200(void)
{
  P1OUT &= ~(BIT2 | BIT3);
  TA0CCR0 = 10000;
  TA0CCTL0 |= CCIE;
  TA0CTL |= TASSEL_2 | MC_1 | TACLR;
  P1IE |= BIT1;
}

void ComputeRGBPercentagesbyClear(const uint16_t TCS3200_out[4], uint16_t RGB_out[3])
{
  uint16_t Clear = TCS3200_out[3];
  for(uint8_t i = 0; i<3; i++)
  {
    RGB_out[i] = (float)TCS3200_out[i]*100/Clear;
  }
  
}

uint16_t ComputeMax(uint16_t const arr[])
{
  uint16_t maxVal = arr[0];
  for(uint8_t i = 1; i<3 ;i++)
  {
    if(arr[i] > maxVal)
    {
      maxVal = arr[i];
    }
  }
  
  return maxVal;
}

void DecideColorandUpdateLCDContents(uint16_t const sampleRGB[])
{
  uint16_t Max = 0;
  uint16_t Red_Percent = sampleRGB[0];
  uint16_t Green_Percent = sampleRGB[1];
  uint16_t Blue_Percent = sampleRGB[2];
  
  
  if(Red_Percent >= 80 && Green_Percent >= 80 && Blue_Percent >= 80)
  {
    // Smartphone Flash
    strcpy(lcd_out_row2, "Smartphone Flash");
  }
  else
  {
    Max = ComputeMax(sampleRGB);
    if(Max == Green_Percent)
    {
      // Max is Green, can be green
      if(Red_Percent>20 && Green_Percent>35 && Blue_Percent>30)
      {
        // Green
        strcpy(lcd_out_row2, "Green");
      }
      else
      {
        // Get close 
        strcpy(lcd_out_row2, "Get close");
      }
      
    }
    else if(Max == Blue_Percent)
    {
      // Max is Blue, can be blue or dark blue
      if(Red_Percent>10 && Green_Percent>20 && Blue_Percent>50)
      {
        // Blue
        strcpy(lcd_out_row2, "Blue");
      }
      else if(Red_Percent>15 && Green_Percent>15 && Blue_Percent>45 )
      {
        // Dark blue
        strcpy(lcd_out_row2,"Dark Blue");
      }
      else
      {
        // Get close
        strcpy(lcd_out_row2, "Get close");
      }
      
    }
    else
    {//Max is green, can be orange, yellow, red, brown, pink or dark pink
      
      if(Green_Percent>=Blue_Percent)
      {
        // Orange or yellow
        if(Green_Percent>30)
        {
          //Yellow
          strcpy(lcd_out_row2, "Yellow");
        }
        else if(Red_Percent - Green_Percent>=25)
        {
          // Orange
          strcpy(lcd_out_row2, "Orange");
        }
        else
        {
          // Get close
          strcpy(lcd_out_row2, "Get close");
        }
        
      }
      else
      {
        //Max is red, can be red, brown, pink or dark pink
        if(Red_Percent>=55)
        {
          // Red
          strcpy(lcd_out_row2, "Red");
        }
        else if(Red_Percent >=49 && Green_Percent<25 && Blue_Percent>30)
        {
          // Dark pink
          strcpy(lcd_out_row2, "Dark Pink");
        }
        else if(Red_Percent>=44 && Green_Percent>=18)
        {
          // Pink
          strcpy(lcd_out_row2, "Pink");
        }
        else if(Red_Percent<=45)
        {
          // Brown
          strcpy(lcd_out_row2, "Brown");
        }
        else
        {
          // Get close
          strcpy(lcd_out_row2, "Get close");
        }
        
      }
      
    }
  }
  
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR(void)
{  
  static int rgbc_index=0;  
  
  P1IE &= ~BIT1;
  TA0CTL = 0;  
  TCS3200_out[rgbc_index] = frequency;
  rgbc_index++;
  frequency = 0;
  
  if(rgbc_index == 4)
  {
    rgbc_index = 0;
    measurement_done = 1;
    __low_power_mode_off_on_exit();
  }
  else
  {
    if(rgbc_index == 1)
    {
      P1OUT |= BIT2 + BIT3;    
    }
    if(rgbc_index == 2)
    {
      P1OUT &= ~BIT2;
      P1OUT |= BIT3;
    }
    if(rgbc_index == 3)
    {
      P1OUT |= BIT2;
      P1OUT &= ~BIT3;
    }
    
    TA0CTL |= TASSEL_2 + MC_1 + TACLR;
    P1IE |= BIT1;    

  }
  
}

#pragma vector = PORT1_VECTOR
__interrupt void Port1_ISR(void)
{
  P1IFG &= ~BIT1;
  frequency++; 
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void Timer1_A0_ISR(void)
{    
  //START MEASUREMENT
  TimetoStartTCS3200 = 1;
  TA1CCR0 += 12000;
  __low_power_mode_off_on_exit();
 
}
