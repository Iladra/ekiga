
/* Ekiga -- A VoIP and Video-Conferencing application
 * Copyright (C) 2000-2007 Damien Sandras
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
 *                         gmtray-x11.c  -  description
 *                         ------------------------
 *   begin                : Sat Jan 7 2006
 *   copyright            : (C) 2006-2007 by Julien Puydt <jpuydt@free.fr>
 *   description          : X11 implementation of the tray
 */

#define __GMTRAY_IMPLEMENTATION__

#include "../../../config.h"

#include "gmtray-internal.h"
#include "eggtrayicon.h"

struct _GmTraySpecific
{
  GtkWidget *eggtray; /* the eggtray */
  GtkImage *image; /* eggtray's image (ie : what we really show) */
};

/* declaration of some helpers */
static void init_tray_with_image (GmTray *tray,
				  GtkWidget *image);

static gint clicked_cb (GtkWidget *unused,
			GdkEventButton *event,
			gpointer data);

static void destroyed_cb (GtkWidget *widget,
			  gpointer data);

/* definition of the helper functions */

static void
init_tray_with_image (GmTray *tray, GtkWidget *image)
{
  GtkWidget *event_box = NULL;
  GtkWidget *eggtray   = NULL;

  eggtray = GTK_WIDGET (egg_tray_icon_new (PACKAGE_NAME));
  event_box = gtk_event_box_new ();

  gtk_container_add (GTK_CONTAINER (event_box), image);
  gtk_container_add (GTK_CONTAINER (eggtray), event_box);

  gtk_widget_show_all (eggtray);

  g_signal_connect (G_OBJECT (event_box), "button_press_event",
		    G_CALLBACK (clicked_cb), tray);
  g_signal_connect (G_OBJECT (event_box), "destroy",
		    G_CALLBACK (destroyed_cb), tray);

  /* FIXME if (tray->specific->eggtray)
     g_object_unref (G_OBJECT (tray->specific->eggtray));*/

  tray->specific->eggtray = eggtray;
  tray->specific->image = GTK_IMAGE (image);
}

/* this function is the one which receives the clicks on the tray, and will
 * decide whether to call the click callback, show the menu, or ignore
 */
static gint
clicked_cb (GtkWidget *unused,
	    GdkEventButton *event,
	    gpointer data)
{
  GmTray *tray = data;

  g_return_val_if_fail (tray != NULL, FALSE);

  if (event->type == GDK_BUTTON_PRESS) {

    if (event->button == 1) {

      if (tray->left_clicked_callback)
	tray->left_clicked_callback (tray->left_clicked_callback_data);
      return TRUE;
    } else if (event->button == 2) {

      if (tray->middle_clicked_callback)
	tray->middle_clicked_callback (tray->middle_clicked_callback_data);
      return TRUE;
    } else if (event->button == 3) {

      gmtray_menu (tray);
      return TRUE;
    }
  }

  return FALSE;
}

static void
destroyed_cb (GtkWidget *widget,
	      gpointer data)
{
  GmTray     *tray  = data;
  GtkWidget  *image = NULL;
  gchar      *stock_id = NULL;
  GtkIconSize size;

  g_return_if_fail (tray != NULL);

  gtk_image_get_stock (GTK_IMAGE (tray->specific->image), &stock_id, &size);

  image = gtk_image_new_from_stock (stock_id, size);

  init_tray_with_image (tray, GTK_WIDGET (image));
}


/* public api implementation */


GmTray *
gmtray_new (const gchar *image)
{
  GmTray    *result    = NULL;
  GtkWidget *my_image  = NULL;

  result = gmtray_new_common (image);
  result->specific = g_new0 (GmTraySpecific, 1);

  my_image = gtk_image_new_from_stock (image, GTK_ICON_SIZE_MENU);

  init_tray_with_image (result, my_image);

  return result;
}


void
gmtray_delete (GmTray *tray)
{
  g_return_if_fail (tray != NULL);

  g_free (tray->specific);
  gmtray_delete_common (tray);
}


gboolean
gmtray_is_embedded (GmTray *tray)
{
  g_return_val_if_fail (tray != NULL, FALSE);

  return TRUE; /* FIXME */
}


void
gmtray_show_image (GmTray *tray,
		   const gchar *image)
{
  g_return_if_fail (tray != NULL);

  gtk_image_set_from_stock (tray->specific->image, image,
			    GTK_ICON_SIZE_MENU);
}


void
gmtray_menu (GmTray *tray)
{
  GtkMenu *menu = NULL;

  g_return_if_fail (tray != NULL);

  if (tray->menu_callback == NULL)
    return;

  menu = tray->menu_callback (tray->menu_callback_data);

  gtk_widget_show_all (GTK_WIDGET (menu));

  gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
		  0, gtk_get_current_event_time ());
}
