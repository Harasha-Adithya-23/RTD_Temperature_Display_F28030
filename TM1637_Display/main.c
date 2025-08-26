#include "DSP2803x_Device.h"
/*
 * main.c
 */
void InitSystem();
void InitGpio();

void Delay_ms(Uint32 ms);

void Start_bit();
void Send_bit(Uint16 data);
void Ack_bit();
void Stop_bit();

void Clear_all();
Uint16 gridAddress(Uint16 pos);

void display(Uint16 data,Uint16 grid);

void numSegment(unsigned char z,Uint16 pos);

void letterSegment(unsigned char z,Uint16 pos);

Uint16 numAddress[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
Uint16 letterAddress[26] = {0x77,0x7C,0X39,0X5E,0X79,0X71,0X3D,0X76,0X30,0X1E,0X40,0X38,0X40,0X54,0X3F,0X73,0X67,0X50,0X6D,0X78,0X3E,0X1C,0X40,0X6E,0X5B};


int main(void)
{
	InitSystem();

	InitGpio();

	while(1)
	{
		Uint16 currentState = GpioDataRegs.GPADAT.bit.GPIO0;
		Uint16 secondState = GpioDataRegs.GPADAT.bit.GPIO2;

		if(!currentState || !secondState)
		{
			GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;
			currentState = GpioDataRegs.GPADAT.bit.GPIO0;
			if(!currentState)
			{
				unsigned char z = 'A';
				for(z = 'A';z <= 'Z';z++)
				{
					letterSegment(z,3);

				}
				letterSegment('E',1);
				letterSegment('n',2);
				letterSegment('d',3);
			}
			else
			{
				unsigned char z = '9';
				for(z = '9';z >= '0';z--)
				{
					numSegment(z,2);
				}
				letterSegment('E',1);
				letterSegment('n',2);
				letterSegment('d',3);
			}
		}

		Delay_ms(20);
		GpioDataRegs.GPASET.bit.GPIO4 = 1;
	}


}

void InitSystem(void)
{
	EALLOW;
	SysCtrlRegs.WDCR = 0x0068;
	SysCtrlRegs.PLLSTS.bit.MCLKSTS = 0;
	SysCtrlRegs.PLLCR.bit.DIV = 6;
	while(SysCtrlRegs.PLLSTS.bit.PLLLOCKS != 1);
	EDIS;
}

void InitGpio(void)
{
	EALLOW;
	GpioCtrlRegs.GPBMUX1.bit.GPIO43 =0; //Disp_CLk
	GpioCtrlRegs.GPBDIR.bit.GPIO43 = 1;
	GpioDataRegs.GPBSET.bit.GPIO43 = 1;

	GpioCtrlRegs.GPBMUX1.bit.GPIO42 =0; //DISP_Data
	GpioCtrlRegs.GPBDIR.bit.GPIO42 = 1;
	GpioDataRegs.GPBSET.bit.GPIO42 = 1;


	GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 0;  // GPIO function
	GpioCtrlRegs.GPADIR.bit.GPIO4 = 1;   // Output
	GpioDataRegs.GPASET.bit.GPIO4 = 1; // Initial state: LED off (GPIO4 = 1)

	// Configure GPIO0 as input (Key) with pull-up

	GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;  // GPIO function
	GpioCtrlRegs.GPADIR.bit.GPIO0 = 0;   // Input
	GpioCtrlRegs.GPAPUD.bit.GPIO0 = 0; // Enable internal pull-up

	GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 0;  // GPIO function
	GpioCtrlRegs.GPADIR.bit.GPIO2 = 0;   // Input
	GpioCtrlRegs.GPAPUD.bit.GPIO2 = 0; // Enable internal pull-up

	EDIS;  // Disable protected register access
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


void Clear_all()
{
	Start_bit();
	Send_bit(0x40); // Data-Command bit
	Ack_bit();
	Stop_bit();

	Start_bit();
	Send_bit(0xC0); //Address Command bit
	Ack_bit();

	Send_bit(0x00); //Digit - A
	Ack_bit();

	Send_bit(0x00); //Digit - 5
	Ack_bit();

	Send_bit(0x00); //Digit - 1
	Ack_bit();
	Stop_bit();

	Start_bit();
	Send_bit(0x8f); // Settings
	Ack_bit();
	Stop_bit();

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
void numSegment(unsigned char z,Uint16 pos)
{
	Uint16 segments = numAddress[z - '0'];
	Uint16 grids = gridAddress(pos);


	display(segments,grids);
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
