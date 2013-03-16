/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006-2007 Imendio AB
 * Copyright (C) 2007-2011 Collabora Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Authors: Kristian Rietveld <kris@imendio.com>
 */

#include <gtk/gtk.h>

#include "gmcellrendererexpander.h"

typedef struct {
	GtkExpanderStyle     expander_style;
	gint                 expander_size;

	guint                activatable : 1;
} EmpathyCellRendererExpanderPriv;

enum {
	PROP_0,
	PROP_EXPANDER_STYLE,
	PROP_EXPANDER_SIZE,
	PROP_ACTIVATABLE
};

static void     gm_cell_renderer_expander_get_property (GObject                         *object,
                                                        guint                            param_id,
                                                        GValue                          *value,
                                                        GParamSpec                      *pspec);
static void     gm_cell_renderer_expander_set_property (GObject                         *object,
                                                        guint                            param_id,
                                                        const GValue                    *value,
                                                        GParamSpec                      *pspec);
static void     gm_cell_renderer_expander_finalize     (GObject                         *object);
static void     gm_cell_renderer_expander_get_size     (GtkCellRenderer                 *cell,
                                                        GtkWidget                       *widget,
                                                        const GdkRectangle              *cell_area,
                                                        gint                            *x_offset,
                                                        gint                            *y_offset,
                                                        gint                            *width,
                                                        gint                            *height);
static void     gm_cell_renderer_expander_render       (GtkCellRenderer                 *cell,
                                                        cairo_t *cr,
                                                        GtkWidget                       *widget,
                                                        const GdkRectangle              *background_area,
                                                        const GdkRectangle              *cell_area,
                                                        GtkCellRendererState             flags);
static gboolean gm_cell_renderer_expander_activate     (GtkCellRenderer                 *cell,
                                                        GdkEvent                        *event,
                                                        GtkWidget                       *widget,
                                                        const gchar                     *path,
                                                        const GdkRectangle              *background_area,
                                                        const GdkRectangle              *cell_area,
                                                        GtkCellRendererState             flags);

G_DEFINE_TYPE (EmpathyCellRendererExpander, gm_cell_renderer_expander, GTK_TYPE_CELL_RENDERER)

static void
gm_cell_renderer_expander_init (EmpathyCellRendererExpander *expander)
{
	EmpathyCellRendererExpanderPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (expander,
		GM_TYPE_CELL_RENDERER_EXPANDER, EmpathyCellRendererExpanderPriv);

	expander->priv = priv;
	priv->expander_style = GTK_EXPANDER_COLLAPSED;
	priv->expander_size = 12;
	priv->activatable = TRUE;

	g_object_set (expander,
		      "xpad", 2,
		      "ypad", 2,
		      "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE,
		      NULL);
}

static void
gm_cell_renderer_expander_class_init (EmpathyCellRendererExpanderClass *klass)
{
	GObjectClass         *object_class;
	GtkCellRendererClass *cell_class;

	object_class  = G_OBJECT_CLASS (klass);
	cell_class = GTK_CELL_RENDERER_CLASS (klass);

	object_class->finalize = gm_cell_renderer_expander_finalize;

	object_class->get_property = gm_cell_renderer_expander_get_property;
	object_class->set_property = gm_cell_renderer_expander_set_property;

	cell_class->get_size = gm_cell_renderer_expander_get_size;
	cell_class->render = gm_cell_renderer_expander_render;
	cell_class->activate = gm_cell_renderer_expander_activate;

	g_object_class_install_property (object_class,
					 PROP_EXPANDER_STYLE,
					 g_param_spec_enum ("expander-style",
							    "Expander Style",
							    "Style to use when painting the expander",
							    GTK_TYPE_EXPANDER_STYLE,
							    GTK_EXPANDER_COLLAPSED,
							    G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_EXPANDER_SIZE,
					 g_param_spec_int ("expander-size",
							   "Expander Size",
							   "The size of the expander",
							   0,
							   G_MAXINT,
							   12,
							   G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_ACTIVATABLE,
					 g_param_spec_boolean ("activatable",
							       "Activatable",
							       "The expander can be activated",
							       TRUE,
							       G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (EmpathyCellRendererExpanderPriv));
}

static void
gm_cell_renderer_expander_get_property (GObject    *object,
                                        guint       param_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
	EmpathyCellRendererExpander     *expander;
	EmpathyCellRendererExpanderPriv *priv;

	expander = GM_CELL_RENDERER_EXPANDER (object);
	priv = expander->priv;

	switch (param_id) {
	case PROP_EXPANDER_STYLE:
		g_value_set_enum (value, priv->expander_style);
		break;

	case PROP_EXPANDER_SIZE:
		g_value_set_int (value, priv->expander_size);
		break;

	case PROP_ACTIVATABLE:
		g_value_set_boolean (value, priv->activatable);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gm_cell_renderer_expander_set_property (GObject      *object,
                                        guint         param_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
	EmpathyCellRendererExpander     *expander;
	EmpathyCellRendererExpanderPriv *priv;

	expander = GM_CELL_RENDERER_EXPANDER (object);
	priv = expander->priv;

	switch (param_id) {
	case PROP_EXPANDER_STYLE:
		priv->expander_style = g_value_get_enum (value);
		break;

	case PROP_EXPANDER_SIZE:
		priv->expander_size = g_value_get_int (value);
		break;

	case PROP_ACTIVATABLE:
		priv->activatable = g_value_get_boolean (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gm_cell_renderer_expander_finalize (GObject *object)
{
	(* G_OBJECT_CLASS (gm_cell_renderer_expander_parent_class)->finalize) (object);
}

GtkCellRenderer *
gm_cell_renderer_expander_new (void)
{
	return g_object_new (GM_TYPE_CELL_RENDERER_EXPANDER, NULL);
}

static void
gm_cell_renderer_expander_get_size (GtkCellRenderer         *cell,
                                    G_GNUC_UNUSED GtkWidget *widget,
                                    const GdkRectangle      *cell_area,
                                    gint                    *x_offset,
                                    gint                    *y_offset,
                                    gint                    *width,
                                    gint                    *height)
{
	EmpathyCellRendererExpander     *expander;
	EmpathyCellRendererExpanderPriv *priv;
	gfloat xalign, yalign;
	guint xpad, ypad;

	expander = (EmpathyCellRendererExpander *) cell;
	priv = expander->priv;

	g_object_get (cell,
		      "xalign", &xalign,
		      "yalign", &yalign,
		      "xpad", &xpad,
		      "ypad", &ypad,
		      NULL);

	if (cell_area) {
		if (x_offset) {
			*x_offset = xalign * (cell_area->width - (priv->expander_size + (2 * xpad)));
			*x_offset = MAX (*x_offset, 0);
		}

		if (y_offset) {
			*y_offset = yalign * (cell_area->height - (priv->expander_size + (2 * ypad)));
			*y_offset = MAX (*y_offset, 0);
		}
	} else {
		if (x_offset)
			*x_offset = 0;

		if (y_offset)
			*y_offset = 0;
	}

	if (width)
		*width = xpad * 2 + priv->expander_size;

	if (height)
		*height = ypad * 2 + priv->expander_size;
}

static void
gm_cell_renderer_expander_render (GtkCellRenderer                    *cell,
				  cairo_t                            *cr,
				  GtkWidget                          *widget,
				  G_GNUC_UNUSED const GdkRectangle   *background_area,
				  const GdkRectangle                 *cell_area,
				  GtkCellRendererState               flags)
{
	EmpathyCellRendererExpander     *expander;
	EmpathyCellRendererExpanderPriv *priv;
	gint                            x_offset, y_offset;
	guint                           xpad, ypad;
	GtkStyleContext                 *style;
	GtkStateFlags                    state;

	expander = (EmpathyCellRendererExpander *) cell;
	priv = expander->priv;

	gm_cell_renderer_expander_get_size (cell, widget,
						(GdkRectangle *) cell_area,
						&x_offset, &y_offset,
						NULL, NULL);

	g_object_get (cell,
		      "xpad", &xpad,
		      "ypad", &ypad,
		      NULL);

	style = gtk_widget_get_style_context (widget);

	gtk_style_context_save (style);
	gtk_style_context_add_class (style, GTK_STYLE_CLASS_EXPANDER);

	state = gtk_cell_renderer_get_state (cell, widget, flags);

	if (priv->expander_style == GTK_EXPANDER_COLLAPSED)
		state |= GTK_STATE_FLAG_NORMAL;
	else
		state |= GTK_STATE_FLAG_ACTIVE;

	gtk_style_context_set_state (style, state);

	gtk_render_expander (style,
			     cr,
			     cell_area->x + x_offset + xpad,
			     cell_area->y + y_offset + ypad,
			     priv->expander_size,
			     priv->expander_size);

	gtk_style_context_restore (style);
}

static gboolean
gm_cell_renderer_expander_activate (GtkCellRenderer                    *cell,
			            G_GNUC_UNUSED GdkEvent             *event,
                                    GtkWidget                          *widget,
                                    const gchar                        *path_string,
                                    G_GNUC_UNUSED const GdkRectangle   *background_area,
                                    G_GNUC_UNUSED const GdkRectangle   *cell_area,
                                    G_GNUC_UNUSED GtkCellRendererState flags)
{
	EmpathyCellRendererExpander     *expander;
	EmpathyCellRendererExpanderPriv *priv;
	GtkTreePath                     *path;

	expander = (EmpathyCellRendererExpander *) cell;
	priv = expander->priv;

	if (!GTK_IS_TREE_VIEW (widget) || !priv->activatable)
		return FALSE;

	path = gtk_tree_path_new_from_string (path_string);

	if (gtk_tree_path_get_depth (path) > 1) {
		gtk_tree_path_free (path);
		return TRUE;
	}

	if (gtk_tree_view_row_expanded (GTK_TREE_VIEW (widget), path)) {
		gtk_tree_view_collapse_row (GTK_TREE_VIEW (widget), path);
	} else {
		gtk_tree_view_expand_row (GTK_TREE_VIEW (widget), path, FALSE);
	}

	gtk_tree_path_free (path);

	return TRUE;
}
