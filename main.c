#include <xc.h>

#pragma config FEXTOSC = HS     // External Oscillator mode selection bits (HS (crystal oscillator) above 4MHz; PFM set to high power)
#pragma config WDTE = OFF       // WDT operating mode (WDT Disabled, SWDTEN is ignored)

#define ZERO 0;

void delay(int i);
void receive_data();
void show_complete();
void show_begin();
void turn_put(int op);

//INTERRUPTS HERE 
void interrupt irs (void){
    int i = 0; //Pixel point counter
    int low_addr = 0;
    int buf_bit = 8; //buffer bit counter
    unsigned char data_buf = 0x00;
    unsigned int delay_flag = 0;
    
    NVMCON1bits.NVMREGS = 1; //select EEPROM
    NVMADRH = 0x70;  
    
    while(1){
        //Determine if need to read new data
        if(buf_bit == 8){
            buf_bit = 0;
            ++low_addr;
            
            if(low_addr == 64){
                low_addr = 0;
            }
            
            //read a byte from EEPROM
            NVMADRL = low_addr;
            NVMCON1bits.RD = 1;
            while(NVMCON1bits.RD == 0);
            data_buf = NVMDATL;
            
            
            if(delay_flag){
                delay(256);
            }
            delay_flag = ~delay_flag;
        }
        
        //mov a bit to MDBIT
        MDCON0bits.MDBIT = ((data_buf >> buf_bit) & 0b1);
        ++buf_bit;
        
        MDCON0bits.MDEN = 1;  //enable DSM
        delay(24);
        MDCON0bits.MDEN = 0;  //disable DSM
        
        i++;
        
        //while i = 256 judge the Switch
        if(i == 256){
            if(LATCbits.LATC1 == 1){
                goto Exit;
            }
            i = 0;
        }
    }
    
Exit:
    turn_put(0);
    PIR0bits.INTF = ZERO;
    return;
}


void main(void )
{
    //TODO?set FOSC
    
    //set interrupts
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.INTEDG = 1;
    PIE0bits.IOCIE = 1;
    // set RC1 interrupts
    // IOCCPbits.IOCCP1 = 1;
    IOCCNbits.IOCCN1 = 1;
    
    //configure PWMx level
    PWM6CONbits.PWM6POL = 1;
    PWM7CONbits.PWM7POL = 0;
    
    T2PRbits.PR2 = 0xff;// TODO:caculate the value
    PWM6DCH = 0x03; // TODO:caculate the value
    PWM6DCL = 0xFF;
    PWM7DCH = 0x03; // TODO:caculate the value
    PWM7DCL = 0xFF;
    
    //set TMR2
    PIR4bits.TMR2IF = 0;
    T2CLKCONbits.CS = 0b0010;
    T2CONbits.T2CKPS = 0b101;
    T2CONbits.ON = 1; 
    
    //enable PWMx
    PWM6CONbits.PWM6EN = 1; 
    PWM7CONbits.PWM7EN = 1; 
    
    //configure DSM
    MDSRCbits.MDMS = 0b00001;
    MDCARHbits.MDCH = 0b1001;
    MDCARLbits.MDCL = 0b1010;
    
    //init port 
    //DSM out : RA0
    RA0PPS = 0x1B;
    PORTA = 0;
    LATA = 0;
    ANSELA = 0;
    TRISAbits.TRISA0 = 0;
    //interrupts:RC1 
    //S1: RC0
    //lightR :RC3
    PORTC = 0;
    LATC = 0;
    ANSELC = 0;
    TRISC = 0b00001011;
    WPUC =  0b00001011;
    
    while(1){
        if(LATCbits.LATC0 == 0){
            // forbid interrupts
            INTCONbits.GIE = 0;
            // data recive
            receive_data();
            // allow interrupts
            INTCONbits.GIE = 1;
        }
    }
}

/*
 *TODO receive date from app
 */
void receive_data()
{
    unsigned char recv_buf[64];
    show_begin();
    
    //receive from light
    //RC3
    unsigned char r = '0';
    for(int i = 0; i < 64; ++i){
        recv_buf[i] = r;
        ++r;
    }
    
    //*********receive and verify**************
     
    
    //*****************************************
    //write to EEPROM
    NVMCON1bits.NVMREGS = 1;
    NVMCON1bits.WREN = 1;
    NVMDATH = 0;
    NVMADRH = 0x70;
    
    for(int i = 0; i < 64; ++i){
        NVMDATL = recv_buf[i];
        NVMADRL = i;
        // UNLOCK NVM 
        NVMCON2 = 0x55;
        NVMCON2 = 0xAA;
        NVMCON1bits.WR = 1;
        
        while(NVMCON1bits.WR == 0);
    }
    
    show_complete();
}

void delay(int i) // delay time
{
    while(i > 0){
        --i;
    }
    return;
}

void show_complete()
{
    for(int i = 0; i < 16; ++ i){
        MDCON0bits.MDBIT = 0;
        MDCON0bits.MDEN = 1;  //enable DSM
        delay(24);
        MDCON0bits.MDEN = 0;  //disable DSM
        delay(256);
    }
}

void show_begin()
{
    for(int i = 0; i < 16; ++ i){
        MDCON0bits.MDBIT = 1;
        MDCON0bits.MDEN = 1;  //enable DSM
        delay(24);
        MDCON0bits.MDEN = 0;  //disable DSM
        delay(256);
    }
}

void turn_put(int op)
{
    MDCON0bits.MDBIT = op;
    MDCON0bits.MDEN = 1;  //enable DSM
    delay(384);
    MDCON0bits.MDEN = 0;  //disable DSM
}