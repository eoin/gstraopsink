/*
 * GStreamer RAOP sink
 * Copyright (C) <2007> Stéphan Kochen <stephan@kochen.nl>
 * Copyright (C) <2007> Eoin Hennessy <eoin@randomrules.org>
 * 
 * Based on gst-plugin from the gst-template package:
 * Copyright 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_RAOPSINK_H__
#define __GST_RAOPSINK_H__

#include <gst/gst.h>
#include "rtsp.h"

G_BEGIN_DECLS

#define GST_TYPE_RAOPSINK (gst_raopsink_get_type())
#define GST_RAOPSINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_RAOPSINK, GstRaopSink))
#define GST_RAOPSINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_RAOPSINK, GstRaopSinkClass))
#define GST_IS_RAOPSINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_RAOPSINK))
#define GST_IS_RAOPSINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_RAOPSINK))

typedef struct _GstRaopSinkClass GstRaopSinkClass;
typedef struct _GstRaopSink GstRaopSink;

struct _GstRaopSinkClass 
{
    GstBinClass parent_class;
};

struct _GstRaopSink
{
    GstBin bin;
    
    GstPad * raopsinkpad;
    
    GstElement * alacfilter;
    GstPad * alacsrcpad;
    GstPad * alacsinkpad;
    
    GstElement * encfilter;
    GstPad * encsrcpad;
    GstPad * encsinkpad;
    
    GstElement * tcpsink;
    GstPad * tcpsinkpad;
    
    gchar * location;
    gchar * urlstr;
    RTSPUrl * url;
    
    gchar * local;
    
    gint8 volume;
    
    RTSPConnection * conn;
    gchar * session;
    
    gchar instance[17];
    gchar id[32];
    gchar *challenge;
    gulong challengelen;
    gchar *iv64buf;
    gulong iv64buflen;
    gchar *key64buf;
    gulong key64buflen;
    guchar iv[16];  // 4x4 bytes, 128 bits
    guchar key[16]; // 128 bits
};

GType gst_raopsink_get_type (void);

G_END_DECLS

#endif /* __GST_RAOPSINK_H__ */
