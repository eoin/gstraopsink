/*
 * GStreamer RAOP sink
 * Copyright (C) <2007> Stéphan Kochen <stephan@kochen.nl>
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

#include <string.h>
#include <glib.h>

#include "bitwriter.h"

#include "gstalacenc.h"

const guint8 masks[8] = {
    0x01, 0x03, 0x07, 0x0F,
    0x1F, 0x3F, 0x7F, 0xFF
    };


BitWriter * bitwriter_new (guint8 * buffer)
{
    BitWriter * retval = g_malloc (sizeof (BitWriter));
    if (retval == NULL)
        return NULL;
    
    bitwriter_reset (retval, buffer);
    
    return retval;
}


void bitwriter_destroy (BitWriter * bb)
{
    g_free (bb);
}


// Adapted from JustePort
void bitwriter_write (BitWriter * bb, guint8 data, gsize numbits)
{
    guint8 bitstowrite;

    if (bb->bitOffset != 0 && bb->bitOffset + numbits > 8)
    {
        int numwritebits = 8 - bb->bitOffset;
        bitstowrite = ((data >> (numbits - numwritebits)) <<
                       (8 - bb->bitOffset - numwritebits));
        *bb->buffer |= bitstowrite;
        numbits -= numwritebits;
        bb->bitOffset = 0;
        bb->buffer++;
		bb->byteOffset++;
    }
    
    while (numbits >= 8)
    {
        bitstowrite = ((data >> (numbits - 8)) & 0xFF);
        *bb->buffer |= bitstowrite;
        numbits -= 8;
        bb->bitOffset = 0;
        bb->buffer++;
		bb->byteOffset++;
    }
    
    if (numbits > 0)
    {
        bitstowrite = ((data & masks[numbits]) <<
                       (8 - bb->bitOffset - numbits));
        *bb->buffer |= bitstowrite;
        bb->bitOffset += numbits;
        if (bb->bitOffset == 8)
        {
            bb->buffer++;
			bb->byteOffset++;
            bb->bitOffset = 0;
        }
    }
}


void bitwriter_reset (BitWriter * bb, guint8 * buffer)
{
    bb->buffer = buffer;
    bb->bitOffset = 0;
}
