/*
 * _25AA1024.c
 *
 * Created: 5/27/2012 6:53:36 PM
 *  Author: Tom Burke
 *			tomburkeii@gmail.com
 */ 

#include "25AA1024.h"
#include "TinySPI.h"


/***********************************************************************
Description:
The Microchip Technology Inc. 25AA1024 is a 1024
Kbit serial EEPROM memory with byte-level and pagelevel
serial EEPROM functions. It also features Page,
Sector and Chip erase functions typically associated
with Flash-based products. These functions are not
required for byte or page write operations. The memory
is accessed via a simple Serial Peripheral Interface
(SPI) compatible serial bus. The bus signals required
are a clock input (SCK) plus separate data in (SI) and
data out (SO) lines. Access to the device is controlled
by a Chip Select (CS) input.

Communication to the device can be paused via the
hold pin (HOLD). While the device is paused, transitions
on its inputs will be ignored, with the exception of
Chip Select, allowing the host to service higher priority
interrupts.

The 25AA1024 contains an 8-bit instruction register.
The device is accessed via the SI pin, with data being
clocked in on the rising edge of SCK. The CS pin must
be low and the HOLD pin must be high for the entire
operation.

Table 2-1 contains a list of the possible instruction
bytes and format for device operation. All instructions,
addresses and data are transferred MSB first, LSB last.
Data (SI) is sampled on the first rising edge of SCK
after CS goes low. If the clock line is shared with other
peripheral devices on the SPI bus, the user can assert
the HOLD input and place the 25AA1024 in ‘HOLD’
mode. After releasing the HOLD pin, operation will
resume from the point when the HOLD was asserted.
************************************************************************/


/************************************************************************
Read Sequence
The device is selected by pulling CS low. The 8-bit
READ instruction is transmitted to the 25AA1024
followed by the 24-bit address, with seven MSBs of the
address being “don’t care” bits. After the correct READ
instruction and address are sent, the data stored in the
memory at the selected address is shifted out on the
SO pin.

The data stored in the memory at the next address can
be read sequentially by continuing to provide clock
pulses. The internal Address Pointer is automatically
incremented to the next higher address after each byte
of data is shifted out. When the highest address is
reached (1FFFFh), the address counter rolls over to
address, 00000h, allowing the read cycle to be continued
indefinitely. The read operation is terminated by
raising the CS pin (Figure 2-1).
************************************************************************/
inline short	ReadData( short chip, long Address, long NumBytes, short* data )
{
	short	TData;			//	Temporary storage for data just read
	
	unsigned long	count;
	
	//	Send the command & address to the memory
	if( SendCommandAndAddress( chip, MREAD, Address ) )
		return MEMFAIL;
	
	//  Command sent, now we need to read back what the chip sends us
	for( count=0 ; count<NumBytes ; count++ )
	{
		//	Read back the data
		if( ReadByte( &TData ) )
			return MEMFAIL;
					
		data[count] = TData;
	}

	//  End read by setting CS high
	if( SetCS( chip ) )
		return MEMFAIL;

	//  All done, let's get out of here!
	return MEMSUCC;	
}




/*************************************************************************
RELEASE FROM DEEP POWERDOWN AND READ ELECTRONIC SIGNATURE
Once the device has entered Deep Power-Down
mode all instructions are ignored except the release
from Deep Power-down and Read Electronic Signature
command. This command can also be used when
the device is not in Deep Power-down, to read the
electronic signature out on the SO pin unless another
command is being executed such as Erase, Program
or Write STATUS register.

Release from Deep Power-Down mode and Read
Electronic Signature is entered by driving CS low,
followed by the RDID instruction code (Figure 2-12)
and then a dummy address of 24 bits (A23-A0). After
the last bit of the dummy address is clocked in, the
8-bit Electronic signature is clocked out on the SO
pin.

After the signature has been read out at least once,
the sequence can be terminated by driving CS high.
The device will then return to Standby mode and will
wait to be selected so it can be given new instructions.
If additional clock cycles are sent after the electronic
signature has been read once, it will continue to output
the signature on the SO line until the sequence is
terminated.
*************************************************************************/
short	WakeMem( short chip )
{
	short	TData;
	
	//	Send Wake Up
	//	Send the command & address to the memory
	if( SendCommandAndAddress( chip, MRDID, (long)0x00A5A5A5 ) )
		return MEMFAIL;
	
	//	Read back the device ID	
	if( ReadByte( &TData ) )
		return MEMFAIL;
		
	//  Set !CS high
	if( SetCS( chip ) )
		return MEMFAIL;
	
	//	Check device ID against Device ID
	if( TData != MDEVICE )
		return MEMFAIL;
		
	return MEMSUCC;
}

/*************************************************************************
DEEP POWER-DOWN MODE

Deep Power-Down mode of the 25AA1024 is its
lowest power consumption state. The device will not
respond to any of the Read or Write commands while
in Deep Power-Down mode, and therefore it can be
used as an additional software write protection feature.
The Deep Power-Down mode is entered by driving CS
low, followed by the instruction code (Figure 2-11) onto
the SI line, followed by driving CS high.
If the CS pin is not driven high after the eighth bit of the
instruction code has been given, the device will not
execute Deep power-down. Once the CS line is driven
high, there is a delay (TDP) before the current settles
to its lowest consumption.

All instructions given during Deep Power-Down mode
are ignored except the Read Electronic Signature
Command (RDID). The RDID command will release
the device from Deep power-down and outputs the
electronic signature on the SO pin, and then returns
the device to Standby mode after delay (TREL)
Deep Power-Down mode automatically releases at
device power-down. Once power is restored to the
device, it will power-up in the Standby mode.
*************************************************************************/
short	SleepMem( short chip )
{
	//	Only do this if defined
	if( WP_USED )
		if( ClearWP( chip ) )
			return MEMFAIL;
		
	//	Set !CS low to talk
	if( ClearCS( chip ) )
		return MEMFAIL;
	
	//	Send Power Down command
	if( SendCommand( chip, MDPD ) )
		return MEMFAIL;
	
	//	Command successfully sent - set !CS high
	if( SetCS( chip ) )
		return MEMFAIL;
	
	
	//	Welp.  Chip is asleep.  Time to go home.
	return MEMSUCC;
}

/*************************************************************************
Read Status Register Instruction
(RDSR)
The Read Status Register instruction (RDSR) provides
access to the STATUS register. The STATUS register
may be read at any time, even during a write cycle. The
STATUS register is formatted as follows:
TABLE 2-2: STATUS REGISTER

The Write-In-Process (WIP) bit indicates whether the
25AA1024 is busy with a write operation. When set to
a ‘1’, a write is in progress, when set to a ‘0’, no write
is in progress. This bit is read-only.

The Write Enable Latch (WEL) bit indicates the status
of the write enable latch and is read-only. When set to
a ‘1’, the latch allows writes to the array, when set to a
‘0’, the latch prohibits writes to the array. The state of
this bit can always be updated via the WREN or WRDI
commands regardless of the state of write protection
on the STATUS register. These commands are shown
in Figure 2-4 and Figure 2-5.

The Block Protection (BP0 and BP1) bits indicate
which blocks are currently write-protected. These bits
are set by the user issuing the WRSR instruction. These
bits are nonvolatile and are shown in Table 2-3.
See Figure 2-6 for the RDSR timing sequence.

WARNING:  Leaves CS low at exit
*************************************************************************/
short	ReadMemStatus( short chip, short* status )
{
	short	temp;
	
	//  Read the status register to ensure the WREN bit was set (or BP bits, etc)
	//	Send the command to read the status register
	if( SendCommand( chip, MRDSR ) )
		return MEMFAIL;
		
	//	Read back the data
	if( ReadByte( &temp ) )
		return MEMFAIL;
	
	*status = temp;
	
	return MEMSUCC;		
}

/*************************************************************************
Write Status Register Instruction
(WRSR)
The Write Status Register instruction (WRSR) allows the
user to write to the nonvolatile bits in the STATUS
register as shown in Table 2-2. The user is able to
select one of four levels of protection for the array by
writing to the appropriate bits in the STATUS register.
The array is divided up into four segments. The user
has the ability to write-protect none, one, two, or all four
of the segments of the array. The partitioning is
controlled as shown in Table 2-3.

The Write-Protect Enable (WPEN) bit is a nonvolatile
bit that is available as an enable bit for the WP pin. The
Write-Protect (WP) pin and the Write-Protect Enable
(WPEN) bit in the STATUS register control the
programmable hardware write-protect feature. Hardware
write protection is enabled when WP pin is low
and the WPEN bit is high. Hardware write protection is
disabled when either the WP pin is high or the WPEN
bit is low. When the chip is hardware write-protected,
only writes to nonvolatile bits in the STATUS register
are disabled. See Table 2-4 for a matrix of functionality
on the WPEN bit.

See Figure 2-7 for the WRSR timing sequence

Get that!?  Gotta set the WPEN bit to write protect the memory!
*************************************************************************/
short	WriteMemStatus( short chip, short* status )
{
	short	temp;
	short	new_status;
	
	temp = *status;
	
	//  De-assert the !WP pin
	if( SetWP( chip ) )
		return MEMFAIL;
	
	//	Send the command to write the status register
	if( SendCommand( chip, MWRSR ) )
		return MEMFAIL;
		
	//	Send the status byte
	if( SendByte( temp ) )
		return MEMFAIL;

	//  Re-assert the !WP pin
	if( ClearWP( chip ) )
		return MEMFAIL;
	
	//  Read back the status for verification
	if( ReadMemStatus( chip, &new_status) )
		return MEMFAIL;
		
	if( (temp & 0x8C) != (new_status & 0x8C) )
		return MEMFAIL;
		
	return MEMSUCC;		
}


/*************************************************************************
based on the input chip number, returns the appropriate CS for control
input:
	short	chip (0-4)
returns:
	short	CS
*************************************************************************/
inline short	GetCS( short chip )
{
	switch( chip )
	{
		case 0: return CS0;
		case 1: return CS1;
		case 2:	return CS2;
		case 3: return CS3;
		default: return CS0;
	}
	
	return CS0;
}

/*************************************************************************
based on the input chip number, returns the appropriate WP for control
input:
	short	chip (0-4)
returns:
	short	WP
*************************************************************************/
short	GetWP( short chip )
{
	switch( chip )
	{
		case 0: return WP0;
		case 1: return WP1;
		case 2:	return WP2;
		case 3: return WP3;
		default: return WP0;
	}
	
	return WP0;
}

/*************************************************************************
Initializes the selected memory (sets control inputs to known values)
	Also "wakes" memory
input:
	short	chip (0-4)
Affects:
	Appropriate CS line (low)
	Appropriate WP line (hi)
*************************************************************************/
short	InitMem( short chip )
{

	switch ( chip )
	{
		case 0: 
			ContDDR |= (1<<CS0);
			ContDDR |= (1<<WP0);
			break;
		case 1: 
			ContDDR |= (1<<CS1);
			ContDDR |= (1<<WP1);
			break;
		case 2: 
			ContDDR |= (1<<CS2);
			ContDDR |= (1<<WP2);
			break;
		case 3: 
			ContDDR |= (1<<CS3);
			ContDDR |= (1<<WP3);
			break;
		default:
			return MEMFAIL;
	}
/*	
	//	Wake the memory
	if( WakeMem( chip ) )
		return MEMFAIL;
*/		
	//  Set !WP low (if used)
	if( WP_USED )
		if( ClearWP( chip ) )	
			return MEMFAIL;
			
	return MEMSUCC;
}

void CloseMem( short chip )
{
	switch ( chip )
	{
		case 0: 
			ContDDR &= ~(1<<CS0);
			ContDDR &= ~(1<<WP0);
			break;
		case 1: 
			ContDDR &= ~(1<<CS1);
			ContDDR &= ~(1<<WP1);
			break;
		case 2: 
			ContDDR &= ~(1<<CS2);
			ContDDR &= ~(1<<WP2);
			break;
		case 3: 
			ContDDR &= ~(1<<CS3);
			ContDDR &= ~(1<<WP3);
			break;
		default:
			return;
	}
	
}


/*************************************************************************
Sends a command and a related address to the selected chip

Inputs:
	short chip	 - the chip to be tested
	short Command	 - chip command
	long Address - the address to be tested
	
Returns
	MEMSUCC	- on success
	MEMFAIL	- on failure
	
NOTE:  Leaves !CS set low on exit	
*************************************************************************/
inline short	SendCommandAndAddress( short chip, short Command, long Address )
{
	//	Send the command to the selected chip
	if( SendCommand( chip, Command ) )
		return MEMFAIL;
		
	if( SendAddress( chip, Address ) )
		return MEMFAIL;
		
	return MEMSUCC;
}

/*************************************************************************
Sends a command to the selected chip

Inputs:
	short chip	 - the chip to be tested
	short Command	 - chip command
	
Returns
	MEMSUCC	- on success
	MEMFAIL	- on failure
	
NOTE:  Leaves !CS set low on exit	
*************************************************************************/
inline short	SendCommand( short chip, short Command )
{
	//	Set CS low
	ClearCS( chip );
	
	//  Send the memory the command
	if( SendByte( Command ) )
		return MEMFAIL;
		
	return MEMSUCC;	
}


/*************************************************************************
Sends a byte

Inputs:
	short Command	 - chip command
	
Returns
	MEMSUCC	- on success
	MEMFAIL	- on failure
	
NOTE:  Leaves !CS set low on exit	
NOTE:	Unlike SendCommand(), this function expects that !CS is 
		already set low.  (and memory already selected)
*************************************************************************/
inline short	SendByte( short byte )
{
	//  Send the memory the command
	if( SPI_Write_Byte( byte ) )
		return MEMFAIL;
		
	return MEMSUCC;	
}


/*************************************************************************
Sends an address to the selected chip

Inputs:
	short chip	 - the chip to be tested
	long Address - the address to be tested
	
Returns
	MEMSUCC	- on success
	MEMFAIL	- on failure
	
NOTE:  Assumes !CS is already low on entrance
		(we should never be sending an address without 1st sending a command)
*************************************************************************/
inline short	SendAddress( short chip, long Address )
{
	short	Addr_Hi;		//  High byte of address (Most significant)
	short	Addr_LoHi;		//	Middle byte of address
	short	Addr_LoLo;		//	Lowest byte of address (Least significant)
	
	//  Split up the address into its various bytes for transmission
	Addr_Hi = (short)((Address >> 16) & 0x000000FF);
	Addr_LoHi = (short)((Address >> 8) & 0x000000FF);
	Addr_LoLo = (short)(Address & 0x000000FF);
	
	//  Send the address
	if( SendByte( Addr_Hi ) )
		return MEMFAIL;

	if( SendByte( Addr_LoHi ) )
		return MEMFAIL;

	if( SendByte( Addr_LoLo ) )
		return MEMFAIL;
		
	return MEMSUCC;	
}

/*************************************************************************
Reads a byte from the selected chip

Inputs:
	short chip	 - the chip to be tested
	
Returns
	short*	byte - pointer to the data loaction
	MEMSUCC	- on success
	MEMFAIL	- on failure
	
NOTE:  Leaves !CS set low on exit	

NOTE:  There should never be data coming from the memory if !CS is not low
		Therefore, we are assuming that CS is low at the start of this function
*************************************************************************/
inline short	ReadByte( short* byte )
{
	//	Read back the data
	if( SPI_Read_Byte( SPIFALSE, byte ) )
		return MEMFAIL;
		
	return MEMSUCC;	
}


/*************************************************************************
Sets !CS high for selected chip (enable comms)

Inputs:
	short chip	 - the chip to be tested
	
Returns
	MEMSUCC	- on success
	MEMFAIL	- on failure
	
NOTE:  Leaves !CS set high on exit	

NOTE:  Don't call this for something timing critical/sensitive, 
		as error checking uses up a few clock ticks
*************************************************************************/
short	SetCS( short chip )
{
	short	CCS;
	short	temp;
	
	//  get correct CS for this device
	CCS = GetCS( chip );
	
	//  Ensure !CS is low for comms
	ContPort |= (1<<CCS);	

	//  Wait a bit to ensure outputs are latched and settled
	_NOP();
	
	//  Read back the port
	temp = RContPort;
	
	if( (temp & (1<<CCS)) )		//	Should NOT be zero
		return MEMSUCC;
		
	return MEMFAIL;
}

/*************************************************************************
Sets !CS low for selected chip (enable comms)

Inputs:
	short chip	 - the chip to be tested
	
Returns
	MEMSUCC	- on success
	MEMFAIL	- on failure
	
NOTE:  Leaves !CS set low on exit	

NOTE:  Don't call this for something timing critical/sensitive, 
		as error checking uses up a few clock ticks
*************************************************************************/
short	ClearCS( short chip )
{
	short	CCS;
	short	temp;
	
	//  get correct CS for this device
	CCS = GetCS( chip );
	
	//  Ensure !CS is low for comms
	ContPort &= ~(1<<CCS);	

	//  Wait a bit to ensure outputs are latched and settled
	_NOP();
	
	//  Read back the port
	temp = RContPort;
	
	if( !(temp & (1<<CCS)) )		//	Should be zero
		return MEMSUCC;
		
	return MEMFAIL;
}


/*************************************************************************
Sets !WP high for selected chip (disable write protection)

Inputs:
	short chip	 - the chip to be tested
	
Returns
	MEMSUCC	- on success
	MEMFAIL	- on failure
	
NOTE:  Leaves !WP set high on exit	

NOTE:  Don't call this for something timing critical/sensitive, 
		as error checking and pin lookup uses up a few clock ticks
*************************************************************************/
short	SetWP( short chip )
{
	short	CWP;
	short	temp;
	
	//	get the correct WP (for this device)	
	CWP = GetWP( chip );
	
	//	Set it high
	ContPort |= (1<<CWP);
	
	//	Wait a bit
	_NOP();
	
	//  Read back the port
	temp = RContPort;
	
	if( (temp & (1<<CWP)) )		//	Should NOT be zero
		return MEMSUCC;
		
	return MEMFAIL;
}

/*************************************************************************
Sets !WP low for selected chip (Write Protect)

Inputs:
	short chip	 - the chip to be tested
	
Returns
	MEMSUCC	- on success
	MEMFAIL	- on failure
	
NOTE:  Leaves !WP set low on exit	

NOTE:  Don't call this for something timing critical/sensitive, 
		as error checking and lookups use up a few clock ticks
*************************************************************************/
short	ClearWP( short chip )
{
	short	CWP;
	short	temp;
	
	//  get correct CS for this device
	CWP = GetWP( chip );
	
	//  Ensure !CS is low for comms
	ContPort &= ~(1<<CWP);	

	//  Wait a bit to ensure outputs are latched and settled
	_NOP();
	
	//  Read back the port
	temp = RContPort;
	
	if( !(temp & (1<<CWP)) )		//	Should be zero
		return MEMSUCC;
		
	return MEMFAIL;
}


/*************************************************************************
Checks the WIP bit in the status word 
*************************************************************************/
short	CheckWIP( short chip )
{
	short	status;
	
	//	Get the status
	if( ReadMemStatus( chip, &status ) )
		return MEMFAIL;
		
	if( (status & (1<<MWIP)) )	//  Will be 1 until WIP bit is cleared
		return MEMTRUE;
		
	return MEMFALSE;
}


/*************************************************************************
Computes the page in which the address resides
*************************************************************************/
int		GetPage( long Address, int* page )
{
	int	temp;
	
	temp = (int)(Address/PAGE_SIZE);
	
	page = &temp;
	
	page--; //	(we start with zero, right?)
	
	if( temp > 511 )
		return MEMFAIL;
		
	return MEMSUCC;
}

/*  Returns the minimum of two integers */
int		Min( int num1, int num2 )
{
	if(num1<num2)
		return num1;
	
	return num2;
}