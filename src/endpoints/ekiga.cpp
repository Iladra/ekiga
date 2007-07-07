
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
 *                         gnomemeeting.cpp  -  description
 *                         --------------------------------
 *   begin                : Sat Dec 23 2000
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains the main class
 *
 */


#include "../../config.h"

#include "ekiga.h"
#include "callbacks.h"
#include "audio.h"
#include "urlhandler.h"
#include "addressbook.h"
#include "preferences.h"
#include "chat.h"
#include "callshistory.h"
#include "druid.h"
#include "tools.h"
#include "statusicon.h"
#include "history.h"
#include "main.h"
#include "misc.h"

#ifdef HAVE_DBUS
#include "dbus.h"
#endif

#include "gmdialog.h"
#include "gmstockicons.h"
#include "gmconf.h"
#include "gmcontacts.h"

#define new PNEW


GnomeMeeting *GnomeMeeting::GM = NULL;

/* The main GnomeMeeting Class  */
GnomeMeeting::GnomeMeeting ()
  : PProcess("", "", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)

{
  /* no endpoint for the moment */
  endpoint = NULL;
  url_handler = NULL;

  
  addressbook_window = NULL;

  GM = this;
  
  endpoint = new GMManager ();
  
  call_number = 0;
}


GnomeMeeting::~GnomeMeeting()
{ 
  Exit ();
}


void 
GnomeMeeting::Connect (PString url)
{
  /* If incoming connection, then answer it */
  if (endpoint->GetCallingState () == GMManager::Called) {

    gm_main_window_push_message (main_window, _("Answering call..."));
    url_handler = new GMURLHandler ("", FALSE);
  }
  else if (endpoint->GetCallingState () == GMManager::Standby
	   && !GMURL (url).IsEmpty ()) {
    
    /* Update the GUI */
    gm_main_window_set_call_url (main_window, url);
 
    /* if we call somebody, and if the URL is not empty */
    url_handler = new GMURLHandler (url);
  }
}


void
GnomeMeeting::Disconnect (H323Connection::CallEndReason reason)
{
  PString call_token;
  
  call_token = endpoint->GetCurrentCallToken ();
  
  /* if we are trying to call somebody */
  if (endpoint->GetCallingState () == GMManager::Calling) {

    endpoint->ClearCall (call_token, reason);
  }
  else {

    /* if we are in call with somebody */
    if (endpoint->GetCallingState () == GMManager::Connected) {

      endpoint->ClearAllCalls (OpalConnection::EndedByLocalUser, FALSE);
    }
    else if (endpoint->GetCallingState () == GMManager::Called) {

      endpoint->ClearCall (call_token,
			   OpalConnection::EndedByAnswerDenied);
    }
    else {

      endpoint->ClearCall (call_token,
			   OpalConnection::EndedByAnswerDenied);
    }
  }
}


void
GnomeMeeting::Init ()
{
  /* Init the endpoint */
  endpoint->Init ();
}


void
GnomeMeeting::Exit ()
{
  RemoveManager ();

  if (addressbook_window) 
    gtk_widget_destroy (addressbook_window);  
  addressbook_window = NULL;
  
  if (prefs_window)
    gtk_widget_destroy (prefs_window);
  prefs_window = NULL;
  
  if (pc2phone_window)
    gtk_widget_destroy (pc2phone_window);
  pc2phone_window = NULL;
  
  if (history_window)
    gtk_widget_destroy (history_window);
  history_window = NULL;
  
  if (main_window)
    gtk_widget_destroy (main_window);
  main_window = NULL;
  
  if (druid_window)
    gtk_widget_destroy (druid_window);
  druid_window = NULL;
  
  if (accounts_window)
    gtk_widget_destroy (accounts_window);
  accounts_window = NULL;
  
  if (chat_window)
    gtk_widget_destroy (chat_window);
  chat_window = NULL;
  
  if (statusicon)
    gtk_widget_destroy (statusicon);
  statusicon = NULL;
  
#ifdef HAVE_DBUS
  if (dbus_component)
    g_object_unref (dbus_component);
  dbus_component = NULL;
#endif
}


BOOL
GnomeMeeting::DetectInterfaces ()
{
  PString config_interface;
  PString iface_noip;
  PString ip;
  PIPSocket::InterfaceTable ifaces;

  PINDEX i = 0;
  PINDEX pos = 0;
  BOOL res = FALSE;

  gchar *conf_interface = NULL;

  PWaitAndSignal m(iface_access_mutex);

  /* Detect the valid interfaces */
  res = PIPSocket::GetInterfaceTable (ifaces);
  interfaces.RemoveAll ();

  conf_interface = gm_conf_get_string (PROTOCOLS_KEY "interface");
  config_interface = conf_interface;
  g_free (conf_interface);

  pos = config_interface.Find("[");
  if (pos != P_MAX_INDEX)
    iface_noip = config_interface.Left (pos).Trim ();
  while (i < ifaces.GetSize ()) {

    ip = " [" + ifaces [i].GetAddress ().AsString () + "]";

    if (ifaces [i].GetName () + ip == config_interface || ifaces [i].GetName () == iface_noip) 
      break;

    i++;
  }

  pos = i;
  i = 0;

  while (i < ifaces.GetSize ()) {

    ip = " [" + ifaces [i].GetAddress ().AsString () + "]";

    if (i != pos) {

      if (ifaces [i].GetAddress ().AsString () != "0.0.0.0") {

        if (ifaces [i].GetName ().Find ("ppp") != P_MAX_INDEX) {

          if (i > 0) {
            interfaces += interfaces [0];
            interfaces [0] = ifaces [i].GetName () + ip;     
          }
          else
            interfaces += ifaces [i].GetName () + ip;
        }
        else if (!ifaces [i].GetAddress ().IsLoopback ())
          interfaces += ifaces [i].GetName () + ip;
      }
    }
    else {

      if (!interfaces [0].IsEmpty ())
        interfaces += interfaces [0];
      interfaces [0] = ifaces [pos].GetName () + ip;
    }

    i++;
  }


  /* Update the GUI, if it is already there */
  if (prefs_window)
    gm_prefs_window_update_interfaces_list (prefs_window, 
                                            interfaces);

  return res;

}
  

BOOL
GnomeMeeting::DetectDevices ()
{
  gchar *audio_plugin = NULL;
  gchar *video_plugin = NULL;

  PINDEX fake_idx;

  audio_plugin = gm_conf_get_string (AUDIO_DEVICES_KEY "plugin");
  video_plugin = gm_conf_get_string (VIDEO_DEVICES_KEY "plugin");
 
  PWaitAndSignal m(dev_access_mutex);
  

  /* Detect the devices */
  gnomemeeting_sound_daemons_suspend ();

  
  /* Detect the plugins */
  audio_managers = PSoundChannel::GetDriverNames ();
  video_managers = PVideoInputDevice::GetDriverNames ();
  
  fake_idx = video_managers.GetValuesIndex (PString ("FakeVideo"));
  if (fake_idx != P_MAX_INDEX)
    video_managers.RemoveAt (fake_idx);

  PTRACE (1, "Detected audio plugins: " << setfill (',') << audio_managers
	  << setfill (' '));
  PTRACE (1, "Detected video plugins: " << setfill (',') << video_managers
	  << setfill (' '));

#ifdef HAX_IXJ
  audio_managers += PString ("Quicknet");
#endif

  PTRACE (1, "Detected audio plugins: " << setfill (',') << audio_managers
	  << setfill (' '));
  PTRACE (1, "Detected video plugins: " << setfill (',') << video_managers
	  << setfill (' '));
  

  fake_idx = video_managers.GetValuesIndex (PString ("Picture"));
  if (fake_idx == P_MAX_INDEX)
    return FALSE;

  /* No audio plugin => Exit */
  if (audio_managers.GetSize () == 0)
    return FALSE;
  
  
  /* Detect the devices */
  video_input_devices = PVideoInputDevice::GetDriversDeviceNames (video_plugin);
 
  audio_input_devices = 
    PSoundChannel::GetDeviceNames (audio_plugin, PSoundChannel::Recorder);
  audio_output_devices = 
    PSoundChannel::GetDeviceNames (audio_plugin, PSoundChannel::Player);

  
  if (audio_input_devices.GetSize () == 0) 
    audio_input_devices += PString (_("No device found"));
  if (audio_output_devices.GetSize () == 0)
    audio_output_devices += PString (_("No device found"));
  if (video_input_devices.GetSize () == 0)
    video_input_devices += PString (_("No device found"));


  PTRACE (1, "Detected the following audio input devices: "
	  << setfill (',') << audio_input_devices << setfill (' ')
	  << " with plugin " << audio_plugin);
  PTRACE (1, "Detected the following audio output devices: "
	  << setfill (',') << audio_output_devices << setfill (' ')
	  << " with plugin " << audio_plugin);
  PTRACE (1, "Detected the following video input devices: "
	  << setfill (',') << video_input_devices << setfill (' ')
	  << " with plugin " << video_plugin);
  
  PTRACE (1, "Detected the following audio input devices: " 
	  << setfill (',') << audio_input_devices << setfill (' ') 
	  << " with plugin " << audio_plugin);
  PTRACE (1, "Detected the following audio output devices: " 
	  << setfill (',') << audio_output_devices << setfill (' ') 
	  << " with plugin " << audio_plugin);
  PTRACE (1, "Detected the following video input devices: " 
	  << setfill (',') << video_input_devices << setfill (' ')  
	  << " with plugin " << video_plugin);

  g_free (audio_plugin);
  g_free (video_plugin);

  gnomemeeting_sound_daemons_resume ();

  /* Update the GUI, if it is already there */
  if (prefs_window)
    gm_prefs_window_update_devices_list (prefs_window, 
					 audio_input_devices,
					 audio_output_devices,
					 video_input_devices);
  return TRUE;
}


BOOL
GnomeMeeting::DetectCodecs ()
{
  OpalMediaFormatList list;

  /* Audio codecs */
  list = endpoint->GetAvailableAudioMediaFormats ();

  PTRACE (1, "Detected audio codecs: " << setfill (',') << list
	  << setfill (' '));

  if (list.GetSize () == 0)
    return FALSE;

  if (prefs_window)
    gm_prefs_window_update_codecs_list (prefs_window, list);
  

  /* Video codecs */
  list = endpoint->GetAvailableVideoMediaFormats ();
  
  PTRACE (1, "Detected video codecs: " << setfill (',') << list
	  << setfill (' '));

  if (prefs_window)
    gm_prefs_window_update_codecs_list (prefs_window, list);

  return TRUE;
}


GMManager *
GnomeMeeting::GetManager ()
{
  GMManager *ep = NULL;
  PWaitAndSignal m(ep_var_mutex);

  ep = endpoint;
  
  return ep;
}


GnomeMeeting *
GnomeMeeting::Process ()
{
  return GM;
}


GtkWidget *
GnomeMeeting::GetMainWindow ()
{
  return main_window;
}


GtkWidget *
GnomeMeeting::GetPrefsWindow ()
{
  return prefs_window;
}


GtkWidget *
GnomeMeeting::GetChatWindow ()
{
  return chat_window;
}


GtkWidget *
GnomeMeeting::GetDruidWindow ()
{
  return druid_window;
}


GtkWidget *
GnomeMeeting::GetAddressbookWindow ()
{
  return addressbook_window;
}


GtkWidget *
GnomeMeeting::GetHistoryWindow ()
{
  return history_window;
}


GtkWidget *
GnomeMeeting::GetPC2PhoneWindow ()
{
  return pc2phone_window;
}


GtkWidget *
GnomeMeeting::GetAccountsWindow ()
{
  return accounts_window;
}


GtkWidget *
GnomeMeeting::GetStatusicon ()
{
  return statusicon;
}

#ifdef HAVE_DBUS
GObject *
GnomeMeeting::GetDbusComponent ()
{
  return dbus_component;
}
#endif

void GnomeMeeting::Main ()
{
}


void GnomeMeeting::BuildGUI ()
{
  /* Init the address book */
  gnomemeeting_addressbook_init (_("On This Computer"), _("Personal"));
  
  /* Init the stock icons */
  gnomemeeting_stock_icons_init ();
  
  /* Build the GUI */
  pc2phone_window = gm_pc2phone_window_new ();  
  prefs_window = gm_prefs_window_new ();  
  history_window = gm_history_window_new ();
  chat_window = gm_text_chat_window_new ();
  addressbook_window = gm_addressbook_window_new ();
  druid_window = gm_druid_window_new ();
  accounts_window = gm_accounts_window_new ();

  main_window = gm_main_window_new ();
#ifdef HAVE_DBUS
  dbus_component = gnomemeeting_dbus_component_new ();
#endif
  statusicon = gm_statusicon_new (); /* must come last (uses the windows) */

  /* GM is started */
  gm_history_window_insert (history_window,
			    _("Started Ekiga %d.%d.%d for user %s"), 
			    MAJOR_VERSION, MINOR_VERSION, BUILD_NUMBER,
			    g_get_user_name ());

  PTRACE (1, "Ekiga version "
	  << MAJOR_VERSION << "." << MINOR_VERSION << "." << BUILD_NUMBER);
  PTRACE (1, "OPAL version " << OPAL_VERSION);
  PTRACE (1, "PWLIB version " << PWLIB_VERSION);
#ifdef HAVE_GNOME
  PTRACE (1, "GNOME support enabled");
#else
  PTRACE (1, "GNOME support disabled");
#endif
#if defined HAVE_XV || defined HAVE_DX
  PTRACE (1, "Accelerated rendering support enabled");
#else
  PTRACE (1, "Accelerated rendering support disabled");
#endif
#ifdef HAVE_DBUS
  PTRACE (1, "DBUS support enabled");
#else
  PTRACE (1, "DBUS support disabled");
#endif
#ifdef HAVE_GCONF
  PTRACE (1, "GConf support enabled");
#else
  PTRACE (1, "GConf support disabled");
#endif
#ifdef HAVE_BONOBO
  PTRACE (1, "Bonobo support enabled");
#else
  PTRACE (1, "Bonobo support disabled");
#endif
#ifdef HAVE_ESD
  PTRACE (1, "ESound support enabled");
#else
  PTRACE (1, "ESound support disabled");
#endif
}


void GnomeMeeting::RemoveManager ()
{
  PWaitAndSignal m(ep_var_mutex);

  if (endpoint) {

    endpoint->Exit ();
    delete (endpoint);
  }
  
  endpoint = NULL;
}


PStringArray 
GnomeMeeting::GetInterfaces ()
{
  PWaitAndSignal m(iface_access_mutex);

  return interfaces;
}


PStringArray 
GnomeMeeting::GetVideoInputDevices ()
{
  PWaitAndSignal m(dev_access_mutex);

  return video_input_devices;
}


PStringArray 
GnomeMeeting::GetAudioInputDevices ()
{
  PWaitAndSignal m(dev_access_mutex);

  return audio_input_devices;
}



PStringArray 
GnomeMeeting::GetAudioOutpoutDevices ()
{
  PWaitAndSignal m(dev_access_mutex);

  return audio_output_devices;
}


PStringArray 
GnomeMeeting::GetAudioPlugins ()
{
  PWaitAndSignal m(dev_access_mutex);

  return audio_managers;
}


PStringArray 
GnomeMeeting::GetVideoPlugins ()
{
  PWaitAndSignal m(dev_access_mutex);

  return video_managers;
}

void GnomeMeeting::SetServices (GmServices* aservices)
{
  services = aservices;
}

GmServices* GnomeMeeting::GetServices ()
{
  return services;
}

