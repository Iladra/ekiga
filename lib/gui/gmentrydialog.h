
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
 *                         gmentrydialog.h  -  description
 *                         -------------------------------
 *   begin                : Sat Jan 03 2004
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : Contains a gmentrydialog widget permitting to
 *                          quickly build GtkDialogs with a +rw GtkEntry
 *                          field.
 *
 */


#ifndef __GM_ENTRY_DIALOG_H__
#define __GM_ENTRY_DIALOG_H__

#include <gtk/gtk.h>


G_BEGIN_DECLS

typedef struct _GmEntryDialogPrivate GmEntryDialogPrivate;

typedef struct
{
  GtkDialog parent;

  GmEntryDialogPrivate* priv;

} GmEntryDialog;


typedef struct
{
  GtkDialogClass parent_class;

} GmEntryDialogClass;


/* The functions */

/* DESCRIPTION  :  /
 * BEHAVIOR     :  Creates a new GmEntryDialog with the specified text and
 *                 and button. The button will give the GTK_RESPONSE_ACCEPT
 *                 answer.
 * PRE          :  Both texts must be != NULL.
 */
GtkWidget* gm_entry_dialog_new (const char* label,
				const char* button_label);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Set the given text for the GtkEntry in the
 *                 given GmEntryDialog.
 * PRE          :  Both parameters must be != NULL.
 */
void gm_entry_dialog_set_text (GmEntryDialog*,
			       const char* text);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Returns the text in the GtkEntry inside the GmEntryDialog.
 * PRE          :  Non-NULL GmEntryDialog.
 */
const gchar* gm_entry_dialog_get_text (GmEntryDialog*);


/* GObject boilerplate */

GType gm_entry_dialog_get_type (void);

#define GM_TYPE_ENTRY_DIALOG (gm_entry_dialog_get_type ())
#define GM_ENTRY_DIALOG(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GM_TYPE_ENTRY_DIALOG, GmEntryDialog))
#define GM_ENTRY_DIALOG_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), GM_TYPE_ENTRY_DIALOG, GmEntryDialogClass))
#define GM_IS_ENTRY_DIALOG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GM_TYPE_ENTRY_DIALOG))
#define GM_IS_ENTRY_DIALOG_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GM_TYPE_ENTRY_DIALOG))
#define GM_ENTRY_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GM_TYPE_ENTRY_DIALOG, GmEntryDialogClass))


G_END_DECLS

#endif
