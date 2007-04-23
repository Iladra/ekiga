
/* Ekiga -- A VoIP and Video-Conferencing application
 * Copyright (C) 2000-2006 Damien Sandras
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * Ekiga is licensed under the GPL license and as a special exception,
 * you have permission to link or otherwise combine this program with the
 * programs OPAL, OpenH323 and PWLIB, and distribute the combination,
 * without applying the requirements of the GNU GPL to the OPAL, OpenH323
 * and PWLIB programs, as long as you do follow the requirements of the
 * GNU GPL for all the rest of the software thus combined.
 */


/*
 *                         videograbber.h  -  description
 *                         ------------------------------
 *   begin                : Mon Feb 12 2001
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : Video4Linux compliant functions to manipulate the 
 *                          webcam device.
 *
 */


#ifndef _VIDEO_GRABBER_H_
#define _VIDEO_GRABBER_H_

#include "common.h"

class GMManager;

class GMVideoGrabber : public PThread
{
  PCLASSINFO(GMVideoGrabber, PThread);

 public:

  /* DESCRIPTION  :  The constructor.
   * BEHAVIOR     :  Initialises the VideoGrabber, the VideoGrabber is opened
   *                 asynchronously given the config parameters. If the opening
   *                 fails, an error popup is displayed.
   * PRE          :  First parameter is TRUE if the VideoGrabber must grab
   *                 once opened. The second one is TRUE if the VideoGrabber
   *                 must be opened synchronously. The last one is a 
   *                 reference to the GMManager.
   */
  GMVideoGrabber (BOOL start_grabbing,
		  BOOL sync,
		  GMManager &endpoint);


  /* DESCRIPTION  :  The destructor.
   * BEHAVIOR     :  /
   * PRE          :  /
   */
  ~GMVideoGrabber (void);


  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Start to grab, i.e. read from the specified device 
   *                 and display images in the main interface.
   * PRE          :  /
   */
  void StartGrabbing (void);

  
  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Stop to grab, i.e. read from the specified device 
   *                 and display images in the main interface.
   * PRE          :  /
   */
  void StopGrabbing (void);

  
  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Returns TRUE if we are grabbing.
   * PRE          :  /
   */
  BOOL IsGrabbing (void);

  
  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Returns the PVideoInputDevice used to capture
   *                 the camera images.
   * PRE          :  /
   */
  PVideoInputDevice *GetInputDevice (void);

  
  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Returns the PVideoOutputDevice used to display
   *                 the camera images.
   * PRE          :  /
   */
  PVideoOutputDevice *GetOutputDevice (void);


  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Sets the colour for the specified device.
   * PRE          :  0 <= int <= 65535
   */
  BOOL SetColour (int colour);


  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Sets the brightness for the specified device.
   * PRE          :  0 <= int <= 65535
   */
  BOOL SetBrightness (int brightness);


  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Sets the whiteness for the specified device.
   * PRE          :  0 <= int <= 65535
   */
  BOOL SetWhiteness (int whiteness);


  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Sets the contrast for the specified device.
   * PRE          :  0 <= int <= 65535
   */
  BOOL SetContrast (int contrast);


  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Returns respectively the whiteness, brightness, 
   *                 colour, contrast for the specified device.
   * PRE          :  Allocated pointers to int. Grabber must be opened.
   */
  void GetParameters (int *whiteness,
		      int *brightness,
		      int *colour,
		      int *contrast);


  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Lock the device, preventing it to be Closed and deleted.
   * PRE          :  /
   */
  void Lock ();

  
  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Unlock the device.
   * PRE          :  /
   */
  void Unlock ();

  
 protected:
  void Main (void);
  void VGOpen (void);
  void VGClose (void);

  int height;
  int width;
  int whiteness;
  int brightness;
  int colour;
  int contrast;

  char video_buffer [3 * GM_CIF_WIDTH * GM_CIF_HEIGHT];

  PVideoInputDevice *grabber;
  PVideoOutputDevice *display;

  BOOL stop;
  BOOL is_grabbing;
  BOOL synchronous;
  BOOL is_opened;

  PMutex var_mutex;      /* To protect variables that are read and written
			    from various threads */
  PMutex quit_mutex;     /* To exit */
  PMutex device_mutex;   /* To Lock and Unlock and not exit until
			    it is unlocked */
  PSyncPoint thread_sync_point;

  GMManager & ep;
};


class GMVideoTester : public PThread
{
  PCLASSINFO(GMVideoTester, PThread);


public:

  /* DESCRIPTION  :  The constructor.
   * BEHAVIOR     :  
   * PRE          :  /
   */
  GMVideoTester (gchar *manager,
		 gchar *recorder);


  /* DESCRIPTION  :  The destructor.
   * BEHAVIOR     :  /
   * PRE          :  /
   */
  ~GMVideoTester ();


  void Main ();


protected:

  PString video_manager;
  PString video_recorder;

  GtkWidget *test_label;
  GtkWidget *test_dialog;

  PMutex quit_mutex;
  PSyncPoint thread_sync_point;
};
#endif
