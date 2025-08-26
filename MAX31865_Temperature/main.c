#include "DSP2803x_Device.h"

void Setup();

void InitSystem();
void InitCLK();
void InitGpio();
void InitSpia();
void spi_fifo_init();

void ConfigMax31865();

Uint16 ReceiveADC();
unsigned char spi_xmit(unsigned int a);
unsigned char Spi_read(unsigned char address);
void Spi_write(unsigned char address,unsigned char data);


void Start_bit();
void Send_bit(Uint16 data);
void Ack_bit();
void Stop_bit();

void display(Uint16 data, Uint16 grid);
Uint16 gridAddress(Uint16 pos);

void numSegment(unsigned char z, Uint16 pos);
void num2Segment(unsigned char z, Uint16 pos);

Uint16 numAddress[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
Uint16 num2Address[10] = {0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,0xFF,0xEF};

void Delay_ms(Uint32 ms);

void main()
 {
    Setup();

    while(1){

    	GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;
        ConfigMax31865();
       Delay_ms(25);

       Spi_read(0x00);

        Uint16 ADC_code = ReceiveADC();

        float ADC = ADC_code;

        float f_temp = ((ADC / 32) - 256) * 10;

        Uint16 temp = f_temp;

         Uint16 Digit[3];
         Digit[0] = temp % 10;
         temp /= 10;
         Digit[1] = temp % 10;
         temp /= 10;
         Digit[2] = temp % 10;

         // Pass as character to numSegment: '0'+digit
         numSegment(Digit[0] , 3);
         num2Segment(Digit[1] , 2);
         numSegment(Digit[2] , 1);

        GpioDataRegs.GPASET.bit.GPIO4 = 1;
        Delay_ms(500); // update every 0.5s
    }
}

void InitSystem()
{
    EALLOW;
    //Disable Watchdog
    SysCtrlRegs.WDCR = 0x0068;

    SysCtrlRegs.PLLSTS.bit.MCLKSTS = 0;

    SysCtrlRegs.PLLCR.bit.DIV = 6;

    while(SysCtrlRegs.PLLSTS.bit.PLLLOCKS  == 0)
    {

    }

    EDIS;
}

void InitCLK()
{
    EALLOW;
    SysCtrlRegs.LOSPCP.bit.LSPCLK = 2;

    SysCtrlRegs.PCLKCR0.bit.SPIAENCLK = 1;
    EDIS;
}

void InitGpio()
{
    EALLOW;

    // SPI Pins
    GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 1;
    GpioCtrlRegs.GPAMUX2.bit.GPIO17 = 1;
    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 1;

    GpioCtrlRegs.GPADIR.bit.GPIO19 = 1;


    GpioCtrlRegs.GPAQSEL2.bit.GPIO16 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO17 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO18 = 3;


    GpioCtrlRegs.GPAPUD.bit.GPIO16 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO17 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO18 = 1;

    // LED on GPIO4
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO4 = 1;
    GpioDataRegs.GPASET.bit.GPIO4 = 1;  // LED OFF initially

	GpioCtrlRegs.GPBMUX1.bit.GPIO43 =0; //Disp_CLk
	GpioCtrlRegs.GPBDIR.bit.GPIO43 = 1;
	GpioDataRegs.GPBSET.bit.GPIO43 = 1;

	GpioCtrlRegs.GPBMUX1.bit.GPIO42 =0; //DISP_Data
	GpioCtrlRegs.GPBDIR.bit.GPIO42 = 1;
	GpioDataRegs.GPBSET.bit.GPIO42 = 1;

    EDIS;
}

void InitSpia()
{
    SpiaRegs.SPICCR.all =0x0007;
    SpiaRegs.SPICTL.all =0x0006;

    SpiaRegs.SPIBRR =0x00FF;
    SpiaRegs.SPICCR.all =0x00C7;
    SpiaRegs.SPIPRI.bit.FREE = 1;
}

void spi_fifo_init()
{
    // Initialize SPI FIFO registers
    SpiaRegs.SPIFFTX.all=0xE040;
    SpiaRegs.SPIFFRX.all=0x204F;
    SpiaRegs.SPIFFCT.all=0x0;
}

void Setup()
{
    InitSystem();
    InitCLK();
    InitGpio();
    spi_fifo_init();
    InitSpia();
}



void Delay_ms(Uint32 ms)
{
    Uint32 i, j;
    for (i = 0; i < ms; i++)
    {
        for (j = 0; j < 6000; j++)  // Approx 1ms at 60 MHz
        {
            asm(" NOP");  // No-operation instruction
        }
    }
}

unsigned char spi_xmit(unsigned int a)
{
	unsigned char Rx_byte = 0x0000;
	SpiaRegs.SPITXBUF = (a << 8);
	while(SpiaRegs.SPIFFRX.bit.RXFFST != 1);
	Rx_byte = SpiaRegs.SPIRXBUF;
	return Rx_byte;
}

void Spi_write(unsigned char address,unsigned char data)
{
	GpioDataRegs.GPADAT.bit.GPIO19 = 0;
	Delay_ms(2);
	spi_xmit(address);
	spi_xmit(data);
	Delay_ms(2);
	GpioDataRegs.GPADAT.bit.GPIO19 = 1;
}

unsigned char Spi_read(unsigned char address)
{
	unsigned char Data = 0x0000;
	GpioDataRegs.GPADAT.bit.GPIO19 = 0;
	Delay_ms(2);
	spi_xmit(address);
	Data = spi_xmit(0x00);
	Delay_ms(2);
	GpioDataRegs.GPADAT.bit.GPIO19 = 1;
	return Data;
}

void ConfigMax31865()
{
	Spi_write(0x80,0xD3);
	Delay_ms(2);
}

Uint16 ReceiveADC()
{
	unsigned char MSB = 0x00, LSB = 0x00;

	MSB = Spi_read(0x01);
	LSB = Spi_read(0x02);

	Uint16 ADC = MSB;
	ADC = ADC << 8;

	ADC = ADC | LSB;

	ADC = ADC >> 1;
	return ADC;
}

void display(Uint16 data, Uint16 grid)
{
    Start_bit();
    Send_bit(0x44);
    Ack_bit();
    Stop_bit();

    Start_bit();
    Send_bit(grid);
    Ack_bit();

    Send_bit(data);
    Ack_bit();
    Stop_bit();

    Start_bit();
    Send_bit(0x8f);
    Ack_bit();
    Stop_bit();
}

Uint16 gridAddress(Uint16 pos)
{
    switch(pos)
    {
    case 1: return 0xC0;
    case 2: return 0xC1;
    case 3: return 0xC2;
    default: return 0xC0;
    }
}

void numSegment(unsigned char z, Uint16 pos)
{
    Uint16 segments = numAddress[z];
    Uint16 grids = gridAddress(pos);
    display(segments, grids);
}

void num2Segment(unsigned char z, Uint16 pos)
{
    Uint16 segments = num2Address[z];
    Uint16 grids = gridAddress(pos);
    display(segments, grids);
}
void Start_bit()
{
    GpioDataRegs.GPBSET.bit.GPIO42 = 1;
    Delay_ms(2);
    GpioDataRegs.GPBSET.bit.GPIO43 = 1;
    Delay_ms(2);
    GpioDataRegs.GPBCLEAR.bit.GPIO42 = 1;
    Delay_ms(2);
    GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1;
}

void Send_bit(Uint16 data)
{
    GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1;
    Delay_ms(2);

    Uint16 i;
    for(i = 0; i < 8; i++)
    {
        if(data & 0x01)
            GpioDataRegs.GPBSET.bit.GPIO42 = 1;
        else
            GpioDataRegs.GPBCLEAR.bit.GPIO42 = 1;

        Delay_ms(2);
        GpioDataRegs.GPBSET.bit.GPIO43 = 1;
        Delay_ms(2);
        GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1;

        data >>= 1;
    }
}

void Ack_bit()
{
    GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1;
    Delay_ms(2);
    GpioDataRegs.GPBSET.bit.GPIO43 = 1;
    Delay_ms(2);
    GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1;
}

void Stop_bit()
{
    GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1;
    Delay_ms(2);
    GpioDataRegs.GPBCLEAR.bit.GPIO42 = 1;
    Delay_ms(2);
    GpioDataRegs.GPBSET.bit.GPIO43 = 1;
    Delay_ms(2);
    GpioDataRegs.GPBSET.bit.GPIO42 = 1;
}
