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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <tomcrypt.h>
#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

#include "gstraopenc.h"
#include "gstraopsink.h"

#include "rtsp.h"


/* Signals and args */
enum
{
	/* FILL ME */
	LAST_SIGNAL
};

enum
{
	PROP_0
};

GST_DEBUG_CATEGORY_STATIC(raopenc_debug);
#define GST_CAT_DEFAULT raopenc_debug

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS ("ANY")
	);

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
	GST_PAD_SINK,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS ("ANY")
	);

GST_BOILERPLATE (GstRaopEnc, gst_raopenc, GstBaseTransform,
	GST_TYPE_BASE_TRANSFORM);


/* Prototypes */
static void gst_raopenc_set_property (GObject * object, guint prop_id,
	const GValue * value, GParamSpec * pspec);
static void gst_raopenc_get_property (GObject * object, guint prop_id,
	GValue * value, GParamSpec * pspec);

static gboolean gst_raopenc_start (GstBaseTransform * filter);
static gboolean gst_raopenc_stop (GstBaseTransform * filter);
static gboolean gst_raopenc_transform_size (GstBaseTransform * trans,
	GstPadDirection direction, GstCaps * caps, guint size, GstCaps * othercaps,
	guint * othersize);
static GstFlowReturn gst_raopenc_transform (GstBaseTransform * filter,
	GstBuffer * inbuf, GstBuffer * outbuf);


/* GObject construction */
static void
gst_raopenc_base_init (gpointer gclass)
{
	static GstElementDetails element_details = {
		"RAOP/Airport encryption filter",
		"Generic/PluginTemplate",
		"A filter element for RAOP Rijndael encryption.",
		"Stéphan Kochen <stephan@kochen.nl>"
	};
	GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

	gst_element_class_add_pad_template (element_class,
			gst_static_pad_template_get (&src_factory));
	gst_element_class_add_pad_template (element_class,
			gst_static_pad_template_get (&sink_factory));
	gst_element_class_set_details (element_class, &element_details);
}


/* initialize the plugin's class */
static void
gst_raopenc_class_init (GstRaopEncClass * klass)
{
	G_OBJECT_CLASS (klass)->set_property = gst_raopenc_set_property;
	G_OBJECT_CLASS (klass)->get_property = gst_raopenc_get_property;
	GST_BASE_TRANSFORM_CLASS (klass)->start = gst_raopenc_start;
	GST_BASE_TRANSFORM_CLASS (klass)->transform_size = gst_raopenc_transform_size;
	GST_BASE_TRANSFORM_CLASS (klass)->transform = gst_raopenc_transform;
	GST_BASE_TRANSFORM_CLASS (klass)->stop = gst_raopenc_stop;
	
	GST_DEBUG_CATEGORY_INIT (raopenc_debug, "raopenc", 0, "raopenc");
}


/* initialize the new element
 * instantiate pads and add them to element
 * set functions
 * initialize structure
 */
static void
gst_raopenc_init (GstRaopEnc * filter, GstRaopEncClass * gclass)
{

}


/* GObject vmethods */
static void
gst_raopenc_set_property (GObject * object, guint prop_id,
	const GValue * value, GParamSpec * pspec)
{
	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}


static void
gst_raopenc_get_property (GObject * object, guint prop_id,
	GValue * value, GParamSpec * pspec)
{
	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}


/* GstBaseTransform vmethods */
static gboolean
gst_raopenc_transform_size (GstBaseTransform * trans, GstPadDirection direction,
	GstCaps * caps, guint size, GstCaps * othercaps, guint * othersize)
{
	if (direction == GST_PAD_SRC)
		*othersize = size;
	if (direction == GST_PAD_SINK)
		*othersize = (size + 16);
	else
		return FALSE;
	return TRUE;
}


static gboolean
gst_raopenc_start (GstBaseTransform * basetransform)
{
	GstRaopEnc * filter = GST_RAOPENC (basetransform);
	int retval;

	retval = register_cipher(&rijndael_enc_desc);
	if (retval != CRYPT_OK) {
		GST_ERROR ("Could not register Rijndael cipher: "
			"register_cipher error %d.", retval);
		return FALSE;
	}

	filter->cipherid = find_cipher("rijndael");
	if (filter->cipherid == -1) {
		GST_ERROR ("Could not initialise Rijndael cipher: "
			"find_cipher failed.");
		return FALSE;
	}

	gst_base_transform_set_in_place (basetransform, 0);
	gst_base_transform_set_passthrough (basetransform, 0);

	return TRUE;
}


static gboolean
gst_raopenc_stop (GstBaseTransform * basetransform)
{
	unregister_cipher(&rijndael_enc_desc);

	return TRUE;
}


static GstFlowReturn
gst_raopenc_transform (GstBaseTransform *basetransform, GstBuffer *inbuf,
	GstBuffer *outbuf)
{
	int retval;
	GstRaopEnc *filter = GST_RAOPENC(basetransform);
	GstRaopSink * sink = GST_RAOPSINK (gst_element_get_parent (filter));
	guchar header[16];
	int enc_size = (GST_BUFFER_SIZE(inbuf) / 16) * 16;

	// create header
	memset(header, 0, 16);
	header[0] = 0x24;
	header[4] = 0xF0;
	header[5] = 0xFF;
	(*(guint16 *)(header + 2)) = GUINT16_TO_BE (GST_BUFFER_SIZE (inbuf) + 12);

	// header is not encrypted
	memcpy(GST_BUFFER_DATA(outbuf), header,  16);

	// this may not be necessary
//	memcpy(GST_BUFFER_DATA(outbuf) + 16, GST_BUFFER_DATA(inbuf), GST_BUFFER_SIZE(inbuf));

	retval = cbc_start(filter->cipherid, sink->iv, sink->key, 16, 0, &filter->cbc);
	if (retval != CRYPT_OK) {
		GST_ERROR ("Could not initialise Rijndael cipher: "
			"cbc_start error %d.", retval);
		return GST_FLOW_ERROR;
	}

	// encrypt
	cbc_encrypt(GST_BUFFER_DATA (inbuf), GST_BUFFER_DATA (outbuf) + 16,
		enc_size, &filter->cbc);

	cbc_done(&filter->cbc);

	return GST_FLOW_OK;
}

