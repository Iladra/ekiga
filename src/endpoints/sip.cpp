
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
 *                         sipendpoint.cpp  -  description
 *                         --------------------------------
 *   begin                : Wed 8 Dec 2004
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains the SIP Endpoint class.
 *
 */


#include "config.h"

#include "sip.h"
#include "pcss.h"
#include "ekiga.h"
#include "urlhandler.h"

#include "main.h"
#include "chat.h"
#include "preferences.h"
#include "history.h"
#include "statusicon.h"
#include "misc.h"
#ifdef HAVE_DBUS
#include "dbus.h"
#endif

#include <gmconf.h>
#include <gmdialog.h>

#include <ptlib/ethsock.h>
#include <opal/transcoders.h>
#include <sip/handlers.h>

#include "presence-core.h"
#include "sip-endpoint.h"

#define new PNEW


/* The class */
GMSIPEndpoint::GMSIPEndpoint (GMManager & ep)
: SIPEndPoint (ep), endpoint (ep)
{
  NoAnswerTimer.SetNotifier (PCREATE_NOTIFIER (OnNoAnswerTimeout));
}


GMSIPEndpoint::~GMSIPEndpoint ()
{
}


void 
GMSIPEndpoint::Init ()
{
  GtkWidget *main_window = NULL;

  gchar *outbound_proxy_host = NULL;
  int binding_timeout = 60;

  main_window = GnomeMeeting::Process ()->GetMainWindow ();

  gnomemeeting_threads_enter ();
  outbound_proxy_host = gm_conf_get_string (SIP_KEY "outbound_proxy_host");
  binding_timeout = gm_conf_get_int (NAT_KEY "binding_timeout");
  gnomemeeting_threads_leave ();


  /* Timeouts */
  SetAckTimeout (PTimeInterval (0, 32));
  SetPduCleanUpTimeout (PTimeInterval (0, 1));
  SetInviteTimeout (PTimeInterval (0, 6));
  SetNonInviteTimeout (PTimeInterval (0, 6));
  SetNATBindingTimeout (PTimeInterval (0, 5));
  SetRetryTimeouts (500, 4000);
  SetMaxRetries (8);


  /* Update the User Agent */
  SetUserAgent ("Ekiga/" PACKAGE_VERSION);
  

  /* Initialise internal parameters */
  if (outbound_proxy_host && !PString (outbound_proxy_host).IsEmpty ())
    SetProxy (outbound_proxy_host);
  SetNATBindingRefreshMethod (SIPEndPoint::EmptyRequest);


  g_free (outbound_proxy_host);
}


BOOL 
GMSIPEndpoint::StartListener (PString iface, 
			      WORD port)
{
  PString iface_noip;
  PString ip;
  PIPSocket::InterfaceTable ifaces;
  PINDEX i = 0;
  PINDEX pos = 0;
  
  gboolean ok = FALSE;
  gboolean found = FALSE;

  gchar *listen_to = NULL;

  RemoveListener (NULL);

  /* Detect the valid interfaces */
  PIPSocket::GetInterfaceTable (ifaces);

  while (i < ifaces.GetSize ()) {
    
    ip = " [" + ifaces [i].GetAddress ().AsString () + "]";
    
    if (ifaces [i].GetName () + ip == iface) {
      listen_to = 
	g_strdup_printf ("udp$%s:%d", 
			 (const char *) ifaces [i].GetAddress().AsString(),
			 port);
      found = TRUE;
    }
      
    i++;
  }

  i = 0;
  pos = iface.Find("[");
  if (pos != P_MAX_INDEX)
    iface_noip = iface.Left (pos).Trim ();
  while (i < ifaces.GetSize() && !found) {

    if (ifaces [i].GetName () == iface_noip) {
      listen_to = 
	g_strdup_printf ("udp$%s:%d", 
			 (const char *) ifaces [i].GetAddress().AsString(),
			 port);
      found = TRUE;
    }
    
    i++;
  }

  /* Start the listener thread for incoming calls */
  if (!listen_to)
    return FALSE;

  ok = StartListeners (PStringArray (listen_to));
  g_free (listen_to);

  return ok;
}


OpalMediaFormatList
GMSIPEndpoint::GetAvailableAudioMediaFormats ()
{
  OpalMediaFormatList list;
  OpalMediaFormatList media_formats;
  OpalMediaFormatList sip_list;

  GMPCSSEndpoint *pcssEP = endpoint.GetPCSSEndpoint ();

  media_formats = pcssEP->GetMediaFormats ();
  list += OpalTranscoder::GetPossibleFormats (media_formats);

  for (int i = 0 ; i < list.GetSize () ; i++) {

    if (list [i].GetDefaultSessionID () == 1) { 
      
      if (PString (list [i].GetEncodingName ()).GetLength () > 0) {

        if (list [i].IsValidForProtocol ("SIP")
            && list [i].GetPayloadType () != RTP_DataFrame::MaxPayloadType)
          sip_list += list [i];
      }
    }
  }
  
  return sip_list;
}


OpalMediaFormatList
GMSIPEndpoint::GetAvailableVideoMediaFormats ()
{
  OpalMediaFormatList list;
  OpalMediaFormatList media_formats;
  OpalMediaFormatList sip_list;

  GMPCSSEndpoint *pcssEP = endpoint.GetPCSSEndpoint ();

  media_formats = pcssEP->GetMediaFormats ();
  list += OpalTranscoder::GetPossibleFormats (media_formats);

  for (int i = 0 ; i < list.GetSize () ; i++) {

    if (list [i].GetDefaultSessionID () == 2) { 
      
      if (list [i].IsValidForProtocol ("SIP")
          && list [i].GetPayloadType () != RTP_DataFrame::MaxPayloadType)
        sip_list += list [i];
    }
  }
  
  return sip_list;
}


void
GMSIPEndpoint::SetUserNameAndAlias ()
{
  PString default_local_name;

  default_local_name = endpoint.GetDefaultDisplayName ();

  if (!default_local_name.IsEmpty ()) {

    SetDefaultDisplayName (default_local_name);
  }
}


void 
GMSIPEndpoint::SetUserInputMode ()
{
  int mode = 0;

  gnomemeeting_threads_enter ();
  mode = gm_conf_get_int (SIP_KEY "dtmf_mode");
  gnomemeeting_threads_leave ();

  switch (mode) 
    {
    case 0:
      SetSendUserInputMode (OpalConnection::SendUserInputAsTone);
      break;
    case 1:
      SetSendUserInputMode (OpalConnection::SendUserInputAsInlineRFC2833);
      break;
    }
}


void
GMSIPEndpoint::PublishPresence (const PString & to,
                                guint state)
{
  PString status;
  PString note;
  PString body;

  switch (state) {

  case CONTACT_ONLINE:
    status = "Online";
    note = "open";
    break;

  case CONTACT_OFFLINE:
  case CONTACT_INVISIBLE:
    status = "Offline";
    note = "closed";
    break;
  
  case CONTACT_DND:
    status = "Do Not Disturb";
    note = "open";
    break;
  
  case CONTACT_AWAY:
    status = "Away";
    note = "open";
    break;
    
  case CONTACT_FREEFORCHAT:
    status = "Free For Chat";
    note = "open";
    break;
  }

  body = SIPPublishHandler::BuildBody (to, note, status);
  Publish (to, body, 500); // FIXME
}


void
GMSIPEndpoint::OnRegistered (const PString & aor,
			     BOOL wasRegistering)
{
  GMManager *ep = NULL;
  Ekiga::ServiceCore *services = NULL;
  SIP::EndPoint *sip_endpoint = NULL;
  
  GtkWidget *accounts_window = NULL;
  GtkWidget *history_window = NULL;
  GtkWidget *main_window = NULL;
#ifdef HAVE_DBUS
  GObject   *dbus_component = NULL;
#endif

  gchar *msg = NULL;
  guint status = CONTACT_ONLINE;

  ep = GnomeMeeting::Process ()->GetManager ();
  accounts_window = GnomeMeeting::Process ()->GetAccountsWindow ();
  main_window = GnomeMeeting::Process ()->GetMainWindow ();
  history_window = GnomeMeeting::Process ()->GetHistoryWindow ();
#ifdef HAVE_DBUS
  dbus_component = GnomeMeeting::Process ()->GetDbusComponent ();
#endif

  gnomemeeting_threads_enter ();
  /* Registering is ok */
  if (wasRegistering) {

    msg = g_strdup_printf (_("Registered %s"), 
			   (const char *) aor);
    gm_accounts_window_update_account_state (accounts_window, 
					     FALSE,
					     (const char *) aor, 
					     _("Registered"),
					     NULL);
  }
  else {

    msg = g_strdup_printf (_("Unregistered %s"),
			   (const char *) aor); 
    gm_accounts_window_update_account_state (accounts_window, 
					     FALSE,
					     (const char *) aor, 
					     _("Unregistered"),
					     NULL);
  }

#ifdef HAVE_DBUS
//  gnomemeeting_dbus_component_account_registration (dbus_component,
//						    username, domain,
//wasRegistering);
// FIXME
#endif

  gm_history_window_insert (history_window, "%s", msg);
  gm_main_window_flash_message (main_window, "%s", msg);
  if (endpoint.GetCallingState() == GMManager::Standby)
    gm_main_window_set_account_info (main_window, 
				     endpoint.GetRegisteredAccounts());
  gnomemeeting_threads_leave ();

  /* Signal the SIPEndpoint */
  SIPEndPoint::OnRegistered (aor, wasRegistering);
  
  /* Signal the SIP::EndPoint of our engine */
  services = GnomeMeeting::Process ()->GetServiceCore ();
  sip_endpoint = 
    dynamic_cast<SIP::EndPoint *>(services->get ("sip-endpoint"));
  if (sip_endpoint)
    sip_endpoint->OnRegistered (aor, wasRegistering);

  /* Publish current state */
  if (wasRegistering)
    status = gm_conf_get_int (PERSONAL_DATA_KEY "status");
  else
    status = CONTACT_OFFLINE;
  PublishPresence (aor, status);

  /* Subscribe for MWI */
  if (!IsSubscribed (SIPSubscribe::MessageSummary, aor)) { 
    SIPSubscribe::SubscribeType t = SIPSubscribe::MessageSummary;
    Subscribe (t, 3600, aor);
  }

  g_free (msg);
}


void
GMSIPEndpoint::OnRegistrationFailed (const PString & aor,
				     SIP_PDU::StatusCodes r,
				     BOOL wasRegistering)
{
  GtkWidget *accounts_window = NULL;
  GtkWidget *history_window = NULL;
  GtkWidget *main_window = NULL;

  gchar *msg_reason = NULL;
  gchar *msg = NULL;

  main_window = GnomeMeeting::Process ()->GetMainWindow ();
  accounts_window = GnomeMeeting::Process ()->GetAccountsWindow ();
  history_window = GnomeMeeting::Process ()->GetHistoryWindow ();

  gnomemeeting_threads_enter ();
  /* Registering is ok */
  switch (r) {

  case SIP_PDU::Failure_BadRequest:
    msg_reason = g_strdup (_("Bad request"));
    break;

  case SIP_PDU::Failure_PaymentRequired:
    msg_reason = g_strdup (_("Payment required"));
    break;

  case SIP_PDU::Failure_UnAuthorised:
  case SIP_PDU::Failure_Forbidden:
    msg_reason = g_strdup (_("Forbidden"));
    break;

  case SIP_PDU::Failure_RequestTimeout:
    msg_reason = g_strdup (_("Timeout"));
    break;

  case SIP_PDU::Failure_Conflict:
    msg_reason = g_strdup (_("Conflict"));
    break;

  case SIP_PDU::Failure_TemporarilyUnavailable:
    msg_reason = g_strdup (_("Temporarily unavailable"));
    break;
    
  case SIP_PDU::Failure_NotAcceptable:
    msg_reason = g_strdup (_("Not Acceptable"));
    break;

  default:
    msg_reason = g_strdup (_("Registration failed"));
  }

  if (wasRegistering) {

    msg = g_strdup_printf (_("Registration of %s failed: %s"), 
			   (const char *) aor, msg_reason);

    gm_accounts_window_update_account_state (accounts_window, 
					     FALSE,
					     (const char *) aor, 
					     _("Registration failed"),
					     NULL);
  }
  else {

    msg = g_strdup_printf (_("Unregistration of %s failed: %s"), 
			   (const char *) aor, msg_reason);

    gm_accounts_window_update_account_state (accounts_window, 
					     FALSE,
					     (const char *) aor, 
					     _("Unregistration failed"),
					     NULL);
  }

  gm_history_window_insert (history_window, "%s", msg);
  gm_main_window_push_message (main_window, "%s", msg);
  gnomemeeting_threads_leave ();

  /* Signal the SIP Endpoint */
  SIPEndPoint::OnRegistrationFailed (aor, r, wasRegistering);


  g_free (msg);
}


BOOL 
GMSIPEndpoint::OnIncomingConnection (OpalConnection &connection,
                                     unsigned options,
                                     OpalConnection::StringOptions * stroptions)
{
  PSafePtr<OpalConnection> con = NULL;
  PSafePtr<OpalCall> call = NULL;

  gchar *forward_host = NULL;

  guint status = CONTACT_ONLINE;
  gboolean busy_forward = FALSE;
  gboolean always_forward = FALSE;
  int no_answer_timeout = FALSE;

  BOOL res = FALSE;

  unsigned reason = 0;

  PTRACE (3, "GMSIPEndpoint\tIncoming connection");

  gnomemeeting_threads_enter ();
  forward_host = gm_conf_get_string (SIP_KEY "forward_host"); 
  busy_forward = gm_conf_get_bool (CALL_FORWARDING_KEY "forward_on_busy");
  always_forward = gm_conf_get_bool (CALL_FORWARDING_KEY "always_forward");
  status = gm_conf_get_int (PERSONAL_DATA_KEY "status");
  no_answer_timeout =
    gm_conf_get_int (CALL_OPTIONS_KEY "no_answer_timeout");
  gnomemeeting_threads_leave ();

  call = endpoint.FindCallWithLock (endpoint.GetCurrentCallToken());
  if (call)
    con = endpoint.GetConnection (call, TRUE);
  if ((con && con->GetIdentifier () == connection.GetIdentifier())) {
    return TRUE;
  }
  
  if (status == CONTACT_DND)
    reason = 1;

  else if (forward_host && always_forward)
    reason = 2; // Forward
  /* We are in a call */
  else if (endpoint.GetCallingState () != GMManager::Standby) {

    if (forward_host && busy_forward)
      reason = 2; // Forward
    else
      reason = 1; // Reject
  }
  else if (status == CONTACT_FREEFORCHAT)
    reason = 4; // Auto Answer
  else
    reason = 0; // Ask the user

  if (reason == 0)
    NoAnswerTimer.SetInterval (0, PMIN (no_answer_timeout, 60));

  res = endpoint.OnIncomingConnection (connection, reason, forward_host);
  g_free (forward_host);

  return res;
}


void 
GMSIPEndpoint::OnMWIReceived (const PString & to,
			      SIPSubscribe::MWIType type,
			      const PString & msgs)
{
  GMManager *ep = NULL;
  GMPCSSEndpoint *pcssEP = NULL;
  
  GtkWidget *main_window = NULL;
  GtkWidget *accounts_window = NULL;
  
  PString user;

  int total = 0;
  
  if (endpoint.GetMWI (to, user) != msgs) {

    total = endpoint.GetMWI ().AsInteger ();

    /* Update UI */
    endpoint.AddMWI (to, user, msgs);

    main_window = GnomeMeeting::Process ()->GetMainWindow ();
    accounts_window = GnomeMeeting::Process ()->GetAccountsWindow ();

    gnomemeeting_threads_enter ();
    gm_main_window_push_message (main_window, 
				 endpoint.GetMissedCallsNumber (), 
				 endpoint.GetMWI ());
    gm_accounts_window_update_account_state (accounts_window,
					     FALSE,
                                             to,
					     NULL,
					     (const char *) msgs);
    gnomemeeting_threads_leave ();

    /* Sound event if new voice mail */
    if (endpoint.GetMWI ().AsInteger () > total) {

      ep = GnomeMeeting::Process ()->GetManager ();
      pcssEP = ep->GetPCSSEndpoint ();
      pcssEP->PlaySoundEvent ("new_voicemail_sound");
    }
  }
}


void 
GMSIPEndpoint::OnReceivedMESSAGE (OpalTransport & transport,
				  SIP_PDU & pdu)
{
  PString *last = NULL;
  PString *val = NULL;
  
  PString from = pdu.GetMIME().GetFrom();   
  PINDEX j = from.Find (';');
  if (j != P_MAX_INDEX)
    from = from.Left(j); // Remove all parameters
  j = from.Find ('<');
  if (j != P_MAX_INDEX && from.Find ('>') == P_MAX_INDEX)
    from += '>';

  PWaitAndSignal m(msgDataMutex);
  last = msgData.GetAt (SIPURL (from).AsString ());
  if (!last || *last != pdu.GetMIME ().GetCallID ()) {

    val = new PString (pdu.GetMIME ().GetCallID ());
    msgData.SetAt (SIPURL (from).AsString (), val);
    endpoint.OnMessageReceived(from, pdu.GetEntityBody());
  }
}


void 
GMSIPEndpoint::OnMessageFailed (const SIPURL & messageUrl,
				SIP_PDU::StatusCodes reason)
{
  GtkWidget *chat_window = NULL;
  gchar *msg = NULL;

  chat_window = GnomeMeeting::Process ()->GetChatWindow ();
  
  switch (reason) {

  case SIP_PDU::Failure_NotFound:
    msg = g_strdup (_("Error: User not found"));
    break;

  case SIP_PDU::Failure_TemporarilyUnavailable:
    msg = g_strdup (_("Error: User offline"));
    break;

  case SIP_PDU::Failure_UnAuthorised:
  case SIP_PDU::Failure_Forbidden:
    msg = g_strdup (_("Error: Forbidden"));
    break;

  case SIP_PDU::Failure_RequestTimeout:
    msg = g_strdup (_("Error: Timeout"));
    break;

  default:
    msg = g_strdup (_("Error: Failed to transmit message"));
  }

  gnomemeeting_threads_enter ();
  gm_text_chat_window_insert (chat_window, messageUrl.AsString (), 
			      NULL, msg, 2);
  gnomemeeting_threads_leave ();

  g_free (msg);
}
      

int
GMSIPEndpoint::GetRegisteredAccounts ()
{
  return SIPEndPoint::GetRegistrationsCount ();
}


SIPURL
GMSIPEndpoint::GetRegisteredPartyName (const PString & host)
{
  GmAccount *account = NULL;

  PString url;
  SIPURL registration_address;

  PSafePtr<SIPHandler> info = activeSIPHandlers.FindSIPHandlerByDomain(host, SIP_PDU::Method_REGISTER, PSafeReadOnly);

  if (info != NULL)
    registration_address = info->GetTargetAddress();

  account = gnomemeeting_get_default_account ("SIP");
  if (account && account->enabled) {

    if (info == NULL || registration_address.GetHostName () == account->host) {

      if (PString(account->username).Find("@") == P_MAX_INDEX)
        url = PString (account->username) + "@" + PString (account->host);
      else
        url = PString (account->username);

      return url;
    }
  }
  if (info != NULL) 
    return registration_address;
  else 
    return SIPEndPoint::GetDefaultRegisteredPartyName (); 
}


void 
GMSIPEndpoint::OnEstablished (OpalConnection &connection)
{
  NoAnswerTimer.Stop ();

  PTRACE (3, "GMSIPEndpoint\t SIP connection established");
  SIPEndPoint::OnEstablished (connection);
}


void 
GMSIPEndpoint::OnReleased (OpalConnection &connection)
{
  NoAnswerTimer.Stop ();

  PTRACE (3, "GMSIPEndpoint\t SIP connection released");
  SIPEndPoint::OnReleased (connection);
}


void 
GMSIPEndpoint::OnPresenceInfoReceived (const PString & user,
                                       const PString & basic,
                                       const PString & note)
{
  Ekiga::ServiceCore *services = NULL;
  SIP::EndPoint *sip_endpoint = NULL;

  /* Signal the SIP::EndPoint of our engine */
  services = GnomeMeeting::Process ()->GetServiceCore ();
  sip_endpoint = 
    dynamic_cast<SIP::EndPoint *>(services->get ("sip-endpoint"));
  if (sip_endpoint)
    sip_endpoint->OnPresenceInfoReceived (user, basic, note);
}


void
GMSIPEndpoint::OnNoAnswerTimeout (PTimer &,
                                  INT) 
{
  gchar *forward_host = NULL;
  gboolean forward_on_no_answer = FALSE;
  
  if (endpoint.GetCallingState () == GMManager::Called) {
   
    gnomemeeting_threads_enter ();
    forward_host = gm_conf_get_string (SIP_KEY "forward_host");
    forward_on_no_answer = 
      gm_conf_get_bool (CALL_FORWARDING_KEY "forward_on_no_answer");
    gnomemeeting_threads_leave ();

    if (forward_host && forward_on_no_answer) {
      
      PSafePtr<OpalCall> call = 
        endpoint.FindCallWithLock (endpoint.GetCurrentCallToken ());
      PSafePtr<OpalConnection> con = 
        endpoint.GetConnection (call, TRUE);
    
      con->ForwardCall (forward_host);
    }
    else
      ClearAllCalls (OpalConnection::EndedByNoAnswer, FALSE);

    g_free (forward_host);
  }
}


