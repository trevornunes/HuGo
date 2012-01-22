#include "hugo.h"
#include "pbutil.h"

//! name of the backup ram filename
char backup_mem[PATH_MAX];

struct host_machine host;
struct hugo_options option;

//! Setup the host machine
/*!
 * \param argc The number of argument on command line
 * \param argv The arguments on command line
 * \return zero on success else non zero value
 */
int
initialisation (int argc, char *argv[])
{


#ifdef MSDOS
  _crt0_startup_flags |= _CRT0_FLAG_NO_LFN;
  // Disable long filename to avoid mem waste in select_rom func.
#endif

  memset (&option, 0, sizeof (option));

  option.want_stereo = TRUE;
  option.want_fullscreen = TRUE;
  option.want_fullscreen_aspect = TRUE;
  option.configure_joypads = TRUE;
  option.want_hardware_scaling = TRUE;
  
  option.window_size = 1;

#if defined(ENABLE_NETPLAY)
  option.local_input_mapping[0] = 0;
  option.local_input_mapping[1] = 1;
  option.local_input_mapping[2] = -1;
  option.local_input_mapping[3] = -1;
  option.local_input_mapping[4] = -1;
  strcpy(option.server_hostname,"localhost");
#endif

  /*
   * Most sound cards won't like this value but we'll accept whatever SDL
   * gives us.  This frequency will get us really close to the original
   * pc engine sound behaviour.
   */
  option.want_snd_freq = 21963;

  // Initialise paths
  osd_init_paths (argc, argv);

  // Create the log file
  init_log_file ();

  // Init the random seed
  srand ((unsigned int) time (NULL));

  // Read configuration in ini file
  parse_INIfile ();

  // Read the command line
  parse_commandline (argc, argv);

  // Initialise the host machine
  if (!osd_init_machine ())
    return -1;

  // If backup memory name hasn't been overriden on command line, use the default
  if ((bmdefault) && (strcmp (bmdefault, "")))
    snprintf (backup_mem, sizeof (backup_mem), "%s%s", short_exe_name,
	      bmdefault);
  else
    snprintf (backup_mem, sizeof (backup_mem), "%sbackup.dat",
	      short_exe_name);

  // In case of crash, try to free audio related ressources if possible
//      atexit (TrashSound);

  // Initialise the input devices
  if (osd_init_input () != 0)
    {
      fprintf (stderr, "Initialization of input system failed\n");
      return (-2);
    }

  return 0;
}


//! Free ressources of the host machine
/*!
 * Deallocate ressources reserved during the initialisation
 */
void
cleanup ()
{

  osd_shutdown_input();

  // Deinitialise the host machine
  osd_shut_machine ();

}

//! Check if a game was asked
/*!
 * \return non zero if a game must be played
 */

int LoadNewGame = 0;

int
game_asked ()
{
  return ((CD_emulation == 1) || (strcmp (cart_name, "")));
}

//! Run an instance of a rom or cd or iso
/*!
 * \return non zero if another game has to be launched
 */

extern int LoadNewGame;

int
play_game (void)
{
  const char *p = RomNext();
  static int gfx_audio_net_init_done = 0;
  LoadNewGame = 0;

  strcpy( cart_name + strlen("/accounts/1000/shared/misc/roms/pce/"), p);
  fprintf(stderr,"play_game: LOAD cart_name = '%s'\n", cart_name);

  // Initialise the target machine (pce)
  if (InitPCE (cart_name, backup_mem) != 0)
  {
	  fprintf(stderr,"InitPCE: failed to initialize ...\n");
	  return 0;
  }
  // Bring up GFX,AUDIO and NET
  if( gfx_audio_net_init_done == 0)
  {
    if (!(*osd_gfx_driver_list[video_driver].init) ())
    {
      Log ("Can't set graphic mode\n");
      printf (MESSAGE[language][cant_set_gmode]);
      return 0;
    }

    if (!osd_snd_init_sound ())
    {
      Log ("Couldn't open any sound hardware on the host machine.\n");
      printf (MESSAGE[language][audio_init_failed]);
    }
      else
         printf (MESSAGE[language][audio_inited], 8, "SDL compatible soundcard",  host.sound.freq);

#if defined(ENABLE_NETPLAY)
    	osd_init_netplay();
#endif
	gfx_audio_net_init_done = 1;
  }

#ifdef __QNXNTO__
  if (!osd_snd_init_sound ())
    {
      Log ("Couldn't open any sound hardware on the host machine.\n");
      printf (MESSAGE[language][audio_init_failed]);
    }
      else
         printf (MESSAGE[language][audio_inited], 8, "SDL compatible soundcard",  host.sound.freq);
#endif

  // Run the emulation
  RunPCE ();

#ifdef __QNXNTO__
  osd_snd_trash_sound ();
#endif


  fprintf(stderr, "Shutting down emulation ...\n");

 // We init once and don't shutdown for Playbook. There is no need
 // to restart SDL services once we are launched we only need the emu core to be reset.

#ifndef __QNXNTO__
#if defined(ENABLE_NETPLAY)
	osd_shutdown_netplay();
#endif

  osd_snd_trash_sound ();

  (*osd_gfx_driver_list[video_driver].shut) ();
#endif

  // Free the target machine (pce)
  TrashPCE (backup_mem);

  fprintf(stderr,"play_game: TrashPCE done ...\n");
#ifdef __QNXNTO__
  cart_reload = 1;
  fprintf(stderr,"QNX: returns to re-start ...\n");
  return 1;
#endif

  return cart_reload;
}


int
main (int argc, char *argv[])
{
  int error;
  error = 0;

  error = initialisation (argc, argv);



#if defined(GTK)

  if (!error)
    {
      if (game_asked ())
	{
	  while (play_game ());
	}
      else
	{
	  build_gtk_interface (argc, argv);
	}
    }

#else // not defined(GTK)

  RomSelectStart();

  if (!error)
    {
    if (game_asked ())
	{
	  while (1)
	  {
		fprintf(stderr,"EMU START:\n");
		play_game();
	    fprintf(stderr,"EMU END:\n");
	  }
	}
      else
	{
	  printf ("No game specified\n");
	}
    }
#endif
  fprintf(stderr,"HuGo says bye!\n");

  cleanup ();

  return 0;
}
