
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
 *                         accounts.h  -  description
 *                         --------------------------
 *   begin                : Sun Feb 13 2005
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains all the functions needed to
 *   			    manipulate H.323/SIP/... provider accounts.
 */


#ifndef _ACCOUNTS_H_
#define _ACCOUNTS_H_

#include "common.h"
#include "services.h"

namespace Opal {
  class CallManager;
}


/* An account is uniquely identified by its Account ID */
struct GmAccount_ {

  gchar *aid;			/* Account ID */
  gchar *account_name;		/* Account Name */
  gchar *protocol_name;		/* Protocol for the account */
  gchar *host;			/* Host to register to */
  gchar *domain;		/* Registration domain/realm */
  gchar *username;		/* Username */
  gchar *auth_username;		/* Authentication username */
  gchar *password;		/* Password */
  gboolean enabled;		/* Account active or not */
  gboolean default_account;	/* Default account or not */
  int timeout;			/* Registration timeout */
  int method;			/* Registration method */
}; 

typedef GmAccount_ GmAccount;


#define GM_ACCOUNT(x)     (GmAccount *) (x)


/* The API */


/* DESCRIPTION  : /
 * BEHAVIOR     : Returns an empty GmAccount. Only the UID field has a unique
 *                value.
 * PRE          : /
 */
GmAccount *gm_account_new ();


/* DESCRIPTION  : /
 * BEHAVIOR     : Deletes the given account and frees the associated memory.
 * PRE          : /
 */
void gm_account_delete (GmAccount *account);


/* DESCRIPTION  : /
 * BEHAVIOR     : Makes a copy of the given account, keeps the same ID.
 * PRE          : /
 */
GmAccount *gm_account_copy (GmAccount *account);


/* DESCRIPTION  : /
 * BEHAVIOR     : Adds the given account to the accounts list. The given 
 * 		  account and its ID must not exist yet in the list.
 * 		  Returns TRUE on success, FALSE on failure.
 * 		  If there is no default account for that protocol yet,
 * 		  then the added account becomes the new default.
 * PRE          : /
 */
gboolean gnomemeeting_account_add (GmAccount *account);


/* DESCRIPTION  : /
 * BEHAVIOR     : Deletes the given account from the accounts list. 
 * 		  Returns TRUE on success, FALSE on failure.
 * 		  If that account was the default account for that protocol,
 * 		  then the first account of the same protocol will become
 * 		  the new default account.
 * PRE          : /
 */
gboolean gnomemeeting_account_delete (GmAccount *account);


/* DESCRIPTION  : /
 * BEHAVIOR     : Modifies the given account in the accounts list. 
 * 		  Returns TRUE on success, FALSE on failure.
 * PRE          : /
 */
gboolean gnomemeeting_account_modify (GmAccount *account);


/* DESCRIPTION  : /
 * BEHAVIOR     : Returns the list of configured GmAccounts.
 * PRE          : /
 */
GSList *gnomemeeting_get_accounts_list ();


/* DESCRIPTION  : /
 * BEHAVIOR     : Returns the account for the given domain or host.
 * PRE          : /
 */
GmAccount *gnomemeeting_get_account (const char *);


/* DESCRIPTION  : /
 * BEHAVIOR     : Returns the default account for the given protocol (if any).
 * PRE          : /
 */
GmAccount *gnomemeeting_get_default_account (const gchar *protocol);

	
/* DESCRIPTION  : /
 * BEHAVIOR     : Toggles the active state of the given account.
 * 		  Returns TRUE on success, FALSE on failure.
 * PRE          : /
 */
gboolean gnomemeeting_account_toggle_active (GmAccount *account);


/* DESCRIPTION  : /
 * BEHAVIOR     : Sets the account as default or not.
 * 		  Returns TRUE on success, FALSE on failure.
 * PRE          : /
 */
gboolean gnomemeeting_account_set_default (GmAccount *account,
					   gboolean default_account);


/* DESCRIPTION  : /
 * BEHAVIOR     : Builds the GMAccounts window GMObject.
 * PRE          : /
 */
GtkWidget *gm_accounts_window_new (Ekiga::ServiceCore &core);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Update the account corresponding to the given domain, and
 * 		   username with the given status message. Enables or not
 * 		   the refreshing state for that account (see below for
 * 		   the implications).
 * PRE          :  /
 */

void gm_accounts_window_update_account_state (GtkWidget *accounts_window,
					      gboolean refreshing,
					      const gchar *aor,
					      const gchar *status,
					      const gchar *voicemails);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Refreshes the accounts list in the GUI to update them from
 *                 the accounts list in the GmConf user configuration.
 *                 If one of the account is in "refreshing state", then all
 *                 other accounts are unsensitive and the busy cursor is 
 *                 displayed for the accounts list box.
 * PRE          :  /
 */
void gm_accounts_window_update_accounts_list (GtkWidget *accounts_window);


#endif
