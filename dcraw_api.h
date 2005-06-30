/*
   dcraw_api.h - an API for dcraw
   by udi Fuchs,

   based on dcraw by Dave Coffin
   http://www.cybercom.net/~dcoffin/

   UFRaw is licensed under the GNU General Public License.
   It uses "dcraw" code to do the actual raw decoding.
*/

#ifndef _DCRAW_API_H
#define _DCRAW_API_H

typedef guint16 dcraw_image_type[4];

typedef struct {
    dcraw_image_type *image;
    int width, height, colors, trim;
} dcraw_image_data;

typedef struct {
    FILE *ifp;
    int width, height, colors, fourColorFilters, filters, use_coeff, trim;
    int flip, shrink;
    dcraw_image_data raw;
    float pre_mul[4], post_mul[4], cam_mul[4], coeff[3][4];
    int rgbMax, black, fuji_width;
    int toneCurveSize, toneCurveOffset;
    char *message;
} dcraw_data;

int dcraw_open(dcraw_data *h, char *filename);
int dcraw_load_raw(dcraw_data *h);
int dcraw_finalize_shrink(dcraw_image_data *f, dcraw_data *h, int scale);
int dcraw_image_resize(dcraw_image_data *image, int size);
int dcraw_flip_image(dcraw_image_data *image, int flip);
int dcraw_set_color_scale(dcraw_data *h, int useAutoWB, int useCameraWB);
int dcraw_finalize_interpolate(dcraw_image_data *f, dcraw_data *h,
	int quick, int fourColor, int rgbWB[4]);
void dcraw_close(dcraw_data *h);

#define DCRAW_SUCCESS 0
#define DCRAW_ERROR 1
#define DCRAW_UNSUPPORTED 2
#define DCRAW_NO_CAMERA_WB 3
#define DCRAW_VERBOSE 4
#define DCRAW_OPEN_ERROR 5

void dcraw_message(int code, char *format, ...);

#endif /*_DCRAW_API_H*/
