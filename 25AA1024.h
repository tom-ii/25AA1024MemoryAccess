/*
 * _25AA1024.h
 *
 * Created: 5/27/2012 6:53:18 PM
 *  Author: Tom Burke
 *			tomburkeii@gmail.com
 */ 

/*************************************************************************
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

NOTE:  All WRITE operations (WRITE, ERASE, etc) require the time to perform 
	the write *PLUS* 6 ms.  The 6ms is internal to the memory to actually
	"burn" the bits.  Thus, a single byte write takes Tw+6ms, whereas
	writing a whole page is 255*Tw+6ms.  Most likely much faster
	
NOTE: while you cannot normally write beyond a page boundary, the code 
	is set to handle this eventuality.  Just don't try to allocate
	more memory than you have, right?

NOTE:  This code may not work if you are running a particularly slow
	IO rate (SCLK) or particularly fast..  I have not done any work 
	towards "jiggling" the !HOLD pin after individual transfers, 
	so it may be easy to miss some stuff.
*************************************************************************/
#include <avr/io.h>


#ifndef _25AA1024_H_
#define _25AA1024_H_

#define	MEMSUCC		0			//	Success
#define MEMFAIL		2			//	Failure
#define MEMTRUE		1
#define MEMFALSE	0

#define WP_USED		MEMFALSE	//  if microcontroller is running the !WP pin(s) (e.g., not hardwired), then change this to MEMTRUE

//  Constants
#define PAGE_SIZE	256			//	Device page size
#define NUM_PAGES	512			//	Number of pages in the device
#define MEMSIZE		0x01FFFF	//	Size of memory (in bytes)

//	These constants are used for checking valid page addresses during erases - 
//		these represent the HIGH address of the UNPROTECTED parts of the memory
#define BP00		0x01FFFF	//	Entire array unprotected
#define	BP01		0x017FFF	//	Lower 3/4 unprotected
#define BP10		0x00FFFF	//	Lower 1/2 unprotected
#define BP11		0x000000	//	Entire array protected


#define IO_SPEED	8			//	This is the IO speed of the controlling device (in MHz)
								//		(e.g. this particular microcontroller)
								//		This is important, as the !CS for the memory IC looks to have a 
								//		Setup and Hold time of ~150ns or so.  If CS is toggled too quickly
								//		after, say, a WREN command, then the write might fail.  Various functions 
								//		look at this constant for timing.  (must be less than 255)

//  TODO:  Extend to the use of multiple chips on same device - up to 4
#define CS0			PORTA0		//  This is the pin that is tied to the memory's !CS pin (mem 0)
#define CS1			PORTA1		//  This is the pin that is tied to the memory's !CS pin (mem 1)
#define CS2			PORTA2		//  This is the pin that is tied to the memory's !CS pin (mem 2)
#define CS3			PORTA3		//  This is the pin that is tied to the memory's !CS pin (mem 3)
#define ContPort	PORTA		//	Port controlling the memory (for writes)
#define RContPort	PINA		//	Port controlling the memory(ies) (for readback)
#define ContDDR		DDRA

#define WP0			PORTA4		//	Pin tied to the memory's !WP pin (if used - mem 0)
#define WP1			PORTA5		//	Pin tied to the memory's !WP pin (if used - mem 1)
#define WP2			PORTA6		//	Pin tied to the memory's !WP pin (if used - mem 2)
#define WP3			PORTA7		//	Pin tied to the memory's !WP pin (if used - mem 3)


#define HOLDPort	PORTB		//	It's not on the control port (see below), so sue me...
#define HOLD		PORTB3		//	Pin attached to the !HOLD pin.  Tied to AVR's !RESET pin, in this instance
#define HOLDDDR		DDRB		//		This is wired this way so that in-circuit programming, which pulls !RESET
								//		low, will "tell" the 25AA memory to ignore all inputs during the 
								//		AVR programming...
								


//  Memory commands
#define MREAD	0x03	//	Read data from memory array beginning at selected address
#define MWRITE	0x02	//	Write data to memory array beginning at selected address
#define MWREN	0x06	//	Set the write enable latch (enable write operations)
#define MWRDI	0x04	//	Reset the write enable latch (disable write operations)
#define MRDSR	0x05	//	Read STATUS register
#define MWRSR	0x01	//	Write STATUS register
#define MPE		0x42	//	Page Erase – erase one page in memory array
#define MSE		0xD8	//	Sector Erase – erase one sector in memory array
#define MCE		0xC7	//	Chip Erase – erase all sectors in memory array
#define MRDID	0xAB	//	Release from Deep power-down and read electronic signature
#define MDPD	0xB9	//	Deep Power-Down mode

//	Status Register Bits
#define MWIP		0
#define MWEL		1
#define MBP0		2
#define	MBP1		3

//	Device ID
#define MDEVICE		0x29	//  Manufacturer's ID

		

//	Variables


//	Functions
short	ReadData( short chip, long Address, long NumBytes, short* data );
short	WriteData( short chip, long Address, long NumBytes, short* data );
short	WriteEnable( short	chip );
short	WriteDisable( short chip );
short	ErasePage( short chip, long Address );
short	EraseSector( short chip, long Address );
short	EraseChip( short chip );
short	WakeMem( short chip );
short	SleepMem( short chip );
short	ReadMemStatus( short chip, short* status );
short	WriteMemStatus( short chip, short* status );
short	GetCS( short chip );
short	GetWP( short chip );
short	InitMem( short chip );
short	CheckProtect( short chip, long Address );
short	SendCommandAndAddress( short chip, short Command, long Address );
short	SendCommand( short chip, short Command );
short	SendAddress( short chip, long Address );
short	ReadByte( short* byte );
short	SendByte( short byte );
short	SetCS( short chip );
short	ClearCS( short chip );
short	SetWP( short chip );
short	ClearWP( short chip );
short	CheckWIP( short chip );
int		GetPage( long Address, int* page );
int		Min( int num1, int num2 );		//  I thought this was a part of std C.  huh...
void	CloseMem( short chip );

//	Macros
#define _NOP() asm volatile ("nop" :: )		//  Needed for AVR

#endif /* 25AA1024_H_ */