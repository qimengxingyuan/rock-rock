#pragma config RSTOSC = HFINT32 // Power-up default value for COSC bits (HFINTOSC with OSCFRQ= 32 MHz and CDIV = 1:1)
#pragma config WDTE = OFF       // WDT operating mode (WDT Disabled, SWDTEN is ignored)

#include <xc.h>

#define OFF 0x00;

#define TURN_ON(_r, _g, _b) do{ \
    while(PIR3bits.TXIF != 1); \
    TX1REG = _g; \
    while(PIR3bits.TXIF != 1); \
    TX1REG = _r; \
    while(PIR3bits.TXIF != 1); \
    TX1REG = _b; \
} while(0);

void init_oc();
void set_pps();
void set_cwg();
void set_clc();
void init_port();
void set_eusart();
void enable_out();
void disable_out();
void set_interrupt();
void out_reset();
void turn_off_all(int num);
void delay(int delay_time);

void wrdt2eeprom();
void load_data_from_eeprom();

unsigned char font_arr[64] = {0x01, 0x00, 0x03, 0x00, 0x07, 0x00, 0x0f, 0x00,
                              0x1f, 0x00, 0x3f, 0x00, 0x7f, 0x00, 0xff, 0x00,
                              0xff, 0x01, 0xff, 0x03, 0xff, 0x07, 0xff, 0x0f,
                              0xff, 0x1f, 0xff, 0x3f, 0xff, 0x7f, 0xff, 0xff,
                              0xff, 0x7f, 0xff, 0x3f, 0xff, 0x1f, 0xff, 0x0f,
                              0xff, 0x07, 0xff, 0x03, 0xff, 0x01, 0xff, 0x00,
                              0x7f, 0x00, 0x3f, 0x00, 0x1f, 0x00, 0x0f, 0x00,
                              0x07, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00};

unsigned char R = 0x04, G = 0x00, B = 0x04;
int LIGHT_NUM = 16;

void interrupt irs_routine()
{
    int buf_bit = 8;
    int font_addr = -1;
    unsigned char font;
    int i = 0;
    
    enable_out();
    out_reset();
    
    while(1){
       if(buf_bit == 8){
            buf_bit = 0;
            ++font_addr;
            
            if(font_addr == 64){
                font_addr = 0;
            }
            
            font = font_arr[font_addr];
        }
        
        //show the bit from font
        if((font >> buf_bit) & 0x01 == 1){
            TURN_ON(R, G, B);
        }
        else{
            TURN_ON(OFF, OFF, OFF);
        }
        
        ++buf_bit;
        if(++i == LIGHT_NUM){
            i = 0;

            disable_out();
            delay(20000);
            enable_out();
            out_reset();
            
            if(PORTAbits.RA5 == 0){
                goto Exit;
            }
       }
    }
    
Exit:
    turn_off_all(LIGHT_NUM);
    disable_out();
    PIR0bits.INTF = 0;
    return;
}

void main(void) {
    //configure all option 
    init_oc();
    init_port();
    set_interrupt();
    
    set_pps();
    set_eusart();
    set_cwg();
    set_clc();
    
    //TODO:receive data from app
    //load_data_from_eeprom();

    while(1){
        if(PORTCbits.RC0 == 0){
             // forbid interrupts
            INTCONbits.GIE = 0;
            // data recive
            wrdt2eeprom(); //recv_data();
            load_data_from_eeprom();
            // allow interrupts
            INTCONbits.GIE = 1;
        }
    }
    
    return;
}

void init_oc()
{
    OSCCON1bits.NOSC = 0b000;
    OSCCON1bits.NDIV = 0b0000;
    OSCFRQbits.HFFRQ = 0b0111;
}

void init_port()
{
    //sw_burn -> RC0
    TRISCbits.TRISC0 = 1;
    ANSELCbits.ANSC0 = 0;
    WPUCbits.WPUC0 = 1;
    
    //sw_hg -> RA5
    TRISAbits.TRISA5 = 1;
    ANSELAbits.ANSA5 = 0;
    WPUAbits.WPUA5 = 1;
    
    //LDR -> RC3
    TRISCbits.TRISC3 = 1;
    ANSELCbits.ANSC3 = 0;
    WPUCbits.WPUC3 = 1;
    
    //RC5
    TRISCbits.TRISC5 = 0;
    ANSELCbits.ANSC5 = 0;
    //RC6
    TRISCbits.TRISC6 = 0;
    ANSELCbits.ANSC6 = 0;
    //RC7
    TRISCbits.TRISC7 = 0;
    ANSELCbits.ANSC7 = 0;
}

void set_interrupt()
{
    INTCONbits.GIE = 1;
    PIE0bits.INTE = 1;
    
    INTCONbits.INTEDG = 0;
    INTPPS = 0x05; //set intpps RA5
}

void set_pps()
{
    //EURSART OUT: DT->RC4, TC/CK->RC5
    RC4PPS = 0x11;
    RC5PPS = 0x10;
    
    //CWG IN: IN1<-CWG clock, IN2<-RC5(CK)
    CWG1PPS = 0x15;
    //CWG OUT: CWGA->RC6, CWGB->RC7
    RC6PPS = 0x05;
    RC7PPS = 0x06;

    //CLC IN: CLC0<-RC4(DT), CLC1<-RC5(CK), CLC2<-RC6(CWGA), CLC3<-RC7(CWGB)
    //CLCIN0PPS = 0x14;
    //CLCIN1PPS = 0x15;
    CLCIN2PPS = 0x16;
    CLCIN3PPS = 0x17;
    //CLC OUT: CLC1->RA0 ->
    RA0PPS = 0x01;
    ANSELAbits.ANSA0 = 0;
    TRISAbits.TRISA0 = 0;
}

void set_eusart()
{
    //set BRC default
    BAUD1CONbits.BRG16 = 1;
    SP1BRGL = 0x09;
    SP1BRGH = 0x00;      
    
    //set sync master clock from BRC
    TX1STAbits.CSRC = 1;
    TX1STAbits.SYNC = 1;
    RC1STAbits.SPEN = 1;
    
    RC1STAbits.SREN = 0;
    RC1STAbits.CREN = 0;
    
    //TX1STAbits.TXEN = 1;
}

void set_cwg()
{
    //port set input
    TRISCbits.TRISC6 = 1;
    TRISCbits.TRISC7 = 1;
    
    CWG1CON0bits.EN = 0;
    CWG1CON0bits.CWG1MODE = 0b100;
    //set dead-band
    CWG1DBR = 0x0C;
    CWG1DBF = 0x07;    

    //set auto-shutdown
    CWG1AS0bits.REN = 0;
    CWG1AS0bits.LSAC = 0b10;
    CWG1AS0bits.LSBD = 0b10;
    CWG1AS1bits.AS0E = 0;
    
    //FOSC->input
    CWG1CLKCONbits.CS = 0;
    CWG1ISMbits.IS = 0b0000;
    
    //port set output
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC7 = 0;
    
    //CWG1CON0bits.EN = 1;
}

void set_clc()
{
    CLC1CONbits.LC1EN = 0;
    
    //sel0<-CWGB(CLCIN3), sel1<-DT, sel2<-CWGA(CLCIN2), sel3<-CK
    CLC1SEL0bits.LC1D1S = 0b000011;
    CLC1SEL1bits.LC1D2S = 0b100100;
    CLC1SEL2bits.LC1D3S = 0b000010;
    CLC1SEL3bits.LC1D4S = 0b100101;
    
    CLC1GLS0 = 0x02;            //~(CWGB) -> lc1gl
    CLC1POLbits.LC1G1POL = 1;
    CLC1GLS1 = 0x08;            //DT -> lc1g2
    CLC1POLbits.LC1G2POL = 0;
    CLC1GLS2 = 0x22;            //~(CWGA+CWGB) -> lc1g3
    CLC1POLbits.LC1G3POL = 1;
    CLC1GLS3 = 0x80;            //CK -> lc1g4
    CLC1POLbits.LC1G4POL = 0;
    
    CLC1CONbits.LC1MODE = 0b000;
    
    //CLC1CONbits.LC1EN = 1;
}

void load_data_from_eeprom()
{
    unsigned char low_addr = 0x21;
    int font_add = 0;
    NVMCON1bits.NVMREGS = 1; //select EEPROM
    NVMADRH = 0x70;

    for(int i = 0; i < 64; ++i, ++low_addr){
        NVMADRL = low_addr;
        NVMCON1bits.RD = 1;
        while(NVMCON1bits.RD != 0);
        font_arr[font_add + i] = NVMDATL;
    }
}

void enable_out()
{
    //ensble eusart
    TX1STAbits.TXEN = 1;
    
    //enable cwg
    CWG1CON0bits.EN = 1;
    
    //enable clc
    CLC1CONbits.LC1EN = 1;
}

void disable_out()
{
    delay(100);
    //ensble eusart
    TX1STAbits.TXEN = 0;
    
    //enable cwg
    CWG1CON0bits.EN = 0;
    
    //enable clc
    CLC1CONbits.LC1EN = 0;
}

void out_reset()
{
    LATAbits.LATA0 = 0;
    delay(100); // i = 95 time = 50.9us; i = 100 time = 53.625us
}

void turn_off_all(int num)
{
    int i = num * 3;
    while(i > 0){
        while(PIR3bits.TXIF != 1);
        TX1REG = OFF;
        --i;
    }
}

void delay(int delay_time)
{
    int i = delay_time;
    while(i > 0){
        --i;
    }
}

void wrdt2eeprom()
{
    NVMCON1bits.NVMREGS = 1;
    NVMCON1bits.WREN = 1;
    NVMDATH = 0;
    NVMADRH = 0x70;
    NVMADRL = 0x20;

    for(int i = 0; i < 64; ++i){
        NVMDATL = 0xc0 + i;
        NVMADRL += 1;
        // UNLOCK NVM
        NVMCON2 = 0x55;
        NVMCON2 = 0xAA;
        NVMCON1bits.WR = 1;

        while(NVMCON1bits.WR != 0);
    }
}
