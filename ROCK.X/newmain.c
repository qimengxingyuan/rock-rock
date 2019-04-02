#pragma config RSTOSC = HFINT32 // Power-up default value for COSC bits (HFINTOSC with OSCFRQ= 32 MHz and CDIV = 1:1)
#pragma config WDTE = OFF       // WDT operating mode (WDT Disabled, SWDTEN is ignored)

#include <xc.h>

#define TURN_OFF do{ \
    while(PIR3bits.TXIF != 1); \
    TX1REG = 0x00; \
    while(PIR3bits.TXIF != 1); \
    TX1REG = 0x00; \
    while(PIR3bits.TXIF != 1); \
    TX1REG = 0x00; \
} while(0);


#define TURN_ON do{ \
    while(PIR3bits.TXIF != 1); \
    TX1REG = 0x10; \
    while(PIR3bits.TXIF != 1); \
    TX1REG = 0x00; \
    while(PIR3bits.TXIF != 1); \
    TX1REG = 0x20; \
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

void out_test();
void delay(int delay_time);

void interrupt irs_routine()
{
    //TODO: 
    enable_out();
    
    out_reset();
    
    out_test();
    
    delay(10);
    
    disable_out();
    
    if(LATCbits.LATC1 == 0){
        goto Exit;
    }
    
Exit:
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
    /*
    while(1){
        if(LATCbits.LATC0 == 0){
             // forbid interrupts
            INTCONbits.GIE = 0;
            // data recive
            recv_data();
            // allow interrupts
            INTCONbits.GIE = 1;
        }
    }
    */
    
    while(1);
    
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
    //TODO: set it
    INTCONbits.GIE = 1;
    PIE0bits.INTE = 1;
    
    INTCONbits.INTEDG = 0;
    INTPPS = 0x05;
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
    
    int i = 100;   // i = 95 time = 50.9us; i = 100 time = 53.625us
    while(i > 0){
        i--;
    }
    i = 0;
}


void out_test()
{
    
    int count = 0;
   
    while(count < 16){
        TURN_ON;
        count++;
    }
    
    out_reset();
    
    TURN_ON;
    
    int count = 0;
    while(count < 8){
        TURN_OFF;
        count++;
    }
}


void delay(int delay_time)
{
    int i = delay_time;
    while(i > 0){
        --i;
    }
    
}