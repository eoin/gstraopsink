/*
 * GStreamer RAOP sink
 * Copyright (C) <2007> Stéphan Kochen <stephan@kochen.nl>
 * Copyright (C) <2007> Eoin Hennessy <eoin@randomrules.org>
 * 
 * Based on the GStreamer .wav encoder
 * Copyright (C) <2002> Iain Holmes <iain@prettypeople.org>
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


#ifndef __GST_ALACENC_H__
#define __GST_ALACENC_H__

#include <gst/gst.h>

#include "bitwriter.h"

G_BEGIN_DECLS

#define GST_TYPE_ALACENC (gst_alacenc_get_type())
#define GST_ALACENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ALACENC,GstAlacEnc))
#define GST_ALACENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ALACENC,GstAlacEncClass))
#define GST_IS_ALACENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ALACENC))
#define GST_IS_ALACENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ALACENC))

typedef struct _GstAlacEncClass GstAlacEncClass;
typedef struct _GstAlacEnc GstAlacEnc;

struct _GstAlacEncClass {
	GstElementClass parent_class;
};

struct _GstAlacEnc {
	GstElement element;

	GstPad *sinkpad, *srcpad;

	GstBuffer *buffer;
	BitWriter *writer;
	gsize remaining;

	gboolean streaming;
};

GType gst_alacenc_get_type (void);

G_END_DECLS

#endif /* __GST_ALACENC_H__ */
