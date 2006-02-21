/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 * by Udi Fuchs,
 *
 * wb_presets.c - White balance preset values for various cameras
 *
 * UFRaw is licensed under the GNU General Public License.
 * It uses DCRaw code to do the actual raw decoding.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include "ufraw.h"

/* Column 1 - "make" of the camera.
 * Column 2 - "model" (use the "make" and "model" as provided by DCRaw).
 * Column 3 - WB name.
 * Column 4 - Fine tuning. MUST be in increasing order. 0 for no fine tuning.
 *	      It is enough to give only the extreme values, the other values
 *	      will be interpolated.
 * Column 5 - Channel multipliers.
 */
const wb_data wb_preset[] = { 

  { "", "", "Manual WB", 0,    { 0, 0, 0, 0 } },
  { "", "", "Camera WB", 0,    { 0, 0, 0, 0 } },
  { "", "", "Auto WB",   0,    { 0, 0, 0, 0 } },

  { "Canon", "PowerShot G2", "Daylight", 0,	{ 2.011483, 1, 1.299522, 0 } },
  { "Canon", "PowerShot G2", "Cloudy", 0,	{ 2.032505, 1, 1.285851, 0 } },
  { "Canon", "PowerShot G2", "Tungsten", 0,	{ 1.976008, 1, 1.332054, 0 } },
  { "Canon", "PowerShot G2", "Fluorescent", 0,	{ 2.022010, 1, 1.295694, 0 } },
  { "Canon", "PowerShot G2", "Daylight Fluorescent", 0, { 2.029637, 1, 1.286807, 0 } },
  { "Canon", "PowerShot G2", "Flash", 0,	{ 2.153576, 1, 1.140680, 0 } },

  { "Canon", "PowerShot G5", "Daylight", 0,	{ 1.639521, 1, 1.528144, 0 } },
  { "Canon", "PowerShot G5", "Cloudy", 0,	{ 1.702153, 1, 1.462919, 0 } },
  { "Canon", "PowerShot G5", "Tungsten", 0,	{ 1.135071, 1, 2.374408, 0 } },
  { "Canon", "PowerShot G5", "Fluorescent", 0,	{ 1.660281, 1, 2.186462, 0 } },
  { "Canon", "PowerShot G5", "Daylight Fluorescent", 0, { 1.463297, 1, 1.764140, 0 } },
  { "Canon", "PowerShot G5", "Flash", 0,	{ 1.603593, 1, 1.562874, 0 } },

  { "Canon", "PowerShot G6", "Daylight", 0,	{ 1.769704, 1, 1.637931, 0 } },
  { "Canon", "PowerShot G6", "Cloudy", 0,	{ 2.062731, 1, 1.442804, 0 } },
  { "Canon", "PowerShot G6", "Tungsten", 0,	{ 1.077106, 1, 2.721234, 0 } },
  { "Canon", "PowerShot G6", "Fluorescent", 0,	{ 1.914922, 1, 2.142670, 0 } },
  { "Canon", "PowerShot G6", "Fluorescent H", 0, { 2.543677, 1, 1.650587, 0 } },
  { "Canon", "PowerShot G6", "Flash", 0,	{ 2.285322, 1, 1.333333, 0 } },

  { "Canon", "PowerShot S50", "Daylight", 0,	{ 1.772506, 1, 1.536496, 0 } },
  { "Canon", "PowerShot S50", "Cloudy", 0,	{ 1.831311, 1, 1.484223, 0 } },
  { "Canon", "PowerShot S50", "Tungsten", 0,	{ 1.185542, 1, 2.480723, 0 } },
  { "Canon", "PowerShot S50", "Fluorescent", 0, { 1.706410, 1, 2.160256, 0 } },
  { "Canon", "PowerShot S50", "Fluorescent H", 0, { 1.562500, 1, 1.817402, 0 } },
  { "Canon", "PowerShot S50", "Flash", 0,	{ 1.776156, 1, 1.531630, 0 } },

  { "Canon", "PowerShot S60", "Daylight", 0,	{ 1.742169, 1, 1.486747, 0 } },
  { "Canon", "PowerShot S60", "Cloudy", 0,	{ 1.881437, 1, 1.366467, 0 } },
  { "Canon", "PowerShot S60", "Tungsten", 0,	{ 1.144910, 1, 2.566467, 0 } },
  { "Canon", "PowerShot S60", "Fluorescent", 0, { 1.714650, 1, 2.053503, 0 } },
  { "Canon", "PowerShot S60", "Fluorescent H", 0, { 2.849655, 1, 2.067586, 0 } },
  { "Canon", "PowerShot S60", "Flash", 0,	{ 2.143229, 1, 1.190104, 0 } },
  { "Canon", "PowerShot S60", "Underwater", 0,  { 2.677109, 1, 1.148193, 0 } },

  { "Canon", "PowerShot S70", "Daylight", 0,	{ 1.943834, 1, 1.456654, 0 } },
  { "Canon", "PowerShot S70", "Cloudy", 0,	{ 2.049939, 1, 1.382460, 0 } },
  { "Canon", "PowerShot S70", "Tungsten", 0,	{ 1.169492, 1, 2.654964, 0 } },
  { "Canon", "PowerShot S70", "Fluorescent", 0, { 1.993456, 1, 2.056283, 0 } },
  { "Canon", "PowerShot S70", "Fluorescent H", 0, { 2.645914, 1, 1.565499, 0 } },
  { "Canon", "PowerShot S70", "Flash", 0,	{ 2.389189, 1, 1.147297, 0 } },
  { "Canon", "PowerShot S70", "Underwater", 0,  { 3.110565, 1, 1.162162, 0 } },

  { "Canon", "EOS 10D", "Sunlight", 0,		{ 2.159856, 1, 1.218750, 0 } }, 
  { "Canon", "EOS 10D", "Shadow", 0,		{ 2.533654, 1, 1.036058, 0 } }, 
  { "Canon", "EOS 10D", "Cloudy", 0,		{ 2.348558, 1, 1.116587, 0 } }, 
  { "Canon", "EOS 10D", "Indescandent", 0,	{ 1.431544, 1, 1.851040, 0 } }, 
  { "Canon", "EOS 10D", "Flourescent", 0,	{ 1.891509, 1, 1.647406, 0 } }, 
  { "Canon", "EOS 10D", "Flash", 0,		{ 2.385817, 1, 1.115385, 0 } },

  { "Canon", "EOS 20D", "Daylight", 0,		{ 1.954680, 1, 1.478818, 0 } },
  { "Canon", "EOS 20D", "Shade", 0,		{ 2.248276, 1, 1.227586, 0 } },
  { "Canon", "EOS 20D", "Cloudy", 0,		{ 2.115271, 1, 1.336946, 0 } },
  { "Canon", "EOS 20D", "Tungsten", 0,		{ 1.368087, 1, 2.417044, 0 } },
  { "Canon", "EOS 20D", "Fluorescent", 0,	{ 1.752709, 1, 2.060098, 0 } },
  { "Canon", "EOS 20D", "Flash", 0,		{ 2.145813, 1, 1.293596, 0 } },

  { "Canon", "EOS 300D DIGITAL", "Daylight", 0, { 2.13702, 1, 1.15745, 0 } }, 
  { "Canon", "EOS 300D DIGITAL", "Cloudy", 0,   { 2.50961, 1, 0.97716, 0 } }, 
  { "Canon", "EOS 300D DIGITAL", "Tungsten", 0, { 2.32091, 1, 1.05529, 0 } }, 
  { "Canon", "EOS 300D DIGITAL", "Flourescent", 0, { 1.39677, 1, 1.79892, 0 } }, 
  { "Canon", "EOS 300D DIGITAL", "Flash", 0,    { 1.84229, 1, 1.60573, 0 } },
  { "Canon", "EOS 300D DIGITAL", "Shade", 0,    { 2.13702, 1, 1.15745, 0 } },

  { "Canon", "EOS 350D DIGITAL", "Tungsten", 0, { 1.554250, 1, 2.377034, 0 } }, 
  { "Canon", "EOS 350D DIGITAL", "Daylight", 0, { 2.392927, 1, 1.487230, 0 } }, 
  { "Canon", "EOS 350D DIGITAL", "Fluorescent", 0, { 1.999040, 1, 1.995202, 0 } }, 
  { "Canon", "EOS 350D DIGITAL", "Shade", 0,    { 2.827112, 1, 1.235756, 0 } },
  { "Canon", "EOS 350D DIGITAL", "Flash", 0,    { 2.715128, 1, 1.295678, 0 } }, 
  { "Canon", "EOS 350D DIGITAL", "Cloudy", 0,   { 2.611984, 1, 1.343811, 0 } },

  { "Canon", "EOS DIGITAL REBEL", "Daylight", 0, { 2.13702, 1, 1.15745, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL", "Cloudy", 0,  { 2.50961, 1, 0.97716, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL", "Tungsten", 0, { 2.32091, 1, 1.05529, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL", "Flourescent", 0, { 1.39677, 1, 1.79892, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL", "Flash", 0,   { 1.84229, 1, 1.60573, 0 } },
  { "Canon", "EOS DIGITAL REBEL", "Shade", 0,   { 2.13702, 1, 1.15745, 0 } },

  { "Canon", "EOS DIGITAL REBEL XT", "Tungsten", 0, { 1.554250, 1, 2.377034, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL XT", "Daylight", 0, { 2.392927, 1, 1.487230, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL XT", "Fluorescent", 0, { 1.999040, 1, 1.995202, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL XT", "Shade", 0, { 2.827112, 1, 1.235756, 0 } },
  { "Canon", "EOS DIGITAL REBEL XT", "Flash", 0, { 2.715128, 1, 1.295678, 0 } }, 
  { "Canon", "EOS DIGITAL REBEL XT", "Cloudy", 0, { 2.611984, 1, 1.343811, 0 } },
  
  { "Canon", "EOS-1D Mark II", "Cloudy", 0,	{ 2.093750, 1, 1.166016, 0 } },
  { "Canon", "EOS-1D Mark II", "Daylight", 0,	{ 1.957031, 1, 1.295898, 0 } },
  { "Canon", "EOS-1D Mark II", "Flash", 0,	{ 2.225586, 1, 1.172852, 0 } },
  { "Canon", "EOS-1D Mark II", "Fluorescent", 0, { 1.785853, 1, 1.785853, 0 } },
  { "Canon", "EOS-1D Mark II", "Shade", 0,	{ 2.220703, 1, 1.069336, 0 } },
  { "Canon", "EOS-1D Mark II", "Tungsten", 0,	{ 1.415480, 1, 2.160142, 0 } },

  { "FUJIFILM", "FinePix S5000", "Incandescent", 0, { 1.212081, 1, 2.672364, 0 } },
  { "FUJIFILM", "FinePix S5000", "Fluorescent", 0, { 1.772316, 1, 2.349902, 0 } },
  { "FUJIFILM", "FinePix S5000", "Direct sunlight", 0, { 1.860403, 1, 1.515946, 0 } },
  { "FUJIFILM", "FinePix S5000", "Flash", 0,	{ 2.202181, 1, 1.423284, 0 } },
  { "FUJIFILM", "FinePix S5000", "Cloudy", 0,	{ 2.036578, 1, 1.382513, 0 } },
  { "FUJIFILM", "FinePix S5000", "Shade", 0,	{ 2.357215, 1, 1.212016, 0 } },

  { "FUJIFILM", "FinePix S7000", "Outdoor fine", 0, { 1.900000, 1, 1.525000, 0 } },
  { "FUJIFILM", "FinePix S7000", "Shade", 0, { 2.137500, 1, 1.350000, 0 } },
  { "FUJIFILM", "FinePix S7000", "Daylight fluorescent", 0, { 2.315217, 1, 1.347826, 0 } },
  { "FUJIFILM", "FinePix S7000", "Warm White fluorescent", 0, { 1.902174, 1, 1.663043, 0 } },
  { "FUJIFILM", "FinePix S7000", "Cool White fluorescent", 0, { 1.836957, 1, 2.130435, 0 } },
  { "FUJIFILM", "FinePix S7000", "Incandescent", 0, { 1.221239, 1, 2.548673, 0 } },

  { "LEICA", "DIGILUX 2", "Sunshine", 0,	{ 1.628906, 1, 1.488281, 0 } },
  { "LEICA", "DIGILUX 2", "Cloudy", 0,		{ 1.835938, 1, 1.343750, 0 } },
  { "LEICA", "DIGILUX 2", "Indoor Halogen", 0,	{ 1.078125, 1, 2.203125, 0 } },
  { "LEICA", "DIGILUX 2", "Flash", 0,		{ 2.074219, 1, 1.304688, 0 } },
  { "LEICA", "DIGILUX 2", "B/W", 0,		{ 1.632812, 1, 1.550781, 0 } },

  { "Minolta", "DiMAGE 7Hi", "Daylight", 0,	{ 1.609375, 1, 1.328125, 0 } },  /* 5500K */
  { "Minolta", "DiMAGE 7Hi", "Tungsten", 0,	{ 1, 1.137778, 2.768889, 0 } },  /* 2800K */
  { "Minolta", "DiMAGE 7Hi", "Fluorescent 1", 0, { 1.664062, 1, 2.105469, 0 } },  /* 4060K*/
  { "Minolta", "DiMAGE 7Hi", "Fluorescent 2", 0, { 1.796875, 1, 1.734375, 0 } },  /* 4938K */
  { "Minolta", "DiMAGE 7Hi", "Cloudy", 0,	{ 1.730469, 1, 1.269531, 0 } },  /* 5823K */

  { "Minolta", "DiMAGE A1", "Daylight", 0,	{ 1.808594, 1, 1.304688, 0 } },
  { "Minolta", "DiMAGE A1", "Tungsten", 0,	{ 1.062500, 1, 2.675781, 0 } },
  { "Minolta", "DiMAGE A1", "Fluorescent", 0,	{ 1.707031, 1, 2.039063, 0 } },
  { "Minolta", "DiMAGE A1", "Cloudy", 0,	{ 1.960938, 1, 1.339844, 0 } },
  { "Minolta", "DiMAGE A1", "Shade", 0,		{ 2.253906, 1, 1.199219, 0 } },
  { "Minolta", "DiMAGE A1", "Shade", 2,		{ 2.000000, 1, 1.183594, 0 } },
  { "Minolta", "DiMAGE A1", "Flash", 0,		{ 1.972656, 1, 1.265625, 0 } },

  { "Minolta", "DiMAGE G500", "Sun", 0,		{ 1.496094, 1, 1.121094, 0 } },
  { "Minolta", "DiMAGE G500", "Cloudy", 0,	{ 1.527344, 1, 1.105469, 0 } },
  { "Minolta", "DiMAGE G500", "Luminescent", 0, { 1.382813, 1, 1.347656, 0 } },
  { "Minolta", "DiMAGE G500", "Lamp", 0,	{ 1.042969, 1, 1.859375, 0 } },

  { "MINOLTA", "DYNAX 5D", "Daylight", 0,	{ 1.660156, 1, 1.625000, 0 } },
  { "MINOLTA", "DYNAX 5D", "Shade", 0,		{ 1.898438, 1, 1.421875, 0 } },
  { "MINOLTA", "DYNAX 5D", "Cloudy", 0,		{ 1.777344, 1, 1.488281, 0 } },
  { "MINOLTA", "DYNAX 5D", "Tungsten", 0,	{ 1.000000, 1, 3.626016, 0 } },
  { "MINOLTA", "DYNAX 5D", "Fluorescent", 0,	{ 1.597656, 1, 2.496094, 0 } },
  { "MINOLTA", "DYNAX 5D", "Flash", 0,		{ 1.812500, 1, 1.445312, 0 } },

  { "MINOLTA", "DYNAX 7D", "Daylight", -3,	{ 1.476562, 1, 1.824219, 0 } },
  { "MINOLTA", "DYNAX 7D", "Daylight", 0,	{ 1.621094, 1, 1.601562, 0 } },
  { "MINOLTA", "DYNAX 7D", "Daylight", 3,	{ 1.785156, 1, 1.414062, 0 } },
  { "MINOLTA", "DYNAX 7D", "Shade", -3,		{ 1.683594, 1, 1.585938, 0 } },
  { "MINOLTA", "DYNAX 7D", "Shade", 0,		{ 1.855469, 1, 1.402344, 0 } },
  { "MINOLTA", "DYNAX 7D", "Shade", 3,		{ 2.031250, 1, 1.226562, 0 } },
  { "MINOLTA", "DYNAX 7D", "Cloudy", -3,	{ 1.593750, 1, 1.671875, 0 } },
  { "MINOLTA", "DYNAX 7D", "Cloudy", 0,		{ 1.738281, 1, 1.464844, 0 } },
  { "MINOLTA", "DYNAX 7D", "Cloudy", 3,		{ 1.925781, 1, 1.296875, 0 } },
  { "MINOLTA", "DYNAX 7D", "Tungsten", -3,	{ 0.867188, 1, 3.765625, 0 } },
  { "MINOLTA", "DYNAX 7D", "Tungsten", 0,	{ 0.945312, 1, 3.292969, 0 } },
  { "MINOLTA", "DYNAX 7D", "Tungsten", 3,	{ 1.050781, 1, 2.921875, 0 } },
  { "MINOLTA", "DYNAX 7D", "Fluorescent", -2,	{ 1.058594, 1, 3.230469, 0 } },
  { "MINOLTA", "DYNAX 7D", "Fluorescent", 0,	{ 1.570312, 1, 2.453125, 0 } },
  { "MINOLTA", "DYNAX 7D", "Fluorescent", 1,	{ 1.625000, 1, 2.226562, 0 } },
  { "MINOLTA", "DYNAX 7D", "Fluorescent", 4,	{ 2.046875, 1, 1.675781, 0 } },
  { "MINOLTA", "DYNAX 7D", "Flash", -3,		{ 1.738281, 1, 1.656250, 0 } },
  { "MINOLTA", "DYNAX 7D", "Flash", 0,		{ 1.890625, 1, 1.445312, 0 } },
  { "MINOLTA", "DYNAX 7D", "Flash", 3,		{ 2.101562, 1, 1.281250, 0 } },

  { "NIKON", "D100", "Incandescent", 0,		{ 1.406250, 1, 2.828125, 0 } },
  { "NIKON", "D100", "Fluorescent", 0,		{ 2.058594, 1, 2.617188, 0 } },
  { "NIKON", "D100", "Sunlight", 0,		{ 2.257813, 1, 1.757813, 0 } },
  { "NIKON", "D100", "Flash", 0,		{ 2.539063, 1, 1.539063, 0 } },
  { "NIKON", "D100", "Overcast", 0,		{ 2.507813, 1, 1.628906, 0 } },
  { "NIKON", "D100", "Shade", 0,		{ 2.906250, 1, 1.437500, 0 } },

  /*
   * D2X with firmware A 1.01 and B 1.01
   * D2X basic WB presets
   */

  /* D2X fine tune presets (0 is basic, above) */
  { "NIKON", "D2X", "Incandescent", -3,		{ 0.98462, 1, 2.61154, 0 } }, /* ~3300K */
  { "NIKON", "D2X", "Incandescent", -2,		{ 0.95880, 1, 2.71536, 0 } }, /* ~3200K */
  { "NIKON", "D2X", "Incandescent", -1,		{ 0.94465, 1, 2.77122, 0 } }, /* 3100K */
  { "NIKON", "D2X", "Incandescent", 0,		{ 0.92086, 1, 2.89928, 0 } }, /* 3000K */
  { "NIKON", "D2X", "Incandescent", 1,		{ 0.89510, 1, 3.03846, 0 } }, /* ~2900K */
  { "NIKON", "D2X", "Incandescent", 2,		{ 0.86486, 1, 3.17905, 0 } }, /* 2800K */
  { "NIKON", "D2X", "Incandescent", 3,		{ 0.83388, 1, 3.34528, 0 } }, /* 2700K */
  { "NIKON", "D2X", "Fluorescent", -3,		{ 2.01562, 1, 1.72266, 0 } }, /* ~~7200K */
  { "NIKON", "D2X", "Fluorescent", -2,		{ 1.67969, 1, 1.42578, 0 } }, /* ~~6500K */
  { "NIKON", "D2X", "Fluorescent", -1,		{ 1.42969, 1, 1.80078, 0 } }, /* ~~5000K */
  { "NIKON", "D2X", "Fluorescent", 0,		{ 1.42969, 1, 2.62891, 0 } }, /* ~~4200K */
  { "NIKON", "D2X", "Fluorescent", 1,		{ 1.13672, 1, 3.02734, 0 } }, /* ~~3700K */
  { "NIKON", "D2X", "Fluorescent", 2,		{ 0.94118, 1, 2.68498, 0 } }, /* ~~3000K */
  { "NIKON", "D2X", "Fluorescent", 3,		{ 0.83388, 1, 3.51140, 0 } }, /* ~~2700K */
  { "NIKON", "D2X", "Direct sunlight", -3,	{ 1.61328, 1, 1.61328, 0 } }, /* 5600K */
  { "NIKON", "D2X", "Direct sunlight", -2,	{ 1.57031, 1, 1.65234, 0 } }, /* ~5400K */
  { "NIKON", "D2X", "Direct sunlight", -1,	{ 1.55078, 1, 1.67578, 0 } }, /* 5300K */
  { "NIKON", "D2X", "Direct sunlight", 0,	{ 1.52734, 1, 1.69531, 0 } }, /* ~5200K */
  { "NIKON", "D2X", "Direct sunlight", 1,	{ 1.48438, 1, 1.74609, 0 } }, /* 5000K */
  { "NIKON", "D2X", "Direct sunlight", 2,	{ 1.45312, 1, 1.76953, 0 } }, /* ~4900K */
  { "NIKON", "D2X", "Direct sunlight", 3,	{ 1.42578, 1, 1.78906, 0 } }, /* 4800K */
  { "NIKON", "D2X", "Flash", -3,		{ 1.71484, 1, 1.48438, 0 } }, /* ~~6000K */
  { "NIKON", "D2X", "Flash", -2,		{ 1.67578, 1, 1.48438, 0 } }, /* ~~5800K */
  { "NIKON", "D2X", "Flash", -1,		{ 1.66797, 1, 1.50781, 0 } }, /* ~~5600K */
  { "NIKON", "D2X", "Flash", 0,			{ 1.66016, 1, 1.53125, 0 } }, /* ~5400K */
  { "NIKON", "D2X", "Flash", 1,			{ 1.64453, 1, 1.54297, 0 } }, /* ~~5200K */
  { "NIKON", "D2X", "Flash", 2,			{ 1.62891, 1, 1.54297, 0 } }, /* ~~5000K */
  { "NIKON", "D2X", "Flash", 3,			{ 1.57031, 1, 1.56641, 0 } }, /* ~~4800K */
  { "NIKON", "D2X", "Cloudy", -3,		{ 1.79297, 1, 1.46875, 0 } }, /* ~6600K */
  { "NIKON", "D2X", "Cloudy", -2,		{ 1.76172, 1, 1.49219, 0 } }, /* ~6400K */
  { "NIKON", "D2X", "Cloudy", -1,		{ 1.72656, 1, 1.51953, 0 } }, /* ~6200K */
  { "NIKON", "D2X", "Cloudy", 0,		{ 1.69141, 1, 1.54688, 0 } }, /* ~6000K */
  { "NIKON", "D2X", "Cloudy", 1,		{ 1.65234, 1, 1.57812, 0 } }, /* ~5800K */
  { "NIKON", "D2X", "Cloudy", 2,		{ 1.61328, 1, 1.61328, 0 } }, /* 5600K */
  { "NIKON", "D2X", "Cloudy", 3,		{ 1.57031, 1, 1.65234, 0 } }, /* ~5400K */
  { "NIKON", "D2X", "Shade", -3,		{ 2.10938, 1, 1.23828, 0 } }, /* ~9200K */
  { "NIKON", "D2X", "Shade", -2,		{ 2.07031, 1, 1.26562, 0 } }, /* ~8800K */
  { "NIKON", "D2X", "Shade", -1,		{ 2.02734, 1, 1.29688, 0 } }, /* ~8400K */
  { "NIKON", "D2X", "Shade", 0,			{ 1.98047, 1, 1.32812, 0 } }, /* ~8000K */
  { "NIKON", "D2X", "Shade", 1,			{ 1.92188, 1, 1.37109, 0 } }, /* ~7500K */
  { "NIKON", "D2X", "Shade", 2,			{ 1.86719, 1, 1.41406, 0 } }, /* 7100K */
  { "NIKON", "D2X", "Shade", 3,			{ 1.80859, 1, 1.45703, 0 } }, /* 6700K */

  /* D2X Kelvin presets */
  { "NIKON", "D2X", "2500K", 0,			{ 0.74203, 1, 3.67536, 0 } },
  { "NIKON", "D2X", "2550K", 0,			{ 0.76877, 1, 3.58859, 0 } },
  { "NIKON", "D2X", "2650K", 0,			{ 0.81529, 1, 3.42675, 0 } },
  { "NIKON", "D2X", "2700K", 0,			{ 0.83388, 1, 3.34528, 0 } },
  { "NIKON", "D2X", "2800K", 0,			{ 0.86486, 1, 3.17905, 0 } },
  { "NIKON", "D2X", "2850K", 0,			{ 0.87973, 1, 3.10309, 0 } },
  { "NIKON", "D2X", "2950K", 0,			{ 0.90780, 1, 2.96454, 0 } },
  { "NIKON", "D2X", "3000K", 0,			{ 0.92086, 1, 2.89928, 0 } },
  { "NIKON", "D2X", "3100K", 0,			{ 0.94465, 1, 2.77122, 0 } },
  { "NIKON", "D2X", "3200K", 0,			{ 0.96970, 1, 2.65530, 0 } },
  { "NIKON", "D2X", "3300K", 0,			{ 0.99611, 1, 2.55642, 0 } },
  { "NIKON", "D2X", "3400K", 0,			{ 1.01953, 1, 2.46484, 0 } },
  { "NIKON", "D2X", "3600K", 0,			{ 1.07422, 1, 2.34375, 0 } },
  { "NIKON", "D2X", "3700K", 0,			{ 1.09766, 1, 2.26172, 0 } },
  { "NIKON", "D2X", "3800K", 0,			{ 1.12500, 1, 2.18750, 0 } },
  { "NIKON", "D2X", "4000K", 0,			{ 1.17969, 1, 2.06250, 0 } },
  { "NIKON", "D2X", "4200K", 0,			{ 1.24219, 1, 1.96094, 0 } },
  { "NIKON", "D2X", "4300K", 0,			{ 1.27344, 1, 1.91797, 0 } },
  { "NIKON", "D2X", "4500K", 0,			{ 1.33594, 1, 1.83984, 0 } },
  { "NIKON", "D2X", "4800K", 0,			{ 1.42578, 1, 1.78906, 0 } },
  { "NIKON", "D2X", "5000K", 0,			{ 1.48438, 1, 1.74609, 0 } },
  { "NIKON", "D2X", "5300K", 0,			{ 1.55078, 1, 1.67578, 0 } },
  { "NIKON", "D2X", "5600K", 0,			{ 1.61328, 1, 1.61328, 0 } },
  { "NIKON", "D2X", "5900K", 0,			{ 1.67188, 1, 1.56250, 0 } },
  { "NIKON", "D2X", "6300K", 0,			{ 1.74219, 1, 1.50391, 0 } },
  { "NIKON", "D2X", "6700K", 0,			{ 1.80859, 1, 1.45703, 0 } },
  { "NIKON", "D2X", "7100K", 0,			{ 1.86719, 1, 1.41406, 0 } },
  { "NIKON", "D2X", "7700K", 0,			{ 1.94531, 1, 1.35547, 0 } },
  { "NIKON", "D2X", "8300K", 0,			{ 2.01562, 1, 1.30469, 0 } },
  { "NIKON", "D2X", "9100K", 0,			{ 2.09766, 1, 1.24609, 0 } },
  { "NIKON", "D2X", "10000K", 0,		{ 2.17578, 1, 1.18359, 0 } },

  /* D200 basic WB presets */
  { "NIKON", "D200", "Incandescent", 0,		{ 1.148438, 1, 2.398438, 0 } },
  { "NIKON", "D200", "Fluorescent", 0,		{ 1.664063, 1, 2.148438, 0 } },
  { "NIKON", "D200", "Sunlight", 0,		{ 1.804688, 1, 1.398438, 0 } },
  { "NIKON", "D200", "Flash", 0,		{ 2.007813, 1, 1.171875, 0 } },
  { "NIKON", "D200", "Overcast", 0,		{ 1.917969, 1, 1.261719, 0 } },
  { "NIKON", "D200", "Shade", 0,		{ 2.234375, 1, 1.125000, 0 } },

  /* D200 Kelvin presets */
  { "NIKON", "D200", "2500K", 0,		{ 1, 1, 3.121094, 0 } },
  { "NIKON", "D200", "2550K", 0,		{ 1, 1, 3.035156, 0 } },
  { "NIKON", "D200", "2650K", 0,		{ 1.011719, 1, 2.878906, 0 } },
  { "NIKON", "D200", "2700K", 0,		{ 1.031250, 1, 2.804688, 0 } },
  { "NIKON", "D200", "2800K", 0,		{ 1.074219, 1, 2.648438, 0 } },
  { "NIKON", "D200", "2850K", 0,		{ 1.089844, 1, 2.589844, 0 } },
  { "NIKON", "D200", "2950K", 0,		{ 1.132813, 1, 2.453125, 0 } },
  { "NIKON", "D200", "3000K", 0,		{ 1.148438, 1, 2.398438, 0 } },
  { "NIKON", "D200", "3100K", 0,		{ 1.183594, 1, 2.289063, 0 } },
  { "NIKON", "D200", "3200K", 0,		{ 1.218750, 1, 2.187500, 0 } },
  { "NIKON", "D200", "3300K", 0,		{ 1.250000, 1, 2.097656, 0 } },
  { "NIKON", "D200", "3400K", 0,		{ 1.281250, 1, 2.015625, 0 } },
  { "NIKON", "D200", "3600K", 0,		{ 1.343750, 1, 1.871094, 0 } },
  { "NIKON", "D200", "3700K", 0,		{ 1.371094, 1, 1.820313, 0 } },
  { "NIKON", "D200", "3800K", 0,		{ 1.402344, 1, 1.761719, 0 } },
  { "NIKON", "D200", "4000K", 0,		{ 1.457031, 1, 1.667969, 0 } },
  { "NIKON", "D200", "4200K", 0,		{ 1.511719, 1, 1.593750, 0 } },
  { "NIKON", "D200", "4300K", 0,		{ 1.535156, 1, 1.558594, 0 } },
  { "NIKON", "D200", "4500K", 0,		{ 1.589844, 1, 1.500000, 0 } },
  { "NIKON", "D200", "4800K", 0,		{ 1.687500, 1, 1.449219, 0 } },
  { "NIKON", "D200", "5000K", 0,		{ 1.746094, 1, 1.425781, 0 } },
  { "NIKON", "D200", "5300K", 0,		{ 1.820313, 1, 1.375000, 0 } },
  { "NIKON", "D200", "5600K", 0,		{ 1.863281, 1, 1.320313, 0 } },
  { "NIKON", "D200", "5900K", 0,		{ 1.902344, 1, 1.273438, 0 } },
  { "NIKON", "D200", "6300K", 0,		{ 1.972656, 1, 1.234375, 0 } },
  { "NIKON", "D200", "6700K", 0,		{ 2.046875, 1, 1.199219, 0 } },
  { "NIKON", "D200", "7100K", 0,		{ 2.105469, 1, 1.175781, 0 } },
  { "NIKON", "D200", "7700K", 0,		{ 2.191406, 1, 1.144531, 0 } },
  { "NIKON", "D200", "8300K", 0,		{ 2.277344, 1, 1.109375, 0 } },
  { "NIKON", "D200", "9300K", 0,		{ 2.367188, 1, 1.070313, 0 } },
  { "NIKON", "D200", "10000K", 0,		{ 2.453125, 1, 1.035156, 0 } },

  { "NIKON", "D50", "Incandescent", 0,		{ 1.328125, 1, 2.500000, 0 } },
  { "NIKON", "D50", "Fluorescent", 0,		{ 1.945312, 1, 2.191406, 0 } },
  { "NIKON", "D50", "Sunny", 0,			{ 2.140625, 1, 1.398438, 0 } },
  { "NIKON", "D50", "Flash", 0,			{ 2.398438, 1, 1.339844, 0 } },
  { "NIKON", "D50", "Shade", 0,			{ 2.746094, 1, 1.156250, 0 } },

  { "NIKON", "D70", "Incandescent", 0,		{ 1.343750, 1, 2.816406, 0 } }, /* 3000K */
  { "NIKON", "D70", "Fluorescent", 0,		{ 1.964844, 1, 2.476563, 0 } }, /* 4200K */
  { "NIKON", "D70", "Direct sunlight", 0,	{ 2.062500, 1, 1.597656, 0 } }, /* 5200K*/
  { "NIKON", "D70", "Flash", 0,			{ 2.441406, 1, 1.500000, 0 } }, /* 5400K */
  { "NIKON", "D70", "Cloudy", 0,		{ 2.257813, 1, 1.457031, 0 } }, /* 6000K */
  { "NIKON", "D70", "Shade", 0,			{ 2.613281, 1, 1.277344, 0 } }, /* 8000K */

  { "OLYMPUS", "E-1", "3000K Tungsten", -7,	{ 1.015625, 1, 1.867188, 0 } },
  { "OLYMPUS", "E-1", "3000K Tungsten", 0,	{ 1, 1.024000, 1.992000, 0 } },
  { "OLYMPUS", "E-1", "3000K Tungsten", 7,	{ 1, 1.075630, 2.168067, 0 } },
  { "OLYMPUS", "E-1", "3300K", -7,		{ 1.109375, 1, 1.695312, 0 } },
  { "OLYMPUS", "E-1", "3300K", 0,		{ 1.070312, 1, 1.773438, 0 } },
  { "OLYMPUS", "E-1", "3300K", 7,		{ 1.015625, 1, 1.851562, 0 } },
  { "OLYMPUS", "E-1", "3600K Incandescent", -7, { 1.195312, 1, 1.562500, 0 } },
  { "OLYMPUS", "E-1", "3600K Incandescent", 0,  { 1.156250, 1, 1.640625, 0 } },
  { "OLYMPUS", "E-1", "3600K Incandescent", 7,  { 1.101562, 1, 1.718750, 0 } },
  { "OLYMPUS", "E-1", "3900K", -7,		{ 1.335938, 1, 1.414062, 0 } },
  { "OLYMPUS", "E-1", "3900K", 0,		{ 1.234375, 1, 1.523438, 0 } },
  { "OLYMPUS", "E-1", "3900K", 7,		{ 1.179688, 1, 1.601562, 0 } },
  { "OLYMPUS", "E-1", "4000K White fluorescent", -7, { 2.296875, 1, 1.445312, 0 } },
  { "OLYMPUS", "E-1", "4000K White fluorescent", 0,  { 2.062500, 1, 1.679688, 0 } },
  { "OLYMPUS", "E-1", "4000K White fluorescent", 7,  { 1.929688, 1, 1.867188, 0 } },
  { "OLYMPUS", "E-1", "4300K", -7,		{ 1.484375, 1, 1.281250, 0 } },
  { "OLYMPUS", "E-1", "4300K", 0,		{ 1.414062, 1, 1.343750, 0 } },
  { "OLYMPUS", "E-1", "4300K", 7,		{ 1.296875, 1, 1.453125, 0 } },
  { "OLYMPUS", "E-1", "4500K Neutral fluorescent", -7, { 1.984375, 1, 1.203125, 0 } },
  { "OLYMPUS", "E-1", "4500K Neutral fluorescent", 0,  { 1.835938, 1, 1.320312, 0 } },
  { "OLYMPUS", "E-1", "4500K Neutral fluorescent", 7,  { 1.617188, 1, 1.515625, 0 } },
  { "OLYMPUS", "E-1", "4800K", -7,		{ 1.601562, 1, 1.179688, 0 } },
  { "OLYMPUS", "E-1", "4800K", 0,		{ 1.546875, 1, 1.226562, 0 } },
  { "OLYMPUS", "E-1", "4800K", 7,		{ 1.460938, 1, 1.289062, 0 } },
  { "OLYMPUS", "E-1", "5300K Clear day", -7,	{ 1.726562, 1, 1.093750, 0 } },
  { "OLYMPUS", "E-1", "5300K Clear day", 0,	{ 1.664062, 1, 1.140625, 0 } },
  { "OLYMPUS", "E-1", "5300K Clear day", 7,	{ 1.593750, 1, 1.187500, 0 } },
  { "OLYMPUS", "E-1", "6000K Cloudy (with flash)", -7, { 2.008130, 1.040650, 1, 0 } },
  { "OLYMPUS", "E-1", "6000K Cloudy (with flash)", 0,  { 1.796875, 1, 1.046875, 0 } },
  { "OLYMPUS", "E-1", "6000K Cloudy (with flash)", 7,  { 1.726562, 1, 1.093750, 0 } },
  { "OLYMPUS", "E-1", "6600K Daylight fluorescent", -7, { 2.819820, 1.153153, 1, 0 } },
  { "OLYMPUS", "E-1", "6600K Daylight fluorescent", 0,  { 2.101562, 1, 1.085938, 0 } },
  { "OLYMPUS", "E-1", "6600K Daylight fluorescent", 7,  { 1.929688, 1, 1.203125, 0 } },
  { "OLYMPUS", "E-1", "7500K Shadows", -7,	{ 2.584906, 1.207547, 1, 0 } },
  { "OLYMPUS", "E-1", "7500K Shadows", 0,	{ 2.196581, 1.094017, 1, 0 } },
  { "OLYMPUS", "E-1", "7500K Shadows", 7,	{ 1.859375, 1, 1, 0 } },

  { "OLYMPUS", "E-10", "3000K Incandescent", 0,	{ 1, 1.153153, 3.441442, 0 } },
  { "OLYMPUS", "E-10", "3700K Incandescent (mood)", 0,  { 1.101562, 1, 2.351562, 0 } },
  { "OLYMPUS", "E-10", "4000K White fluorescent", 0, { 1.460938, 1, 2.546875, 0 } },
  { "OLYMPUS", "E-10", "4500K Daylight fluorescent", 0, { 1.460938, 1, 1.843750, 0 } },
  { "OLYMPUS", "E-10", "5500K Clear day", 0,	{ 1.523438, 1, 1.617188, 0 } },
  { "OLYMPUS", "E-10", "6500K Cloudy day", 0,	{ 1.687500, 1, 1.437500, 0 } },
  { "OLYMPUS", "E-10", "7500K Shadows", 0,	{ 1.812500, 1, 1.312500, 0 } },

  { "Panasonic", "DMC-FZ30", "Sunny", 0,	{ 1.757576, 1, 1.446970, 0 } },
  { "Panasonic", "DMC-FZ30", "Cloudy", 0,	{ 1.943182, 1, 1.276515, 0 } },
  { "Panasonic", "DMC-FZ30", "Fluorescent", 0,	{ 1.098485, 1, 2.106061, 0 } },
  { "Panasonic", "DMC-FZ30", "Flash", 0,	{ 1.965909, 1, 1.303030, 0 } },

  { "PENTAX", "*ist DS", "Daylight", 0,		{ 1.632812, 1, 1, 0 } },
  { "PENTAX", "*ist DS", "Shade", 0,		{ 1.964844, 1, 1, 0 } },
  { "PENTAX", "*ist DS", "Cloudy", 0,		{ 1.761719, 1, 1, 0 } },
  { "PENTAX", "*ist DS", "Daylight colors fluorescent", 0, { 1.910156, 1, 1, 0 } },
  { "PENTAX", "*ist DS", "Daylight white fluorescent", 0, { 1.521569, 1.003922, 1, 0 } },
  { "PENTAX", "*ist DS", "White light fluorescent", 0, { 1.496094, 1, 1.023438, 0 } },
  { "PENTAX", "*ist DS", "Tungsten", 0,		{ 1, 1, 2.027344, 0 } },
  { "PENTAX", "*ist DS", "Flash", 0,		{ 1.695312, 1, 1, 0 } },

};

const int wb_preset_count = sizeof(wb_preset) / sizeof(wb_data);
