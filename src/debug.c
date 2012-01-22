/***************************************************************************/
/*                                                                         */
/*                       Debugging Source File                             */
/*                                                                         */
/*  Functions are here to help developpers making cool games and/or help   */
/*  hackers modifying (i.e. changing sprites, fonts, translating, putting  */
/*  training, ...) existing games                                          */
/*                                                                         */
/*  First intended to help Dave Shadoff with his open source game          */
/*  Now helps me and others to emulate CDs                                 */
/*                                                                         */
/***************************************************************************/
#include "debug.h"

UInt16 Bp_pos_to_restore;

// Used in RESTORE_BP to know where we must restore a BP

Breakpoint Bp_list[MAX_BP];

// Set of breakpoints

UChar save_background = 1;


#ifdef ALLEGRO

// If set, means we got to preserve the true display
// If not, we just go ahead e.g. for stepping

PALETTE save_pal;

// The true palette

BITMAP *save_bg;

// The true display


#endif

/* Way to accedate to the PC Engine memory */
/*
#if defined(MSDOS) || defined(WIN32)

unsigned char
Op6502 (register unsigned A)
{
  register char __AUX;

  __asm__ __volatile__ ("movl _PageR (, %%eax, 4), %%ecx "
			"movb (%%edx, %%ecx), %%al "
                        :"=a" (__AUX)
                        :"d" (A),
			"a" (A >> 13):"%ebx", "%ecx");

  return __AUX;
};

#else 
       */

unsigned char
Op6502 (unsigned int A)
{
  return (PageR[A >> 13][A]);
}

/*
#endif 
        */

void
disass_menu ()
{

#ifdef ALLEGRO
  if (save_background)

    {

      save_bg = create_bitmap (vwidth, vheight);

      blit (screen, save_bg, 0, 0, 0, 0, vwidth, vheight);

      get_palette (save_pal);

    }
#endif

#ifndef KERNEL_DS
  disassemble (M.PC.W);
#else
  disassemble (reg_pc);
#endif

#ifdef ALLEGRO
  if (save_background)

    {

      set_palette (save_pal);

      blit (save_bg, screen, 0, 0, 0, 0, vwidth, vheight);

      destroy_bitmap (save_bg);

    }

#endif
  return;

};


/*****************************************************************************

    Function:  toggle_user_breakpoint

    Description: set a breakpoint at the specified address if don't exist
	              or unset it if existant
    Parameters: unsigned short where, a PCE-style address
    Return: 0 on error
	         1 on success

*****************************************************************************/

int
toggle_user_breakpoint (UInt16 where)
{

  UChar dum;

  for (dum = 0; dum < MAX_USER_BP; dum++)

    {


      if ((Bp_list[dum].position == where) && (Bp_list[dum].flag != NOT_USED))

	{			//Already set, unset it and put the right opcode

	  Bp_list[dum].flag = NOT_USED;

	  Wr6502 (where, Bp_list[dum].original_op);

	  return 1;

	}


      if (Bp_list[dum].flag == NOT_USED)

	break;


    }

  if (dum == MAX_USER_BP)

    return 0;


  Bp_list[dum].flag = ENABLED;

  Bp_list[dum].position = where;

  Bp_list[dum].original_op = Op6502 (where);


  Wr6502 (where, (UChar)(0xB + 0x10 * dum));

  // Put an invalid opcode

  return 1;

}


/*****************************************************************************

    Function:  display_debug_help

    Description: display help on debugging
    Parameters: none
    Return: nothing

*****************************************************************************/

void
display_debug_help ()
{
#ifdef ALLEGRO
  UInt32 x;

  BITMAP *bg;


  bg = create_bitmap (vwidth, vheight);

  blit (screen, bg, 0, 0, 0, 0, vwidth, vheight);

  clear (screen);


  for (x = 0; x < help_debug_size; x++)

    textout_centre (screen, font, MESSAGE[language][help_debug + x],
		    vwidth / 2, blit_y + 10 * x, -1);

  while (osd_keypressed ())
    osd_readkey ();

  osd_readkey ();


  blit (bg, screen, 0, 0, 0, 0, vwidth, vheight);

  destroy_bitmap (bg);

  return;
#endif
}



/*****************************************************************************

    Function:  cvt_num

    Description: convert a hexa string without prefix "0x" into a number
    Parameters: char* string, the string to convert
    Return: -1 on error else the given number

	 directly taken from Dave Shadoff emulator TGSIM*

*****************************************************************************/
UInt32 cvtnum (char *string)
{

  UInt32 value = 0;

  char *c = string;


  while (*c != '\0')
    {


      value *= 16;


      if ((*c >= '0') && (*c <= '9'))
	{

	  value += (*c - '0');


	}
      else if ((*c >= 'A') && (*c <= 'F'))
	{

	  value += (*c - 'A' + 10);


	}
      else if ((*c >= 'a') && (*c <= 'f'))
	{

	  value += (*c - 'a' + 10);


	}
      else
	{


	  return (-1);

	}


      c++;

    }


  return (value);

}


/*****************************************************************************

    Function: set_bp_following

    Description: very tricky function, set a bp to the next IP, must handle
       correctly jump, ret,...
                 Well, the trick isn't in a 10 line function ;)
                 but in the optable.following_IP funcs
    Parameters: UInt16 where, the current address
                UChar nb, the nb of the breakpoint to set
    Return: nothing

*****************************************************************************/
void
set_bp_following (UInt16 where, UChar nb)
{

  UInt16 next_pos;

  UChar op = Op6502 (where);


  next_pos = (*optable_debug[op].following_IP) (where);


  Bp_list[nb].position = next_pos;


  Bp_list[nb].flag = ENABLED;


  Bp_list[nb].original_op = Op6502 (next_pos);


  Wr6502 (next_pos, (UChar)(0xB + 0x10 * nb));


  return;

}


/*****************************************************************************

    Function:  change_value

    Description: change the value
    Parameters: int X, int Y : position on screen
                UChar lenght : # of characters allowed
    Return: the new value in int pointed by result

*****************************************************************************/
UChar change_value (int X, int Y, UChar length, UInt32 * result)
{
#ifdef ALLEGRO
  char index = 0;

  char value[9] = "\0\0\0\0\0\0\0\0";

  int ch;


  do
    {
      rectfill (screen, X, Y, X + 16, Y + 9, -15);
      textout (screen, font, value, X, Y + 1, -1);
      ch = osd_readkey ();

      // first switch by scancode
      switch (ch >> 8)
	{
	case KEY_ESC:
	  return 0;
	case KEY_ENTER:
	  *result = cvtnum (value);
	  return 1;
	case KEY_BACKSPACE:
	  if (index)
	    value[--index] = 0;
	  break;
	}

      // Now by ascii code
      switch (ch & 0xff)
	{
	case '0'...'9':
	case 'a'...'f':
	case 'A'...'F':
	  if (index < length)
	    value[index++] = toupper (ch & 0xff);
	  break;
	}
    }
  while (1);
#endif

  return 0;
}
