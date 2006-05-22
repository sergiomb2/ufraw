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

extern "C" int ufraw_exif_from_exiv2(void *ifp, char *filename,
	unsigned char **exifBuf, unsigned int *exifBufLen)
try {
    ifp = ifp;
    *exifBuf = NULL;
    *exifBufLen = 0;

    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
    assert(image.get() != 0);
    image->readMetadata();

    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty()) {
        std::string error(filename);
        error += ": No Exif data found in the file";
        throw Exiv2::Error(1, error);
    }
    /* Reset orientation tag since UFRaw already rotates the image */
    Exiv2::ExifKey key("Exif.Image.Orientation");
    Exiv2::ExifData::iterator pos = exifData.findKey(key);
    if (pos != exifData.end()) {
	std::stringstream str;
       	str << *pos;
	ufraw_message(UFRAW_SET_LOG, "Reseting %s from '%d - %s' to '1'\n",
		pos->key().c_str(), pos->value().toLong(),
		str.str().c_str());
	pos->setValue("1"); /* 1 = Normal orientation */
    }
    Exiv2::DataBuf const buf(exifData.copy());
    const unsigned char ExifHeader[] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};
    *exifBufLen = buf.size_ + sizeof(ExifHeader);
    *exifBuf = g_new(unsigned char, *exifBufLen);
    memcpy(*exifBuf, ExifHeader, sizeof(ExifHeader));
    memcpy(*exifBuf+sizeof(ExifHeader), buf.pData_, buf.size_);
    ufraw_message(UFRAW_SET_LOG, "EXIF data read using exiv2, buflen %d\n",
	    *exifBufLen);

    Exiv2::ExifData::const_iterator end = exifData.end();
    for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
	ufraw_message(UFRAW_SET_LOG, "%s: %s\n",
		i->key().c_str(), i->value().toString().c_str());
    }
    return UFRAW_SUCCESS;
}
catch (Exiv2::AnyError& e) {
    ufraw_message(UFRAW_SET_WARNING, "%s\n", e.what().c_str());
    return UFRAW_ERROR;
}
#endif /* HAVE_EXIV2 */
