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

/**
 * SECTION:element-plugin
 *
 * <refsect2>
 * <title>Example launch line</title>
 * <para>
 * <programlisting>
 * gst-launch -v -m audiotestsrc ! raopsink address=10.0.1.4
 * </programlisting>
 * </para>
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <string.h>
//#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define REDEF_SHIT
#include <tomcrypt.h>
#include <tommath.h>
#include <glib/gprintf.h>
#include <gst/gst.h>

#include "gstraopsink.h"

#include "rtsp.h"

const guchar itunes_rsakey_mod[256] = {
	231, 215,  68, 242, 162, 226, 120, 139, 108,  31,  85, 160, 142, 183,   5,  68,
	168, 250, 121,  69, 170, 139, 230, 198,  44, 229, 245,  28, 189, 212, 220, 104,
	66, 254,  61,  16, 131, 221,  46, 222, 193, 191, 212,  37,  45, 192,  46, 111,
	57, 139, 223,  14,  97,  72, 234, 132, 133,  94,  46,  68,  45, 166, 214,  38,
	100, 246, 116, 161, 243,   4, 146, 154, 222,  79, 104, 147, 239,  45, 246, 231,
	17, 168, 199, 122,  13, 145, 201, 217, 128, 130,  46,  80, 209,  41,  34, 175,
	234,  64, 234, 159,  14,  20, 192, 247, 105,  56, 197, 243, 136,  47, 192,  50,
	61, 217, 254,  85,  21,  95,  81, 187,  89,  33, 194,   1,  98, 159, 215,  51,
	82, 213, 226, 239, 170, 191, 155, 160,  72, 215, 184,  19, 162, 182, 118, 127,
	108,  60, 207,  30, 180, 206, 103,  61,   3, 123,  13,  46, 163,  12,  95, 255,
	235,   6, 248, 208, 138, 221, 228,   9,  87,  26, 156, 104, 159, 239,  16, 114,
	136,  85, 221, 140, 251, 154, 139, 239,  92, 137,  67, 239,  59,  95, 170,  21,
	221, 230, 152, 190, 221, 243,  89, 150,   3, 235,  62, 111,  97,  55,  43, 182,
	40, 246,  85, 159,  89, 154, 120, 191,  80,   6, 135, 170, 127,  73, 118, 192,
	86,  45,  65,  41,  86, 248, 152, 158,  24, 166,  53,  91, 216,  21, 151, 130,
	94,  15, 200, 117,  52,  62, 199, 130,  17, 118,  37, 205, 191, 152,  68, 123,
};

const guchar itunes_rsakey_exp[3] = {
	1,   0,   1
};


GST_DEBUG_CATEGORY_STATIC (raopsink_debug);
#define GST_CAT_DEFAULT raopsink_debug

static const GstElementDetails element_details = GST_ELEMENT_DETAILS (
	"RAOP/Airport sink",
	"Generic/PluginTemplate",
	"A sink element for streaming with the RAOP protocol.",
	"Stéphan Kochen <stephan@kochen.nl>"
	);

/* Signals and args */
enum
{
	/* FILL ME */
	LAST_SIGNAL
};

enum
{
	PROP_0,
	PROP_LOCATION,
	PROP_KEY,
	PROP_IV
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

GST_BOILERPLATE (GstRaopSink, gst_raopsink, GstElement,
	GST_TYPE_ELEMENT);


/* Prototypes */

static void gst_raopsink_set_property (GObject * object, guint prop_id,
	const GValue * value, GParamSpec * pspec);
static void gst_raopsink_get_property (GObject * object, guint prop_id,
	GValue * value, GParamSpec * pspec);

static GstStateChangeReturn gst_raopsink_change_state (GstElement *element,
	GstStateChange transition);

static gboolean gst_raopsink_open (GstRaopSink * sink);
static gboolean gst_raopsink_create_random_data (GstRaopSink * sink);
static gboolean gst_raopsink_announce_stream (GstRaopSink * sink);
static gboolean gst_raopsink_setup_stream (GstRaopSink * sink);
static gboolean gst_raopsink_update_volume (GstRaopSink * sink);
static gboolean gst_raopsink_play (GstRaopSink * sink);
static gboolean gst_raopsink_pause (GstRaopSink * sink);
static void gst_raopsink_close (GstRaopSink * sink);


/* GObject construction */
static void
gst_raopsink_base_init (gpointer gclass)
{
	GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

	gst_element_class_add_pad_template (element_class,
		gst_static_pad_template_get (&sink_factory));

	gst_element_class_set_details (element_class, &element_details);
}


/* initialize the plugin's class */
static void
gst_raopsink_class_init (GstRaopSinkClass * klass)
{
	G_OBJECT_CLASS (klass)->set_property = gst_raopsink_set_property;
	G_OBJECT_CLASS (klass)->get_property = gst_raopsink_get_property;
	GST_ELEMENT_CLASS (klass)->change_state = gst_raopsink_change_state;

	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_LOCATION,
		g_param_spec_string ("location", "RAOP Location",
			"Location of the RAOP url to stream to", NULL, G_PARAM_READWRITE));
	
	GST_DEBUG_CATEGORY_INIT (raopsink_debug, "raopsink", 0, "raopsink");
}


/* initialize the new element
 * set functions
 * initialize structure
 */
static void
gst_raopsink_init (GstRaopSink * sink, GstRaopSinkClass * gclass)
{
	sink->tcpsink = NULL;
	sink->session = NULL;

	memset(sink->iv, 0, 16);
	memset(sink->key, 0, 16);

	sink->alacfilter = gst_element_factory_make("alacenc", NULL);
	g_assert(sink->alacfilter != NULL);
	gst_object_set_parent(GST_OBJECT(sink->alacfilter), GST_OBJECT(sink));

	sink->alacsinkpad = gst_element_get_pad(sink->alacfilter, "sink");
	g_assert(sink->alacsinkpad != NULL);
	sink->alacsrcpad = gst_element_get_pad(sink->alacfilter, "src");
	g_assert(sink->alacsrcpad != NULL);

	sink->encfilter = gst_element_factory_make("raopenc", NULL);
	g_assert(sink->encfilter != NULL);
	gst_object_set_parent(GST_OBJECT(sink->encfilter), GST_OBJECT(sink));

	sink->encsinkpad = gst_element_get_pad(sink->encfilter, "sink");
	g_assert(sink->encsinkpad != NULL);
	sink->encsrcpad = gst_element_get_pad (sink->encfilter, "src");
	g_assert (sink->encsrcpad != NULL);

	sink->raopsinkpad = gst_ghost_pad_new("sink", sink->alacsinkpad);
	g_assert(sink->raopsinkpad != NULL);
	g_assert(gst_element_add_pad (GST_ELEMENT(sink), sink->raopsinkpad));

	g_assert(gst_pad_link (sink->alacsrcpad, sink->encsinkpad) ==
		GST_PAD_LINK_OK);
}


/* GObject vmethods */
static void
gst_raopsink_set_property (GObject * object, guint prop_id,
	const GValue * value, GParamSpec * pspec)
{
	switch (prop_id) {
		case PROP_LOCATION:
			g_free (GST_RAOPSINK (object)->location);
			GST_RAOPSINK (object)->location = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}


static void
gst_raopsink_get_property (GObject * object, guint prop_id,
	GValue * value, GParamSpec * pspec)
{
	switch (prop_id) {
		case PROP_LOCATION:
			g_value_set_string (value, GST_RAOPSINK (object)->location);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}


/* GstElement vmethods */
static GstStateChangeReturn
gst_raopsink_change_state (GstElement * element, GstStateChange transition)
{
	GstRaopSink * raopsink = GST_RAOPSINK (element);
	GstStateChangeReturn retval;

	switch (transition) {
		case GST_STATE_CHANGE_READY_TO_PAUSED:
			if (gst_raopsink_open (raopsink) == FALSE)
				return GST_STATE_CHANGE_FAILURE;
			break;
		case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
			if (gst_raopsink_play (raopsink) == FALSE)
				return GST_STATE_CHANGE_FAILURE;
			break;
		default:
			break;
	}

	retval = GST_ELEMENT_CLASS (parent_class)->
		change_state (element, transition);
	if (retval == GST_STATE_CHANGE_FAILURE)
		return retval;

	switch (transition)
	{
		case GST_STATE_CHANGE_READY_TO_PAUSED:
			// FIXME: ??
			retval = GST_STATE_CHANGE_NO_PREROLL; 
			break;
		case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
			if (gst_raopsink_pause (raopsink) == FALSE)
				retval = GST_STATE_CHANGE_FAILURE;
			break;
		case GST_STATE_CHANGE_PAUSED_TO_READY:
			// No need to check for this, who cares?
			gst_raopsink_close (raopsink);
			break;
		default:
			break;
	}

	return retval;
}


/* Private methods */
static gboolean
gst_raopsink_open(GstRaopSink* sink)
{
	RTSPResult res;
	int retval;
	struct sockaddr_in localaddr;
	socklen_t localaddrlen = sizeof (struct sockaddr_in);
	gchar * local;

	if (gst_raopsink_create_random_data (sink) == FALSE)
		return FALSE;

	// Create the url from sink->location
	sink->urlstr = g_malloc ((strlen (sink->location) + 14)
		* sizeof(gchar));
	g_sprintf (sink->urlstr, "rtsp://%s:5000/", sink->location);

	// Parse URL
	res = rtsp_url_parse (sink->urlstr, &sink->url);
	if (res != RTSP_OK) {
		GST_ERROR ("error parsing url \"%s\"", sink->urlstr);
		return FALSE;
	}

	// Connect
	res = rtsp_connection_open (sink->url, &sink->conn);
	if (res != RTSP_OK) {
		GST_ERROR ("error opening connection to \"%s\"", sink->location);
		return FALSE;
	}
	GST_INFO("opened connection to %s", sink->location);

	// Get local address
	retval = getsockname (sink->conn->fd,
		(struct sockaddr *)&localaddr, &localaddrlen);
	if (retval != 0 || localaddr.sin_family != AF_INET)
	{
		GST_ERROR ("error getting local address: "
			"getsockname error %d.", errno); 
		return FALSE;
	}
	local = inet_ntoa (localaddr.sin_addr);
	if (local == NULL)
	{
		GST_ERROR ("error getting local address: "
			"inet_ntoa failed.");
		return FALSE;
	}
	sink->local = g_strdup (local);
	if (sink->local == NULL)
	{
		GST_ERROR ("error getting local address: "
			"g_strdup failed.");
		return FALSE;
	}

	sink->conn->cseq = 1;

	// Setup the stream
	if (gst_raopsink_announce_stream (sink) == FALSE)
		return FALSE;
	if (gst_raopsink_setup_stream (sink) == FALSE)
		return FALSE;
	if (gst_raopsink_update_volume (sink) == FALSE)
		return FALSE;

	return TRUE;
}


static inline int
base64_encode_nopad(const unsigned char *in,  unsigned long len, 
	unsigned char *out, unsigned long *outlen)
{
	int retval = CRYPT_OK;

	retval = base64_encode (in, len, out, outlen);
	if (retval != CRYPT_OK)
		return retval;

	unsigned char * padding = out + (*outlen - 1);
	if (*padding == '=')
	{
		for (padding--; *padding == '='; padding--);
		*(++padding) = '\0';
		*outlen = (unsigned long)(padding - out);
	}

	return CRYPT_OK;
}


static gboolean
gst_raopsink_create_random_data (GstRaopSink * sink)
{
	guchar randbuffer[60];  // 4 + 8 + 16 + 16 + 16
	gint prngid, hashid, retval;
	mp_int mod, exp;
	rsa_key key;
	guchar enckey[256];
	gulong enckeylen = 256;

	sink->challengelen = 64;
	sink->iv64buflen = 64;
	sink->key64buflen = 1024;

	/* Register a math library */
	ltc_mp = ltm_desc;

	/* Generate random data */
	retval = rng_get_bytes (randbuffer, 60, NULL);
	if (retval != 60)
	{
		GST_ERROR ("Could not get enough random data.");
		return FALSE;
	}
	memset(randbuffer, 0, 60);

	/* Create the client ID string */
	g_sprintf (sink->id, "%010u", *(guint32*) randbuffer);

	/* Create the client instance string */
	g_sprintf (sink->instance, "%08X%08X", *(guint32*)(randbuffer + 4),
			*(guint32*)(randbuffer + 8));

	/* Create challenge string */
	retval = base64_encode_nopad (randbuffer + 12, 16,
			sink->challenge, &sink->challengelen);
	if (retval != CRYPT_OK)
	{
		GST_ERROR ("Could not encode challenge: "
				"base64_encode_nopad error %d.", retval);
		return FALSE;
	}

	/* Init Rijndael IV */
	memcpy (sink->iv, randbuffer + 28, 16);
	retval = base64_encode_nopad (randbuffer + 28, 16,
			sink->iv64buf, &sink->iv64buflen);
	if (retval != CRYPT_OK)
	{
		GST_ERROR ("Could not encode initial IV: "
				"base64_encode_nopad error %d.", retval);
		return FALSE;
	}

	// Register PRNG for RSA encryption
	retval = register_prng (&sprng_desc);
	if (retval != CRYPT_OK)
	{
		GST_ERROR ("Could not register SPRNG: "
				"register_prng error %d.", retval);
		return FALSE;
	}
	prngid = find_prng ("sprng");
	if (prngid == -1)
	{
		GST_ERROR ("Could not initialise SPRNG: "
				"find_prng failed.");
		return FALSE;
	}

	// Register hash for RSA encryption
	retval = register_hash (&sha1_desc);
	if (retval != CRYPT_OK)
	{
		GST_ERROR ("Could not register SHA1 hash: "
				"register_hash error %d.", retval);
		return FALSE;
	}
	hashid = find_hash ("sha1");
	if (hashid == -1)
	{
		GST_ERROR ("Could not initialise SHA1 hash: "
				"find_hash failed.");
		return FALSE;
	}

	// Initialise the key struct
	retval = ltc_init_multi (&key.e, &key.d, &key.N, &key.dQ, 
			&key.dP, &key.qP, &key.p, &key.q, NULL);
	if (retval != CRYPT_OK) {
		GST_ERROR ("Could not initialise RSA key: "
				"ltc_init_multi %d.", retval);
		return retval;
	}
	retval = mp_init (&mod);
	if (retval != 0) 
	{
		GST_ERROR ("Could not initialise RSA key: "
				"mp_init(1) error %d.", retval);
		return retval;
	}
	retval = mp_init (&exp);
	if (retval != 0) 
	{
		GST_ERROR ("Could not initialise RSA key: "
				"mp_init(2) error %d.", retval);
		return retval;
	}
	retval = mp_read_unsigned_bin (&mod, itunes_rsakey_mod, 256);
	if (retval != 0)
	{
		GST_ERROR ("Could not initialise RSA key: "
				"mp_read_unsigned_bin(1) %d.", retval);
		return retval;
	}
	retval = mp_read_unsigned_bin (&exp, itunes_rsakey_exp, 3);
	if (retval != 0)
	{
		GST_ERROR ("Could not initialise RSA key: "
				"mp_read_unsigned_bin(2) %d.", retval);
		return retval;
	}
	key.type = PK_PUBLIC;
	key.N = &mod;
	key.e = &exp;

	/* Create Rijndael key */
	memcpy (sink->key, randbuffer + 44, 16);
	retval = rsa_encrypt_key (randbuffer + 44, 16,
			enckey, &enckeylen, NULL, 0, NULL, prngid, hashid, &key);
	if (retval != CRYPT_OK)
	{
		GST_ERROR ("Could not encrypt Rijndael key: "
				"rsa_encrypt_key error %d.", retval);
		return FALSE;
	}

	unregister_hash (&sha1_desc);
	unregister_prng (&sprng_desc);
	mp_clear (&mod);
	mp_clear (&exp);

	retval = base64_encode_nopad (enckey, enckeylen,
			sink->key64buf, &sink->key64buflen);
	if (retval != CRYPT_OK)
	{
		GST_ERROR ("Could not encode Rijndael key: "
				"base64_encode_nopad error %d.", retval);
		return FALSE;
	}

	return TRUE;
}


static gboolean
gst_raopsink_announce_stream (GstRaopSink * sink)
{
	RTSPResult res;
	RTSPMessage request = { 0 }, response = { 0 };
	gchar body[2048];

	sink->urlstr = g_malloc ((strlen (sink->local) + sizeof(sink->id) + 9)
			* sizeof(gchar));
	g_sprintf (sink->urlstr, "rtsp://%s/%s", sink->local, sink->id);

	/* Setup the announce request */
	res = rtsp_message_init_request ( 
			RTSP_ANNOUNCE, sink->urlstr, &request);
	if (res != RTSP_OK) {
		GST_ERROR ("error creating request.");
		return FALSE;
	}

	rtsp_message_add_header (&request, RTSP_HDR_CONTENT_TYPE,
			"application/sdp");
	rtsp_message_add_header (&request, RTSP_HDR_USER_AGENT,
			"iTunes/4.6 (Macintosh; U; PPC Mac OS X 10.3)");
	rtsp_message_add_header (&request, RTSP_HDR_CLIENT_INSTANCE,
			sink->instance);
	rtsp_message_add_header (&request, RTSP_HDR_APPLE_CHALLENGE,
			(gchar *) sink->challenge);

	g_sprintf (body,
		"v=0\r\n"
		"o=iTunes %s 0 IN IP4 %s\r\n"
		"s=iTunes\r\n"
		"c=IN IP4 %s\r\n"
		"t=0 0\r\n"
		"m=audio 0 RTP/AVP 96\r\n"
		"a=rtpmap:96 AppleLossless\r\n"
		"a=fmtp:96 4096 0 16 40 10 14 2 255 0 0 44100\r\n"
		"a=rsaaeskey:%s\r\n"
		"a=aesiv:%s\r\n",
		sink->id, sink->local, sink->url->host,
		sink->key64buf, sink->iv64buf);
	rtsp_message_set_body (&request, (guchar *) body, strlen (body));

	res = rtsp_connection_send (sink->conn, &request);
	if (res != RTSP_OK) {
		GST_ERROR ("error sending request");
		return FALSE;
	}

	res = rtsp_connection_receive (sink->conn, &response);
	if (res != RTSP_OK) {
		GST_ERROR ("error receiving response");
		return FALSE;
	}

	return TRUE;
}


static gboolean
gst_raopsink_setup_stream (GstRaopSink * sink)
{
	RTSPResult res;
	RTSPMessage request = { 0 }, response = { 0 };
	GValue hostprop = {0,}, portprop = {0,}, syncprop = {0,};
	gchar * session, * transport, * port;
	gchar ** splittransport;
	gchar * key, * value;
	guint i;

	res = rtsp_message_init_request ( 
		RTSP_SETUP, sink->urlstr, &request);
	if (res != RTSP_OK) {
		GST_ERROR ("error creating request.");
		return FALSE;
	}

	rtsp_message_add_header (&request, RTSP_HDR_USER_AGENT,
		"iTunes/4.6 (Macintosh; U; PPC Mac OS X 10.3)");
	rtsp_message_add_header (&request, RTSP_HDR_CLIENT_INSTANCE,
		sink->instance);
	rtsp_message_add_header (&request, RTSP_HDR_TRANSPORT,
		"RTP/AVP/TCP;unicast;interleaved=0-1;mode=record");

	res = rtsp_connection_send (sink->conn, &request);
	if (res != RTSP_OK) {
		GST_ERROR ("error sending request");
		return FALSE;
	}

	res = rtsp_connection_receive (sink->conn, &response);
	if (res != RTSP_OK) {
		GST_ERROR ("error receiving response");
		return FALSE;
	}

	res = rtsp_message_get_header (&response, RTSP_HDR_SESSION, &session);
	if (res != RTSP_OK) {
		GST_ERROR ("no session in response");
		return FALSE;
	}
	res = rtsp_message_get_header (&response, RTSP_HDR_TRANSPORT,
			&transport);
	if (res != RTSP_OK) {
		GST_ERROR ("no transport in response");
		return FALSE;
	}
	GST_INFO("transport: %s", transport);

	// Needed for all future commands
	sink->session = g_strdup (session);

	// FIXME: Audio-Jack-Status

	// Find the server port
	splittransport = g_strsplit_set (transport, ";=", 0);
	port = NULL;
	for (i = 0; splittransport[i] != NULL; i++)
	{
		key = splittransport[i];
		if (splittransport[++i] == NULL)
			break;
		value = splittransport[i];

		if (strcmp (key, "server_port") == 0)
			port = value; 
	}
	if (port == NULL) {
		GST_ERROR ("no server port in response");
		return FALSE;
	}

	// Create the TCP sink
	sink->tcpsink = gst_element_factory_make ("tcpclientsink", NULL);
	if (sink->tcpsink == NULL)
	{
		GST_ERROR ("failed to create TCP sink element");
		return FALSE;
	}
	
	gst_object_set_parent (GST_OBJECT (sink->tcpsink), GST_OBJECT (sink));

	// Set the host property on the TCP sink
	g_value_init (&hostprop, G_TYPE_STRING);
	g_value_set_string (&hostprop, sink->url->host);
	g_object_set_property (G_OBJECT (sink->tcpsink), "host", &hostprop);

	// Set the port property
	g_value_init (&portprop, G_TYPE_INT);
	g_value_set_int (&portprop, atoi (port));
	g_object_set_property (G_OBJECT (sink->tcpsink), "port", &portprop);

	// Set the port property
	g_value_init (&syncprop, G_TYPE_BOOLEAN);
	g_value_set_boolean (&syncprop, TRUE);
	g_object_set_property (G_OBJECT (sink->tcpsink), "sync", &syncprop);

	// No longer need the split array
	g_strfreev (splittransport);

	sink->tcpsinkpad = gst_element_get_pad(sink->tcpsink, "sink");
	g_assert (sink->tcpsinkpad != NULL);
	
	// Link the encsrc to tcpsink
	if (gst_pad_link(sink->encsrcpad, sink->tcpsinkpad) != GST_PAD_LINK_OK) {
		GST_ERROR ("failed linking encryption element to tcp element");
		return FALSE;
	}

	// Activate child elements
	if (gst_element_set_state(GST_ELEMENT (sink->alacfilter), GST_STATE_PLAYING)
			== GST_STATE_CHANGE_FAILURE) {
		GST_ERROR ("failed to set ALAC encoder element to PLAYING state");
		return FALSE;
	}

	if (gst_element_set_state(GST_ELEMENT (sink->encfilter), GST_STATE_PLAYING)
			== GST_STATE_CHANGE_FAILURE) {
		GST_ERROR ("failed to set encryption element to PLAYING state");
		return FALSE;
	}
	
	if (gst_element_set_state(GST_ELEMENT (sink->tcpsink), GST_STATE_PLAYING)
			== GST_STATE_CHANGE_FAILURE) {
		GST_ERROR ("failed to set tcp element to PLAYING state");
		return FALSE;
	}
	
	
	return TRUE;
}


static gboolean
gst_raopsink_play(GstRaopSink * sink)
{
	RTSPResult res;
	RTSPMessage request = { 0 }, response = { 0 };

	g_assert (sink->session != NULL);

	res = rtsp_message_init_request ( 
		RTSP_RECORD, sink->urlstr, &request);
	if (res != RTSP_OK) {
		GST_ERROR ("error creating request.");
		return FALSE;
	}

	rtsp_message_add_header (&request, RTSP_HDR_USER_AGENT,
		"iTunes/4.6 (Macintosh; U; PPC Mac OS X 10.3)");
	rtsp_message_add_header (&request, RTSP_HDR_CLIENT_INSTANCE,
		sink->instance);
	rtsp_message_add_header (&request, RTSP_HDR_SESSION,
		sink->session);
	rtsp_message_add_header (&request, RTSP_HDR_RANGE,
		"npt=0-");
	rtsp_message_add_header (&request, RTSP_HDR_RTP_INFO,
		"seq=0;rtptime=0");

	res = rtsp_connection_send (sink->conn, &request);
	if (res != RTSP_OK) {
		GST_ERROR ("error sending request");
		return FALSE;
	}

	res = rtsp_connection_receive (sink->conn, &response);
	if (res != RTSP_OK) {
		GST_ERROR ("error receiving response");
		return FALSE;
	}
	
	return TRUE;
}


static gboolean
gst_raopsink_pause (GstRaopSink * sink)
{
	RTSPResult res;
	RTSPMessage request = { 0 }, response = { 0 };

	g_assert (sink->session != NULL);

	res = rtsp_message_init_request( 
			RTSP_PAUSE, sink->urlstr, &request);
	if (res != RTSP_OK) {
		GST_ERROR ("error creating request.");
		return FALSE;
	}

	rtsp_message_add_header(&request, RTSP_HDR_USER_AGENT,
		"iTunes/4.6 (Macintosh; U; PPC Mac OS X 10.3)");
	rtsp_message_add_header(&request, RTSP_HDR_CLIENT_INSTANCE,
		sink->instance);
	rtsp_message_add_header(&request, RTSP_HDR_SESSION,
		sink->session);
	rtsp_message_add_header(&request, RTSP_HDR_RANGE,
		"npt=0-");

	res = rtsp_connection_send(sink->conn, &request);
	if (res != RTSP_OK) {
		GST_ERROR ("error sending request");
		return FALSE;
	}

	res = rtsp_connection_receive(sink->conn, &response);
	if (res != RTSP_OK) {
		GST_ERROR ("error receiving response");
		return FALSE;
	}

	return TRUE;
}


static gboolean
gst_raopsink_update_volume (GstRaopSink * sink)
{
	RTSPResult res;
	RTSPMessage request = { 0 }, response = { 0 };

	gchar body[20];
	gsize bodylen;

	/* There's nothing to update, that's no problem. */
	if (sink->session == NULL)
		return TRUE;

	res = rtsp_message_init_request ( 
		RTSP_SET_PARAMETER, sink->urlstr, &request);
	if (res != RTSP_OK) {
		GST_ERROR ("error creating request.");
		return FALSE;
	}

	rtsp_message_add_header (&request, RTSP_HDR_USER_AGENT,
		"iTunes/4.6 (Macintosh; U; PPC Mac OS X 10.3)");
	rtsp_message_add_header (&request, RTSP_HDR_CLIENT_INSTANCE,
		sink->instance);
	rtsp_message_add_header (&request, RTSP_HDR_SESSION,
		sink->session);

	sink->volume = -30;
	g_sprintf (body, "volume: %f\r\n", (gfloat) sink->volume);
	bodylen = strlen (body);
	rtsp_message_set_body (&request, (guchar *) body, bodylen);

	res = rtsp_connection_send (sink->conn, &request);
	if (res != RTSP_OK) {
		GST_ERROR ("error sending request");
		return FALSE;
	}

	res = rtsp_connection_receive (sink->conn, &response);
	if (res != RTSP_OK) {
		GST_ERROR ("error receiving response");
		return FALSE;
	}

	return TRUE;
}


static void
gst_raopsink_close (GstRaopSink * sink)
{    
	/* FIXME: Teardown */

	g_assert (sink->session != NULL);
	g_free (sink->session);
	sink->session = NULL;

	rtsp_connection_close (sink->conn);
	rtsp_connection_free (sink->conn);
	sink->conn = NULL;
}

