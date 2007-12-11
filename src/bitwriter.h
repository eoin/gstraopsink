/*
 * GStreamer RAOP sink
 * Copyright (C) <2007> Stéphan Kochen <stephan@kochen.nl>
 * Copyright (C) <2007> Eoin Hennessy <eoin@randomrules.org>
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

#ifndef __BITBUFFER_H__
#define __BITBUFFER_H__

#include <glib.h>


typedef struct _BitWriter BitWriter;

struct _BitWriter {
    guint8 * buffer;
    guint8 bitOffset;    
    guint byteOffset;    
};

BitWriter * bitwriter_new (guint8 * buffer);
void bitwriter_destroy (BitWriter * bb);
void bitwriter_write (BitWriter * bb, guint8 data, gsize numbits);
void bitwriter_reset (BitWriter * bb, guint8 * buffer);

#endif // __BITBUFFER_H__
