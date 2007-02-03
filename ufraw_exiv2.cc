/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by Udi Fuchs
 *
 * ufraw_exiv2.cc - read the EXIF data from the RAW file using exiv2.
 *
 * Based on a sample program from exiv2 and neftags2jpg.
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses DCRaw code to do the actual raw decoding.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_EXIV2
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <sstream>
#include <cassert>
extern "C" {
#include <glib.h>
#include "ufraw.h"
}

// Exiv2 versions before 0.10 didn't have this file and the macros
#ifndef EXIV2_CHECK_VERSION
#define EXIV2_CHECK_VERSION(a,b,c) (false)
#endif

extern "C" int ufraw_exif_from_exiv2(ufraw_data *uf)
{
    /* Redirect exiv2 errors to a string buffer */
    std::ostringstream stderror;
    std::streambuf *savecerr = std::cerr.rdbuf();
    std::cerr.rdbuf(stderror.rdbuf());

try {
    uf->exifBuf = NULL;
    uf->exifBufLen = 0;

    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(uf->filename);
 
    assert(image.get() != 0);
    image->readMetadata();

    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty()) {
        std::string error(uf->filename);
        error += ": No Exif data found in the file";
        throw Exiv2::Error(1, error);
    }
    /* Reset orientation tag since UFRaw already rotates the image */
    Exiv2::ExifData::iterator pos;
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.Orientation")))
	    != exifData.end() ) {
	ufraw_message(UFRAW_SET_LOG, "Reseting %s from '%d'  to '1'\n",
		pos->key().c_str(), pos->value().toLong());
	pos->setValue("1"); /* 1 = Normal orientation */
    }
    /* Delete original TIFF data, which is irrelevant*/
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.StripOffsets")))
	    != exifData.end() )
	exifData.erase(pos);
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.RowsPerStrip")))
	    != exifData.end() )
	exifData.erase(pos);
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.StripByteCounts")))
	    != exifData.end() )
	exifData.erase(pos);

    Exiv2::DataBuf buf(exifData.copy());
    const unsigned char ExifHeader[] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};
    /* If buffer too big for JPEG, try deleting some stuff. */
    if ( buf.size_+sizeof(ExifHeader)>65533 ) {
	if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Photo.MakerNote")))
		!= exifData.end() ) {
	    exifData.erase(pos);
	    ufraw_message(UFRAW_SET_LOG,
		    "buflen %d too big, erasing Exif.Photo.MakerNote\n",
		    buf.size_+sizeof(ExifHeader));
	    buf = exifData.copy();
	}
    }
    uf->exifBufLen = buf.size_ + sizeof(ExifHeader);
    uf->exifBuf = g_new(unsigned char, uf->exifBufLen);
    memcpy(uf->exifBuf, ExifHeader, sizeof(ExifHeader));
    memcpy(uf->exifBuf+sizeof(ExifHeader), buf.pData_, buf.size_);
    ufraw_message(UFRAW_SET_LOG, "EXIF data read using exiv2, buflen %d\n",
	    uf->exifBufLen);
    g_strlcpy(uf->conf->exifSource, EXV_PACKAGE_STRING, max_name);

    /* List of tag names taken from exiv2's printSummary() in actions.cpp */
    /* Read shutter time */
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureTime")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->shutterText, str.str().c_str(), max_name);
    } else if ( (pos=exifData.findKey(
		    Exiv2::ExifKey("Exif.Photo.ShutterSpeedValue")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->shutterText, str.str().c_str(), max_name);
    }
    /* Read aperture */
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->apertureText, str.str().c_str(), max_name);
    } else if ( (pos=exifData.findKey(
		    Exiv2::ExifKey("Exif.Photo.ApertureValue")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->apertureText, str.str().c_str(), max_name);
    }
    /* Read ISO speed */
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Photo.ISOSpeedRatings")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->isoText, str.str().c_str(), max_name);
    } else if ( (pos=exifData.findKey(Exiv2::ExifKey(
		    // Group name changed in exiv2-0.11
		    EXIV2_CHECK_VERSION(0,11,0) ?
			"Exif.CanonSi.ISOSpeed" :
			"Exif.CanonCs2.ISOSpeed"))) != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->isoText, str.str().c_str(), max_name);
    } else if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Nikon1.ISOSpeed")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->isoText, str.str().c_str(), max_name);
    } else if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Nikon2.ISOSpeed")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->isoText, str.str().c_str(), max_name);
    } else if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Nikon3.ISOSpeed")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->isoText, str.str().c_str(), max_name);
    } else if ( (pos=exifData.findKey(
		    Exiv2::ExifKey("Exif.MinoltaCsNew.ISOSpeed")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->isoText, str.str().c_str(), max_name);
    } else if ( (pos=exifData.findKey(
		    Exiv2::ExifKey("Exif.MinoltaCsOld.ISOSpeed")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->isoText, str.str().c_str(), max_name);
    } else if ( (pos=exifData.findKey(
		    Exiv2::ExifKey("Exif.MinoltaCs5D.ISOSpeed")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->isoText, str.str().c_str(), max_name);
    } else if ( (pos=exifData.findKey(Exiv2::ExifKey(
			"Exif.MinoltaCs7D.ISOSpeed")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->isoText, str.str().c_str(), max_name);
    }
    /* Read focal length */
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Photo.FocalLength")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->focalLenText, str.str().c_str(), max_name);
    }
    /* Read focal length in 35mm equivalent */
    if ( (pos=exifData.findKey(Exiv2::ExifKey(
		"Exif.Photo.FocalLengthIn35mmFilm")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->focalLen35Text, str.str().c_str(), max_name);
    }
    /* Read lens name */
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Nikon3.Lens")))
	    != exifData.end() ) {
	std::stringstream str;
       	str << *pos;
	g_strlcpy(uf->conf->lensText, str.str().c_str(), max_name);
    }
    std::cerr.rdbuf(savecerr);
    ufraw_message(UFRAW_SET_LOG, "%s\n", stderror.str().c_str());

    return UFRAW_SUCCESS;
}
catch (Exiv2::AnyError& e) {
    std::cerr.rdbuf(savecerr);
    ufraw_message(UFRAW_SET_WARNING, "%s\n", e.what().c_str());
    return UFRAW_ERROR;
}

}
#endif /* HAVE_EXIV2 */
