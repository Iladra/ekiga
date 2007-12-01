
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
 *                         pcssendpoint.h  -  description
 *                         ------------------------------
 *   begin                : Sun Oct 24 2004
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains the PCSS Endpoint class.
 *
 */


#ifndef _PCSS_ENDPOINT_H_
#define _PCSS_ENDPOINT_H_

#include "common.h"
#include "manager.h"


class GMPCSSEndpoint : public OpalPCSSEndPoint
{
  PCLASSINFO (GMPCSSEndpoint, OpalPCSSEndPoint);

public:
  GMPCSSEndpoint (GMManager &);

  
  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Starts a call, play the dialtone.
   * PRE          :  /
   */
  bool MakeConnection (OpalCall & call, 
                       const PString & party,  
                       void * userData);


  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Accept the current incoming call.
   * PRE          :  /
   */
  void AcceptCurrentIncomingCall ();
  

  /* DESCRIPTION  :  This callback is called when there is an 
   * 		     incoming PCSS connection. This only happens
   * 		     when the SIP/H.323 connection is not rejected
   * 		     or forwarded.
   * 		     It triggers the appropriate timeouts (no answer, ringing).
   *		     Display a popup if required.
   * PRE          :  /
   */
  virtual bool OnShowIncoming (const OpalPCSSConnection &connection);


  /* DESCRIPTION  :  This callback is called when there is an 
   * 		     outgoing PCSS connection and the remote endpoint is
   * 		     ringing.
   * 		     It triggers the appropriate timeout in order
   * 		     to generate a ring tone.
   * PRE          :  /
   */
  virtual bool OnShowOutgoing (const OpalPCSSConnection &connection);
  
  
  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Plays a sound event on its device.
   * PRE          :  /
   */
  void PlaySoundEvent (PString ev);
  
  
  /* DESCRIPTION  :  This callback is called when an audio channel has to
   *                 be opened.
   * BEHAVIOR     :  Open the Audio Channel or warn the user if it was
   *                 impossible.
   * PRE          :  /
   */
  PSoundChannel *CreateSoundChannel (const OpalPCSSConnection &connection,
				     const OpalMediaFormat &format,
				     bool is_source);

  
  /* DESCRIPTION  :  This callback is called when the connection is 
   *                 established and everything is ok.
   * BEHAVIOR     :  Stops the timers.
   * PRE          :  /
   */
  void OnEstablished (OpalConnection &);

  
  /* DESCRIPTION  :  This callback is called when a connection to a remote
   *                 endpoint is cleared.
   * BEHAVIOR     :  Stops the timers.
   * PRE          :  /
   */
  void OnReleased (OpalConnection &connection);


  // FIXME
  virtual PString OnGetDestination (const OpalPCSSConnection &connection);  
  

  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Returns the playing and recording volume levels.
   * PRE          :  /
   */
  void GetDeviceVolume (unsigned int &play_vol,
			unsigned int &record_vol);


  /* DESCRIPTION  :  /
   * BEHAVIOR     :  Updates the playing and recording volume levels.
   *                 Returns FALSE if it fails.
   * PRE          :  /
   */
  bool SetDeviceVolume (unsigned int play_vol,
			unsigned int record_vol);

private:

  /* DESCRIPTION  :  Notifier called every second while waiting for an answer
   *                 for an incoming call.
   * BEHAVIOR     :  Display an animation in the docklet and play a ring
   *                 sound.
   * PRE          :  /
   */
  PDECLARE_NOTIFIER(PTimer, GMPCSSEndpoint, OnCallPending);

  
  /* DESCRIPTION  :  Notifier called every 2 seconds while waiting for an
   *                 answer for an outging call.
   * BEHAVIOR     :  Display an animation in the main winow and play a ring
   *                 sound.
   * PRE          :  /
   */
  PDECLARE_NOTIFIER(PTimer, GMPCSSEndpoint, OnOutgoingCall);
  

  PTimer CallPendingTimer;
  PTimer OutgoingCallTimer;
  
  GMManager & endpoint;

  PString incomingConnectionToken; 

  PMutex sound_event_mutex;
};

#endif
