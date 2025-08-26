#include "DSP2803x_Device.h"

// Init Function Prototypes
void InitSystem();
void InitGpio();
//Scia functions;
void InitScia();
void InitSciaFF();

// Interrupts
__interrupt void SCIRXIR();
__interrupt void SCITXIR();

void PieCtrl();
void InitPieVectTable();


void Start_bit();
void Send_bit(Uint16 data);
void Ack_bit();
void Stop_bit();

Uint16 gridAddress(Uint16 pos);

void display(Uint16 data,Uint16 grid);

void letterSegment(unsigned char z,Uint16 pos);


Uint16 letterAddress[26] = {0x77,0x7C,0X39,0X5E,0X79,0X71,0X3D,0X76,0X30,0X1E,0X40,0X38,0X40,0X54,0X3F,0X73,0X67,0X50,0X6D,0X78,0X3E,0X1C,0X40,0X6E,0X5B};



void Delay_ms(Uint32 ms);


/*
 * Global Variables
 */
char rx_buffer[6];
char tx_buffer[6] = "ABCDEF";
Uint16 tx_index= 0;
Uint16 tx_length = 6;
Uint16 rx_index = 0;


/*
 * Main Function
 */
int main(void)

{
    InitSystem();
    InitGpio();

    display(0x00,0xC0);
    display(0x00,0xC1);

    InitScia();
    InitSciaFF();

    DINT;              // Disable global CPU interrupts
    IER = 0x0000;      // Clear interrupt enable register
    IFR = 0x0000;      // Clear interrupt flags

    PieCtrl();         // Safely disable PIE interrupts
    InitPieVectTable();  // Assign ISRs and enable PIE group 9

    IER |= 0x0100;     // Enable CPU interrupt group 9
    EINT;              // Enable global CPU interrupts

    while (1) {

            Delay_ms(2);
            GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;
            tx_index = 0;
            SciaRegs.SCIFFTX.bit.TXFFIENA = 1;  //Kickstart TX interrupt

            Delay_ms(2);

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


void InitGpio()
{

    EALLOW;

    // SCI Pins
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1; // SCIRXDA
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1; // SCITXDA

    // LED on GPIO4
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO4 = 1;
    GpioDataRegs.GPASET.bit.GPIO4 = 1;  // LED OFF initially

    // Push button on GPIO0
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 0;  // Enable pull-up

	GpioCtrlRegs.GPBMUX1.bit.GPIO43 =0; //Disp_CLk
	GpioCtrlRegs.GPBDIR.bit.GPIO43 = 1;
	GpioDataRegs.GPBSET.bit.GPIO43 = 1;

	GpioCtrlRegs.GPBMUX1.bit.GPIO42 =0; //DISP_Data
	GpioCtrlRegs.GPBDIR.bit.GPIO42 = 1;
	GpioDataRegs.GPBSET.bit.GPIO42 = 1;

    EDIS;
}


void InitScia()
{
	EALLOW;
	SysCtrlRegs.LOSPCP.bit.LSPCLK = 2;

	SysCtrlRegs.PCLKCR0.bit.SCIAENCLK = 1;

	SciaRegs.SCICCR.all = 0x0007;
	SciaRegs.SCICCR.bit.LOOPBKENA = 0;
	SciaRegs.SCICTL1.all = 0x0003;

	SciaRegs.SCIHBAUD = 0x0000;

	SciaRegs.SCILBAUD = 0x00C3;

	SciaRegs.SCICTL2.bit.RXBKINTENA = 1;
	SciaRegs.SCICTL2.bit.TXINTENA = 1;

	SciaRegs.SCICTL1.all = 0x0023;
}

void InitSciaFF()
{
	SciaRegs.SCIFFTX.all = 0xE060;

	SciaRegs.SCIFFRX.all = 0x2061;

	SciaRegs.SCIFFCT.all = 0x0000;
}

__interrupt void SCIRXINTA_ISR()
{

	GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;

	rx_buffer[rx_index] = SciaRegs.SCIRXBUF.all;
	letterSegment(rx_buffer[rx_index],3);
	rx_index++;

	Delay_ms(2);

	SciaRegs.SCIFFRX.bit.RXFFOVRCLR = 1;

	SciaRegs.SCIFFRX.bit.RXFFINTCLR = 1;

	PieCtrlRegs.PIEACK.all = 0x100;
	GpioDataRegs.GPASET.bit.GPIO4 = 1;
}

__interrupt void SCITXINTA_ISR()
{
	GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;
	if(tx_index < tx_length)
	{
		SciaRegs.SCITXBUF = tx_buffer[tx_index++];
		Delay_ms(2);
	}
	else
	{
		SciaRegs.SCIFFTX.bit.TXFFIENA = 0;
		tx_index = 0;
		rx_index = 0;
		Uint16 i;
		for(i = 0;i < tx_length;i++)
		{
			rx_buffer[i] = 0;
		}
	}

	SciaRegs.SCIFFTX.bit.TXFFINTCLR = 1;


	PieCtrlRegs.PIEACK.all = 0x100;
	GpioDataRegs.GPASET.bit.GPIO4 = 1;
}

void PieCtrl()
{
	DINT;

	PieCtrlRegs.PIECTRL.bit.ENPIE = 0;

	PieCtrlRegs.PIEIER9.bit.INTx1 = 0;
	PieCtrlRegs.PIEIER9.bit.INTx2 = 0;

	PieCtrlRegs.PIECTRL.bit.ENPIE = 1;

}

void InitPieVectTable()
{
	EALLOW;
	PieVectTable.SCIRXINTA = &SCIRXINTA_ISR;
	PieVectTable.SCITXINTA = &SCITXINTA_ISR;
	EDIS;

	PieCtrlRegs.PIEIER9.bit.INTx1 = 1;
	PieCtrlRegs.PIEIER9.bit.INTx2 = 1;

	IER = 0x100;
	EINT;
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
	for(i = 0;i < 8;i++)
	{
		if(data & 0x01)
		{
			GpioDataRegs.GPBSET.bit.GPIO42 = 1;
		}
		else
		{
			GpioDataRegs.GPBCLEAR.bit.GPIO42 = 1;
		}

		Delay_ms(2);
		GpioDataRegs.GPBSET.bit.GPIO43 = 1;
		Delay_ms(2);
		GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1;

		data = data >> 1;
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

void display(Uint16 data,Uint16 grid)
{
	Start_bit();
	Send_bit(0x44); // Data-Command bit
	Ack_bit();
	Stop_bit();

	Start_bit();
	Send_bit(grid); //Address Command bit
	Ack_bit();

	Send_bit(data); //Digit
	Ack_bit();
	Stop_bit();

	Start_bit();
	Send_bit(0x8f); // Settings
	Ack_bit();
	Stop_bit();
}

Uint16 gridAddress(Uint16 pos)
{
	switch(pos)
	{
	case 1:
		return 0xC0;
	case 2:
		return 0xC1;

	case 3:
		return 0xC2;

	default:
		return 0xC0;
	}
}

void letterSegment(unsigned char z,Uint16 pos)
{
	Uint16 segments;
	Uint16 grids = gridAddress(pos);

	if(z >= 'A' && z <= 'Z')
	{
		segments = letterAddress[z - 'A'];
	}
	if(z >= 'a' && z <= 'z')
	{
		segments = letterAddress[z - 'a'];
	}


	display(segments,grids);

}
