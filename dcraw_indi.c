/*
 * dcraw_indi.c - dcraw functions made indipended
 *  
 * by Udi Fuchs,
 *
 * based on dcraw by Dave Coffin
 * http://www.cybercom.net/~dcoffin/
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses "dcraw" code to do the actual raw decoding.
 *
 * This file contains some functions taken from dcraw.c that where modified
 * to work with UFRaw. Biggest change is that these functions are now
 * independed of dcraw's global variables.
 *
 * NOTICE: One must check if updates in dcraw.c effect this code.
 * This file was last synchronized with dcraw 7.20
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <dcraw_api.h>

#define ushort UshORt
typedef unsigned short ushort;
typedef gint64 INT64;

#define camera_red  cam_mul[0]
#define camera_blue cam_mul[2]

#define FORC3 for (c=0; c < 3; c++)
#define FORC4 for (c=0; c < 4; c++)
#define FORCC for (c=0; c < colors; c++)

/*
   In order to inline this calculation, I make the risky
   assumption that all filter patterns can be described
   by a repeating pattern of eight rows and two columns

   Return values are either 0/1/2/3 = G/M/C/Y or 0/1/2/3 = R/G1/B/G2
 */
#define FC(row,col) \
	(int)(filters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3)

#define CLASS

void CLASS merror (void *ptr, char *where);

void CLASS scale_colors_INDI(ushort (*image)[4], int maximum,
       const int black, const int use_auto_wb, const int use_camera_wb,
       const float cam_mul[4],
       const int height, const int width, const int colors,
       float pre_mul[4], const unsigned filters, /*const*/ ushort white[8][8],
       const char *ifname)
{
  int row, col, c, val; // , shift=0; /*UF*/
  int min[4], max[4], count[4];
  double sum[4], dmin;

  maximum -= black; /* maximum is changed only locally */
  if (use_auto_wb || (use_camera_wb && camera_red == -1)) {
    FORCC min[c] = INT_MAX;
    FORCC max[c] = count[c] = sum[c] = 0;
    for (row=0; row < height; row++)
      for (col=0; col < width; col++)
	FORCC {
	  val = image[row*width+col][c];
	  if (!val) continue;
	  if (min[c] > val) min[c] = val;
	  if (max[c] < val) max[c] = val;
	  val -= black;
	  if (val > maximum-25) continue;
	  if (val < 0) val = 0;
	  sum[c] += val;
	  count[c]++;
	}
    FORCC pre_mul[c] = count[c] / sum[c];
  }
  if (use_camera_wb && camera_red != -1) {
    FORCC count[c] = sum[c] = 0;
    for (row=0; row < 8; row++)
      for (col=0; col < 8; col++) {
	c = FC(row,col);
	if ((val = white[row][col] - black) > 0)
	  sum[c] += val;
	count[c]++;
      }
    val = 1;
    FORCC if (sum[c] == 0) val = 0;
    if (val)
      FORCC pre_mul[c] = count[c] / sum[c];
    else if (camera_red && camera_blue)
      memcpy (pre_mul, cam_mul, 4*sizeof(float));
    else
      dcraw_message (DCRAW_NO_CAMERA_WB,
	      "%s: Cannot use camera white balance.\n", ifname);
  }
  dmin = DBL_MAX;
  FORCC if (dmin > pre_mul[c])
	    dmin = pre_mul[c];
  FORCC pre_mul[c] /= dmin;
    
  dcraw_message(DCRAW_VERBOSE, "Scaling with black=%d, pre_mul[] =", black);
  FORCC dcraw_message(DCRAW_VERBOSE, " %f", pre_mul[c]);
  dcraw_message(DCRAW_VERBOSE, "\n");
 
/* The rest of the scaling is done somewhere else UF*/
#if 0
  if (raw_color) {
    pre_mul[0] *= red_scale;
    pre_mul[2] *= blue_scale;
  }
  while (maximum << shift < 0x8000) shift++;
  FORCC pre_mul[c] *= 1 << shift;
  maximum <<= shift;

  if (write_fun != write_ppm || bright < 1) {
    maximum *= bright;
    if (maximum > 0xffff)
	maximum = 0xffff;
    FORCC pre_mul[c] *= bright;
  }
  clip_max = clip_color ? maximum : 0xffff;
  for (row=0; row < height; row++)
    for (col=0; col < width; col++)
      FORCC {
	val = image[row*width+col][c];
	if (!val) continue;
	val -= black;
	val *= pre_mul[c];
	if (val < 0) val = 0;
	if (val > clip_max) val = clip_max;
	image[row*width+col][c] = val;
      }
#endif
}

/*
   This algorithm is officially called:

   "Interpolation using a Threshold-based variable number of gradients"

   described in http://www-ise.stanford.edu/~tingchen/algodep/vargra.html

   I've extended the basic idea to work with non-Bayer filter arrays.
   Gradients are numbered clockwise from NW=0 to W=7.
 */
void CLASS vng_interpolate_INDI(ushort (*image)[4], const unsigned filters,
        const int width, const int height, const int colors,
        const int quick_interpolate, const int clip_max) /*UF*/
{
  static const signed char *cp, terms[] = {
    -2,-2,+0,-1,0,0x01, -2,-2,+0,+0,1,0x01, -2,-1,-1,+0,0,0x01,
    -2,-1,+0,-1,0,0x02, -2,-1,+0,+0,0,0x03, -2,-1,+0,+1,1,0x01,
    -2,+0,+0,-1,0,0x06, -2,+0,+0,+0,1,0x02, -2,+0,+0,+1,0,0x03,
    -2,+1,-1,+0,0,0x04, -2,+1,+0,-1,1,0x04, -2,+1,+0,+0,0,0x06,
    -2,+1,+0,+1,0,0x02, -2,+2,+0,+0,1,0x04, -2,+2,+0,+1,0,0x04,
    -1,-2,-1,+0,0,0x80, -1,-2,+0,-1,0,0x01, -1,-2,+1,-1,0,0x01,
    -1,-2,+1,+0,1,0x01, -1,-1,-1,+1,0,0x88, -1,-1,+1,-2,0,0x40,
    -1,-1,+1,-1,0,0x22, -1,-1,+1,+0,0,0x33, -1,-1,+1,+1,1,0x11,
    -1,+0,-1,+2,0,0x08, -1,+0,+0,-1,0,0x44, -1,+0,+0,+1,0,0x11,
    -1,+0,+1,-2,1,0x40, -1,+0,+1,-1,0,0x66, -1,+0,+1,+0,1,0x22,
    -1,+0,+1,+1,0,0x33, -1,+0,+1,+2,1,0x10, -1,+1,+1,-1,1,0x44,
    -1,+1,+1,+0,0,0x66, -1,+1,+1,+1,0,0x22, -1,+1,+1,+2,0,0x10,
    -1,+2,+0,+1,0,0x04, -1,+2,+1,+0,1,0x04, -1,+2,+1,+1,0,0x04,
    +0,-2,+0,+0,1,0x80, +0,-1,+0,+1,1,0x88, +0,-1,+1,-2,0,0x40,
    +0,-1,+1,+0,0,0x11, +0,-1,+2,-2,0,0x40, +0,-1,+2,-1,0,0x20,
    +0,-1,+2,+0,0,0x30, +0,-1,+2,+1,1,0x10, +0,+0,+0,+2,1,0x08,
    +0,+0,+2,-2,1,0x40, +0,+0,+2,-1,0,0x60, +0,+0,+2,+0,1,0x20,
    +0,+0,+2,+1,0,0x30, +0,+0,+2,+2,1,0x10, +0,+1,+1,+0,0,0x44,
    +0,+1,+1,+2,0,0x10, +0,+1,+2,-1,1,0x40, +0,+1,+2,+0,0,0x60,
    +0,+1,+2,+1,0,0x20, +0,+1,+2,+2,0,0x10, +1,-2,+1,+0,0,0x80,
    +1,-1,+1,+1,0,0x88, +1,+0,+1,+2,0,0x08, +1,+0,+2,-1,0,0x40,
    +1,+0,+2,+1,0,0x10
  }, chood[] = { -1,-1, -1,0, -1,+1, 0,+1, +1,+1, +1,0, +1,-1, 0,-1 };
  ushort (*brow[5])[4], *pix;
  int code[8][2][320], *ip, gval[8], gmin, gmax, sum[4];
  int row, col, shift, x, y, x1, x2, y1, y2, t, weight, grads, color, diag;
  int g, diff, thold, num, c;

  for (row=0; row < 8; row++) {		/* Precalculate for bilinear */
    for (col=1; col < 3; col++) {
      ip = code[row][col & 1];
      memset (sum, 0, sizeof sum);
      for (y=-1; y <= 1; y++)
	for (x=-1; x <= 1; x++) {
	  shift = (y==0) + (x==0);
	  if (shift == 2) continue;
	  color = FC(row+y,col+x);
	  *ip++ = (width*y + x)*4 + color;
	  *ip++ = shift;
	  *ip++ = color;
	  sum[color] += 1 << shift;
	}
      FORCC
	if (c != FC(row,col)) {
	  *ip++ = c;
	  *ip++ = sum[c];
	}
    }
  }
  for (row=1; row < height-1; row++) {	/* Do bilinear interpolation */
    for (col=1; col < width-1; col++) {
      pix = image[row*width+col];
      ip = code[row & 7][col & 1];
      memset (sum, 0, sizeof sum);
      for (g=8; g--; ) {
	diff = pix[*ip++];
	diff <<= *ip++;
	sum[*ip++] += diff;
      }
      for (g=colors; --g; ) {
	c = *ip++;
	pix[c] = sum[c] / *ip++;
      }
    }
  }
  if (quick_interpolate)
    return;

  for (row=0; row < 8; row++) {		/* Precalculate for VNG */
    for (col=0; col < 2; col++) {
      ip = code[row][col];
      for (cp=terms, t=0; t < 64; t++) {
	y1 = *cp++;  x1 = *cp++;
	y2 = *cp++;  x2 = *cp++;
	weight = *cp++;
	grads = *cp++;
	color = FC(row+y1,col+x1);
	if (FC(row+y2,col+x2) != color) continue;
	diag = (FC(row,col+1) == color && FC(row+1,col) == color) ? 2:1;
	if (abs(y1-y2) == diag && abs(x1-x2) == diag) continue;
	*ip++ = (y1*width + x1)*4 + color;
	*ip++ = (y2*width + x2)*4 + color;
	*ip++ = weight;
	for (g=0; g < 8; g++)
	  if (grads & 1<<g) *ip++ = g;
	*ip++ = -1;
      }
      *ip++ = INT_MAX;
      for (cp=chood, g=0; g < 8; g++) {
	y = *cp++;  x = *cp++;
	*ip++ = (y*width + x) * 4;
	color = FC(row,col);
	if (FC(row+y,col+x) != color && FC(row+y*2,col+x*2) == color)
	  *ip++ = (y*width + x) * 8 + color;
	else
	  *ip++ = 0;
      }
    }
  }
  brow[4] = calloc (width*3, sizeof **brow);
  merror (brow[4], "vng_interpolate()");
  for (row=0; row < 3; row++)
    brow[row] = brow[4] + row*width;
  for (row=2; row < height-2; row++) {		/* Do VNG interpolation */
    for (col=2; col < width-2; col++) {
      pix = image[row*width+col];
      ip = code[row & 7][col & 1];
      memset (gval, 0, sizeof gval);
      while ((g = ip[0]) != INT_MAX) {		/* Calculate gradients */
	num = (diff = pix[g] - pix[ip[1]]) >> 31;
	gval[ip[3]] += (diff = ((diff ^ num) - num) << ip[2]);
	ip += 5;
	if ((g = ip[-1]) == -1) continue;
	gval[g] += diff;
	while ((g = *ip++) != -1)
	  gval[g] += diff;
      }
      ip++;
      gmin = gmax = gval[0];			/* Choose a threshold */
      for (g=1; g < 8; g++) {
	if (gmin > gval[g]) gmin = gval[g];
	if (gmax < gval[g]) gmax = gval[g];
      }
      if (gmax == 0) {
	memcpy (brow[2][col], pix, sizeof *image);
	continue;
      }
      thold = gmin + (gmax >> 1);
      memset (sum, 0, sizeof sum);
      color = FC(row,col);
      for (num=g=0; g < 8; g++,ip+=2) {		/* Average the neighbors */
	if (gval[g] <= thold) {
	  FORCC
	    if (c == color && ip[1])
	      sum[c] += (pix[c] + pix[ip[1]]) >> 1;
	    else
	      sum[c] += pix[ip[0] + c];
	  num++;
	}
      }
      FORCC {					/* Save to buffer */
	t = pix[color];
	if (c != color) {
	  t += (sum[c] - sum[color])/num;
	  if (t < 0) t = 0;
	  if (t > clip_max) t = clip_max;
	}
	brow[2][col][c] = t;
      }
    }
    if (row > 3)				/* Write buffer to image */
      memcpy (image[(row-2)*width+2], brow[0]+2, (width-4)*sizeof *image);
    for (g=0; g < 4; g++)
      brow[(g-1) & 3] = brow[g];
  }
  memcpy (image[(row-2)*width+2], brow[0]+2, (width-4)*sizeof *image);
  memcpy (image[(row-1)*width+2], brow[1]+2, (width-4)*sizeof *image);
  free (brow[4]);
}

/*
   Convert the entire image to RGB colorspace and build a histogram.
 */
void CLASS convert_to_rgb_INDI(ushort (*image)[4], const int document_mode,
        int colors, const int trim, const int height, const int width,
        const unsigned filters, const int use_coeff,
        /*const*/ float coeff[3][4], const int clip_max)/*UF*/
{
  int row, col, r, g, c=0;
  ushort *img;
  float rgb[3];

/* Disabled features UF
  if (document_mode)
    colors = 1;
  memset (histogram, 0, sizeof histogram);
*/
  for (row = trim; row < height-trim; row++)
    for (col = trim; col < width-trim; col++) {
      img = image[row*width+col];
      if (document_mode)
	c = FC(row,col);
      if (colors == 4 && !use_coeff)	/* Recombine the greens */
	img[1] = (img[1] + img[3]) >> 1;
      if (colors == 1)			/* RGB from grayscale */
	for (r=0; r < 3; r++)
	  rgb[r] = img[c];
      else if (use_coeff) {		/* RGB via coeff[][] */
	for (r=0; r < 3; r++)
	  for (rgb[r]=g=0; g < colors; g++)
	    rgb[r] += img[g] * coeff[r][g];
        for (r=0; r < 3; r++) {
	  if (rgb[r] < 0)        rgb[r] = 0;
	  if (rgb[r] > clip_max) rgb[r] = clip_max;
	  img[r] = rgb[r];
        }
      }
    }
}

void CLASS fuji_rotate_INDI(ushort (**image_p)[4], int *height_p,
    int *width_p, int *fuji_width_p, const int colors, const double step)
{
  int height = *height_p, width = *width_p, fuji_width = *fuji_width_p; /*UF*/
  ushort (*image)[4] = *image_p; /*UF*/
  int i, wide, high, row, col;
//  double step;
  float r, c, fr, fc;
  unsigned ur, uc;
  ushort (*img)[4], (*pix)[4];

  if (!fuji_width) return;
  dcraw_message (DCRAW_VERBOSE, "Rotating image 45 degrees...\n");
//  fuji_width = (fuji_width + shrink) >> shrink;
//  step = 0.5;
  wide = fuji_width / step;
  high = (height - fuji_width) / step;
  img = calloc (wide*high, sizeof *img);
  merror (img, "fuji_rotate()");

  for (row=0; row < high; row++)
    for (col=0; col < wide; col++) {
      ur = r = fuji_width + (row-col)*step;
      uc = c = (row+col)*step;
      if ((int)ur > height-2 || (int)uc > width-2) continue;
      fr = r - ur;
      fc = c - uc;
      pix = image + ur*width + uc;
      for (i=0; i < colors; i++)
	img[row*wide+col][i] =
	  (pix[    0][i]*(1-fc) + pix[      1][i]*fc) * (1-fr) +
	  (pix[width][i]*(1-fc) + pix[width+1][i]*fc) * fr;
    }
 free (image);
  width  = wide;
  height = high;
  image  = img;
  fuji_width = 0;
  *height_p = height; /* INDI - UF*/
  *width_p = width;
  *fuji_width_p = fuji_width;
  *image_p = image;
}

void CLASS flip_image_INDI(ushort (*image)[4], int *height_p, int *width_p,
    /*const*/ int flip, int *ymag_p, int *xmag_p) /*UF*/
{
  unsigned *flag;
  int size, base, dest, next, row, col, temp;
  INT64 *img, hold;
  int height = *height_p, width = *width_p,
              ymag = *ymag_p, xmag = *xmag_p;/* INDI - UF*/

  img = (INT64 *) image;
  size = height * width;
  flag = calloc ((size+31) >> 5, sizeof *flag);
  merror (flag, "flip_image()");
  for (base = 0; base < size; base++) {
    if (flag[base >> 5] & (1 << (base & 31)))
      continue;
    dest = base;
    hold = img[base];
    while (1) {
      if (flip & 4) {
	row = dest % height;
	col = dest / height;
      } else {
	row = dest / width;
	col = dest % width;
      }
      if (flip & 2)
	row = height - 1 - row;
      if (flip & 1)
	col = width - 1 - col;
      next = row * width + col;
      if (next == base) break;
      flag[next >> 5] |= 1 << (next & 31);
      img[dest] = img[next];
      dest = next;
    }
    img[dest] = hold;
  }
  free (flag);
  if (flip & 4) {
    temp = height;
    height = width;
    width = temp;
    temp = ymag;
    ymag = xmag;
    xmag = temp;
  }
  *height_p = height; /* INDI - UF*/
  *width_p = width;
  *ymag_p = ymag;
  *xmag_p = xmag;
}
