
#include "osd_freebsd_cd.h"


int cd_drive_handle = 0;


int osd_cd_init(char *device)
{
/*
  Log("Init linux cdrom device\n");

  if (strcmp(device, ""))
    cd_drive_handle = open(device, O_RDONLY | O_NONBLOCK);
  else
    cd_drive_handle = open("/dev/cdrom", O_RDONLY | O_NONBLOCK);

  return (cd_drive_handle == 0);
*/  
}


void osd_cd_stop_audio()
{
/*
  if (ioctl(cd_drive_handle, CDROMSTOP) == -1)
    perror("osd_cd_stop_audio");
*/    
}


void osd_cd_close()
{
/*
  osd_cd_stop_audio();
  close(cd_drive_handle);
*/  
}


int osd_cd_read(UChar *p, UInt32 sector)
{
/*
  int retries = 0;
  char buf[128];

  while ((lseek(cd_drive_handle, 2048 * sector, SEEK_SET) < 0) && (retries < 3)) {
    sprintf(buf, "osd_cd_read:lseek (sector=%d, retry=%d)", sector, retries);
    perror(buf);
    retries++;
  }

  while ((read(cd_drive_handle, p, 2048) < 0) && (retries++ < 3)) {
    sprintf(buf, "osd_cd_read:read (sector=%d, retry=%d)", sector, retries);
    perror(buf);
  }
*/  
}

extern unsigned char binbcd[];

void osd_cd_subchannel_info(unsigned short offset)
{
/*
  struct cdrom_subchnl subc;

  subc.cdsc_format = CDROM_MSF;

  if (ioctl(cd_drive_handle, CDROMSUBCHNL, &subc) == -1)
    perror("osd_cd_subchannel_info");
*/

/*
  switch(subc.cdsc_audiostatus) {
    case CDROM_AUDIO_PLAY:
      put_8bit_addr(offset, 0);
      break;
    case CDROM_AUDIO_PAUSED:
      put_8bit_addr(offset, 1);
      break;
    case CDROM_AUDIO_COMPLETED:
      put_8bit_addr(offset, 3);
      break;
    default:
      put_8bit_addr(offset, 4);
      break;
  }
*/

/*
  put_8bit_addr(offset, subc.cdsc_audiostatus - 0x11);

  put_8bit_addr(offset + 1, subc.cdsc_ctrl);
  put_8bit_addr(offset + 2, binbcd[subc.cdsc_trk]);
  put_8bit_addr(offset + 3, binbcd[subc.cdsc_ind]);
  put_8bit_addr(offset + 4, binbcd[subc.cdsc_reladdr.msf.minute]);
  put_8bit_addr(offset + 5, binbcd[subc.cdsc_reladdr.msf.second]);
  put_8bit_addr(offset + 6, binbcd[subc.cdsc_reladdr.msf.frame]);
  put_8bit_addr(offset + 7, binbcd[subc.cdsc_absaddr.msf.minute]);
  put_8bit_addr(offset + 8, binbcd[subc.cdsc_absaddr.msf.second]);
  put_8bit_addr(offset + 9, binbcd[subc.cdsc_absaddr.msf.frame]);
*/
}


void osd_cd_status(int *status)
{
/*
  struct cdrom_subchnl subc;

  subc.cdsc_format = CDROM_MSF;

  if (ioctl(cd_drive_handle, CDROMSUBCHNL, &subc) == -1)
    perror("osd_cd_status");

  *status = subc.cdsc_audiostatus - 0x10;
//  switch(subc.cdsc_audiostatus) {
//    case CDROM_AUDIO_PLAY:
//      *status = 1;
//      break;
//    default:
//      *status = subc.cdsc_audiostatus;
//      break;
//  }
*/
}


void osd_cd_track_info(UChar track, int *min, int *sec, int *fra, int *control)
{
/*
  struct cdrom_tocentry tocentry;
  
  tocentry.cdte_track = track;
  tocentry.cdte_format = CDROM_MSF;

  if (ioctl(cd_drive_handle, CDROMREADTOCENTRY, &tocentry) == -1)
    perror("osd_cd_track_info");

//  Log("Track %d begins at %d and got command byte of %d\n", track,
//      tocentry.cdte_addr.lba + 150, tocentry.cdte_ctrl);

//  Frame2Time(tocentry.cdte_addr.lba + 150, min, sec, fra);

  *min = tocentry.cdte_addr.msf.minute;
  *sec = tocentry.cdte_addr.msf.second;
  *fra = tocentry.cdte_addr.msf.frame;
  *control = tocentry.cdte_ctrl;
*/
}


void osd_cd_nb_tracks(int *first, int *last)
{
/*
  struct cdrom_tochdr track_info;

  if (ioctl(cd_drive_handle, CDROMREADTOCHDR, &track_info) == -1)
    perror("cd_nb_tracks");

  *first = track_info.cdth_trk0;
  *last = track_info.cdth_trk1;
*/
}


void osd_cd_length(int *min, int *sec, int *fra)
{
/*
  struct cdrom_tocentry toc_info;

  toc_info.cdte_track = CDROM_LEADOUT;
  toc_info.cdte_format = CDROM_MSF;

  if (ioctl(cd_drive_handle, CDROMREADTOCENTRY, &toc_info) == -1)
    perror("cd_length");

  *min = toc_info.cdte_addr.msf.minute;
  *sec = toc_info.cdte_addr.msf.second;
  *fra = toc_info.cdte_addr.msf.frame;
*/  
}


void osd_cd_pause(void)
{
/*
  if (ioctl(cd_drive_handle, CDROMPAUSE) == -1)
    perror("osd_cd_pause");
*/    
}


void osd_cd_resume(void)
{
/*
  if (ioctl(cd_drive_handle, CDROMRESUME) == -1)
    perror("osd_cd_resume");
*/    
}


/* TODO : check for last track asked */
void osd_cd_play_audio_track(UChar track)
{
/*
  struct cdrom_ti cdrom_ti_dat = 
    {
      track,
      0,
      track + 1,
      0
    };

  if (ioctl(cd_drive_handle, CDROMPLAYTRKIND, &cdrom_ti_dat) == -1)
    perror("play_audio_track");
*/    
}


void osd_cd_play_audio_range(UChar min_from, UChar sec_from, UChar fra_from,
                             UChar min_to, UChar sec_to, UChar fra_to)
{
/*
  struct cdrom_msf cdrom_msf_dat =
  {
    min_from,
    sec_from,
    fra_from,
    min_to,
    sec_to,
    fra_to
  };

  if (ioctl(cd_drive_handle, CDROMPLAYMSF, &cdrom_msf_dat) == -1)
    perror("play_audio_range");
*/
}

#ifndef __QNXNTO__
void osd_cd_subchannel_info(unsigned short offset)
{ }
 
void osd_cd_status(int *status)
{}

void osd_cd_resume(void)
{}

void osd_cd_pause(void)
{}
#endif

