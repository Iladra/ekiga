
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
 *                         videograbber.cpp  -  description
 *                         --------------------------------
 *   begin                : Mon Feb 12 2001
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : Video4Linux compliant functions to manipulate the 
 *                          webcam device.
 *
 */


#include "config.h"

#include "videoinput.h"
#include "ekiga.h"
#include "misc.h"
#include "main.h"
#include "druid.h"

#include "gmdialog.h"
#include "gmconf.h"

#define new PNEW


/* The functions */
GMVideoGrabber::GMVideoGrabber (BOOL start_grabbing,
				BOOL sync,
				GMManager & endpoint)
  : PThread (1000, NoAutoDeleteThread), ep (endpoint)
{
  /* Variables */
  height = 0;
  width = 0;

  whiteness = 0;
  brightness = 0;
  colour = 0;
  contrast = 0;

  
  /* Internal state */
  stop = FALSE;
  is_grabbing = start_grabbing;
  synchronous = sync;
  is_opened = FALSE;

  
  /* Initialisation */
  display = NULL;
  grabber = NULL;

  if (synchronous)
    VGOpen ();
  
  /* Start the thread */
  this->Resume ();
  thread_sync_point.Wait ();
}


GMVideoGrabber::~GMVideoGrabber ()
{
  is_grabbing = FALSE;
  stop = TRUE;

  /* Wait for the device to be unlocked */
  PWaitAndSignal q(device_mutex);

  /* Wait for the Main () method to be terminated */
  PWaitAndSignal m(quit_mutex);
  PTRACE(4, "GMVidGra\tGMVideoGrabber::~GMVideoGrabber ()"); //FIXME: There seems to be a problem in win32 at this point since 2.0.x
}


void
GMVideoGrabber::Main ()
{
  PBYTEArray frame;

  PWaitAndSignal m(quit_mutex);
  thread_sync_point.Signal ();
 
  if (!synchronous)
    VGOpen ();

  while (!stop) {

    var_mutex.Wait ();
    if (is_grabbing == 1) {

      grabber->GetFrame (frame);
      if (frame != NULL)
	display->SetFrameData (0, 0, 
			       grabber->GetFrameWidth (), 
			       grabber->GetFrameHeight (), 
			       frame);
    }
    var_mutex.Signal ();

    Current()->Sleep (5);
  }

  VGClose ();
}


void
GMVideoGrabber::StartGrabbing (void)
{
  PWaitAndSignal m(var_mutex);
  
  is_grabbing = 1;
}


void
GMVideoGrabber::StopGrabbing (void)
{
  PWaitAndSignal m(var_mutex);

  is_grabbing = 0;
}


BOOL
GMVideoGrabber::IsGrabbing (void)
{
  PWaitAndSignal m(var_mutex);

  return is_grabbing;
}


PVideoInputDevice *
GMVideoGrabber::GetInputDevice (void)
{
  PWaitAndSignal m(var_mutex);
  
  return grabber;
}


PVideoOutputDevice *
GMVideoGrabber::GetOutputDevice (void)
{
  PWaitAndSignal m(var_mutex);
  
  return display;
}


BOOL
GMVideoGrabber::SetColour (int _colour)
{
  PWaitAndSignal m(var_mutex);

  if (grabber)
    return grabber->SetColour (_colour);

  return FALSE;
}


BOOL
GMVideoGrabber::SetBrightness (int _brightness)
{
  PWaitAndSignal m(var_mutex);

  if (grabber)
    return grabber->SetBrightness (_brightness);

  return FALSE;
}


BOOL
GMVideoGrabber::SetWhiteness (int _whiteness)
{
  PWaitAndSignal m(var_mutex);

  if (grabber)
    return grabber->SetWhiteness (_whiteness);

  return FALSE;
}


BOOL
GMVideoGrabber::SetContrast (int constrast)
{
  PWaitAndSignal m(var_mutex);
  
  if (grabber)
    return grabber->SetContrast (constrast);

  return FALSE;
}


void
GMVideoGrabber::GetParameters (int *_whiteness,
			       int *_brightness, 
			       int *_colour,
			       int *_contrast)
{
  int hue = 0;
  
  PWaitAndSignal m(var_mutex);
  
  grabber->GetParameters (_whiteness, _brightness, _colour, _contrast, &hue);

  *_whiteness = (int) *_whiteness / 256;
  *_brightness = (int) *_brightness / 256;
  *_colour = (int) *_colour / 256;
  *_contrast = (int) *_contrast / 256;
}


void
GMVideoGrabber::Lock ()
{
  device_mutex.Wait ();
}


void
GMVideoGrabber::Unlock ()
{
  device_mutex.Signal ();
}


void
GMVideoGrabber::VGOpen (void)
{
  GtkWidget *main_window = NULL;

  PString input_device;
  PString plugin;

  gchar *dialog_title = NULL;
  gchar *dialog_msg = NULL;
  gchar *tmp_msg = NULL;
  gchar *conf_value = NULL;
  
  int error_code = 0;
  int channel = 0;
  int size = 0;
  int frame_rate = 0;

  BOOL no_device_found = FALSE;
  
  PVideoDevice::VideoFormat format = PVideoDevice::PAL;

  main_window = GnomeMeeting::Process ()->GetMainWindow ();
  
  if (!is_opened) {
    
    /* Get the video device options from the configuration database */
    gnomemeeting_threads_enter ();

    conf_value = gm_conf_get_string (VIDEO_DEVICES_KEY "input_device");
    input_device = conf_value;
    g_free (conf_value);
    
    conf_value = gm_conf_get_string (VIDEO_DEVICES_KEY "plugin");
    plugin = conf_value;
    g_free (conf_value);
    
    channel = gm_conf_get_int (VIDEO_DEVICES_KEY "channel");

    size = gm_conf_get_int (VIDEO_DEVICES_KEY "size");

    frame_rate = gm_conf_get_int (VIDEO_CODECS_KEY "frame_rate");

    format =
      (PVideoDevice::VideoFormat) gm_conf_get_int (VIDEO_DEVICES_KEY "format");

    height = video_sizes[size].height;
    width = video_sizes[size].width;

    no_device_found = (input_device == _("No device found"));
    gnomemeeting_threads_leave ();


    /* If there is no device, directly open the fake device */
    if (!no_device_found) {
 
      var_mutex.Wait ();
      grabber = 
	PVideoInputDevice::CreateOpenedDevice (plugin, input_device, FALSE);
      if (!grabber)
	error_code = 1;
      else if (!grabber->SetVideoFormat (format))
	error_code = 2;
      else if (!grabber->SetChannel (channel))
	error_code = 3;
      else if (!grabber->SetColourFormatConverter ("YUV420P"))
	error_code = 4;
      else if (!grabber->SetFrameRate (frame_rate))
	error_code = 5;
      else if (!grabber->SetFrameSizeConverter (width, height, PVideoFrameInfo::eScale))
	error_code = 6;
      var_mutex.Signal ();
    

      /* If error */
      if (error_code) {
	
	/* If we want to open the fake device for a real error, and not because
	   the user chose the Picture device */
	gnomemeeting_threads_enter ();
	dialog_title =
	  g_strdup_printf (_("Error while opening video device %s"),
			   (const char *) input_device);

	tmp_msg = g_strdup (_("A moving logo will be transmitted during calls. Notice that you can always transmit a given image or the moving logo by choosing \"Picture\" as video plugin and \"Moving logo\" or \"Static picture\" as device."));
	switch (error_code) {
	  
	case 1:
	  dialog_msg = g_strconcat (tmp_msg, "\n\n", _("There was an error while opening the device. Please check your permissions and make sure that the appropriate driver is loaded."), NULL);
	  break;
	  
	case 2:
	  dialog_msg = g_strconcat (tmp_msg, "\n\n", _("Your video driver doesn't support the requested video format."), NULL);
	  break;

	case 3:
	  dialog_msg = g_strconcat (tmp_msg, "\n\n", _("Could not open the chosen channel."), NULL);
	  break;
      
	case 4:
	  dialog_msg = g_strconcat (tmp_msg, "\n\n", _("Your driver doesn't seem to support any of the color formats supported by Ekiga.\n Please check your kernel driver documentation in order to determine which Palette is supported."), NULL);
	  break;
	  
	case 5:
	  dialog_msg = g_strconcat (tmp_msg, "\n\n", _("Error while setting the frame rate."), NULL);
	  break;

	case 6:
	  dialog_msg = g_strconcat (tmp_msg, "\n\n", _("Error while setting the frame size."), NULL);
	  break;

	default:
	  break;
	}

	gnomemeeting_warning_dialog_on_widget (GTK_WINDOW (main_window),
					       VIDEO_DEVICES_KEY "enable_preview",
					       dialog_title,
					       "%s", dialog_msg);
	g_free (dialog_msg);
	g_free (dialog_title);
	g_free (tmp_msg);

	gnomemeeting_threads_leave ();
      }
    }
      
    if (error_code || no_device_found) {
	
      /* delete the failed grabber and open the fake grabber */
      var_mutex.Wait ();
      if (grabber) {
	
	delete grabber;
	grabber = NULL;
      }

      grabber =
	PVideoInputDevice::CreateOpenedDevice ("Picture",
					       "Moving logo",
					       FALSE);
      if (grabber) {
	
	grabber->SetColourFormatConverter ("YUV420P");
	grabber->SetVideoFormat (PVideoDevice::PAL);
	grabber->SetChannel (1);    
	grabber->SetFrameRate (6);
	grabber->SetFrameSizeConverter (width, height, FALSE);
      }
      var_mutex.Signal ();
    }
   

    grabber->Start ();

    var_mutex.Wait ();

    display = PVideoOutputDevice::CreateDevice ("EKIGA");
    display->Open ("EKIGAIN", FALSE);
    display->SetFrameSizeConverter (width, height, FALSE);
    display->SetColourFormatConverter ("YUV420P");

    is_opened = TRUE;
    var_mutex.Signal ();
  
      
    /* Setup the video settings */
    GetParameters (&whiteness, &brightness, &colour, &contrast);
    if (whiteness > 0 || brightness > 0 || colour > 0 || contrast > 0) {

      gnomemeeting_threads_enter ();
      gm_main_window_set_video_sliders_values (main_window,
					       whiteness,
					       brightness,
					       colour, 
					       contrast);
      gnomemeeting_threads_leave ();
    }
    else {

      /* Driver made a reset, keep the old values */
      gnomemeeting_threads_enter ();
      gm_main_window_get_video_sliders_values (main_window,
					       whiteness,
					       brightness,
					       colour,
					       contrast);
      gnomemeeting_threads_leave ();

      if (whiteness > 0)
	SetWhiteness (whiteness << 8);
      if (brightness > 0)
	SetBrightness (brightness << 8);
      if (colour > 0)
	SetColour (colour << 8);
      if (contrast > 0)
	SetContrast (contrast << 8);
    }

      
    /* Update the GUI sensitivity if not in a call */
    if (ep.GetCallingState () == GMManager::Standby) {

      gnomemeeting_threads_enter ();      
      gm_main_window_update_sensitivity (main_window, TRUE, FALSE, TRUE);
      gnomemeeting_threads_leave ();
    }
  }
}
  

void
GMVideoGrabber::VGClose ()
{
  GtkWidget *main_window = NULL;

  main_window = GnomeMeeting::Process ()->GetMainWindow ();

  if (is_opened) {

    var_mutex.Wait ();
    is_grabbing = FALSE;
    var_mutex.Signal ();


    /* Update menu sensitivity if we are not in a call */
    gnomemeeting_threads_enter ();
    if (ep.GetCallingState () == GMManager::Standby
	&& !gm_conf_get_bool (VIDEO_DEVICES_KEY "enable_preview")) {

      gm_main_window_update_sensitivity (main_window, TRUE, FALSE, FALSE);
      gm_main_window_update_logo (main_window);
    }
    gnomemeeting_threads_leave ();


    /* Initialisation */
    var_mutex.Wait ();
    is_opened = FALSE;
    delete grabber;
    delete display;
    display = NULL;
    grabber = NULL;
    var_mutex.Signal ();
  }

  
  /* Quick Hack for buggy drivers that return from the ioctl before the device
     is really closed */
  PThread::Current ()->Sleep (1000);
}


/* The video tester class */
GMVideoTester::GMVideoTester (gchar *manager,
			      gchar *recorder)
  :PThread (1000, AutoDeleteThread)
{
  if (manager)
    video_manager = PString (manager);
  if (recorder)
    video_recorder = PString (recorder);

  test_dialog = NULL;
  test_label = NULL;
  
  this->Resume ();
  thread_sync_point.Wait ();
}


GMVideoTester::~GMVideoTester ()
{
 PWaitAndSignal m(quit_mutex);
}


void GMVideoTester::Main ()
{
  GtkWidget *druid_window = NULL;

  PVideoInputDevice *grabber = NULL;
  
  int height = GM_QCIF_HEIGHT; 
  int width = GM_QCIF_WIDTH; 
  int error_code = -1;
  int cpt = 0;

  gchar *dialog_msg = NULL;
  gchar *tmp = NULL;

  druid_window = GnomeMeeting::Process ()->GetDruidWindow (); 
  
  PWaitAndSignal m(quit_mutex);
  thread_sync_point.Signal ();

  if (video_recorder.IsEmpty ()
      || video_manager.IsEmpty ()
      || video_recorder == PString (_("No device found"))
      || video_recorder == PString (_("Picture")))
    return;
  
  gdk_threads_enter ();
  test_dialog =
    gtk_dialog_new_with_buttons ("Video test running",
				 GTK_WINDOW (druid_window),
				 (GtkDialogFlags) (GTK_DIALOG_MODAL),
				 GTK_STOCK_OK,
				 GTK_RESPONSE_ACCEPT,
				 NULL);
  dialog_msg = 
    g_strdup_printf (_("Ekiga is now testing the %s video device. If you experience machine crashes, then report a bug to the video driver author."), (const char *) video_recorder);
  test_label = gtk_label_new (dialog_msg);
  gtk_label_set_line_wrap (GTK_LABEL (test_label), true);
  g_free (dialog_msg);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (test_dialog)->vbox), test_label,
		      FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (test_dialog)->vbox), 
		      gtk_hseparator_new (), FALSE, FALSE, 2);

  test_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (test_dialog)->vbox), 
		      test_label, FALSE, FALSE, 2);

  g_signal_connect (G_OBJECT (test_dialog), "delete-event",
		    G_CALLBACK (gtk_widget_hide_on_delete), NULL);

  gtk_window_set_transient_for (GTK_WINDOW (test_dialog),
				GTK_WINDOW (druid_window));
  gtk_widget_show_all (GTK_DIALOG (test_dialog)->vbox);
  gnomemeeting_threads_dialog_show (test_dialog);
  gdk_threads_leave ();

  
  while (cpt < 6 && error_code == -1) {

    if (!video_recorder.IsEmpty ()
	&& !video_manager.IsEmpty ()) {

      error_code = -1;
      
      grabber = 
	PVideoInputDevice::CreateOpenedDevice (video_manager,
					       video_recorder,
					       FALSE);

      if (!grabber)
	error_code = 0;
      else
	if (!grabber->SetVideoFormat (PVideoDevice::Auto))
	  error_code = 2;
      else
        if (!grabber->SetChannel (0))
          error_code = 2;
      else
	if (!grabber->SetColourFormatConverter ("YUV420P"))
	  error_code = 3;
      else
	if (!grabber->SetFrameRate (30))
	  error_code = 4;
      else
	if (!grabber->SetFrameSizeConverter (width, height, FALSE))
	  error_code = 5;
      else
	grabber->Close ();

      if (grabber)
	delete (grabber);
    }


    if (error_code == -1) 
      dialog_msg = g_strdup_printf (_("Test %d done"), cpt);
    else
      dialog_msg = g_strdup_printf (_("Test %d failed"), cpt);

    tmp = g_strdup_printf ("<b>%s</b>", dialog_msg);
    gdk_threads_enter ();
    gtk_label_set_markup (GTK_LABEL (test_label), tmp);
    gdk_threads_leave ();
    g_free (dialog_msg);
    g_free (tmp);

    cpt++;
    PThread::Current () ->Sleep (100);
  }

  
  if (error_code != - 1) {
    
    switch (error_code)	{
	  
    case 0:
      dialog_msg = g_strdup_printf (_("Error while opening %s."),
			     (const char *) video_recorder);
      break;
      
    case 1:
      dialog_msg = g_strdup_printf (_("Your video driver doesn't support the requested video format."));
      break;
      
    case 2:
      dialog_msg = g_strdup_printf (_("Could not open the chosen channel with the chosen video format."));
      break;
      
    case 3:
      dialog_msg = g_strdup_printf (_("Your driver doesn't support any of the color formats tried by Ekiga"));
      break;
      
    case 4:
      dialog_msg = g_strdup_printf ( _("Error with the frame rate."));
      break;
      
    case 5:
      dialog_msg = g_strdup_printf (_("Error with the frame size."));
      break;

    default:
      break;
    }

    gdk_threads_enter ();
    gnomemeeting_error_dialog (GTK_WINDOW (druid_window),
			       _("Failed to open the device"),
			       "%s", dialog_msg);
    gdk_threads_leave ();
    
    g_free (dialog_msg);
  }

  gdk_threads_enter ();
  gm_druid_window_set_test_buttons_sensitivity (druid_window, FALSE);
  if (test_dialog)
    gnomemeeting_threads_widget_destroy (test_dialog);
  gdk_threads_leave ();
}
