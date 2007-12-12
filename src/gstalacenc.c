/*
 * GStreamer RAOP sink
 * Copyright (C) <2007> Stéphan Kochen <stephan@kochen.nl>
 * Copyright (C) <2007> Eoin Hennessy <eoin@randomrules.org>
 * 
 * Based on the GStreamer .wav encoder
 * Copyright (C) <2002> Iain Holmes <iain@prettypeople.org>
 * Copyright (C) <2006> Tim-Philipp Müller <tim centricular net>
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
#include "config.h"
#endif

#include <string.h>

#include "gstalacenc.h"

GST_DEBUG_CATEGORY_STATIC (alacenc_debug);
#define GST_CAT_DEFAULT alacenc_debug

// 4096 samples per frame, stereo, 16-bit.
#define ALAC_BUFFER_SIZE (4096*2*2)

static const GstElementDetails gst_alacenc_details =
GST_ELEMENT_DETAILS ("ALAC encoder",
	"Codec/Muxer/Audio",
	"Encode raw audio into ALAC",
	"Stéphan Kochen <stephan@kochen.nl>");

enum
{
	PROP_0,
	PROP_ENCODE
};

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
	GST_PAD_SINK,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS (
		"audio/x-raw-int, "
		"width = (int) 16, "
		"depth = (int) 16, "
		"endianness = (int) LITTLE_ENDIAN, "
		"channels = (int) 2, "
		"rate = (int) 44100"
		)
	);

// FIXME
static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS ("ANY")
	);

GST_BOILERPLATE (GstAlacEnc, gst_alacenc, GstElement, GST_TYPE_ELEMENT);

static void gst_alacenc_set_property (GObject * object, guint prop_id,
	const GValue * value, GParamSpec * pspec);
static void gst_alacenc_get_property (GObject * object, guint prop_id,
	GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_alacenc_chain (GstPad * pad, GstBuffer * buf);
static gboolean gst_alacenc_event (GstPad * pad, GstEvent * event);
static GstStateChangeReturn gst_alacenc_change_state (GstElement * element,
		GstStateChange transition);


static void
gst_alacenc_base_init (gpointer g_class)
{
	GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

	gst_element_class_set_details (element_class, &gst_alacenc_details);

	gst_element_class_add_pad_template (element_class,
			gst_static_pad_template_get (&src_factory));
	gst_element_class_add_pad_template (element_class,
			gst_static_pad_template_get (&sink_factory));
}


static void
gst_alacenc_class_init (GstAlacEncClass * klass)
{
	G_OBJECT_CLASS (klass)->set_property = gst_alacenc_set_property;
	G_OBJECT_CLASS (klass)->get_property = gst_alacenc_get_property;
	
	GST_ELEMENT_CLASS(klass)->change_state = gst_alacenc_change_state;

	GST_DEBUG_CATEGORY_INIT(alacenc_debug, "alacenc", 0, "alacenc");
}

static void
gst_alacenc_set_property (GObject * object, guint prop_id,
	const GValue * value, GParamSpec * pspec)
{
	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}


static void
gst_alacenc_get_property (GObject * object, guint prop_id,
	GValue * value, GParamSpec * pspec)
{
	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}


static void
gst_alacenc_init(GstAlacEnc *alacenc, GstAlacEncClass *klass)
{
	alacenc->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
	gst_pad_set_chain_function(alacenc->sinkpad, GST_DEBUG_FUNCPTR(gst_alacenc_chain));
	gst_pad_set_event_function(alacenc->sinkpad, GST_DEBUG_FUNCPTR(gst_alacenc_event));
	gst_element_add_pad(GST_ELEMENT (alacenc), alacenc->sinkpad);

	alacenc->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
	gst_element_add_pad(GST_ELEMENT (alacenc), alacenc->srcpad);
}


static void
gst_alacenc_write_header (GstAlacEnc * alacenc)
{
	// write the header
	bitwriter_write(alacenc->writer, 1, 3);  // channels: 0 mono, 1 stereo
	bitwriter_write(alacenc->writer, 0, 4);  // unknown
	bitwriter_write(alacenc->writer, 0, 12); // unknown
	bitwriter_write(alacenc->writer, 0, 1);  // 'has size' flag
	bitwriter_write(alacenc->writer, 0, 2);  // unknown
	bitwriter_write(alacenc->writer, 1, 1);  // 'no compression' flag
}


static GstFlowReturn
gst_alacenc_flush(GstAlacEnc *alacenc)
{
	GstFlowReturn flow = GST_FLOW_OK;

	// if the buffer is not empty, flush
	if (G_LIKELY(alacenc->remaining == ALAC_BUFFER_SIZE)) {
		GST_INFO("Flush requested with empty buffer");
		return flow;
	}

	flow = gst_pad_push(alacenc->srcpad, alacenc->buffer);
	if (!GST_FLOW_IS_SUCCESS(flow)) {
		GST_INFO("Failed to push buffer to srcpad");
		return flow;
	}

	alacenc->buffer = gst_buffer_new_and_alloc(ALAC_BUFFER_SIZE + 3);
	if (alacenc->buffer == NULL) {
		GST_ERROR ("Could not allocate buffer");
		return GST_FLOW_ERROR;
	}

	// clear buffer and write header
	memset(GST_BUFFER_DATA(alacenc->buffer), 0, GST_BUFFER_SIZE(alacenc->buffer));
	bitwriter_reset(alacenc->writer, GST_BUFFER_DATA(alacenc->buffer));
	gst_alacenc_write_header(alacenc);
	alacenc->remaining = ALAC_BUFFER_SIZE;

	return flow;
}


static gboolean
gst_alacenc_event (GstPad *pad, GstEvent *event)
{
	gboolean res = TRUE;
	GstAlacEnc *alacenc;
	gboolean update;

	alacenc = GST_ALACENC (gst_pad_get_parent (pad));

	switch (GST_EVENT_TYPE (event)) {
		case GST_EVENT_NEWSEGMENT:
			gst_event_parse_new_segment (event, &update,
				NULL, NULL, NULL, NULL, NULL);

			// ignore updates
			if (G_UNLIKELY (update))
				break;

			alacenc->streaming = TRUE;

			res = gst_pad_push_event (alacenc->srcpad, event);
			break;

		case GST_EVENT_EOS:
			if (!GST_FLOW_IS_SUCCESS (gst_alacenc_flush (alacenc))) {
				res = FALSE;
				break;
			}
			alacenc->streaming = FALSE;
			res = gst_pad_event_default (pad, event);
			break;

		default:
			res = gst_pad_event_default (pad, event);
			break;
	}

	return res;
}


static GstFlowReturn
gst_alacenc_chain (GstPad * pad, GstBuffer * buf)
{
	GstAlacEnc *alacenc = GST_ALACENC (GST_PAD_PARENT (pad));
	GstFlowReturn flow = GST_FLOW_OK;

	guint8 *data = GST_BUFFER_DATA (buf);
	gsize offset, remaining, towrite, i, end;

	if (G_UNLIKELY(alacenc->streaming == FALSE))
		return GST_FLOW_UNEXPECTED;

	offset = 0;
	while (offset < GST_BUFFER_SIZE(buf)) {
		remaining = GST_BUFFER_SIZE(buf) - offset;
		towrite = MIN(remaining, alacenc->remaining);
		end = offset + towrite;
		for (i=offset; i<end; i+=2) {
			// swap endianness
			bitwriter_write(alacenc->writer, data[i+1], 8);
			bitwriter_write(alacenc->writer, data[i], 8);
		}
		alacenc->remaining -= towrite;
		offset = end;

		if (alacenc->remaining == 0) {
			flow = gst_alacenc_flush(alacenc);
			if (!GST_FLOW_IS_SUCCESS(flow))
				break;
		}
	}

	//flow = gst_pad_push(alacenc->srcpad, buf);
	return flow;
}


static GstStateChangeReturn
gst_alacenc_change_state (GstElement * element, GstStateChange transition)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	GstAlacEnc *alacenc = GST_ALACENC (element);

	switch (transition) {
		case GST_STATE_CHANGE_NULL_TO_READY:
			// 3 bytes for the header
			alacenc->buffer = gst_buffer_new_and_alloc (ALAC_BUFFER_SIZE + 3);
			if (alacenc->buffer == NULL)
				return GST_STATE_CHANGE_FAILURE;
			
			// clear buffer and write header
			memset(GST_BUFFER_DATA(alacenc->buffer), 0, GST_BUFFER_SIZE(alacenc->buffer));

			alacenc->writer = bitwriter_new (GST_BUFFER_DATA (alacenc->buffer));
			if (alacenc->writer == NULL)
				return GST_STATE_CHANGE_FAILURE;

			gst_alacenc_write_header (alacenc);

			alacenc->remaining = ALAC_BUFFER_SIZE;
			alacenc->streaming = FALSE;
			break;

		default:
			break;
	}

	ret = parent_class->change_state (element, transition);
	if (ret == GST_STATE_CHANGE_FAILURE)
		return ret;

	switch (transition) {
		case GST_STATE_CHANGE_READY_TO_NULL:
			bitwriter_destroy (alacenc->writer);
			gst_buffer_unref (alacenc->buffer);
			break;
		default:
			break;
	}

	return ret;
}

