/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/***************************************************************************/
/*                                                                         */
/*                         HARDware PCEngine                               */
/*                                                                         */
/* This source file implements all functions relatives to pc engine inner  */
/* hardware (memory access e.g.)                                           */
/*                                                                         */
/***************************************************************************/

#include "utils.h"
#include "hard_pce.h"
#include "pce.h"
#if defined(BSD_CD_HARDWARE_SUPPORT)
#include "pcecd.h"
#else
	// nothing yet on purpose
#endif

/**
  * Variables declaration
  * cf explanations in the header file
  **/

struct_hard_pce *hard_pce;

UChar *RAM;

// Video
UInt16 *SPRAM;
UChar *VRAM2;
UChar *VRAMS;
UChar  *Pal;
UChar  *vchange;
UChar  *vchanges;
UChar  *WRAM;
UChar  *VRAM;
UInt32 *p_scanline;

// Audio
UChar *PCM;

// I/O
IO *p_io;

// CD
/**/ UChar * cd_read_buffer;
UChar *cd_sector_buffer;
UChar *cd_extra_mem;
UChar *cd_extra_super_mem;
UChar *ac_extra_mem;

UInt32 pce_cd_read_datacnt;
/**/ UChar cd_sectorcnt;
UChar pce_cd_curcmd;
/**/
// Memory
UChar *zp_base;
UChar *sp_base;
UChar *mmr;
UChar *IOAREA;

// Interruption
UInt32 *p_cyclecount;
UInt32 *p_cyclecountold;

const UInt32 TimerPeriod = 1097;

// registers

#if defined(SHARED_MEMORY)

//! Shared memory handle
static int shm_handle;

UInt16 *p_reg_pc;
UChar *p_reg_a;
UChar *p_reg_x;
UChar *p_reg_y;
UChar *p_reg_p;
UChar *p_reg_s;

#else

UInt16 reg_pc;
UChar reg_a;
UChar reg_x;
UChar reg_y;
UChar reg_p;
UChar reg_s;

#endif

// Mapping
UChar *PageR[8];
UChar *ROMMapR[256];

UChar *PageW[8];
UChar *ROMMapW[256];

UChar* trap_ram_read;
UChar* trap_ram_write;

// Miscellaneous
UInt32 *p_cycles;
SInt32 *p_external_control_cpu;

//! External rom size hack for shared memory indication
extern int ROM_size;

/**
  * Predeclaration of access functions
	**/
UChar read_memory_simple(UInt16);
UChar read_memory_sf2(UInt16);
UChar read_memory_arcade_card(UInt16);

void write_memory_simple(UInt16,UChar);
void write_memory_arcade_card(UInt16,UChar);



//! Function to write into memory. Defaulted to the basic one
void (*write_memory_function)(UInt16,UChar) = write_memory_simple;

//! Function to read from memory. Defaulted to the basic one
UChar (*read_memory_function)(UInt16) = read_memory_simple;

/**
  * Initialize the hardware
  **/
void
hard_init (void)
{

	trap_ram_read = malloc(0x2000);
	trap_ram_write = malloc(0x2000);

	if ((trap_ram_read == NULL) || (trap_ram_write == NULL))
		fprintf(stderr, "Couldn't allocate trap_ram* (%s:%d)", __FILE__, __LINE__);

#if defined(SHARED_MEMORY)
	shm_handle =
		shmget ((key_t) SHM_HANDLE, sizeof (struct_hard_pce),
			IPC_CREAT | IPC_EXCL | 0666);
	if (shm_handle == -1)
		fprintf (stderr, "Couldn't get shared memory\n");
	else
	{
		hard_pce = (struct_hard_pce *) shmat (shm_handle, NULL, 0);
		if (hard_pce == NULL)
			fprintf (stderr, "Couldn't attach shared memory\n");

		p_reg_pc = &hard_pce->s_reg_pc;
		p_reg_a = &hard_pce->s_reg_a;
		p_reg_x = &hard_pce->s_reg_x;
		p_reg_y = &hard_pce->s_reg_y;
		p_reg_p = &hard_pce->s_reg_p;
		p_reg_s = &hard_pce->s_reg_s;
		p_external_control_cpu = &hard_pce->s_external_control_cpu;

	}
#else
	hard_pce = (struct_hard_pce *) malloc(sizeof(struct_hard_pce));
#endif

	memset(hard_pce, 0, sizeof(struct_hard_pce));

	RAM = hard_pce->RAM;
	PCM = hard_pce->PCM;
	WRAM = hard_pce->WRAM;
	VRAM = hard_pce->VRAM;
	VRAM2 = hard_pce->VRAM2;
	VRAMS = (UChar*)hard_pce->VRAMS;
	vchange = hard_pce->vchange;
	vchanges = hard_pce->vchanges;

	cd_extra_mem = hard_pce->cd_extra_mem;
	cd_extra_super_mem = hard_pce->cd_extra_super_mem;
	ac_extra_mem = hard_pce->ac_extra_mem;
	cd_sector_buffer = hard_pce->cd_sector_buffer;

	SPRAM = hard_pce->SPRAM;
	Pal = hard_pce->Pal;

	p_scanline = &hard_pce->s_scanline;

	p_cyclecount = &hard_pce->s_cyclecount;
	p_cyclecountold = &hard_pce->s_cyclecountold;

	p_cycles = &hard_pce->s_cycles;

	mmr = hard_pce->mmr;

	p_io = &hard_pce->s_io;

#if defined(SHARED_MEMORY)
	/* Add debug on beginning option by setting 0 here */
	external_control_cpu = -1;

	hard_pce->rom_shared_memory_size = 0x2000 * ROM_size;

#endif

	if ((option.want_arcade_card_emulation) && (CD_emulation > 0))
		{
			read_memory_function = read_memory_arcade_card;
			write_memory_function = write_memory_arcade_card;
		}
	else
		{
			read_memory_function = read_memory_simple;
			write_memory_function = write_memory_simple;
		}
}

/**
  *  Terminate the hardware
  **/
void
hard_term (void)
{
#if defined(SHARED_MEMORY)
	if (shmctl (shm_handle, IPC_RMID, NULL) == -1)
		fprintf (stderr, "Couldn't destroy shared memory\n");
#else
	free(hard_pce);
#endif
	free(trap_ram_read);
	free(trap_ram_write);
}

/**
 * Functions to access PCE hardware
 **/

int return_value_mask_tab_0002[32] =
  {
    0xFF,
    0xFF,
    0xFF,
    0xFF, /* unused */
    0xFF, /* unused */
    0xFF,
    0xFF,
    0xFF,
    0xFF, /* 8 */
    0xFF,
    0x1F, /* A */
    0x7F,
    0x1F, /* C */
    0xFF,
    0xFF, /* E */
    0x1F,
    0xFF, /* 10 */
    0xFF,
    /* No data for remaining reg, assuming 0xFF */
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF
  };

int return_value_mask_tab_0003[32] =
  {
    0xFF,
    0xFF,
    0xFF,
    0xFF, /* unused */
    0xFF, /* unused */
    0x1F,
    0x03,
    0x03,
    0x01, /* 8 */	/* ?? */
    0x00,
    0x7F, /* A */
    0x7F,
    0xFF, /* C */
    0x01,
    0x00, /* E */
    0x00,
    0xFF, /* 10 */
    0xFF,
    /* No data for remaining reg, assuming 0xFF */
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF
  };


int return_value_mask_tab_0400[8] =
  {
    0xFF,
    0x00,
    0xFF,
    0x01,
    0xFF,
    0x01,
    0xFF, /* unused */
    0xFF  /* unused */
  };

int return_value_mask_tab_0800[16] =
  {
    0x03,
    0xFF,
    0xFF,
    0x0F,
    0xDF,
    0xFF,
    0x1F,
    0x9F,
    0xFF,
    0x83,
    /* No data for remainig reg, assuming 0xFF */
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF
  };

int return_value_mask_tab_0c00[2] =
  {
    0x7F,
    0x01
  };

int return_value_mask_tab_1400[4] =
  {
    0xFF,
    0xFF,
    0x03,
    0x03
  };


//! Returns the useful value mask depending on port value
static int
return_value_mask(UInt16 A)
{
  if (A < 0x400) // VDC
    {
      if ((A & 0x3) == 0x02)
        {
          return return_value_mask_tab_0002[io.vdc_reg];
        }
      else
        if ((A & 0x3) == 0x03)
          {
            return return_value_mask_tab_0003[io.vdc_reg];
          }
        else
          return 0xFF;
    }

  if (A < 0x800) // VCE
    return return_value_mask_tab_0400[A & 0x07];

  if (A < 0xC00) /* PSG */
    return return_value_mask_tab_0800[A & 0x0F];

  if (A < 0x1000) /* Timer */
    return return_value_mask_tab_0c00[A & 0x01];

  if (A < 0x1400) /* Joystick / IO port */
    return 0xFF;

  if (A < 0x1800) /* Interruption acknowledgement */
    return return_value_mask_tab_1400[A & 0x03];

  /* We don't know for higher ports */
  return 0xFF;
}

/* read */
UChar
IO_read_raw (UInt16 A)
{
	UChar ret;

#ifndef FINAL_RELEASE
	if ((A & 0x1F00) == 0x1A00)
		Log ("AC Read at %04x\n", A);
#endif

	switch (A & 0x1FC0)
	{
	case 0x0000:		/* VDC */
		switch (A & 3)
		{
		case 0:
#if defined(GFX_DEBUG)
			gfx_debug_printf("Returning vdc_status = 0x%02x", io.vdc_status);
#endif
			ret = io.vdc_status;
			io.vdc_status = 0;	//&=VDC_InVBlank;//&=~VDC_BSY;
#if defined(GFX_DEBUG)
			Log("$0000 returns %02X\n", ret);
#endif
			return ret;
		case 1:
			return 0;
		case 2:
			if (io.vdc_reg == VRR)
				return VRAM[io.VDC[MARR].W * 2];
			else
				return io.VDC[io.vdc_reg].B.l;
		case 3:
			if (io.vdc_reg == VRR)
			{
				ret = VRAM[io.VDC[MARR].W * 2 + 1];
				io.VDC[MARR].W += io.vdc_inc;
				return ret;
			}
			else
				return io.VDC[io.vdc_reg].B.h;
		}
		break;

	case 0x0400:		/* VCE */
		switch (A & 7)
		{
		case 4:
			return io.VCE[io.vce_reg.W].B.l;
		case 5:
			return io.VCE[io.vce_reg.W++].B.h;
		}
		break;
	case 0x0800:		/* PSG */
		switch (A & 15)
		{
		case 0:
			return io.psg_ch;
		case 1:
			return io.psg_volume;
		case 2:
			return io.PSG[io.psg_ch][2];
		case 3:
			return io.PSG[io.psg_ch][3];
		case 4:
			return io.PSG[io.psg_ch][4];
		case 5:
			return io.PSG[io.psg_ch][5];
		case 6:
		{
			int ofs = io.PSG[io.psg_ch][PSG_DATA_INDEX_REG];
			io.PSG[io.psg_ch][PSG_DATA_INDEX_REG] = (UChar)((io.PSG[io.psg_ch][PSG_DATA_INDEX_REG] + 1) & 31);
			return io.wave[io.psg_ch][ofs];
		}
		case 7:
			return io.PSG[io.psg_ch][7];

		case 8:
			return io.psg_lfo_freq;
		case 9:
			return io.psg_lfo_ctrl;
		default:
			return NODATA;
		}
		break;
	case 0x0c00:		/* timer */
		return io.timer_counter;

	case 0x1000:		/* joypad */
		ret = io.JOY[io.joy_counter] ^ 0xff;
		if (io.joy_select & 1)
			ret >>= 4;
		else
		{
			ret &= 15;
			io.joy_counter = (UChar)((io.joy_counter + 1) % 5);
		}

/* return ret | Country; *//* country 0:JPN 1<<6=US */
		return ret | 0x30; // those 2 bits are always on, bit 6 = 0 (Jap), bit 7 = 0 (Attached cd)

	case 0x1400:		/* IRQ */
		switch (A & 15)
		{
		case 2:
			return io.irq_mask;
		case 3:
			ret = io.irq_status;
			io.irq_status = 0;
			return ret;
		}
		break;


	case 0x18C0:		// Memory management ?
		switch (A & 15)
			{
				case 5:
				case 1:
					return 0xAA;
				case 2:
				case 6:
					return 0x55;
				case 3:
				case 7:
					return 0x03;
			}
		break;

	case 0x1AC0:
		switch (A & 15)
		{
		case 0:
			return (UChar) (io.ac_shift);
		case 1:
			return (UChar) (io.ac_shift >> 8);
		case 2:
			return (UChar) (io.ac_shift >> 16);
		case 3:
			return (UChar) (io.ac_shift >> 24);
		case 4:
			return io.ac_shiftbits;
		case 5:
			return io.ac_unknown4;
		case 14:
			return (UChar)(option.want_arcade_card_emulation ? 0x10 : NODATA);
		case 15:
			return (UChar)(option.want_arcade_card_emulation ? 0x51 : NODATA);
		default:
			Log ("Unknown Arcade card port access : 0x%04X\n", A);
		}
		break;

	case 0x1A00:
	{
		UChar ac_port = (UChar)((A >> 4) & 3);
		switch (A & 15)
		{
		case 0:
		case 1:
			/*
			 * switch (io.ac_control[ac_port] & (AC_USE_OFFSET | AC_USE_BASE))
			 * {
			 * case 0:
			 * return ac_extra_mem[0];
			 * case AC_USE_OFFSET:
			 * ret = ac_extra_mem[io.ac_offset[ac_port]];
			 * if (!(io.ac_control[ac_port] & AC_INCREMENT_BASE))
			 * io.ac_offset[ac_port]+=io.ac_incr[ac_port];
			 * return ret;
			 * case AC_USE_BASE:
			 * ret = ac_extra_mem[io.ac_base[ac_port]];
			 * if (io.ac_control[ac_port] & AC_INCREMENT_BASE)
			 * io.ac_base[ac_port]+=io.ac_incr[ac_port];
			 * return ret;
			 * default:
			 * ret = ac_extra_mem[io.ac_base[ac_port] + io.ac_offset[ac_port]];
			 * if (io.ac_control[ac_port] & AC_INCREMENT_BASE)
			 * io.ac_base[ac_port]+=io.ac_incr[ac_port];
			 * else
			 * io.ac_offset[ac_port]+=io.ac_incr[ac_port];
			 * return ret;
			 * }
			 * return 0;
			 */


#if defined(CD_DEBUG) && !defined(FINAL_RELEASE)
			printf("Reading from AC main port. ac_port = %d. %suse offset. %sincrement. %sincrement base\n",
				ac_port,
				io.ac_control[ac_port] & AC_USE_OFFSET ? "": "not ",
				io.ac_control[ac_port] & AC_ENABLE_INC ? "": "not ",
				io.ac_control[ac_port] & AC_INCREMENT_BASE ? "" : "not ");
#endif
			if (io.ac_control[ac_port] & AC_USE_OFFSET)
				ret = ac_extra_mem[((io.ac_base[ac_port] +
						     io.
						     ac_offset[ac_port]) &
						    0x1fffff)];
			else
				ret = ac_extra_mem[((io.
						     ac_base[ac_port]) &
						    0x1fffff)];

			if (io.ac_control[ac_port] & AC_ENABLE_INC)
			{
				if (io.
				    ac_control[ac_port] & AC_INCREMENT_BASE)
					io.ac_base[ac_port] =
						(io.ac_base[ac_port] +
						 io.
						 ac_incr[ac_port]) & 0xffffff;
				else
					io.ac_offset[ac_port] = (UInt16)
						((io.ac_offset[ac_port] +
						 io.
						 ac_incr[ac_port]) & 0xffff);
			}

#if defined(CD_DEBUG) && !defined(FINAL_RELEASE)
			printf("Returned 0x%02x. now, base = 0x%x. offset = 0x%x, increment = 0x%x\n", ret, io.ac_base[ac_port], io.ac_offset[ac_port], io.ac_incr[ac_port]);
#endif
			return ret;


		case 2:
			return (UChar) (io.ac_base[ac_port]);
		case 3:
			return (UChar) (io.ac_base[ac_port] >> 8);
		case 4:
			return (UChar) (io.ac_base[ac_port] >> 16);
		case 5:
			return (UChar) (io.ac_offset[ac_port]);
		case 6:
			return (UChar) (io.ac_offset[ac_port] >> 8);
		case 7:
			return (UChar) (io.ac_incr[ac_port]);
		case 8:
			return (UChar) (io.ac_incr[ac_port] >> 8);
		case 9:
			return io.ac_control[ac_port];
		default:
			Log ("Unknown Arcade card port access : 0x%04X\n", A);
		}
		break;
	}
	case 0x1800:		// CD-ROM extention
#if defined(BSD_CD_HARDWARE_SUPPORT)
          return pce_cd_handle_read_1800(A);
#else
          return gpl_pce_cd_handle_read_1800(A);
#endif
	}
#ifndef FINAL_RELEASE
#if !defined(KERNEL_DS)
    fprintf (stderr, "ignore I/O read %04X\nat PC = %04X\n", A, M.PC.W);
#endif
#endif
	return NODATA;
}

//! Adds the io_buffer feature
UChar IO_read (UInt16 A)
{
  int mask;
  UChar temporary_return_value;

  if ((A < 0x800) || (A >= 0x1800)) // latch isn't affected out of the 0x800 - 0x1800 range
    return IO_read_raw(A);

  mask = return_value_mask(A);

  temporary_return_value = IO_read_raw(A);

  io.io_buffer = temporary_return_value | (io.io_buffer & ~mask);

  return io.io_buffer;
}

#if defined(TEST_ROM_RELOCATED)
extern UChar* ROM;
#endif

/**
  * Change bank setting
  **/
void
bank_set (UChar P, UChar V)
{

#if defined(CD_DEBUG)
  if (V >= 0x40 && V <= 0x43)
		printf("AC pseudo bank switching !!! (mmr[%d] = %d)\n", P, V);
#endif

#if defined(TEST_ROM_RELOCATED)
	if ((P >= 2) && ((V < 0x68) || (V >= 0x88)))
		{
			int physical_bank = mmr[reg_pc >> 13];
			if (physical_bank >= 0x68)
				physical_bank -= 0x68;
			printf("Relocation error PC = 0x%04x (logical bank 0x%0x(0x%02x), physical bank 0x%0x(0x%02x), offset 0x%04x, global offset 0x%x)\nBank %x into MMR %d\nPatching into BRK\n",
				reg_pc,
				mmr[reg_pc >> 13],
				mmr[reg_pc >> 13],
				physical_bank,
				physical_bank,
				reg_pc & 0x1FFF,
				physical_bank * 0x2000 + (reg_pc & 0x1FFF),
				V,
				P
			);
			if (V >= 0x80) {
				printf("Not a physical bank, aborting patching\n");
			} else {
				V += 0x68;
				patch_rom(cart_name, physical_bank * 0x2000 + (reg_pc & 0x1FFF), 0);
				ROM[physical_bank * 0x2000 + (reg_pc & 0x1FFF)] = 0;
			}
		}
fprintf(stderr, "Bank set MMR[%d]=%02x at %04x\n", P, V, reg_pc);
#endif

	mmr[P] = V;
	if (ROMMapR[V] == IOAREA)
		{
			PageR[P] = IOAREA;
			PageW[P] = IOAREA;
		}
	else
		{
			PageR[P] = ROMMapR[V] - P * 0x2000;
			PageW[P] = ROMMapW[V] - P * 0x2000;
		}
}

void write_memory_simple(UInt16 A, UChar V)
	{
		if (PageW[A >> 13] == IOAREA)
    	IO_write (A, V);
  	else
	    PageW[A >> 13][A] = V;
	}

void write_memory_sf2(UInt16 A, UChar V)
	{
		if (PageW[A >> 13] == IOAREA)
			IO_write (A, V);
		else
			/* support for SF2CE silliness */
			if ((A & 0x1ffc) == 0x1ff0)
				{
					int i;

					ROMMapR[0x40] = ROMMapR[0] + 0x80000;
					ROMMapR[0x40] += (A & 3) * 0x80000;

					for (i = 0x41; i <= 0x7f; i++)
						{
							ROMMapR[i] = ROMMapR[i - 1] + 0x2000;
							// This could be slightly sped up by setting a fixed RAMMapW
							ROMMapW[i] = ROMMapW[i - 1] + 0x2000;
						}
				}
			else
				PageW[A >> 13][A] = V;
	}

void write_memory_arcade_card(UInt16 A, UChar V)
	{
	  if ((mmr[A >> 13] >= 0x40)  && (mmr[A >> 13] <= 0x43))
			{
				/*
				#if defined(CD_DEBUG)
				fprintf(stderr, "writing 0x%02x to AC pseudo bank (%d)\n", V, mmr[A >> 13] - 0x40);
				#endif
				*/
				IO_write((UInt16)(0x1A00 + ((mmr[A >> 13] - 0x40) << 4)), V);
			}
		else
			if (PageW[A >> 13] == IOAREA)
				IO_write (A, V);
			else
				PageW[A >> 13][A] = V;
	}

UChar read_memory_simple(UInt16 A)
	{
	  if (PageR[A >> 13] != IOAREA)
	    return PageR[A >> 13][A];
  	else
    	return IO_read (A);
	}

UChar read_memory_arcade_card(UInt16 A)
	{
		if ((mmr[A >> 13] >= 0x40)  && (mmr[A >> 13] <= 0x43))
			{
				/*
				#if defined(CD_DEBUG)
				fprintf(stderr, "reading AC pseudo bank (%d)\n", mmr[A >> 13] - 0x40);
				#endif
				*/
				return IO_read((UInt16)(0x1A00 + ((mmr[A >> 13] - 0x40) << 4)));
			}
		else
			if (PageR[A >> 13] != IOAREA)
				return PageR[A >> 13][A];
			else
				return IO_read (A);
	}

static char opcode_long_buffer[256];
static UInt16 opcode_long_position;

char * get_opcode_long()
	{
		//! size of data used by the current opcode
    int size;

		unsigned char opcode;

		//! Buffer of opcode data, maximum is 7 (for xfer opcodes)
		unsigned char opbuf[7];

		int i;

		opcode = Rd6502(opcode_long_position);

		size	= addr_info_debug[optable_debug[opcode].addr_mode].size;

	  opbuf[0] = opcode;
	  opcode_long_position++;
	  for (i = 1; i < size; i++)
	    opbuf[i] = Rd6502 (opcode_long_position++);

	  /* This line is the real 'meat' of the disassembler: */

      (*addr_info_debug[optable_debug[opcode].addr_mode].func)      /* function      */
	    	(opcode_long_buffer, opcode_long_position - size, opbuf, optable_debug[opcode].opname);	/* parm's passed */

		return opcode_long_buffer;

	}


void dump_pce_cpu_environment() {

	int i;

	Log("Dumping PCE cpu environement\n");

	Log("PC = 0x%04x\n", reg_pc);
	Log("A = 0x%02x\n", reg_a);
	Log("X = 0x%02x\n", reg_x);
	Log("Y = 0x%02x\n", reg_y);
	Log("P = 0x%02x\n", reg_p);
	Log("S = 0x%02x\n", reg_s);

	for (i = 0; i < 8; i++)
	  {
		 Log("MMR[%d] = 0x%02x\n", i, mmr[i]);
	  }

	opcode_long_position = reg_pc & 0xE000;

	while (opcode_long_position <= reg_pc)
		{
			Log("%04X: %s\n", opcode_long_position, get_opcode_long());
		}

	Log("--------------------------------------------------------\n");

}
