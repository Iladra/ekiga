
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
 *                         vfakeio.cpp  -  description
 *                         ---------------------------
 *   begin                : Tue Jul 30 2003
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains a descendant of a Fake Input 
 *                          Device that display the GM logo when connected to
 *                          a remote party without using a camera.
 *
 */


#include "../../config.h"

#include "fakevideoinput.h"

#include "misc.h"
#include "gmconf.h"
#include "gmstockicons.h"

#include <ptlib/vconvert.h>

#define DEFAULT_ICON_SIZE 72

PVideoInputDevice_Picture::PVideoInputDevice_Picture ()
{
  orig_pix = NULL;
  cached_pix = NULL;
	
  pos = 0;
  increment = 1;

  moving = false;

  SetFrameRate (12);
}


PVideoInputDevice_Picture::~PVideoInputDevice_Picture ()
{
  Close ();
}


BOOL
PVideoInputDevice_Picture::Open (const PString &name,
				 BOOL start_immediate)
{
  gchar *image_name = NULL;
  GdkPixbuf *icon = NULL;
    
  if (IsOpen ())
    return FALSE;
  
  if (name == "MovingLogo") {
  
    moving = true;
    icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                     GM_ICON_LOGO, DEFAULT_ICON_SIZE,
                                     GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
    if (icon != NULL) {
    
      /* Transparency color is white */
      orig_pix = gdk_pixbuf_composite_color_simple (icon, DEFAULT_ICON_SIZE,
                                                    DEFAULT_ICON_SIZE, 
                                                    GDK_INTERP_BILINEAR,
                                                    255, 1, 0xFFFFFFFF,
                                                    0xFFFFFFFF);
      g_object_unref (icon);
      icon = NULL;
    }
  
    return TRUE;
  }
  else {

    /* from there on, we're in the static picture case! */
    moving = false;

    image_name = gm_conf_get_string (VIDEO_DEVICES_KEY "image");

    PWaitAndSignal m(pixbuf_mutex);
    if (orig_pix != NULL) {

      g_object_unref (G_OBJECT (orig_pix));
      orig_pix = NULL;
    }

    orig_pix = gdk_pixbuf_new_from_file (image_name, NULL);
    g_free (image_name);
    
    if (!orig_pix) {

      icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                       GM_ICON_LOGO, DEFAULT_ICON_SIZE,
                                       GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
      if (icon != NULL) {

        /* Transparency color is white */
        orig_pix = gdk_pixbuf_composite_color_simple (icon, DEFAULT_ICON_SIZE,
                                                      DEFAULT_ICON_SIZE, 
                                                      GDK_INTERP_BILINEAR,
                                                      255, 1, 0xFFFFFFFF,
                                                      0xFFFFFFFF);
        g_object_unref (icon);
        icon = NULL;
      }
    }

    if (orig_pix) 
      return TRUE;

    return FALSE;
  }
}


BOOL
PVideoInputDevice_Picture::IsOpen ()
{
  if (orig_pix) 
    return TRUE;
  
  return FALSE;
}


BOOL
PVideoInputDevice_Picture::Close ()
{
  gnomemeeting_threads_enter ();
    
  PWaitAndSignal m(pixbuf_mutex);

  if (orig_pix != NULL) {
  
    g_object_unref (G_OBJECT (orig_pix));
    orig_pix = NULL;
  }
  
  if (cached_pix != NULL) {
  
    g_object_unref (G_OBJECT (cached_pix));
    cached_pix = NULL;
  }
  
  gnomemeeting_threads_leave ();
  
  return TRUE;
}

  
BOOL
PVideoInputDevice_Picture::Start ()
{
  return TRUE;
}

  
BOOL
PVideoInputDevice_Picture::Stop ()
{
  return TRUE;
}


BOOL
PVideoInputDevice_Picture::IsCapturing ()
{
  return IsCapturing ();
}


PStringList
PVideoInputDevice_Picture::GetInputDeviceNames ()
{
  PStringList l;

  l.AppendString ("StaticPicture");
  l.AppendString ("MovingLogo");

  return l;
}


BOOL
PVideoInputDevice_Picture::SetFrameSize (unsigned int width,
					 unsigned int height)
{
  if (!PVideoDevice::SetFrameSize (width, height))
    return FALSE;

  return TRUE;
}


BOOL
PVideoInputDevice_Picture::GetFrameData (BYTE *a, PINDEX *i)
{
  WaitFinishPreviousFrame ();

  GetFrameDataNoDelay (a, i);

  *i = CalculateFrameBytes (frameWidth, frameHeight, colourFormat);

  return TRUE;
}


BOOL PVideoInputDevice_Picture::GetFrameDataNoDelay (BYTE *frame, PINDEX *i)
{
  GdkPixbuf *scaled_pix = NULL;

  guchar *data = NULL;

  unsigned width = 0;
  unsigned height = 0;

  int orig_width = 0;
  int orig_height = 0;

  double scale_w = 0.0;
  double scale_h = 0.0;
  double scale = 0.0;

  GetFrameSize (width, height);

  PWaitAndSignal m(pixbuf_mutex);

  if (orig_pix == NULL)
    return FALSE;

  gnomemeeting_threads_enter ();

  if (!cached_pix) {

    cached_pix = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 
				 TRUE, 
				 8,
				 width, 
				 height);
    gdk_pixbuf_fill (cached_pix, 0xFFFFFFFF);
  }

  if (!moving) { /* create the ever-displayed picture */


    orig_width = gdk_pixbuf_get_width (orig_pix);
    orig_height = gdk_pixbuf_get_height (orig_pix);

    if ((unsigned) orig_width <= width && (unsigned) orig_height <= height) {

      /* the picture fits in the  target space: center it */
      gdk_pixbuf_copy_area (orig_pix, 
			    0, 0, orig_width, orig_height,
			    cached_pix, 
			    (width - orig_width) / 2, 
			    (height - orig_height) / 2);
    }
    else { 

      /* the picture doesn't fit: scale 1:1, and center */
      scale_w = (double) width / orig_width;
      scale_h = (double) height / orig_height;

      if (scale_w < scale_h) /* one of them is known to be < 1 */
	scale = scale_w;
      else
	scale = scale_h;

      scaled_pix = 
	gdk_pixbuf_scale_simple (orig_pix, 
				 (int) (scale * orig_width),
				 (int) (scale * orig_height), 
				 GDK_INTERP_BILINEAR);

      gdk_pixbuf_copy_area (scaled_pix, 
			    0, 0, 
			    (int) (scale * orig_width), 
			    (int) (scale * orig_height), 
			    cached_pix,
			    (width - (int) (scale * orig_width)) / 2, 
			    (height - (int)(scale * orig_height)) / 2);

      g_object_unref (G_OBJECT (scaled_pix));
    }
  }
  else { /* Moving logo */

    orig_width = gdk_pixbuf_get_width (orig_pix);
    orig_height = gdk_pixbuf_get_height (orig_pix);

    gdk_pixbuf_fill (cached_pix, 0xFFFFFFFF);
    gdk_pixbuf_copy_area (orig_pix, 
			  0, 0, 
			  orig_width, orig_height, 
			  cached_pix, 
			  (width - orig_width) / 2, 
			  pos);

    pos = pos + increment;

    if ((int) pos > (int) height - orig_height - 10) 
      increment = -1;
    if (pos < 10) 
      increment = +1;
  }

  data = gdk_pixbuf_get_pixels (cached_pix);

  if (converter)
    converter->Convert (data, frame);

  gnomemeeting_threads_leave ();

  return TRUE;
}


BOOL
PVideoInputDevice_Picture::TestAllFormats ()
{
  return TRUE;
}


PINDEX
PVideoInputDevice_Picture::GetMaxFrameBytes ()
{
  return CalculateFrameBytes (frameWidth, frameHeight, colourFormat);
}


void
PVideoInputDevice_Picture::WaitFinishPreviousFrame ()
{
  m_Pacing.Delay (1000 / GetFrameRate ());
}


BOOL
PVideoInputDevice_Picture::SetVideoFormat (VideoFormat newFormat)
{
  return PVideoDevice::SetVideoFormat (newFormat);
}


int
PVideoInputDevice_Picture::GetNumChannels()
{
  return 1;
}


BOOL
PVideoInputDevice_Picture::SetChannel (int newChannel)
{
  return TRUE;
}


BOOL
PVideoInputDevice_Picture::SetColourFormat (const PString &newFormat)
{
  if (newFormat == "RGB32") 
    return PVideoDevice::SetColourFormat (newFormat);

  return FALSE;  
}


BOOL
PVideoInputDevice_Picture::SetFrameRate (unsigned rate)
{
  if (moving)
    PVideoDevice::SetFrameRate (12);
  else
    PVideoDevice::SetFrameRate (1);
 
  return TRUE;
}


BOOL
PVideoInputDevice_Picture::GetFrameSizeLimits (unsigned & minWidth,
					       unsigned & minHeight,
					       unsigned & maxWidth,
					       unsigned & maxHeight)
{
  minWidth  = 10;
  minHeight = 10;
  maxWidth  = 1000;
  maxHeight =  800;

  return TRUE;
}


BOOL PVideoInputDevice_Picture::GetParameters (int *whiteness,
					       int *brightness,
					       int *colour,
					       int *contrast,
					       int *hue)
{
  *whiteness = 0;
  *brightness = 0;
  *colour = 0;
  *contrast = 0;
  *hue = 0;

  return TRUE;
}
