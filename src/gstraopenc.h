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

#ifndef __GST_RAOPENC_H__
#define __GST_RAOPENC_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <openssl/evp.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>


G_BEGIN_DECLS

#define GST_TYPE_RAOPENC (gst_raopenc_get_type())
#define GST_RAOPENC(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_RAOPENC, GstRaopEnc))
#define GST_RAOPENC_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_RAOPENC, GstRaopEncClass))
#define GST_IS_RAOPENC(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_RAOPENC))
#define GST_IS_RAOPENC_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_RAOPENC))

typedef struct _GstRaopEncClass GstRaopEncClass;
typedef struct _GstRaopEnc GstRaopEnc;

struct _GstRaopEncClass 
{
	GstBaseTransformClass parent_class;
};

struct _GstRaopEnc
{
	GstBaseTransform basetransform;

	GstPad * srcpad;
	GstPad * sinkpad;
};

GType gst_raopenc_get_type (void);

G_END_DECLS

#endif /* __GST_RAOPENC_H__ */
