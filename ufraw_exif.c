/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_exif.c - read the EXIF data from the RAW file.
 * Copyright 2004-2007 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation. You should have received
 * a copy of the license along with this program.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include "ufraw.h"

#ifdef HAVE_EXIV2
int ufraw_exif_from_exiv2(ufraw_data *uf);
#endif

int ufraw_exif_from_raw(ufraw_data *uf)
{
    uf->exifBuf = NULL;
    uf->exifBufLen = 0;
#ifdef HAVE_EXIV2
    int status = ufraw_exif_from_exiv2(uf);
    if (status==UFRAW_SUCCESS)
	return status;
    return status;
#endif
    uf = uf;
    ufraw_message(UFRAW_SET_LOG, "ufraw built without EXIF support\n");
    return UFRAW_ERROR;
}
