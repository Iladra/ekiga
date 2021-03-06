
/* Ekiga -- A VoIP and Video-Conferencing application
 * Copyright (C) 2000-2009 Damien Sandras <dsandras@seconix.com>
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
 *   			    manipulate accounts.
 */


#ifndef _ACCOUNTS_H_
#define _ACCOUNTS_H_

#include <glib.h>
#include <gtk/gtk.h>

#include "services.h"

#include "gmwindow.h"
#include "account-core.h"
#include "personal-details.h"

typedef struct _AccountsWindow AccountsWindow;
typedef struct _AccountsWindowPrivate AccountsWindowPrivate;
typedef struct _AccountsWindowClass AccountsWindowClass;

/* GObject thingies */
struct _AccountsWindow
{
  GmWindow parent;

  AccountsWindowPrivate *priv;
};

struct _AccountsWindowClass
{
  GmWindowClass parent;
};


#define ACCOUNTS_WINDOW_TYPE (accounts_window_get_type ())

#define ACCOUNTS_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ACCOUNTS_WINDOW_TYPE, AccountsWindow))

#define IS_ACCOUNTS_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ACCOUNTS_WINDOW_TYPE))

#define ACCOUNTS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), ACCOUNTS_WINDOW_TYPE, AccountsWindowClass))

#define IS_ACCOUNTS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ACCOUNTS_WINDOW_TYPE))

#define ACCOUNTS_WINDOW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), ACCOUNTS_WINDOW_TYPE, AccountsWindowClass))

GType accounts_window_get_type ();


/* The API */

/* DESCRIPTION  : /
 * BEHAVIOR     : Builds the GMAccounts window GObject.
 * PRE          : /
 */
GtkWidget* accounts_window_new (boost::shared_ptr<Ekiga::AccountCore> account_core,
				boost::shared_ptr<Ekiga::PersonalDetails> details);

#endif
