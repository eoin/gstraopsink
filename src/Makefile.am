# plugindir is set in configure

plugin_LTLIBRARIES = libgstraopsink.la

# sources used to compile this plug-in
libgstraopsink_la_SOURCES = gstraop.c gstraopenc.c gstraopsink.c gstalacenc.c bitwriter.c rtspconnection.c rtspdefs.c rtspmessage.c rtsptransport.c rtspurl.c

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstraopsink_la_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) $(GST_CONTROLLER_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS) $(OPENSSL_CFLAGS)
libgstraopsink_la_LIBADD = $(GST_LIBS) $(GST_BASE_LIBS) $(GST_CONTROLLER_LIBS) $(GST_PLUGINS_BASE_LIBS) $(OPENSSL_LIBS)
libgstraopsink_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstraopsink.h rtsp.h rtspconnection.h rtspdefs.h rtspmessage.h rtsptransport.h rtspurl.h
