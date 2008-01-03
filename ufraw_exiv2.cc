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

extern "C" {
#include "uf_glib.h"
#include "ufraw.h"
}

#ifdef HAVE_EXIV2
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <sstream>
#include <cassert>

/**
 * Helper function to copy a string to a buffer, converting it from
 * current locale (in which exiv2 often returns strings) to UTF-8.
 */
static void strlcpy_to_utf8 (char *dest, size_t dest_max, Exiv2::ExifData::iterator pos)
{
	std::stringstream str;
	str << *pos;

	char *s = g_locale_to_utf8 (str.str ().c_str (), str.str ().length (),
				    NULL, NULL, NULL);
	if (s) {
		g_strlcpy (dest, s, dest_max);
		g_free (s);
	}
	else
		g_strlcpy (dest, str.str ().c_str (), dest_max);
}

extern "C" int ufraw_exif_from_exiv2(ufraw_data *uf)
{
    /* Redirect exiv2 errors to a string buffer */
    std::ostringstream stderror;
    std::streambuf *savecerr = std::cerr.rdbuf();
    std::cerr.rdbuf(stderror.rdbuf());

try {
    uf->exifBuf = NULL;
    uf->exifBufLen = 0;

    Exiv2::Image::AutoPtr image;
    if ( uf->unzippedBuf!=NULL ) {
	image = Exiv2::ImageFactory::open(
		(const Exiv2::byte*)uf->unzippedBuf, uf->unzippedBufLen);
    } else {
	char *filename = uf_win32_locale_filename_from_utf8(uf->filename);
	image = Exiv2::ImageFactory::open(filename);
	uf_win32_locale_filename_free(filename);
    }
    assert(image.get() != 0);
    image->readMetadata();

    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty()) {
	std::string error(uf->filename);
	error += ": No Exif data found in the file";
	throw Exiv2::Error(1, error);
    }
    Exiv2::ExifData::iterator pos;
    if ( uf->conf->rotate ) {
	/* Reset orientation tag since UFRaw already rotates the image */
	if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Image.Orientation")))
		!= exifData.end() ) {
	    ufraw_message(UFRAW_SET_LOG, "Reseting %s from '%d'  to '1'\n",
		    pos->key().c_str(), pos->value().toLong());
	    pos->setValue("1"); /* 1 = Normal orientation */
	}
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
    if ( buf.size_+sizeof(ExifHeader)>65533 ) {
	exifData.eraseThumbnail();
	ufraw_message(UFRAW_SET_LOG,
		"buflen %d too big, erasing Thumbnail\n",
		buf.size_+sizeof(ExifHeader));
	buf = exifData.copy();
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
	strlcpy_to_utf8(uf->conf->shutterText, max_name, pos);
	uf->conf->shutter = pos->toFloat ();
    } else if ( (pos=exifData.findKey(
		    Exiv2::ExifKey("Exif.Photo.ShutterSpeedValue")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->shutterText, max_name, pos);
	uf->conf->shutter = 1.0 / pos->toFloat ();
    }
    /* Read aperture */
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->apertureText, max_name, pos);
	uf->conf->aperture = pos->toFloat ();
    } else if ( (pos=exifData.findKey(
		    Exiv2::ExifKey("Exif.Photo.ApertureValue")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->apertureText, max_name, pos);
	uf->conf->aperture = pos->toFloat ();
    }
    /* Read ISO speed */
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Photo.ISOSpeedRatings")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->isoText, max_name, pos);
    } else if ( (pos=exifData.findKey(Exiv2::ExifKey(
			"Exif.CanonSi.ISOSpeed"))) != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->isoText, max_name, pos);
    } else if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Nikon1.ISOSpeed")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->isoText, max_name, pos);
    } else if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Nikon2.ISOSpeed")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->isoText, max_name, pos);
    } else if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Nikon3.ISOSpeed")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->isoText, max_name, pos);
    } else if ( (pos=exifData.findKey(
		    Exiv2::ExifKey("Exif.MinoltaCsNew.ISOSpeed")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->isoText, max_name, pos);
    } else if ( (pos=exifData.findKey(
		    Exiv2::ExifKey("Exif.MinoltaCsOld.ISOSpeed")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->isoText, max_name, pos);
    } else if ( (pos=exifData.findKey(
		    Exiv2::ExifKey("Exif.MinoltaCs5D.ISOSpeed")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->isoText, max_name, pos);
    } else if ( (pos=exifData.findKey(Exiv2::ExifKey(
			"Exif.MinoltaCs7D.ISOSpeed")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->isoText, max_name, pos);
    }
    /* Read focal length */
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Photo.FocalLength")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->focalLenText, max_name, pos);
	uf->conf->focal_len = pos->toFloat ();
    }
    /* Read focal length in 35mm equivalent */
    if ( (pos=exifData.findKey(Exiv2::ExifKey(
		"Exif.Photo.FocalLengthIn35mmFilm")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->focalLen35Text, max_name, pos);
    }
    /* Read lens name */
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Nikon3.Lens")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->lensText, max_name, pos);
    } else if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Canon.0x0095")))
		!= exifData.end() ) {
	strlcpy_to_utf8(uf->conf->lensText, max_name, pos);
    }
    /* Read flash mode */
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Photo.Flash")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->flashText, max_name, pos);
    }
    /* Read White Balance Setting */
    if ( (pos=exifData.findKey(Exiv2::ExifKey("Exif.Photo.WhiteBalance")))
	    != exifData.end() ) {
	strlcpy_to_utf8(uf->conf->whiteBalanceText, max_name, pos);
    }
    std::cerr.rdbuf(savecerr);
    ufraw_message(UFRAW_SET_LOG, "%s\n", stderror.str().c_str());

    return UFRAW_SUCCESS;
}
catch (Exiv2::AnyError& e) {
    std::cerr.rdbuf(savecerr);
    std::string s(e.what());
    ufraw_message(UFRAW_SET_WARNING, "%s\n", s.c_str());
    return UFRAW_ERROR;
}

}
#else
extern "C" int ufraw_exif_from_exiv2(ufraw_data *uf)
{
    uf = uf;
    ufraw_message(UFRAW_SET_LOG, "ufraw built without EXIF support\n");
    return UFRAW_ERROR;
}
#endif /* HAVE_EXIV2 */
