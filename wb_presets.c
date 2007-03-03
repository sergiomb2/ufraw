/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * wb_presets.c - White balance preset values for various cameras
 * Copyright 2004-2007 by Udi Fuchs
 *
 * Thanks goes for all the people who sent in the preset values
 * for their cameras.
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
#include <glib/gi18n.h>
#include "ufraw.h"

/* Column 1 - "make" of the camera.
 * Column 2 - "model" (use the "make" and "model" as provided by DCRaw).
 * Column 3 - WB name.
 * Column 4 - Fine tuning. MUST be in increasing order. 0 for no fine tuning.
 *	      It is enough to give only the extreme values, the other values
 *	      will be interpolated.
 * Column 5 - Channel multipliers.
 *
 * WB name is standardized to one of the following:
 * "Daylight" - "Sunlight" and other variation should be switched to this.
 * "Direct sunlight" - probably the same as the above.
 * "Cloudy"
 * "Shade" - "Shadows" should be switched to this.
 * "Incandescent"
 * "Incandescent warm"
 * "Tungsten" - same as "Incandescent"
 * "Fluorescent"
 * "Fluorescent high" - used only in Canon cameras.
 * "Cool white fluorescent"
 * "Warm white fluorescent"
 * "Daylight fluorescent"
 * "Neutral fluorescent"
 * "White fluorescent"
 * "Flash"
 * "Flash (auto mode)" - for Olympus with no real "Flash" preset.
 * "Evening sun"
 * "Underwater"
 * "Black & white"
 *
 * Minolta's ALPHA and MAXXUM models are treated as the DYNAX model.
 */
const wb_data wb_preset[] = {

  { "", "", N_("Manual WB"), 0,    { 0, 0, 0, 0 } },
  { "", "", N_("Camera WB"), 0,    { 0, 0, 0, 0 } },
  { "", "", N_("Auto WB"),   0,    { 0, 0, 0, 0 } },

  { "Canon", "PowerShot G2", N_("Daylight"), 0,	{ 2.011483, 1, 1.299522, 0 } },
  { "Canon", "PowerShot G2", N_("Cloudy"), 0,	{ 2.032505, 1, 1.285851, 0 } },
  { "Canon", "PowerShot G2", N_("Tungsten"), 0,	{ 1.976008, 1, 1.332054, 0 } },
  { "Canon", "PowerShot G2", N_("Fluorescent"), 0, { 2.022010, 1, 1.295694, 0 } },
  { "Canon", "PowerShot G2", N_("Fluorescent high"), 0, { 2.029637, 1, 1.286807, 0 } },
  { "Canon", "PowerShot G2", N_("Flash"), 0,	{ 2.153576, 1, 1.140680, 0 } },

  { "Canon", "PowerShot G3", N_("Daylight"), 0,	{ 1.858513, 1, 1.387290, 0 } },
  { "Canon", "PowerShot G3", N_("Cloudy"), 0,	{ 1.951132, 1, 1.305125, 0 } },
  { "Canon", "PowerShot G3", N_("Tungsten"), 0,	{ 1.128386, 1, 2.313310, 0 } },
  { "Canon", "PowerShot G3", N_("Fluorescent"), 0, { 1.715573, 1, 2.194337, 0 } },
  { "Canon", "PowerShot G3", N_("Fluorescent high"), 0, { 2.580563, 1, 1.496164, 0 } },
  { "Canon", "PowerShot G3", N_("Flash"), 0,	{ 2.293173, 1, 1.187416, 0 } },

  { "Canon", "PowerShot G5", N_("Daylight"), 0,	{ 1.639521, 1, 1.528144, 0 } },
  { "Canon", "PowerShot G5", N_("Cloudy"), 0,	{ 1.702153, 1, 1.462919, 0 } },
  { "Canon", "PowerShot G5", N_("Tungsten"), 0,	{ 1.135071, 1, 2.374408, 0 } },
  { "Canon", "PowerShot G5", N_("Fluorescent"), 0, { 1.660281, 1, 2.186462, 0 } },
  { "Canon", "PowerShot G5", N_("Fluorescent high"), 0, { 1.463297, 1, 1.764140, 0 } },
  { "Canon", "PowerShot G5", N_("Flash"), 0,	{ 1.603593, 1, 1.562874, 0 } },

  { "Canon", "PowerShot G6", N_("Daylight"), 0,	{ 1.769704, 1, 1.637931, 0 } },
  { "Canon", "PowerShot G6", N_("Cloudy"), 0,	{ 2.062731, 1, 1.442804, 0 } },
  { "Canon", "PowerShot G6", N_("Tungsten"), 0,	{ 1.077106, 1, 2.721234, 0 } },
  { "Canon", "PowerShot G6", N_("Fluorescent"), 0, { 1.914922, 1, 2.142670, 0 } },
  { "Canon", "PowerShot G6", N_("Fluorescent high"), 0, { 2.543677, 1, 1.650587, 0 } },
  { "Canon", "PowerShot G6", N_("Flash"), 0,	{ 2.285322, 1, 1.333333, 0 } },

  /* Canon PowerShot S3 IS does not support native WB presets. These are made
     as custom WB presets. */
  { "Canon", "PowerShot S3 IS", N_("Daylight"), 0,	{ 1.627271, 1, 1.823491, 0 } },
  { "Canon", "PowerShot S3 IS", N_("Cloudy"), 0,	{ 1.794382, 1, 1.618412, 0 } },
  { "Canon", "PowerShot S3 IS", N_("Tungsten"), 0,	{ 1, 1.192243, 4.546950, 0 } },
  { "Canon", "PowerShot S3 IS", N_("Flash"), 0,	{ 1.884691, 1, 1.553869, 0 } },

  { "Canon", "PowerShot S30", N_("Daylight"), 0, { 1.741088, 1, 1.318949, 0 } },
  { "Canon", "PowerShot S30", N_("Cloudy"), 0,	{ 1.766635, 1, 1.298969, 0 } },
  { "Canon", "PowerShot S30", N_("Tungsten"), 0, { 1.498106, 1, 1.576705, 0 } },
  { "Canon", "PowerShot S30", N_("Fluorescent"), 0, { 1.660075, 1, 1.394539, 0 } },
  { "Canon", "PowerShot S30", N_("Fluorescent high"), 0, { 1.753515, 1, 1.306467, 0 } },
  { "Canon", "PowerShot S30", N_("Flash"), 0,	{ 2.141705 , 1, 1.097926, 0 } },

  { "Canon", "PowerShot S45", N_("Daylight"), 0, { 2.325175, 1, 1.080420, 0 } },
  { "Canon", "PowerShot S45", N_("Cloudy"), 0,	{ 2.145047, 1, 1.173349, 0 } },
  { "Canon", "PowerShot S45", N_("Tungsten"), 0, { 1.213018, 1, 2.087574, 0 } },
  { "Canon", "PowerShot S45", N_("Fluorescent"), 0, { 1.888183, 1, 1.822109, 0 } },
  { "Canon", "PowerShot S45", N_("Fluorescent high"), 0, { 2.964422, 1, 1.354511, 0 } },
  { "Canon", "PowerShot S45", N_("Flash"), 0,	{ 2.534884, 1, 1.065663, 0 } },

  { "Canon", "PowerShot S50", N_("Daylight"), 0, { 1.772506, 1, 1.536496, 0 } },
  { "Canon", "PowerShot S50", N_("Cloudy"), 0,	{ 1.831311, 1, 1.484223, 0 } },
  { "Canon", "PowerShot S50", N_("Tungsten"), 0, { 1.185542, 1, 2.480723, 0 } },
  { "Canon", "PowerShot S50", N_("Fluorescent"), 0, { 1.706410, 1, 2.160256, 0 } },
  { "Canon", "PowerShot S50", N_("Fluorescent high"), 0, { 1.562500, 1, 1.817402, 0 } },
  { "Canon", "PowerShot S50", N_("Flash"), 0,	{ 1.776156, 1, 1.531630, 0 } },

  { "Canon", "PowerShot S60", N_("Daylight"), 0, { 1.742169, 1, 1.486747, 0 } },
  { "Canon", "PowerShot S60", N_("Cloudy"), 0,	{ 1.881437, 1, 1.366467, 0 } },
  { "Canon", "PowerShot S60", N_("Tungsten"), 0, { 1.144910, 1, 2.566467, 0 } },
  { "Canon", "PowerShot S60", N_("Fluorescent"), 0, { 1.714650, 1, 2.053503, 0 } },
  { "Canon", "PowerShot S60", N_("Fluorescent high"), 0, { 2.849655, 1, 2.067586, 0 } },
  { "Canon", "PowerShot S60", N_("Flash"), 0,	{ 2.143229, 1, 1.190104, 0 } },
  { "Canon", "PowerShot S60", N_("Underwater"), 0,  { 2.677109, 1, 1.148193, 0 } },

  { "Canon", "PowerShot S70", N_("Daylight"), 0, { 1.943834, 1, 1.456654, 0 } },
  { "Canon", "PowerShot S70", N_("Cloudy"), 0,	{ 2.049939, 1, 1.382460, 0 } },
  { "Canon", "PowerShot S70", N_("Tungsten"), 0, { 1.169492, 1, 2.654964, 0 } },
  { "Canon", "PowerShot S70", N_("Fluorescent"), 0, { 1.993456, 1, 2.056283, 0 } },
  { "Canon", "PowerShot S70", N_("Fluorescent high"), 0, { 2.645914, 1, 1.565499, 0 } },
  { "Canon", "PowerShot S70", N_("Flash"), 0,	{ 2.389189, 1, 1.147297, 0 } },
  { "Canon", "PowerShot S70", N_("Underwater"), 0,  { 3.110565, 1, 1.162162, 0 } },

  { "Canon", "PowerShot Pro1", N_("Daylight"), 0, { 1.829238, 1, 1.571253, 0 } },
  { "Canon", "PowerShot Pro1", N_("Cloudy"), 0,	{ 1.194139, 1, 2.755800, 0 } },
  { "Canon", "PowerShot Pro1", N_("Tungsten"), 0, { 1.701416, 1, 2.218790, 0 } },
  { "Canon", "PowerShot Pro1", N_("Fluorescent"), 0, { 2.014066, 1, 1.776215, 0 } },
  { "Canon", "PowerShot Pro1", N_("Fluorescent high"), 0, { 2.248663, 1, 1.227273, 0 } },
  { "Canon", "PowerShot Pro1", N_("Flash"), 0,	{ 2.130081, 1, 1.422764, 0 } },

  { "Canon", "EOS D60", N_("Daylight"), 0,	{ 2.472594, 1, 1.225335, 0 } },
  { "Canon", "EOS D60", N_("Cloudy"), 0,	{ 2.723926, 1, 1.137423, 0 } },
  { "Canon", "EOS D60", N_("Tungsten"), 0,	{ 1.543054, 1, 1.907003, 0 } },
  { "Canon", "EOS D60", N_("Fluorescent"), 0,	{ 1.957346, 1, 1.662322, 0 } },
  { "Canon", "EOS D60", N_("Flash"), 0,		{ 2.829840, 1, 1.108508, 0 } },

  { "Canon", "EOS 5D", N_("Flash"), 0,		{ 2.211914, 1, 1.260742, 0 } }, /*6550K*/
  { "Canon", "EOS 5D", N_("Fluorescent"), 0,	{ 1.726054, 1, 2.088123, 0 } }, /*3850K*/
  { "Canon", "EOS 5D", N_("Tungsten"), 0,	{ 1.373285, 1, 2.301006, 0 } }, /*3250K*/
  { "Canon", "EOS 5D", N_("Cloudy"), 0,		{ 2.151367, 1, 1.321289, 0 } }, /*6100K*/
  { "Canon", "EOS 5D", N_("Shade"), 0,		{ 2.300781, 1, 1.208008, 0 } }, /*7200K*/
  { "Canon", "EOS 5D", N_("Daylight"), 0,	{ 1.988281, 1, 1.457031, 0 } }, /*5250K*/

  { "Canon", "EOS 10D", N_("Daylight"), 0,	{ 2.159856, 1, 1.218750, 0 } },
  { "Canon", "EOS 10D", N_("Shade"), 0,		{ 2.533654, 1, 1.036058, 0 } },
  { "Canon", "EOS 10D", N_("Cloudy"), 0,	{ 2.348558, 1, 1.116587, 0 } },
  { "Canon", "EOS 10D", N_("Tungsten"), 0,	{ 1.431544, 1, 1.851040, 0 } },
  { "Canon", "EOS 10D", N_("Fluorescent"), 0,	{ 1.891509, 1, 1.647406, 0 } },
  { "Canon", "EOS 10D", N_("Flash"), 0,		{ 2.385817, 1, 1.115385, 0 } },

  { "Canon", "EOS 20D", N_("Daylight"), 0,	{ 1.954680, 1, 1.478818, 0 } },
  { "Canon", "EOS 20D", N_("Shade"), 0,		{ 2.248276, 1, 1.227586, 0 } },
  { "Canon", "EOS 20D", N_("Cloudy"), 0,	{ 2.115271, 1, 1.336946, 0 } },
  { "Canon", "EOS 20D", N_("Tungsten"), 0,	{ 1.368087, 1, 2.417044, 0 } },
  { "Canon", "EOS 20D", N_("Fluorescent"), 0,	{ 1.752709, 1, 2.060098, 0 } },
  { "Canon", "EOS 20D", N_("Flash"), 0,		{ 2.145813, 1, 1.293596, 0 } },

  { "Canon", "EOS 30D", N_("Direct sunlight"), 0, { 1.920898, 1, 1.514648, 0 } },
  { "Canon", "EOS 30D", N_("Shade"), 0,		{ 2.211914, 1, 1.251953, 0 } },
  { "Canon", "EOS 30D", N_("Cloudy"), 0,	{ 2.073242, 1, 1.373047, 0 } },
  { "Canon", "EOS 30D", N_("Tungsten"), 0,	{ 1.340369, 1, 2.383465, 0 } },
  { "Canon", "EOS 30D", N_("Fluorescent"), 0,	{ 1.684063, 1, 2.245107, 0 } },
  { "Canon", "EOS 30D", N_("Flash"), 0,		{ 2.093750, 1, 1.330078, 0 } },

  { "Canon", "EOS 300D DIGITAL", N_("Daylight"), 0, { 2.13702, 1, 1.15745, 0 } },
  { "Canon", "EOS 300D DIGITAL", N_("Cloudy"), 0, { 2.50961, 1, 0.97716, 0 } },
  { "Canon", "EOS 300D DIGITAL", N_("Tungsten"), 0, { 2.32091, 1, 1.05529, 0 } },
  { "Canon", "EOS 300D DIGITAL", N_("Fluorescent"), 0, { 1.39677, 1, 1.79892, 0 } },
  { "Canon", "EOS 300D DIGITAL", N_("Flash"), 0, { 1.84229, 1, 1.60573, 0 } },
  { "Canon", "EOS 300D DIGITAL", N_("Shade"), 0, { 2.13702, 1, 1.15745, 0 } },

  { "Canon", "EOS DIGITAL REBEL", N_("Daylight"), 0, { 2.13702, 1, 1.15745, 0 } },
  { "Canon", "EOS DIGITAL REBEL", N_("Cloudy"), 0, { 2.50961, 1, 0.97716, 0 } },
  { "Canon", "EOS DIGITAL REBEL", N_("Tungsten"), 0, { 2.32091, 1, 1.05529, 0 } },
  { "Canon", "EOS DIGITAL REBEL", N_("Fluorescent"), 0, { 1.39677, 1, 1.79892, 0 } },
  { "Canon", "EOS DIGITAL REBEL", N_("Flash"), 0, { 1.84229, 1, 1.60573, 0 } },
  { "Canon", "EOS DIGITAL REBEL", N_("Shade"), 0, { 2.13702, 1, 1.15745, 0 } },

  { "Canon", "EOS Kiss Digital", N_("Daylight"), 0, { 2.13702, 1, 1.15745, 0 } },
  { "Canon", "EOS Kiss Digital", N_("Cloudy"), 0, { 2.50961, 1, 0.97716, 0 } },
  { "Canon", "EOS Kiss Digital", N_("Tungsten"), 0, { 2.32091, 1, 1.05529, 0 } },
  { "Canon", "EOS Kiss Digital", N_("Fluorescent"), 0, { 1.39677, 1, 1.79892, 0 } },
  { "Canon", "EOS Kiss Digital", N_("Flash"), 0, { 1.84229, 1, 1.60573, 0 } },
  { "Canon", "EOS Kiss Digital", N_("Shade"), 0, { 2.13702, 1, 1.15745, 0 } },

  { "Canon", "EOS 350D DIGITAL", N_("Tungsten"), 0, { 1.554250, 1, 2.377034, 0 } },
  { "Canon", "EOS 350D DIGITAL", N_("Daylight"), 0, { 2.392927, 1, 1.487230, 0 } },
  { "Canon", "EOS 350D DIGITAL", N_("Fluorescent"), 0, { 1.999040, 1, 1.995202, 0 } },
  { "Canon", "EOS 350D DIGITAL", N_("Shade"), 0, { 2.827112, 1, 1.235756, 0 } },
  { "Canon", "EOS 350D DIGITAL", N_("Flash"), 0, { 2.715128, 1, 1.295678, 0 } },
  { "Canon", "EOS 350D DIGITAL", N_("Cloudy"), 0, { 2.611984, 1, 1.343811, 0 } },

  { "Canon", "EOS DIGITAL REBEL XT", N_("Tungsten"), 0, { 1.554250, 1, 2.377034, 0 } },
  { "Canon", "EOS DIGITAL REBEL XT", N_("Daylight"), 0, { 2.392927, 1, 1.487230, 0 } },
  { "Canon", "EOS DIGITAL REBEL XT", N_("Fluorescent"), 0, { 1.999040, 1, 1.995202, 0 } },
  { "Canon", "EOS DIGITAL REBEL XT", N_("Shade"), 0, { 2.827112, 1, 1.235756, 0 } },
  { "Canon", "EOS DIGITAL REBEL XT", N_("Flash"), 0, { 2.715128, 1, 1.295678, 0 } },
  { "Canon", "EOS DIGITAL REBEL XT", N_("Cloudy"), 0, { 2.611984, 1, 1.343811, 0 } },

  { "Canon", "EOS Kiss Digital N", N_("Tungsten"), 0, { 1.554250, 1, 2.377034, 0 } },
  { "Canon", "EOS Kiss Digital N", N_("Daylight"), 0, { 2.392927, 1, 1.487230, 0 } },
  { "Canon", "EOS Kiss Digital N", N_("Fluorescent"), 0, { 1.999040, 1, 1.995202, 0 } },
  { "Canon", "EOS Kiss Digital N", N_("Shade"), 0, { 2.827112, 1, 1.235756, 0 } },
  { "Canon", "EOS Kiss Digital N", N_("Flash"), 0, { 2.715128, 1, 1.295678, 0 } },
  { "Canon", "EOS Kiss Digital N", N_("Cloudy"), 0, { 2.611984, 1, 1.343811, 0 } },

  { "Canon", "EOS 400D DIGITAL", N_("Daylight"), 0, { 2.207031, 1, 1.516602, 0 } },
  { "Canon", "EOS 400D DIGITAL", N_("Shade"), 0, { 2.592773, 1, 1.250000, 0 } },
  { "Canon", "EOS 400D DIGITAL", N_("Cloudy"), 0, { 2.403320, 1, 1.371094, 0 } },
  { "Canon", "EOS 400D DIGITAL", N_("Incandescent"), 0, { 1.472578, 1, 2.431444, 0 } },
  { "Canon", "EOS 400D DIGITAL", N_("Fluorescent"), 0, { 1.842459, 1, 2.164265, 0 } },
  { "Canon", "EOS 400D DIGITAL", N_("Flash"), 0, { 2.456055, 1, 1.339844, 0 } },

  { "Canon", "EOS DIGITAL REBEL XTi", N_("Daylight"), 0, { 2.207031, 1, 1.516602, 0 } },
  { "Canon", "EOS DIGITAL REBEL XTi", N_("Shade"), 0, { 2.592773, 1, 1.250000, 0 } },
  { "Canon", "EOS DIGITAL REBEL XTi", N_("Cloudy"), 0, { 2.403320, 1, 1.371094, 0 } },
  { "Canon", "EOS DIGITAL REBEL XTi", N_("Incandescent"), 0, { 1.472578, 1, 2.431444, 0 } },
  { "Canon", "EOS DIGITAL REBEL XTi", N_("Fluorescent"), 0, { 1.842459, 1, 2.164265, 0 } },
  { "Canon", "EOS DIGITAL REBEL XTi", N_("Flash"), 0, { 2.456055, 1, 1.339844, 0 } },

  { "Canon", "EOS Kiss Digital X", N_("Daylight"), 0, { 2.207031, 1, 1.516602, 0 } },
  { "Canon", "EOS Kiss Digital X", N_("Shade"), 0, { 2.592773, 1, 1.250000, 0 } },
  { "Canon", "EOS Kiss Digital X", N_("Cloudy"), 0, { 2.403320, 1, 1.371094, 0 } },
  { "Canon", "EOS Kiss Digital X", N_("Incandescent"), 0, { 1.472578, 1, 2.431444, 0 } },
  { "Canon", "EOS Kiss Digital X", N_("Fluorescent"), 0, { 1.842459, 1, 2.164265, 0 } },
  { "Canon", "EOS Kiss Digital X", N_("Flash"), 0, { 2.456055, 1, 1.339844, 0 } },

  { "Canon", "EOS-1D Mark II", N_("Cloudy"), 0,	{ 2.093750, 1, 1.166016, 0 } },
  { "Canon", "EOS-1D Mark II", N_("Daylight"), 0, { 1.957031, 1, 1.295898, 0 } },
  { "Canon", "EOS-1D Mark II", N_("Flash"), 0,	{ 2.225586, 1, 1.172852, 0 } },
  { "Canon", "EOS-1D Mark II", N_("Fluorescent"), 0, { 1.785853, 1, 1.785853, 0 } },
  { "Canon", "EOS-1D Mark II", N_("Shade"), 0,	{ 2.220703, 1, 1.069336, 0 } },
  { "Canon", "EOS-1D Mark II", N_("Tungsten"), 0, { 1.415480, 1, 2.160142, 0 } },

  { "FUJIFILM", "FinePix E900", N_("Daylight"), 0, { 1.571875, 1, 1.128125, 0 } },
  { "FUJIFILM", "FinePix E900", N_("Shade"), 0, { 1.668750, 1, 1.006250, 0 } },
  { "FUJIFILM", "FinePix E900", N_("Daylight fluorescent"), 0, { 1.907609, 1, 1.016304, 0 } },
  { "FUJIFILM", "FinePix E900", N_("Warm white fluorescent"), 0, { 1.654891, 1, 1.241848, 0 } },
  { "FUJIFILM", "FinePix E900", N_("Cool white fluorescent"), 0, { 1.554348, 1, 1.519022, 0 } },
  { "FUJIFILM", "FinePix E900", N_("Incandescent"), 0, { 1.037611, 1, 1.842920, 0 } },

  { "FUJIFILM", "FinePix F700", N_("Daylight"), 0, { 1.725000, 1, 1.500000, 0 } },
  { "FUJIFILM", "FinePix F700", N_("Shade"), 0,	{ 1.950000, 1, 1.325000, 0 } },
  { "FUJIFILM", "FinePix F700", N_("Daylight fluorescent"), 0, { 2.032609, 1, 1.336957, 0 } },
  { "FUJIFILM", "FinePix F700", N_("Warm white fluorescent"), 0, { 1.706522, 1, 1.663043, 0 } },
  { "FUJIFILM", "FinePix F700", N_("Cool white fluorescent"), 0, { 1.684783, 1, 2.152174, 0 } },
  { "FUJIFILM", "FinePix F700", N_("Incandescent"), 0, { 1.168142, 1, 2.477876, 0 } },

  { "FUJIFILM", "FinePix S20Pro", N_("Daylight"), 0, { 1.712500, 1, 1.500000, 0 } },
  { "FUJIFILM", "FinePix S20Pro", N_("Cloudy"), 0, { 1.887500, 1, 1.262500, 0 } },
  { "FUJIFILM", "FinePix S20Pro", N_("Daylight fluorescent"), 0, { 2.097826, 1, 1.304348, 0 } },
  { "FUJIFILM", "FinePix S20Pro", N_("Warm white fluorescent"), 0, { 1.782609, 1, 1.619565, 0 } },
  { "FUJIFILM", "FinePix S20Pro", N_("Cool white fluorescent"), 0, { 1.670213, 1, 2.063830, 0 } },
  { "FUJIFILM", "FinePix S20Pro", N_("Incandescent"), 0, { 1.069565, 1, 2.486957 } },

  { "FUJIFILM", "FinePix S2Pro", N_("Daylight"), 0, { 1.509804, 1, 1.401961, 0 } },
  { "FUJIFILM", "FinePix S2Pro", N_("Cloudy"), 0, { 1.666667, 1, 1.166667, 0 } },
  { "FUJIFILM", "FinePix S2Pro", N_("Flash"), 0, { 1, 1.014084, 2.542253, 0 } },
  { "FUJIFILM", "FinePix S2Pro", N_("Daylight fluorescent"), 0,	{ 1.948718, 1, 1.230769, 0 } },
  { "FUJIFILM", "FinePix S2Pro", N_("Warm white fluorescent"), 0, { 1.675214, 1, 1.572650, 0 } },
  { "FUJIFILM", "FinePix S2Pro", N_("Cool white fluorescent"), 0, { 1.649573, 1, 2.094017, 0 } },

  { "FUJIFILM", "FinePix S5000", N_("Incandescent"), 0, { 1.212081, 1, 2.672364, 0 } },
  { "FUJIFILM", "FinePix S5000", N_("Fluorescent"), 0, { 1.772316, 1, 2.349902, 0 } },
  { "FUJIFILM", "FinePix S5000", N_("Daylight"), 0, { 1.860403, 1, 1.515946, 0 } },
  { "FUJIFILM", "FinePix S5000", N_("Flash"), 0, { 2.202181, 1, 1.423284, 0 } },
  { "FUJIFILM", "FinePix S5000", N_("Cloudy"), 0, { 2.036578, 1, 1.382513, 0 } },
  { "FUJIFILM", "FinePix S5000", N_("Shade"), 0, { 2.357215, 1, 1.212016, 0 } },

  { "FUJIFILM", "FinePix S5500", N_("Daylight"), 0, { 1.712500, 1, 1.550000, 0 } },
  { "FUJIFILM", "FinePix S5500", N_("Shade"), 0, { 1.912500, 1, 1.375000, 0 } },
  { "FUJIFILM", "FinePix S5500", N_("Daylight fluorescent"), 0, { 1.978261, 1, 1.380435, 0 } },
  { "FUJIFILM", "FinePix S5500", N_("Warm white fluorescent"), 0, { 1.673913, 1, 1.673913, 0 } },
  { "FUJIFILM", "FinePix S5500", N_("Cool white fluorescent"), 0, { 1.663043, 1, 2.163043, 0 } },
  { "FUJIFILM", "FinePix S5500", N_("Incandescent"), 0, { 1.115044, 1, 2.566372, 0 } },

  { "FUJIFILM", "FinePix S5600", N_("Daylight"), 0, { 1.587500, 1, 1.381250, 0 } },
  { "FUJIFILM", "FinePix S5600", N_("Shade"), 0, { 1.946875, 1, 1.175000, 0 } },
  { "FUJIFILM", "FinePix S5600", N_("Daylight fluorescent"), 0, { 1.948370, 1, 1.187500, 0 } },
  { "FUJIFILM", "FinePix S5600", N_("Warm white fluorescent"), 0, { 1.682065, 1, 1.437500, 0 } },
  { "FUJIFILM", "FinePix S5600", N_("Cool white fluorescent"), 0, { 1.595109, 1, 1.839674, 0 } },
  { "FUJIFILM", "FinePix S5600", N_("Incandescent"), 0, { 1.077434, 1, 2.170354, 0 } },

  { "FUJIFILM", "FinePix S7000", N_("Daylight"), 0, { 1.900000, 1, 1.525000, 0 } },
  { "FUJIFILM", "FinePix S7000", N_("Shade"), 0, { 2.137500, 1, 1.350000, 0 } },
  { "FUJIFILM", "FinePix S7000", N_("Daylight fluorescent"), 0, { 2.315217, 1, 1.347826, 0 } },
  { "FUJIFILM", "FinePix S7000", N_("Warm white fluorescent"), 0, { 1.902174, 1, 1.663043, 0 } },
  { "FUJIFILM", "FinePix S7000", N_("Cool white fluorescent"), 0, { 1.836957, 1, 2.130435, 0 } },
  { "FUJIFILM", "FinePix S7000", N_("Incandescent"), 0, { 1.221239, 1, 2.548673, 0 } },

  { "FUJIFILM", "FinePix S9500", N_("Daylight"), 0, { 1.618750, 1, 1.231250, 0 } },
  { "FUJIFILM", "FinePix S9500", N_("Cloudy"), 0, { 1.700000, 1, 1.046875, 0 } },
  { "FUJIFILM", "FinePix S9500", N_("Daylight fluorescent"), 0, { 1.902174, 1, 1.057065, 0 } },
  { "FUJIFILM", "FinePix S9500", N_("Warm white fluorescent"), 0, { 1.633152, 1, 1.293478, 0 } },
  { "FUJIFILM", "FinePix S9500", N_("Cool white fluorescent"), 0, { 1.546196, 1, 1.622283, 0 } },
  { "FUJIFILM", "FinePix S9500", N_("Incandescent"), 0, { 1.064159, 1, 1.960177, 0 } },

  { "KODAK", "P850 ZOOM", N_("Daylight"), 0,	{ 1.859375, 1, 1.566406, 0 } },
  { "KODAK", "P850 ZOOM", N_("Cloudy"), 0,	{ 1.960938, 1, 1.570313, 0 } },
  { "KODAK", "P850 ZOOM", N_("Shade"), 0,	{ 2.027344, 1, 1.519531, 0 } },
  { "KODAK", "P850 ZOOM", N_("Evening sun"), 0,	{ 1.679688, 1, 1.812500, 0 } },
  { "KODAK", "P850 ZOOM", N_("Tungsten"), 0,	{ 1.140625, 1, 2.726563, 0 } },
  { "KODAK", "P850 ZOOM", N_("Fluorescent"), 0,	{ 1.113281, 1, 2.949219, 0 } },

  { "Leica Camera AG", "R8 - Digital Back DMR DNG", N_("Incandescent"), 0, { 1, 1.109985, 2.430664, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", N_("Fluorescent"), 0, { 1.234985, 1, 1.791138, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", N_("Daylight"), 0, { 1.459961, 1, 1.184937, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", N_("Flash"), 0, { 1.395020, 1, 1.144897, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", N_("Cloudy"), 0, { 1.541992, 1, 1.052856, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", N_("Shade"), 0, { 1.644897, 1.033936, 1, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "2600K", 0, { 1, 1.220825, 2.999390, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "2700K", 0, { 1, 1.172607, 2.747192, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "2800K", 0, { 1, 1.129639, 2.527710, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "2900K", 0, { 1, 1.088867, 2.333130, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "3000K", 0, { 1, 1.049438, 2.156494, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "3100K", 0, { 1, 1.015503, 2.008423, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "3200K", 0, { 1.008789, 1, 1.904663, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "3300K", 0, { 1.032349, 1, 1.841187, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "3400K", 0, { 1.056763, 1, 1.780273, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "3500K", 0, { 1.081543, 1, 1.723755, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "3600K", 0, { 1.105591, 1, 1.673828, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "3700K", 0, { 1.128052, 1, 1.625732, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "3800K", 0, { 1.149536, 1, 1.580688, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "3900K", 0, { 1.170532, 1, 1.540527, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "4000K", 0, { 1.191040, 1, 1.504150, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "4100K", 0, { 1.209106, 1, 1.466919, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "4200K", 0, { 1.226807, 1, 1.433228, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "4300K", 0, { 1.244019, 1, 1.402466, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "4400K", 0, { 1.261108, 1, 1.374268, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "4500K", 0, { 1.276611, 1, 1.346924, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "4600K", 0, { 1.290771, 1, 1.320435, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "4700K", 0, { 1.304565, 1, 1.295898, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "4800K", 0, { 1.318115, 1, 1.273315, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "4900K", 0, { 1.331543, 1, 1.252441, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "5000K", 0, { 1.344360, 1, 1.233032, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "5200K", 0, { 1.365479, 1, 1.193970, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "5400K", 0, { 1.385498, 1, 1.160034, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "5600K", 0, { 1.404663, 1, 1.130127, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "5800K", 0, { 1.421387, 1, 1.102661, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "6000K", 0, { 1.435303, 1, 1.076782, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "6200K", 0, { 1.448608, 1, 1.053833, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "6400K", 0, { 1.461304, 1, 1.032959, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "6600K", 0, { 1.473511, 1, 1.014160, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "6800K", 0, { 1.488647, 1.003906, 1, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "7000K", 0, { 1.522705, 1.021118, 1, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "7200K", 0, { 1.555176, 1.037476, 1, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "7400K", 0, { 1.586182, 1.052979, 1, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "7600K", 0, { 1.615967, 1.067627, 1, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "7800K", 0, { 1.644409, 1.081665, 1, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "8000K", 0, { 1.671875, 1.094849, 1, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "8300K", 0, { 1.708740, 1.114624, 1, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "8600K", 0, { 1.743286, 1.133057, 1, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "8900K", 0, { 1.775879, 1.150269, 1.000000, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "9200K", 0, { 1.806274, 1.166382, 1.000000, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "9500K", 0, { 1.835449, 1.181519, 1.000000, 0 } },
  { "Leica Camera AG", "R8 - Digital Back DMR DNG", "9800K", 0, { 1.862793, 1.195801, 1.000000, 0 } },

  { "Leica Camera AG", "R9 - Digital Back DMR DNG", N_("Incandescent"), 0, { 1, 1.109985, 2.430664, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", N_("Fluorescent"), 0, { 1.234985, 1, 1.791138, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", N_("Daylight"), 0, { 1.459961, 1, 1.184937, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", N_("Flash"), 0, { 1.395020, 1, 1.144897, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", N_("Cloudy"), 0, { 1.541992, 1, 1.052856, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", N_("Shade"), 0, { 1.644897, 1.033936, 1, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "2600K", 0, { 1, 1.220825, 2.999390, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "2700K", 0, { 1, 1.172607, 2.747192, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "2800K", 0, { 1, 1.129639, 2.527710, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "2900K", 0, { 1, 1.088867, 2.333130, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "3000K", 0, { 1, 1.049438, 2.156494, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "3100K", 0, { 1, 1.015503, 2.008423, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "3200K", 0, { 1.008789, 1, 1.904663, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "3300K", 0, { 1.032349, 1, 1.841187, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "3400K", 0, { 1.056763, 1, 1.780273, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "3500K", 0, { 1.081543, 1, 1.723755, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "3600K", 0, { 1.105591, 1, 1.673828, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "3700K", 0, { 1.128052, 1, 1.625732, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "3800K", 0, { 1.149536, 1, 1.580688, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "3900K", 0, { 1.170532, 1, 1.540527, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "4000K", 0, { 1.191040, 1, 1.504150, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "4100K", 0, { 1.209106, 1, 1.466919, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "4200K", 0, { 1.226807, 1, 1.433228, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "4300K", 0, { 1.244019, 1, 1.402466, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "4400K", 0, { 1.261108, 1, 1.374268, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "4500K", 0, { 1.276611, 1, 1.346924, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "4600K", 0, { 1.290771, 1, 1.320435, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "4700K", 0, { 1.304565, 1, 1.295898, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "4800K", 0, { 1.318115, 1, 1.273315, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "4900K", 0, { 1.331543, 1, 1.252441, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "5000K", 0, { 1.344360, 1, 1.233032, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "5200K", 0, { 1.365479, 1, 1.193970, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "5400K", 0, { 1.385498, 1, 1.160034, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "5600K", 0, { 1.404663, 1, 1.130127, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "5800K", 0, { 1.421387, 1, 1.102661, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "6000K", 0, { 1.435303, 1, 1.076782, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "6200K", 0, { 1.448608, 1, 1.053833, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "6400K", 0, { 1.461304, 1, 1.032959, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "6600K", 0, { 1.473511, 1, 1.014160, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "6800K", 0, { 1.488647, 1.003906, 1, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "7000K", 0, { 1.522705, 1.021118, 1, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "7200K", 0, { 1.555176, 1.037476, 1, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "7400K", 0, { 1.586182, 1.052979, 1, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "7600K", 0, { 1.615967, 1.067627, 1, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "7800K", 0, { 1.644409, 1.081665, 1, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "8000K", 0, { 1.671875, 1.094849, 1, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "8300K", 0, { 1.708740, 1.114624, 1, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "8600K", 0, { 1.743286, 1.133057, 1, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "8900K", 0, { 1.775879, 1.150269, 1.000000, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "9200K", 0, { 1.806274, 1.166382, 1.000000, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "9500K", 0, { 1.835449, 1.181519, 1.000000, 0 } },
  { "Leica Camera AG", "R9 - Digital Back DMR DNG", "9800K", 0, { 1.862793, 1.195801, 1.000000, 0 } },

  { "LEICA", "DIGILUX 2", N_("Daylight"), 0,	{ 1.628906, 1, 1.488281, 0 } },
  { "LEICA", "DIGILUX 2", N_("Cloudy"), 0,	{ 1.835938, 1, 1.343750, 0 } },
  { "LEICA", "DIGILUX 2", N_("Incandescent"), 0, { 1.078125, 1, 2.203125, 0 } },
  { "LEICA", "DIGILUX 2", N_("Flash"), 0,	{ 2.074219, 1, 1.304688, 0 } },
  { "LEICA", "DIGILUX 2", N_("Black & white"), 0, { 1.632812, 1, 1.550781, 0 } },

  { "Minolta", "DiMAGE 5", N_("Daylight"), 0,	{ 2.023438, 1, 1.371094, 0 } },
  { "Minolta", "DiMAGE 5", N_("Incandescent"), 0, { 1.113281, 1, 2.480469, 0 } },
  { "Minolta", "DiMAGE 5", N_("Fluorescent"), 0, { 1.957031, 1, 2.058594, 0 } },
  { "Minolta", "DiMAGE 5", N_("Cloudy"), 0,	{ 2.199219, 1, 1.300781, 0 } },

  { "Minolta", "DiMAGE 7", N_("Cloudy"), 0,	{ 2.082031, 1, 1.226562, 0 } },
  { "Minolta", "DiMAGE 7", N_("Daylight"), 0,	{ 1.914062, 1, 1.527344, 0 } },
  { "Minolta", "DiMAGE 7", N_("Fluorescent"), 0, { 1.917969, 1, 2.007812, 0 } },
  { "Minolta", "DiMAGE 7", N_("Tungsten"), 0,	{ 1.050781, 1, 2.437500, 0 } },

  { "Minolta", "DiMAGE 7i", N_("Daylight"), 0,	{ 1.441406, 1, 1.457031, 0 } },
  { "Minolta", "DiMAGE 7i", N_("Tungsten"), 0,	{ 1, 1.333333, 3.572917, 0 } },
  { "Minolta", "DiMAGE 7i", N_("Fluorescent"), 0, { 1.554688, 1, 2.230469, 0 } },
  { "Minolta", "DiMAGE 7i", N_("Cloudy"), 0,	{ 1.550781, 1, 1.402344, 0 } },

  { "Minolta", "DiMAGE 7Hi", N_("Daylight"), 0,	{ 1.609375, 1, 1.328125, 0 } },  /*5500K*/
  { "Minolta", "DiMAGE 7Hi", N_("Tungsten"), 0,	{ 1, 1.137778, 2.768889, 0 } },  /*2800K*/
  { "Minolta", "DiMAGE 7Hi", N_("White fluorescent"), 0, { 1.664062, 1, 2.105469, 0 } },  /*4060K*/
  { "Minolta", "DiMAGE 7Hi", N_("Cool white fluorescent"), 0, { 1.796875, 1, 1.734375, 0 } },  /*4938K*/
  { "Minolta", "DiMAGE 7Hi", N_("Cloudy"), 0,	{ 1.730469, 1, 1.269531, 0 } },  /*5823K*/

  { "Minolta", "DiMAGE A1", N_("Daylight"), 0,	{ 1.808594, 1, 1.304688, 0 } },
  { "Minolta", "DiMAGE A1", N_("Tungsten"), 0,	{ 1.062500, 1, 2.675781, 0 } },
  { "Minolta", "DiMAGE A1", N_("Fluorescent"), 0, { 1.707031, 1, 2.039063, 0 } },
  { "Minolta", "DiMAGE A1", N_("Cloudy"), 0,	{ 1.960938, 1, 1.339844, 0 } },
  { "Minolta", "DiMAGE A1", N_("Shade"), 0,	{ 2.253906, 1, 1.199219, 0 } },
  { "Minolta", "DiMAGE A1", N_("Shade"), 2,	{ 2.000000, 1, 1.183594, 0 } },
  { "Minolta", "DiMAGE A1", N_("Flash"), 0,	{ 1.972656, 1, 1.265625, 0 } },

  { "Minolta", "DiMAGE A2", N_("Cloudy"), -3,	{ 2.109375, 1, 1.578125, 0 } },
  { "Minolta", "DiMAGE A2", N_("Cloudy"), 0,	{ 2.203125, 1, 1.296875, 0 } },
  { "Minolta", "DiMAGE A2", N_("Cloudy"), 3,	{ 2.296875, 1, 1.015625, 0 } },
  { "Minolta", "DiMAGE A2", N_("Daylight"), -3,	{ 1.867188, 1, 1.683594, 0 } },
  { "Minolta", "DiMAGE A2", N_("Daylight"), 0,	{ 1.960938, 1, 1.402344, 0 } },
  { "Minolta", "DiMAGE A2", N_("Daylight"), 3,	{ 2.054688, 1, 1.121094, 0 } },
  { "Minolta", "DiMAGE A2", N_("Flash"), -3,	{ 1.945312, 1, 1.613281, 0 } },
  { "Minolta", "DiMAGE A2", N_("Flash"), 0,	{ 2.039062, 1, 1.332031, 0 } },
  { "Minolta", "DiMAGE A2", N_("Flash"), 3,	{ 2.132812, 1, 1.050781, 0 } },
  { "Minolta", "DiMAGE A2", N_("Fluorescent"), -2, { 1.136719, 1, 2.746094, 0 } },
  { "Minolta", "DiMAGE A2", N_("Fluorescent"), 0, { 1.722656, 1, 2.132812, 0 } },
  { "Minolta", "DiMAGE A2", N_("Fluorescent"), 4, { 2.347656, 1, 1.535156, 0 } },
  { "Minolta", "DiMAGE A2", N_("Shade"), -3,	{ 2.273438, 1, 1.546875, 0 } },
  { "Minolta", "DiMAGE A2", N_("Shade"), 0,	{ 2.367188, 1, 1.265625, 0 } },
  { "Minolta", "DiMAGE A2", N_("Shade"), 3,	{ 2.500000, 1.015873, 1, 0 } },
  { "Minolta", "DiMAGE A2", N_("Tungsten"), -3,	{ 1.003906, 1, 3.164062, 0 } },
  { "Minolta", "DiMAGE A2", N_("Tungsten"), 0,	{ 1.097656, 1, 2.882812, 0 } },
  { "Minolta", "DiMAGE A2", N_("Tungsten"), 3,	{ 1.191406, 1, 2.601562, 0 } },

  { "Minolta", "DiMAGE G500", N_("Daylight"), 0, { 1.496094, 1, 1.121094, 0 } },
  { "Minolta", "DiMAGE G500", N_("Cloudy"), 0,	{ 1.527344, 1, 1.105469, 0 } },
  { "Minolta", "DiMAGE G500", N_("Fluorescent"), 0, { 1.382813, 1, 1.347656, 0 } },
  { "Minolta", "DiMAGE G500", N_("Tungsten"), 0, { 1.042969, 1, 1.859375, 0 } },

  { "MINOLTA", "DYNAX 5D", N_("Daylight"), -3,	{ 1.593750, 1, 1.875000, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Daylight"), -2,	{ 1.644531, 1, 1.792969, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Daylight"), -1,	{ 1.699219, 1, 1.718750, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Daylight"), 0,	{ 1.757812, 1, 1.636719, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Daylight"), 1,	{ 1.804688, 1, 1.566406, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Daylight"), 2,	{ 1.863281, 1, 1.500000, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Daylight"), 3,	{ 1.925781, 1, 1.437500, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Shade"), -3,	{ 1.835938, 1, 1.644531, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Shade"), -2,	{ 1.894531, 1, 1.574219, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Shade"), -1,	{ 1.957031, 1, 1.507812, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Shade"), 0,	{ 2.011719, 1, 1.433594, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Shade"), 1,	{ 2.078125, 1, 1.375000, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Shade"), 2,	{ 2.148438, 1, 1.316406, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Shade"), 3,	{ 2.218750, 1, 1.261719, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Cloudy"), -3,	{ 1.718750, 1, 1.738281, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Cloudy"), -2,	{ 1.773438, 1, 1.664062, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Cloudy"), -1,	{ 1.835938, 1, 1.593750, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Cloudy"), 0,	{ 1.886719, 1, 1.500000, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Cloudy"), 1,	{ 1.945312, 1, 1.460938, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Cloudy"), 2,	{ 2.007812, 1, 1.390625, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Cloudy"), 3,	{ 2.078125, 1, 1.332031, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Tungsten"), -3,	{ 1, 1.066667, 4.262500, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Tungsten"), -2,	{ 1, 1.032258, 3.951613, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Tungsten"), -1,	{ 1, 1.000000, 3.671875, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Tungsten"), 0,	{ 1.023438, 1, 3.496094, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Tungsten"), 1,	{ 1.062500, 1, 3.367188, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Tungsten"), 2,	{ 1.097656, 1, 3.203125, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Tungsten"), 3,	{ 1.132812, 1, 3.070312, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Fluorescent"), -2, { 1.148438, 1, 3.429688, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Fluorescent"), -1, { 1.285156, 1, 3.250000, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Fluorescent"), 0, { 1.703125, 1, 2.582031, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Fluorescent"), 1, { 1.761719, 1, 2.335938, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Fluorescent"), 2, { 1.730469, 1, 1.878906, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Fluorescent"), 3, { 1.996094, 1, 1.527344, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Fluorescent"), 4, { 2.218750, 1, 1.714844, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Flash"), -3,	{ 1.738281, 1, 1.683594, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Flash"), -2,	{ 1.792969, 1, 1.609375, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Flash"), -1,	{ 1.855469, 1, 1.542969, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Flash"), 0,	{ 1.917969, 1, 1.457031, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Flash"), 1,	{ 1.968750, 1, 1.406250, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Flash"), 2,	{ 2.031250, 1, 1.347656, 0 } },
  { "MINOLTA", "DYNAX 5D", N_("Flash"), 3,	{ 2.101562, 1, 1.289062, 0 } },

  { "MINOLTA", "DYNAX 7D", N_("Daylight"), -3,	{ 1.476562, 1, 1.824219, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Daylight"), 0,	{ 1.621094, 1, 1.601562, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Daylight"), 3,	{ 1.785156, 1, 1.414062, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Shade"), -3,	{ 1.683594, 1, 1.585938, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Shade"), 0,	{ 1.855469, 1, 1.402344, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Shade"), 3,	{ 2.031250, 1, 1.226562, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Cloudy"), -3,	{ 1.593750, 1, 1.671875, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Cloudy"), 0,	{ 1.738281, 1, 1.464844, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Cloudy"), 3,	{ 1.925781, 1, 1.296875, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Tungsten"), -3,	{ 0.867188, 1, 3.765625, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Tungsten"), 0,	{ 0.945312, 1, 3.292969, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Tungsten"), 3,	{ 1.050781, 1, 2.921875, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Fluorescent"), -2, { 1.058594, 1, 3.230469, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Fluorescent"), 0, { 1.570312, 1, 2.453125, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Fluorescent"), 1, { 1.625000, 1, 2.226562, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Fluorescent"), 4, { 2.046875, 1, 1.675781, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Flash"), -3,	{ 1.738281, 1, 1.656250, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Flash"), 0,	{ 1.890625, 1, 1.445312, 0 } },
  { "MINOLTA", "DYNAX 7D", N_("Flash"), 3,	{ 2.101562, 1, 1.281250, 0 } },

  { "NIKON", "D1H", N_("Incandescent"), -3,	{ 1.503906, 1, 1.832031, 0 } },
  { "NIKON", "D1H", N_("Incandescent"), 0,	{ 1.363281, 1, 1.996094, 0 } },
  { "NIKON", "D1H", N_("Incandescent"), 3,	{ 1.246094, 1, 2.148438, 0 } },
  { "NIKON", "D1H", N_("Fluorescent"), -3,	{ 2.546875, 1, 1.175781, 0 } },
  { "NIKON", "D1H", N_("Fluorescent"), 0,	{ 1.925781, 1, 2.054688, 0 } },
  { "NIKON", "D1H", N_("Fluorescent"), 3,	{ 1.234375, 1, 2.171875, 0 } },
  { "NIKON", "D1H", N_("Direct sunlight"), -3,	{ 2.230469, 1, 1.187500, 0 } },
  { "NIKON", "D1H", N_("Direct sunlight"), 0,	{ 2.148438, 1, 1.246094, 0 } },
  { "NIKON", "D1H", N_("Direct sunlight"), 3,	{ 2.066406, 1, 1.316406, 0 } },
  { "NIKON", "D1H", N_("Flash"), -3,		{ 2.453125, 1, 1.117188, 0 } },
  { "NIKON", "D1H", N_("Flash"), 0,		{ 2.347656, 1, 1.140625, 0 } },
  { "NIKON", "D1H", N_("Flash"), 3,		{ 2.242188, 1, 1.164062, 0 } },
  { "NIKON", "D1H", N_("Cloudy"), -3,		{ 2.441406, 1, 1.046875, 0 } },
  { "NIKON", "D1H", N_("Cloudy"), 0,		{ 2.300781, 1, 1.128906, 0 } },
  { "NIKON", "D1H", N_("Cloudy"), 3,		{ 2.207031, 1, 1.199219, 0 } },
  { "NIKON", "D1H", N_("Shade"), -3,		{ 2.839844, 1, 1.000000, 0 } },
  { "NIKON", "D1H", N_("Shade"), 0,		{ 2.628906, 1, 1.011719, 0 } },
  { "NIKON", "D1H", N_("Shade"), 3,		{ 2.441406, 1, 1.046875, 0 } },

  { "NIKON", "D1X", N_("Incandescent"), -3,	{ 1.503906, 1, 1.832031, 0 } }, /*3250K*/
  { "NIKON", "D1X", N_("Incandescent"), -2,	{ 1.445312, 1, 1.890625, 0 } }, /*3150K*/
  { "NIKON", "D1X", N_("Incandescent"), -1,	{ 1.410156, 1, 1.937500, 0 } }, /*3100K*/
  { "NIKON", "D1X", N_("Incandescent"), 0,	{ 1.363281, 1, 1.996094, 0 } }, /*3000K*/
  { "NIKON", "D1X", N_("Incandescent"), 1,	{ 1.316406, 1, 2.042969, 0 } }, /*2900K*/
  { "NIKON", "D1X", N_("Incandescent"), 2,	{ 1.281250, 1, 2.101562, 0 } }, /*2800K*/
  { "NIKON", "D1X", N_("Incandescent"), 3,	{ 1.246094, 1, 2.148438, 0 } }, /*2700K*/
  { "NIKON", "D1X", N_("Fluorescent"), -3,	{ 2.546875, 1, 1.175781, 0 } }, /*7200K*/
  { "NIKON", "D1X", N_("Fluorescent"), -2,	{ 2.464844, 1, 1.210938, 0 } }, /*6500K*/
  { "NIKON", "D1X", N_("Fluorescent"), -1,	{ 2.160156, 1, 1.386719, 0 } }, /*5000K*/
  { "NIKON", "D1X", N_("Fluorescent"), 0,	{ 1.925781, 1, 2.054688, 0 } }, /*4200K*/
  { "NIKON", "D1X", N_("Fluorescent"), 1,	{ 1.703125, 1, 2.277344, 0 } }, /*3700K*/
  { "NIKON", "D1X", N_("Fluorescent"), 2,	{ 1.328125, 1, 2.394531, 0 } }, /*3000K*/
  { "NIKON", "D1X", N_("Fluorescent"), 3,	{ 1.234375, 1, 2.171875, 0 } }, /*2700K*/
  { "NIKON", "D1X", N_("Direct sunlight"), -3,	{ 2.230469, 1, 1.187500, 0 } }, /*5600K*/
  { "NIKON", "D1X", N_("Direct sunlight"), -2,	{ 2.207031, 1, 1.210938, 0 } }, /*5400K*/
  { "NIKON", "D1X", N_("Direct sunlight"), -1,	{ 2.171875, 1, 1.222656, 0 } }, /*5300K*/
  { "NIKON", "D1X", N_("Direct sunlight"), 0,	{ 2.148438, 1, 1.246094, 0 } }, /*5200K*/
  { "NIKON", "D1X", N_("Direct sunlight"), 1,	{ 2.113281, 1, 1.269531, 0 } }, /*5000K*/
  { "NIKON", "D1X", N_("Direct sunlight"), 2,	{ 2.089844, 1, 1.292969, 0 } }, /*4900K*/
  { "NIKON", "D1X", N_("Direct sunlight"), 3,	{ 2.066406, 1, 1.316406, 0 } }, /*4800K*/
  { "NIKON", "D1X", N_("Flash"), -3,		{ 2.453125, 1, 1.117188, 0 } }, /*6000K*/
  { "NIKON", "D1X", N_("Flash"), -2,		{ 2.417969, 1, 1.128906, 0 } }, /*5800K*/
  { "NIKON", "D1X", N_("Flash"), -1,		{ 2.382812, 1, 1.128906, 0 } }, /*5600K*/
  { "NIKON", "D1X", N_("Flash"), 0,		{ 2.347656, 1, 1.140625, 0 } }, /*5400K*/
  { "NIKON", "D1X", N_("Flash"), 1,		{ 2.312500, 1, 1.152344, 0 } }, /*5200K*/
  { "NIKON", "D1X", N_("Flash"), 2,		{ 2.277344, 1, 1.164062, 0 } }, /*5000K*/
  { "NIKON", "D1X", N_("Flash"), 3,		{ 2.242188, 1, 1.164062, 0 } }, /*4800K*/
  { "NIKON", "D1X", N_("Cloudy"), -3,		{ 2.441406, 1, 1.046875, 0 } }, /*6600K*/
  { "NIKON", "D1X", N_("Cloudy"), -2,		{ 2.394531, 1, 1.082031, 0 } }, /*6400K*/
  { "NIKON", "D1X", N_("Cloudy"), -1,		{ 2.347656, 1, 1.105469, 0 } }, /*6200K*/
  { "NIKON", "D1X", N_("Cloudy"), 0,		{ 2.300781, 1, 1.128906, 0 } }, /*6000K*/
  { "NIKON", "D1X", N_("Cloudy"), 1,		{ 2.253906, 1, 1.164062, 0 } }, /*5800K*/
  { "NIKON", "D1X", N_("Cloudy"), 2,		{ 2.230469, 1, 1.187500, 0 } }, /*5600K*/
  { "NIKON", "D1X", N_("Cloudy"), 3,		{ 2.207031, 1, 1.199219, 0 } }, /*5400K*/
  { "NIKON", "D1X", N_("Shade"), -3,		{ 2.839844, 1, 1.000000, 0 } }, /*9200K*/
  { "NIKON", "D1X", N_("Shade"), -2,		{ 2.769531, 1, 1.000000, 0 } }, /*8800K*/
  { "NIKON", "D1X", N_("Shade"), -1,		{ 2.699219, 1, 1.000000, 0 } }, /*8400K*/
  { "NIKON", "D1X", N_("Shade"), 0,		{ 2.628906, 1, 1.011719, 0 } }, /*8000K*/
  { "NIKON", "D1X", N_("Shade"), 1,		{ 2.558594, 1, 1.023438, 0 } }, /*7500K*/
  { "NIKON", "D1X", N_("Shade"), 2,		{ 2.500000, 1, 1.035156, 0 } }, /*7100K*/
  { "NIKON", "D1X", N_("Shade"), 3,		{ 2.441406, 1, 1.046875, 0 } }, /*6700K*/

  { "NIKON", "D100", N_("Incandescent"), 0,	{ 1.406250, 1, 2.828125, 0 } },
  { "NIKON", "D100", N_("Fluorescent"), 0,	{ 2.058594, 1, 2.617188, 0 } },
  { "NIKON", "D100", N_("Daylight"), 0,		{ 2.257813, 1, 1.757813, 0 } },
  { "NIKON", "D100", N_("Flash"), 0,		{ 2.539063, 1, 1.539063, 0 } },
  { "NIKON", "D100", N_("Cloudy"), 0,		{ 2.507813, 1, 1.628906, 0 } },
  { "NIKON", "D100", N_("Shade"), 0,		{ 2.906250, 1, 1.437500, 0 } },

  /*
   * D2X with firmware A 1.01 and B 1.01
   */

  /* D2X basic + fine tune presets */
  { "NIKON", "D2X", N_("Incandescent"), -3,	{ 0.98462, 1, 2.61154, 0 } }, /*3300K*/
  { "NIKON", "D2X", N_("Incandescent"), -2,	{ 0.95880, 1, 2.71536, 0 } }, /*3200K*/
  { "NIKON", "D2X", N_("Incandescent"), -1,	{ 0.94465, 1, 2.77122, 0 } }, /*3100K*/
  { "NIKON", "D2X", N_("Incandescent"), 0,	{ 0.92086, 1, 2.89928, 0 } }, /*3000K*/
  { "NIKON", "D2X", N_("Incandescent"), 1,	{ 0.89510, 1, 3.03846, 0 } }, /*2900K*/
  { "NIKON", "D2X", N_("Incandescent"), 2,	{ 0.86486, 1, 3.17905, 0 } }, /*2800K*/
  { "NIKON", "D2X", N_("Incandescent"), 3,	{ 0.83388, 1, 3.34528, 0 } }, /*2700K*/
  { "NIKON", "D2X", N_("Fluorescent"), -3,	{ 2.01562, 1, 1.72266, 0 } }, /*7200K*/
  { "NIKON", "D2X", N_("Fluorescent"), -2,	{ 1.67969, 1, 1.42578, 0 } }, /*6500K*/
  { "NIKON", "D2X", N_("Fluorescent"), -1,	{ 1.42969, 1, 1.80078, 0 } }, /*5000K*/
  { "NIKON", "D2X", N_("Fluorescent"), 0,	{ 1.42969, 1, 2.62891, 0 } }, /*4200K*/
  { "NIKON", "D2X", N_("Fluorescent"), 1,	{ 1.13672, 1, 3.02734, 0 } }, /*3700K*/
  { "NIKON", "D2X", N_("Fluorescent"), 2,	{ 0.94118, 1, 2.68498, 0 } }, /*3000K*/
  { "NIKON", "D2X", N_("Fluorescent"), 3,	{ 0.83388, 1, 3.51140, 0 } }, /*2700K*/
  { "NIKON", "D2X", N_("Direct sunlight"), -3,	{ 1.61328, 1, 1.61328, 0 } }, /*5600K*/
  { "NIKON", "D2X", N_("Direct sunlight"), -2,	{ 1.57031, 1, 1.65234, 0 } }, /*5400K*/
  { "NIKON", "D2X", N_("Direct sunlight"), -1,	{ 1.55078, 1, 1.67578, 0 } }, /*5300K*/
  { "NIKON", "D2X", N_("Direct sunlight"), 0,	{ 1.52734, 1, 1.69531, 0 } }, /*5200K*/
  { "NIKON", "D2X", N_("Direct sunlight"), 1,	{ 1.48438, 1, 1.74609, 0 } }, /*5000K*/
  { "NIKON", "D2X", N_("Direct sunlight"), 2,	{ 1.45312, 1, 1.76953, 0 } }, /*4900K*/
  { "NIKON", "D2X", N_("Direct sunlight"), 3,	{ 1.42578, 1, 1.78906, 0 } }, /*4800K*/
  { "NIKON", "D2X", N_("Flash"), -3,		{ 1.71484, 1, 1.48438, 0 } }, /*6000K*/
  { "NIKON", "D2X", N_("Flash"), -2,		{ 1.67578, 1, 1.48438, 0 } }, /*5800K*/
  { "NIKON", "D2X", N_("Flash"), -1,		{ 1.66797, 1, 1.50781, 0 } }, /*5600K*/
  { "NIKON", "D2X", N_("Flash"), 0,		{ 1.66016, 1, 1.53125, 0 } }, /*5400K*/
  { "NIKON", "D2X", N_("Flash"), 1,		{ 1.64453, 1, 1.54297, 0 } }, /*5200K*/
  { "NIKON", "D2X", N_("Flash"), 2,		{ 1.62891, 1, 1.54297, 0 } }, /*5000K*/
  { "NIKON", "D2X", N_("Flash"), 3,		{ 1.57031, 1, 1.56641, 0 } }, /*4800K*/
  { "NIKON", "D2X", N_("Cloudy"), -3,		{ 1.79297, 1, 1.46875, 0 } }, /*6600K*/
  { "NIKON", "D2X", N_("Cloudy"), -2,		{ 1.76172, 1, 1.49219, 0 } }, /*6400K*/
  { "NIKON", "D2X", N_("Cloudy"), -1,		{ 1.72656, 1, 1.51953, 0 } }, /*6200K*/
  { "NIKON", "D2X", N_("Cloudy"), 0,		{ 1.69141, 1, 1.54688, 0 } }, /*6000K*/
  { "NIKON", "D2X", N_("Cloudy"), 1,		{ 1.65234, 1, 1.57812, 0 } }, /*5800K*/
  { "NIKON", "D2X", N_("Cloudy"), 2,		{ 1.61328, 1, 1.61328, 0 } }, /*5600K*/
  { "NIKON", "D2X", N_("Cloudy"), 3,		{ 1.57031, 1, 1.65234, 0 } }, /*5400K*/
  { "NIKON", "D2X", N_("Shade"), -3,		{ 2.10938, 1, 1.23828, 0 } }, /*9200K*/
  { "NIKON", "D2X", N_("Shade"), -2,		{ 2.07031, 1, 1.26562, 0 } }, /*8800K*/
  { "NIKON", "D2X", N_("Shade"), -1,		{ 2.02734, 1, 1.29688, 0 } }, /*8400K*/
  { "NIKON", "D2X", N_("Shade"), 0,		{ 1.98047, 1, 1.32812, 0 } }, /*8000K*/
  { "NIKON", "D2X", N_("Shade"), 1,		{ 1.92188, 1, 1.37109, 0 } }, /*7500K*/
  { "NIKON", "D2X", N_("Shade"), 2,		{ 1.86719, 1, 1.41406, 0 } }, /*7100K*/
  { "NIKON", "D2X", N_("Shade"), 3,		{ 1.80859, 1, 1.45703, 0 } }, /*6700K*/

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

  /* D200 basic + fine tune WB presets */
  { "NIKON", "D200", N_("Incandescent"), -2,	{ 1.199219, 1, 2.238281, 0 } },
  { "NIKON", "D200", N_("Incandescent"), -1,	{ 1.183594, 1, 2.289063, 0 } },
  { "NIKON", "D200", N_("Incandescent"), 0,	{ 1.148437, 1, 2.398438, 0 } },
  { "NIKON", "D200", N_("Incandescent"), 1,	{ 1.113281, 1, 2.519531, 0 } },
  { "NIKON", "D200", N_("Incandescent"), 2,	{ 1.074219, 1, 2.648438, 0 } },
  { "NIKON", "D200", N_("Incandescent"), 3,	{ 1.03125, 1, 2.804688, 0 } },
  { "NIKON", "D200", N_("Fluorescent"), -3,	{ 2.273438, 1, 1.410156, 0 } },
  { "NIKON", "D200", N_("Fluorescent"), -2,	{ 1.933594, 1, 1.152344, 0 } },
  { "NIKON", "D200", N_("Fluorescent"), -1,	{ 1.675781, 1, 1.453125, 0 } },
  { "NIKON", "D200", N_("Fluorescent"), 0,	{ 1.664062, 1, 2.148437, 0 } },
  { "NIKON", "D200", N_("Fluorescent"), 1,	{ 1.335937, 1, 2.453125, 0 } },
  { "NIKON", "D200", N_("Fluorescent"), 2,	{ 1.140625, 1, 2.214844, 0 } },
  { "NIKON", "D200", N_("Fluorescent"), 3,	{ 1.035156, 1, 2.410156, 0 } },
  { "NIKON", "D200", N_("Direct sunlight"), -3,	{ 1.863281, 1, 1.320312, 0 } },
  { "NIKON", "D200", N_("Direct sunlight"), -2,	{ 1.835938, 1, 1.355469, 0 } },
  { "NIKON", "D200", N_("Direct sunlight"), -1,	{ 1.820313, 1, 1.375, 0 } },
  { "NIKON", "D200", N_("Direct sunlight"), 0,	{ 1.804688, 1, 1.398437, 0 } },
  { "NIKON", "D200", N_("Direct sunlight"), 1,	{ 1.746094, 1, 1.425781, 0 } },
  { "NIKON", "D200", N_("Direct sunlight"), 2,	{ 1.714844, 1, 1.4375, 0 } },
  { "NIKON", "D200", N_("Direct sunlight"), 3,	{ 1.6875, 1, 1.449219, 0 } },
  { "NIKON", "D200", N_("Flash"), -3,		{ 2.066406, 1, 1.183594, 0 } },
  { "NIKON", "D200", N_("Flash"), -2,		{ 2.046875, 1, 1.191406, 0 } },
  { "NIKON", "D200", N_("Flash"), -1,		{ 2.027344, 1, 1.199219, 0 } },
  { "NIKON", "D200", N_("Flash"), 0,		{ 2.007813, 1, 1.171875, 0 } },
  { "NIKON", "D200", N_("Flash"), 1,		{ 1.984375, 1, 1.207031, 0 } },
  { "NIKON", "D200", N_("Flash"), 2,		{ 1.964844, 1, 1.214844, 0 } },
  { "NIKON", "D200", N_("Flash"), 3,		{ 1.945312, 1, 1.222656, 0 } },
  { "NIKON", "D200", N_("Cloudy"), -3,		{ 2.027344, 1, 1.210937, 0 } },
  { "NIKON", "D200", N_("Cloudy"), -2,		{ 1.992187, 1, 1.226562, 0 } },
  { "NIKON", "D200", N_("Cloudy"), -1,		{ 1.953125, 1, 1.242187, 0 } },
  { "NIKON", "D200", N_("Cloudy"), 0,		{ 1.917969, 1, 1.261719, 0 } },
  { "NIKON", "D200", N_("Cloudy"), 1,		{ 1.890625, 1, 1.285156, 0 } },
  { "NIKON", "D200", N_("Cloudy"), 2,		{ 1.863281, 1, 1.320312, 0 } },
  { "NIKON", "D200", N_("Cloudy"), 3,		{ 1.835938, 1, 1.355469, 0 } },
  { "NIKON", "D200", N_("Shade"), -3,		{ 2.378906, 1, 1.066406, 0 } },
  { "NIKON", "D200", N_("Shade"), -2,		{ 2.332031, 1, 1.085938, 0 } },
  { "NIKON", "D200", N_("Shade"), -1,		{ 2.289063, 1, 1.105469, 0 } },
  { "NIKON", "D200", N_("Shade"), 0,		{ 2.234375, 1, 1.125, 0 } },
  { "NIKON", "D200", N_("Shade"), 1,		{ 2.167969, 1, 1.152344, 0 } },
  { "NIKON", "D200", N_("Shade"), 2,		{ 2.105469, 1, 1.175781, 0 } },
  { "NIKON", "D200", N_("Shade"), 3,		{ 2.046875, 1, 1.199219, 0 } },

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

  { "NIKON", "D50", N_("Incandescent"), 0,	{ 1.328125, 1, 2.500000, 0 } },
  { "NIKON", "D50", N_("Fluorescent"), 0,	{ 1.945312, 1, 2.191406, 0 } },
  { "NIKON", "D50", N_("Direct sunlight"), 0,	{ 2.140625, 1, 1.398438, 0 } },
  { "NIKON", "D50", N_("Flash"), 0,		{ 2.398438, 1, 1.339844, 0 } },
  { "NIKON", "D50", N_("Shade"), 0,		{ 2.746094, 1, 1.156250, 0 } },

  { "NIKON", "D70", N_("Incandescent"), -3,	{ 1.429688, 1, 2.539062, 0 } },
  { "NIKON", "D70", N_("Incandescent"), 0,	{ 1.343750, 1, 2.816406, 0 } }, /*3000K*/
  { "NIKON", "D70", N_("Incandescent"), 3,	{ 1.253906, 1, 3.250000, 0 } },
  { "NIKON", "D70", N_("Fluorescent"), -3,	{ 2.734375, 1, 1.621094, 0 } },
  { "NIKON", "D70", N_("Fluorescent"), 0,	{ 1.964844, 1, 2.476563, 0 } }, /*4200K*/
  { "NIKON", "D70", N_("Fluorescent"), 3,	{ 1.312500, 1, 2.562500, 0 } },
  { "NIKON", "D70", N_("Direct sunlight"), -3,	{ 2.156250, 1, 1.523438, 0 } },
  { "NIKON", "D70", N_("Direct sunlight"), 0,	{ 2.062500, 1, 1.597656, 0 } }, /*5200K*/
  { "NIKON", "D70", N_("Direct sunlight"), 3,	{ 1.953125, 1, 1.695312, 0 } },
  { "NIKON", "D70", N_("Flash"), -3,		{ 2.578125, 1, 1.476562, 0 } },
  { "NIKON", "D70", N_("Flash"), 0,		{ 2.441406, 1, 1.500000, 0 } }, /*5400K*/
  { "NIKON", "D70", N_("Flash"), 3,		{ 2.378906, 1, 1.523438, 0 } },
  { "NIKON", "D70", N_("Cloudy"), -3,		{ 2.375000, 1, 1.386719, 0 } },
  { "NIKON", "D70", N_("Cloudy"), 0,		{ 2.257813, 1, 1.457031, 0 } }, /*6000K*/
  { "NIKON", "D70", N_("Cloudy"), 3,		{ 2.109375, 1, 1.562500, 0 } },
  { "NIKON", "D70", N_("Shade"), -3,		{ 2.757812, 1, 1.226562, 0 } },
  { "NIKON", "D70", N_("Shade"), 0,		{ 2.613281, 1, 1.277344, 0 } }, /*8000K*/
  { "NIKON", "D70", N_("Shade"), 3,		{ 2.394531, 1, 1.375000, 0 } },

  { "NIKON", "D70s", N_("Incandescent"), -3,	{ 1.429688, 1, 2.539062, 0 } },
  { "NIKON", "D70s", N_("Incandescent"), 0,	{ 1.343750, 1, 2.816406, 0 } }, /*3000K*/
  { "NIKON", "D70s", N_("Incandescent"), 3,	{ 1.253906, 1, 3.250000, 0 } },
  { "NIKON", "D70s", N_("Fluorescent"), -3,	{ 2.734375, 1, 1.621094, 0 } },
  { "NIKON", "D70s", N_("Fluorescent"), 0,	{ 1.964844, 1, 2.476563, 0 } }, /*4200K*/
  { "NIKON", "D70s", N_("Fluorescent"), 3,	{ 1.312500, 1, 2.562500, 0 } },
  { "NIKON", "D70s", N_("Direct sunlight"), -3,	{ 2.156250, 1, 1.523438, 0 } },
  { "NIKON", "D70s", N_("Direct sunlight"), 0,	{ 2.062500, 1, 1.597656, 0 } }, /*5200K*/
  { "NIKON", "D70s", N_("Direct sunlight"), 3,	{ 1.953125, 1, 1.695312, 0 } },
  { "NIKON", "D70s", N_("Flash"), -3,		{ 2.578125, 1, 1.476562, 0 } },
  { "NIKON", "D70s", N_("Flash"), 0,		{ 2.441406, 1, 1.500000, 0 } }, /*5400K*/
  { "NIKON", "D70s", N_("Flash"), 3,		{ 2.378906, 1, 1.523438, 0 } },
  { "NIKON", "D70s", N_("Cloudy"), -3,		{ 2.375000, 1, 1.386719, 0 } },
  { "NIKON", "D70s", N_("Cloudy"), 0,		{ 2.257813, 1, 1.457031, 0 } }, /*6000K*/
  { "NIKON", "D70s", N_("Cloudy"), 3,		{ 2.109375, 1, 1.562500, 0 } },
  { "NIKON", "D70s", N_("Shade"), -3,		{ 2.757812, 1, 1.226562, 0 } },
  { "NIKON", "D70s", N_("Shade"), 0,		{ 2.613281, 1, 1.277344, 0 } }, /*8000K*/
  { "NIKON", "D70s", N_("Shade"), 3,		{ 2.394531, 1, 1.375000, 0 } },

  { "NIKON", "D80", N_("Incandescent"), -3,	{ 1.234375, 1, 2.140625, 0 } },
  { "NIKON", "D80", N_("Incandescent"), 0,	{ 1.148438, 1, 2.386719, 0 } },
  { "NIKON", "D80", N_("Incandescent"), 3,	{ 1.039062, 1, 2.734375, 0 } },
  { "NIKON", "D80", N_("Fluorescent"), -3,	{ 2.296875, 1, 1.398438, 0 } },
  { "NIKON", "D80", N_("Fluorescent"), 0,	{ 1.683594, 1, 2.117188, 0 } },
  { "NIKON", "D80", N_("Fluorescent"), 3,	{ 1.000000, 1, 2.527344, 0 } },
  { "NIKON", "D80", N_("Daylight"), -3,		{ 1.882812, 1, 1.300781, 0 } },
  { "NIKON", "D80", N_("Daylight"), 0,		{ 1.792969, 1, 1.371094, 0 } },
  { "NIKON", "D80", N_("Daylight"), 3,		{ 1.695312, 1, 1.437500, 0 } },
  { "NIKON", "D80", N_("Flash"), -3,		{ 2.070312, 1, 1.144531, 0 } },
  { "NIKON", "D80", N_("Flash"), 0,		{ 2.007812, 1, 1.242188, 0 } },
  { "NIKON", "D80", N_("Flash"), 3,		{ 1.972656, 1, 1.156250, 0 } },
  { "NIKON", "D80", N_("Cloudy"), -3,		{ 2.070312, 1, 1.191406, 0 } },
  { "NIKON", "D80", N_("Cloudy"), 0,		{ 1.960938, 1, 1.253906, 0 } },
  { "NIKON", "D80", N_("Cloudy"), 3,		{ 1.835938, 1, 1.332031, 0 } },
  { "NIKON", "D80", N_("Shade"), -3,		{ 2.414062, 1, 1.042969, 0 } },
  { "NIKON", "D80", N_("Shade"), 0,		{ 2.277344, 1, 1.089844, 0 } },
  { "NIKON", "D80", N_("Shade"), 3,		{ 2.085938, 1, 1.183594, 0 } },
  { "NIKON", "D80", "4300K", 0,			{ 1.562500, 1, 1.523438, 0 } },
  { "NIKON", "D80", "5000K", 0,			{ 1.746094, 1, 1.410156, 0 } },
  { "NIKON", "D80", "5900K", 0,			{ 1.941406, 1, 1.265625, 0 } },

  { "NIKON", "E5400", N_("Daylight"), -3,	{ 2.046875, 1, 1.449219, 0 } },
  { "NIKON", "E5400", N_("Daylight"), 0,	{ 1.800781, 1, 1.636719, 0 } },
  { "NIKON", "E5400", N_("Daylight"), 3,	{ 1.539062, 1, 1.820312, 0 } },
  { "NIKON", "E5400", N_("Incandescent"), -3,	{ 1.218750, 1, 2.656250, 0 } },
  { "NIKON", "E5400", N_("Incandescent"), 0,	{ 1.218750, 1, 2.656250, 0 } },
  { "NIKON", "E5400", N_("Incandescent"), 3,	{ 1.382812, 1, 2.351562, 0 } },
  { "NIKON", "E5400", N_("Fluorescent"), -3,	{ 1.703125, 1, 2.460938, 0 } },
  { "NIKON", "E5400", N_("Fluorescent"), 0,	{ 1.218750, 1, 2.656250, 0 } },
  { "NIKON", "E5400", N_("Fluorescent"), 3,	{ 1.953125, 1, 1.906250, 0 } },
  { "NIKON", "E5400", N_("Cloudy"), -3,		{ 1.703125, 1, 2.460938, 0 } },
  { "NIKON", "E5400", N_("Cloudy"), 0,		{ 1.996094, 1, 1.421875, 0 } },
  { "NIKON", "E5400", N_("Cloudy"), 3,		{ 2.265625, 1, 1.261719, 0 } },
  { "NIKON", "E5400", N_("Flash"), -3,		{ 2.792969, 1, 1.152344, 0 } },
  { "NIKON", "E5400", N_("Flash"), 0,		{ 2.328125, 1, 1.386719, 0 } },
  { "NIKON", "E5400", N_("Flash"), 3,		{ 2.328125, 1, 1.386719, 0 } },
  { "NIKON", "E5400", N_("Shade"), -3,		{ 2.722656, 1, 1.011719, 0 } },
  { "NIKON", "E5400", N_("Shade"), 0,		{ 2.269531, 1, 1.218750, 0 } },
  { "NIKON", "E5400", N_("Shade"), 3,		{ 2.269531, 1, 1.218750, 0 } },

  { "NIKON", "E8700", N_("Daylight"), 0,	{ 1.968750, 1, 1.582031, 0 } },
  { "NIKON", "E8700", N_("Incandescent"), 0,	{ 1.265625, 1, 2.765625, 0 } },
  { "NIKON", "E8700", N_("Fluorescent"), 0,	{ 1.863281, 1, 2.304688, 0 } },
  { "NIKON", "E8700", N_("Cloudy"), 0,		{ 2.218750, 1, 1.359375, 0 } },
  { "NIKON", "E8700", N_("Flash"), 0,		{ 2.535156, 1, 1.273438, 0 } },
  { "NIKON", "E8700", N_("Shade"), 0,		{ 2.527344, 1, 1.175781, 0 } },

  { "OLYMPUS", "C5050Z", N_("Shade"), -7,	{ 3.887324, 1.201878, 1, 0 } },
  { "OLYMPUS", "C5050Z", N_("Shade"), 0,	{ 1.757812, 1, 1.437500, 0 } },
  { "OLYMPUS", "C5050Z", N_("Shade"), 7,	{ 1.019531, 1, 2.140625, 0 } },
  { "OLYMPUS", "C5050Z", N_("Cloudy"), -7,	{ 3.255507, 1.127753, 1, 0 } },
  { "OLYMPUS", "C5050Z", N_("Cloudy"), 0,	{ 1.570312, 1, 1.531250, 0 } },
  { "OLYMPUS", "C5050Z", N_("Cloudy"), 7,	{ 1, 1.098712, 2.506438, 0 } },
  { "OLYMPUS", "C5050Z", N_("Daylight"), -7,	{ 2.892116, 1.062241, 1, 0 } },
  { "OLYMPUS", "C5050Z", N_("Daylight"), 0,	{ 1.480469, 1, 1.628906, 0 } },
  { "OLYMPUS", "C5050Z", N_("Daylight"), 7,	{ 1, 1.168950, 2.835616, 0 } },
  { "OLYMPUS", "C5050Z", N_("Evening sun"), -7,	{ 3.072649, 1.094017, 1, 0 } },
  { "OLYMPUS", "C5050Z", N_("Evening sun"), 0,	{ 1.527344, 1, 1.578125, 0 } },
  { "OLYMPUS", "C5050Z", N_("Evening sun"), 7,	{ 1, 1.132743, 2.659292, 0 } },
  { "OLYMPUS", "C5050Z", N_("Daylight fluorescent"),-7, { 3.321267, 1.158371, 1, 0 } }, /*6700K*/
  { "OLYMPUS", "C5050Z", N_("Daylight fluorescent"), 0, { 1.558594, 1, 1.492188, 0 } }, /*6700K*/
  { "OLYMPUS", "C5050Z", N_("Daylight fluorescent"), 7, { 1, 1.108225, 2.463203, 0 } }, /*6700K*/
  { "OLYMPUS", "C5050Z", N_("Neutral fluorescent"),-7, { 2.606426, 1.028112, 1, 0 } }, /*5000K*/
  { "OLYMPUS", "C5050Z", N_("Neutral fluorescent"), 0, { 1.378906, 1, 1.679688, 0 } }, /*5000K*/
  { "OLYMPUS", "C5050Z", N_("Neutral fluorescent"), 7, { 1, 1.254902, 3.137255, 0 } }, /*5000K*/
  { "OLYMPUS", "C5050Z", N_("Cool white fluorescent"),-7, { 2.519531, 1, 1.281250, 0 } }, /*4200K*/
  { "OLYMPUS", "C5050Z", N_("Cool white fluorescent"), 0, { 1.371094, 1, 2.210938, 0 } }, /*4200K*/
  { "OLYMPUS", "C5050Z", N_("Cool white fluorescent"), 7, { 1, 1.261084, 4.152709, 0 } }, /*4200K*/
  { "OLYMPUS", "C5050Z", N_("White fluorescent"),-7, { 1.707031, 1, 1.699219, 0 } }, /*3500K*/
  { "OLYMPUS", "C5050Z", N_("White fluorescent"), 0, { 1, 1.075630, 3.151261, 0 } }, /*3500K*/
  { "OLYMPUS", "C5050Z", N_("White fluorescent"), 7, { 1, 1.855072, 8.094203, 0 } }, /*3500K*/
  { "OLYMPUS", "C5050Z", N_("Incandescent"),-7,	{ 1.679688, 1, 1.652344, 0 } }, /*3000K*/
  { "OLYMPUS", "C5050Z", N_("Incandescent"), 0,	{ 1, 1.094017, 3.123932, 0 } }, /*3000K*/
  { "OLYMPUS", "C5050Z", N_("Incandescent"), 7,	{ 1, 1.896296, 8.066667, 0 } }, /*3000K*/

  { "OLYMPUS", "C5060WZ", N_("Shade"), 0,	{ 1.949219, 1, 1.195312, 0 } },
  { "OLYMPUS", "C5060WZ", N_("Cloudy"), 0,	{ 1.621094, 1, 1.410156, 0 } },
  { "OLYMPUS", "C5060WZ", N_("Direct sunlight"), 0, { 1.511719, 1, 1.500000, 0 } },
  { "OLYMPUS", "C5060WZ", N_("Evening sun"), 0, { 1.636719, 1, 1.496094, 0 } },
  { "OLYMPUS", "C5060WZ", N_("Daylight fluorescent"), 0, { 1.734375, 1, 1.343750, 0 } },
  { "OLYMPUS", "C5060WZ", N_("Neutral fluorescent"), 0,	{ 1.457031, 1, 1.691406, 0 } },
  { "OLYMPUS", "C5060WZ", N_("Cool white fluorescent"), 0, { 1.417969, 1, 2.230469, 0 } },
  { "OLYMPUS", "C5060WZ", N_("White fluorescent"), 0, { 1, 1.103448, 3.422414, 0 } },
  { "OLYMPUS", "C5060WZ", N_("Incandescent"), 0, { 1, 1.153153, 3.662162, 0 } },
  { "OLYMPUS", "C5060WZ", N_("Flash (auto mode)"), 0, { 1.850000, 1, 1.308044, 0 } },

  // Olympus C8080WZ - firmware 757-78
  { "OLYMPUS", "C8080WZ", N_("Shade"), -7,	{ 1.515625, 1, 1.773438, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), -6,	{ 1.671875, 1, 1.691406, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), -5,	{ 1.832031, 1, 1.605469, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), -4,	{ 1.988281, 1, 1.523438, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), -3,	{ 2.144531, 1, 1.441406, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), -2,	{ 2.300781, 1, 1.355469, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), -1,	{ 2.457031, 1, 1.273438, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), 0,	{ 2.617188, 1, 1.191406, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), 1,	{ 2.929688, 1, 1.117188, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), 2,	{ 3.242188, 1, 1.046875, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), 3,	{ 3.644000, 1.024000, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), 4,	{ 4.290043, 1.108225, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), 5,	{ 5.032864, 1.201878, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), 6,	{ 5.907692, 1.312821, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Shade"), 7,	{ 7.000000, 1.454545, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), -7,	{ 1.277344, 1, 2.164062, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), -6,	{ 1.406250, 1, 2.062500, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), -5,	{ 1.539062, 1, 1.960938, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), -4,	{ 1.671875, 1, 1.859375, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), -3,	{ 1.804688, 1, 1.757812, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), -2,	{ 1.937500, 1, 1.656250, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), -1,	{ 2.070312, 1, 1.554688, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), 0,	{ 2.203125, 1, 1.453125, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), 1,	{ 2.464844, 1, 1.363281, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), 2,	{ 2.730469, 1, 1.277344, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), 3,	{ 2.996094, 1, 1.191406, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), 4,	{ 3.257812, 1, 1.101562, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), 5,	{ 3.523438, 1, 1.015625, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), 6,	{ 4.075630, 1.075630, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Cloudy"), 7,	{ 4.823256, 1.190698, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), -7,	{ 1.234375, 1, 2.343750, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), -6,	{ 1.359375, 1, 2.234375, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), -5,	{ 1.488281, 1, 2.125000, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), -4,	{ 1.617188, 1, 2.011719, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), -3,	{ 1.742188, 1, 1.902344, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), -2,	{ 1.871094, 1, 1.792969, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), -1,	{ 2.000000, 1, 1.683594, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), 0,	{ 2.128906, 1, 1.574219, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), 1,	{ 2.382812, 1, 1.476562, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), 2,	{ 2.636719, 1, 1.382812, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), 3,	{ 2.894531, 1, 1.289062, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), 4,	{ 3.148438, 1, 1.195312, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), 5,	{ 3.406250, 1, 1.101562, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), 6,	{ 3.660156, 1, 1.003906, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight"), 7,	{ 4.300429, 1.098712, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), -7, { 1.308594, 1, 2.199219, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), -6, { 1.445312, 1, 2.093750, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), -5, { 1.582031, 1, 1.992188, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), -4, { 1.718750, 1, 1.886719, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), -3, { 1.851562, 1, 1.785156, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), -2, { 1.988281, 1, 1.679688, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), -1, { 2.125000, 1, 1.578125, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), 0, { 2.261719, 1, 1.476562, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), 1, { 2.531250, 1, 1.386719, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), 2, { 2.800781, 1, 1.296875, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), 3, { 3.074219, 1, 1.207031, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), 4, { 3.343750, 1, 1.121094, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), 5, { 3.617188, 1, 1.031250, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), 6, { 4.128631, 1.062241, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Evening sun"), 7, { 4.863014, 1.168950, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), -7, { 1.488281, 1, 2.214844, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), -6, { 1.652344, 1, 2.105469, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), -5, { 1.812500, 1, 1.992188, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), -4, { 1.976562, 1, 1.882812, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), -3, { 2.117188, 1, 1.773438, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), -2, { 2.253906, 1, 1.675781, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), -1, { 2.425781, 1, 1.585938, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), 0, { 2.570312, 1, 1.468750, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), 1, { 2.890625, 1, 1.386719, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), 2, { 3.199219, 1, 1.308594, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), 3, { 3.500000, 1, 1.214844, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), 4, { 3.820312, 1, 1.125000, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), 5, { 4.128906, 1, 1.039062, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), 6, { 4.711934, 1.053498, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Flash (auto mode)"), 7, { 5.450450, 1.153153, 1, 0 } },
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), -7, { 1.425781, 1, 2.097656, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), -6, { 1.574219, 1, 2.000000, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), -5, { 1.722656, 1, 1.902344, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), -4, { 1.867188, 1, 1.804688, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), -3, { 2.015625, 1, 1.703125, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), -2, { 2.164062, 1, 1.605469, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), -1, { 2.312500, 1, 1.507812, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), 0, { 2.460938, 1, 1.410156, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), 1, { 2.753906, 1, 1.324219, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), 2, { 3.050781, 1, 1.238281, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), 3, { 3.343750, 1, 1.156250, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), 4, { 3.640625, 1, 1.070312, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), 5, { 4.000000, 1.015873, 1, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), 6, { 4.688312, 1.108225, 1, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Daylight fluorescent"), 7, { 5.545455, 1.224880, 1, 0 } }, /*6700K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), -7, { 1.195312, 1, 2.589844, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), -6, { 1.316406, 1, 2.464844, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), -5, { 1.441406, 1, 2.343750, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), -4, { 1.566406, 1, 2.222656, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), -3, { 1.687500, 1, 2.101562, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), -2, { 1.812500, 1, 1.980469, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), -1, { 1.937500, 1, 1.859375, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), 0,	{ 2.062500, 1, 1.738281, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), 1,	{ 2.308594, 1, 1.632812, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), 2,	{ 2.554688, 1, 1.527344, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), 3,	{ 2.804688, 1, 1.421875, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), 4,	{ 3.050781, 1, 1.320312, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), 5,	{ 3.296875, 1, 1.214844, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), 6,	{ 3.546875, 1, 1.109375, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Neutral fluorescent"), 7,	{ 3.792969, 1, 1.007812, 0 } }, /*5000K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), -7, { 1.109375, 1, 3.257812, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), -6, { 1.226562, 1, 3.105469, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), -5, { 1.339844, 1, 2.953125, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), -4, { 1.457031, 1, 2.796875, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), -3, { 1.570312, 1, 2.644531, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), -2, { 1.687500, 1, 2.492188, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), -1, { 1.800781, 1, 2.339844, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), 0, { 1.917969, 1, 2.187500, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), 1, { 2.144531, 1, 2.054688, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), 2, { 2.375000, 1, 1.921875, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), 3, { 2.605469, 1, 1.792969, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), 4, { 2.835938, 1, 1.660156, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), 5, { 3.066406, 1, 1.531250, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), 6, { 3.296875, 1, 1.398438, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("Cool white fluorescent"), 7, { 3.527344, 1, 1.265625, 0 } }, /*4200K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), -7, { 1, 1.347368, 5.963158, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), -6, { 1, 1.224880, 5.167464, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), -5, { 1, 1.117904, 4.484716, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), -4, { 1, 1.028112, 3.911647, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), -3, { 1.046875, 1, 3.593750, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), -2, { 1.125000, 1, 3.386719, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), -1, { 1.203125, 1, 3.179688, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), 0, { 1.281250, 1, 2.972656, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), 1, { 1.433594, 1, 2.792969, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), 2, { 1.585938, 1, 2.613281, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), 3, { 1.742188, 1, 2.437500, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), 4, { 1.894531, 1, 2.257812, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), 5, { 2.046875, 1, 2.078125, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), 6, { 2.203125, 1, 1.902344, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("White fluorescent"), 7, { 2.355469, 1, 1.722656, 0 } }, /*3500K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), -7,	{ 1, 1.488372, 6.988372, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), -6,	{ 1, 1.347368, 6.026316, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), -5,	{ 1, 1.230769, 5.235577, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), -4,	{ 1, 1.132743, 4.566372, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), -3,	{ 1, 1.049180, 4.000000, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), -2,	{ 1.023438, 1, 3.589844, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), -1,	{ 1.093750, 1, 3.371094, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), 0,	{ 1.164062, 1, 3.152344, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), 1,	{ 1.300781, 1, 2.960938, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), 2,	{ 1.441406, 1, 2.773438, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), 3,	{ 1.582031, 1, 2.582031, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), 4,	{ 1.722656, 1, 2.394531, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), 5,	{ 1.722656, 1, 2.394531, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), 6,	{ 2.000000, 1, 2.015625, 0 } }, /*3000K*/
  { "OLYMPUS", "C8080WZ", N_("Tungsten"), 7,	{ 2.140625, 1, 1.828125, 0 } }, /*3000K*/
// Fin ajout

  { "OLYMPUS", "E-1", N_("Tungsten"), -7,	{ 1.015625, 1, 1.867188, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), -6,	{ 1.007812, 1, 1.875000, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), -5,	{ 1, 1, 1.890625, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), -4,	{ 1, 1.007874, 1.913386, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), -3,	{ 1, 1.015873, 1.944444, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), -2,	{ 1, 1.015873, 1.952381, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), -1,	{ 1, 1.024000, 1.984000, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), 0,	{ 1, 1.024000, 1.992000, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), 1,	{ 1, 1.032258, 2.008065, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), 2,	{ 1, 1.040650, 2.040650, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), 3,	{ 1, 1.040650, 2.048780, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), 4,	{ 1, 1.049180, 2.081967, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), 5,	{ 1, 1.057851, 2.107438, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), 6,	{ 1, 1.066667, 2.141667, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", N_("Tungsten"), 7,	{ 1, 1.075630, 2.168067, 0 } }, /*3000K*/
  { "OLYMPUS", "E-1", "3300K", -7,		{ 1.109375, 1, 1.695312, 0 } },
  { "OLYMPUS", "E-1", "3300K", -6,              { 1.101562, 1, 1.710938, 0 } },
  { "OLYMPUS", "E-1", "3300K", -5,              { 1.093750, 1, 1.718750, 0 } },
  { "OLYMPUS", "E-1", "3300K", -4,              { 1.093750, 1, 1.734375, 0 } },
  { "OLYMPUS", "E-1", "3300K", -3,              { 1.085938, 1, 1.742188, 0 } },
  { "OLYMPUS", "E-1", "3300K", -2,              { 1.078125, 1, 1.750000, 0 } },
  { "OLYMPUS", "E-1", "3300K", -1,              { 1.070312, 1, 1.765625, 0 } },
  { "OLYMPUS", "E-1", "3300K", 0,		{ 1.070312, 1, 1.773438, 0 } },
  { "OLYMPUS", "E-1", "3300K", 1,               { 1.054688, 1, 1.781250, 0 } },
  { "OLYMPUS", "E-1", "3300K", 2,               { 1.046875, 1, 1.796875, 0 } },
  { "OLYMPUS", "E-1", "3300K", 3,               { 1.046875, 1, 1.804688, 0 } },
  { "OLYMPUS", "E-1", "3300K", 4,               { 1.039062, 1, 1.820312, 0 } },
  { "OLYMPUS", "E-1", "3300K", 5,               { 1.031250, 1, 1.828125, 0 } },
  { "OLYMPUS", "E-1", "3300K", 6,               { 1.023438, 1, 1.843750, 0 } },
  { "OLYMPUS", "E-1", "3300K", 7,		{ 1.015625, 1, 1.851562, 0 } },
  { "OLYMPUS", "E-1", N_("Incandescent"), -7,	{ 1.195312, 1, 1.562500, 0 } },  /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), -6,	{ 1.187500, 1, 1.578125, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), -5,	{ 1.187500, 1, 1.585938, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), -4,	{ 1.179688, 1, 1.601562, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), -3,	{ 1.171875, 1, 1.609375, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), -2,	{ 1.164062, 1, 1.617188, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), -1,	{ 1.156250, 1, 1.632812, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), 0,	{ 1.156250, 1, 1.640625, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), 1,	{ 1.140625, 1, 1.648438, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), 2,	{ 1.132812, 1, 1.664062, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), 3,	{ 1.125000, 1, 1.671875, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), 4,	{ 1.117188, 1, 1.679688, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), 5,	{ 1.117188, 1, 1.695312, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), 6,	{ 1.109375, 1, 1.703125, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", N_("Incandescent"), 7,	{ 1.101562, 1, 1.718750, 0 } }, /*3600K*/
  { "OLYMPUS", "E-1", "3900K", -7,		{ 1.335938, 1, 1.414062, 0 } },
  { "OLYMPUS", "E-1", "3900K", -6,              { 1.320312, 1, 1.429688, 0 } },
  { "OLYMPUS", "E-1", "3900K", -5,              { 1.304688, 1, 1.445312, 0 } },
  { "OLYMPUS", "E-1", "3900K", -4,              { 1.289062, 1, 1.460938, 0 } },
  { "OLYMPUS", "E-1", "3900K", -3,              { 1.273438, 1, 1.476562, 0 } },
  { "OLYMPUS", "E-1", "3900K", -2,              { 1.257812, 1, 1.492188, 0 } },
  { "OLYMPUS", "E-1", "3900K", -1,              { 1.242188, 1, 1.507812, 0 } },
  { "OLYMPUS", "E-1", "3900K", 0,		{ 1.234375, 1, 1.523438, 0 } },
  { "OLYMPUS", "E-1", "3900K", 1,               { 1.218750, 1, 1.531250, 0 } },
  { "OLYMPUS", "E-1", "3900K", 2,               { 1.210938, 1, 1.546875, 0 } },
  { "OLYMPUS", "E-1", "3900K", 3,               { 1.203125, 1, 1.554688, 0 } },
  { "OLYMPUS", "E-1", "3900K", 4,               { 1.195312, 1, 1.562500, 0 } },
  { "OLYMPUS", "E-1", "3900K", 5,               { 1.187500, 1, 1.578125, 0 } },
  { "OLYMPUS", "E-1", "3900K", 6,               { 1.187500, 1, 1.585938, 0 } },
  { "OLYMPUS", "E-1", "3900K", 7,		{ 1.179688, 1, 1.601562, 0 } },
  { "OLYMPUS", "E-1", N_("White fluorescent"), -7, { 2.296875, 1, 1.445312, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), -6, { 2.273438, 1, 1.468750, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), -5, { 2.242188, 1, 1.492188, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), -4, { 2.210938, 1, 1.523438, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), -3, { 2.171875, 1, 1.562500, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), -2, { 2.132812, 1, 1.601562, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), -1, { 2.093750, 1, 1.640625, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), 0,  { 2.062500, 1, 1.679688, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), 1,  { 2.039062, 1, 1.703125, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), 2,  { 2.015625, 1, 1.734375, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), 3,  { 2.000000, 1, 1.757812, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), 4,  { 1.984375, 1, 1.789062, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), 5,  { 1.968750, 1, 1.812500, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), 6,  { 1.945312, 1, 1.835938, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", N_("White fluorescent"), 7,  { 1.929688, 1, 1.867188, 0 } }, /*4000K*/
  { "OLYMPUS", "E-1", "4300K", -7,		{ 1.484375, 1, 1.281250, 0 } },
  { "OLYMPUS", "E-1", "4300K", -6,              { 1.468750, 1, 1.289062, 0 } },
  { "OLYMPUS", "E-1", "4300K", -5,              { 1.460938, 1, 1.296875, 0 } },
  { "OLYMPUS", "E-1", "4300K", -4,              { 1.445312, 1, 1.304688, 0 } },
  { "OLYMPUS", "E-1", "4300K", -3,              { 1.437500, 1, 1.312500, 0 } },
  { "OLYMPUS", "E-1", "4300K", -2,              { 1.429688, 1, 1.328125, 0 } },
  { "OLYMPUS", "E-1", "4300K", -1,              { 1.414062, 1, 1.335938, 0 } },
  { "OLYMPUS", "E-1", "4300K", 0,		{ 1.414062, 1, 1.343750, 0 } },
  { "OLYMPUS", "E-1", "4300K", 1,               { 1.390625, 1, 1.359375, 0 } },
  { "OLYMPUS", "E-1", "4300K", 2,               { 1.375000, 1, 1.375000, 0 } },
  { "OLYMPUS", "E-1", "4300K", 3,               { 1.359375, 1, 1.390625, 0 } },
  { "OLYMPUS", "E-1", "4300K", 4,               { 1.343750, 1, 1.406250, 0 } },
  { "OLYMPUS", "E-1", "4300K", 5,               { 1.328125, 1, 1.421875, 0 } },
  { "OLYMPUS", "E-1", "4300K", 6,               { 1.312500, 1, 1.437500, 0 } },
  { "OLYMPUS", "E-1", "4300K", 7,		{ 1.296875, 1, 1.453125, 0 } },
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), -7, { 1.984375, 1, 1.203125, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), -6, { 1.960938, 1, 1.218750, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), -5, { 1.937500, 1, 1.234375, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), -4, { 1.921875, 1, 1.257812, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), -3, { 1.898438, 1, 1.273438, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), -2, { 1.875000, 1, 1.289062, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), -1, { 1.851562, 1, 1.304688, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), 0,  { 1.835938, 1, 1.320312, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), 1,  { 1.804688, 1, 1.343750, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), 2,  { 1.773438, 1, 1.367188, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), 3,  { 1.750000, 1, 1.390625, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), 4,  { 1.718750, 1, 1.414062, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), 5,  { 1.695312, 1, 1.437500, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), 6,  { 1.656250, 1, 1.476562, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", N_("Neutral fluorescent"), 7,  { 1.617188, 1, 1.515625, 0 } }, /*4500K*/
  { "OLYMPUS", "E-1", "4800K", -7,		{ 1.601562, 1, 1.179688, 0 } },
  { "OLYMPUS", "E-1", "4800K", -6,              { 1.593750, 1, 1.187500, 0 } },
  { "OLYMPUS", "E-1", "4800K", -5,              { 1.585938, 1, 1.195312, 0 } },
  { "OLYMPUS", "E-1", "4800K", -4,              { 1.578125, 1, 1.203125, 0 } },
  { "OLYMPUS", "E-1", "4800K", -3,              { 1.562500, 1, 1.203125, 0 } },
  { "OLYMPUS", "E-1", "4800K", -2,              { 1.554688, 1, 1.210938, 0 } },
  { "OLYMPUS", "E-1", "4800K", -1,              { 1.546875, 1, 1.218750, 0 } },
  { "OLYMPUS", "E-1", "4800K", 0,		{ 1.546875, 1, 1.226562, 0 } },
  { "OLYMPUS", "E-1", "4800K", 1,               { 1.531250, 1, 1.234375, 0 } },
  { "OLYMPUS", "E-1", "4800K", 2,               { 1.515625, 1, 1.242188, 0 } },
  { "OLYMPUS", "E-1", "4800K", 3,               { 1.507812, 1, 1.257812, 0 } },
  { "OLYMPUS", "E-1", "4800K", 4,               { 1.500000, 1, 1.265625, 0 } },
  { "OLYMPUS", "E-1", "4800K", 5,               { 1.484375, 1, 1.273438, 0 } },
  { "OLYMPUS", "E-1", "4800K", 6,               { 1.476562, 1, 1.281250, 0 } },
  { "OLYMPUS", "E-1", "4800K", 7,		{ 1.460938, 1, 1.289062, 0 } },
  { "OLYMPUS", "E-1", N_("Daylight"), -7,	{ 1.726562, 1, 1.093750, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), -6,	{ 1.710938, 1, 1.101562, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), -5,	{ 1.703125, 1, 1.109375, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), -4,	{ 1.695312, 1, 1.117188, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), -3,	{ 1.687500, 1, 1.117188, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), -2,	{ 1.671875, 1, 1.125000, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), -1,	{ 1.664062, 1, 1.132812, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), 0,	{ 1.664062, 1, 1.140625, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), 1,	{ 1.648438, 1, 1.148438, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), 2,	{ 1.640625, 1, 1.156250, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), 3,	{ 1.632812, 1, 1.164062, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), 4,	{ 1.617188, 1, 1.164062, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), 5,	{ 1.609375, 1, 1.171875, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), 6,	{ 1.601562, 1, 1.179688, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Daylight"), 7,	{ 1.593750, 1, 1.187500, 0 } }, /*5300K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), -7,		{ 2.008130, 1.040650, 1, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), -6,		{ 1.967742, 1.032258, 1, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), -5,		{ 1.920635, 1.015873, 1, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), -4,		{ 1.867188, 1, 1, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), -3,		{ 1.851562, 1, 1.007812, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), -2,		{ 1.828125, 1, 1.023438, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), -1,		{ 1.812500, 1, 1.031250, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), 0,		{ 1.796875, 1, 1.046875, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), 1,		{ 1.781250, 1, 1.054688, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), 2,		{ 1.773438, 1, 1.062500, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), 3,		{ 1.757812, 1, 1.070312, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), 4,		{ 1.750000, 1, 1.070312, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), 5,		{ 1.742188, 1, 1.078125, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), 6,		{ 1.734375, 1, 1.085938, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Cloudy"), 7,		{ 1.726562, 1, 1.093750, 0 } }, /*6000K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), -7, { 2.819820, 1.153153, 1, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), -6, { 2.669565, 1.113043, 1, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), -5, { 2.521008, 1.075630, 1, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), -4, { 2.390244, 1.040650, 1, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), -3, { 2.259843, 1.007874, 1, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), -2, { 2.195312, 1, 1.023438, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), -1, { 2.140625, 1, 1.054688, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), 0,  { 2.101562, 1, 1.085938, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), 1,  { 2.070312, 1, 1.101562, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), 2,  { 2.046875, 1, 1.117188, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), 3,  { 2.023438, 1, 1.132812, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), 4,  { 2.000000, 1, 1.156250, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), 5,  { 1.976562, 1, 1.171875, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), 6,  { 1.953125, 1, 1.187500, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Daylight fluorescent"), 7,  { 1.929688, 1, 1.203125, 0 } }, /*6600K*/
  { "OLYMPUS", "E-1", N_("Shade"), -7,		{ 2.584906, 1.207547, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), -6,		{ 2.532710, 1.196262, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), -5,		{ 2.467890, 1.174312, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), -4,		{ 2.396396, 1.153153, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), -3,		{ 2.357143, 1.142857, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), -2,		{ 2.289474, 1.122807, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), -1,		{ 2.252174, 1.113043, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), 0,		{ 2.196581, 1.094017, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), 1,		{ 2.126050, 1.075630, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), 2,		{ 2.091667, 1.066667, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), 3,		{ 2.032787, 1.049180, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), 4,		{ 2.000000, 1.040650, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), 5,		{ 1.944000, 1.024000, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), 6,		{ 1.897638, 1.007874, 1, 0 } }, /*7500K*/
  { "OLYMPUS", "E-1", N_("Shade"), 7,		{ 1.859375, 1, 1, 0 } }, /*7500K*/

  { "OLYMPUS", "E-10", N_("Incandescent"), 0,	{ 1, 1.153153, 3.441442, 0 } }, /*3000K*/
  { "OLYMPUS", "E-10", N_("Incandescent warm"), 0, { 1.101562, 1, 2.351562, 0 } }, /*3700K*/
  { "OLYMPUS", "E-10", N_("White fluorescent"), 0, { 1.460938, 1, 2.546875, 0 } }, /*4000K*/
  { "OLYMPUS", "E-10", N_("Daylight fluorescent"), 0, { 1.460938, 1, 1.843750, 0 } }, /*4500K*/
  { "OLYMPUS", "E-10", N_("Daylight"), 0,	{ 1.523438, 1, 1.617188, 0 } }, /*5500K*/
  { "OLYMPUS", "E-10", N_("Cloudy"), 0,		{ 1.687500, 1, 1.437500, 0 } }, /*6500K*/
  { "OLYMPUS", "E-10", N_("Shade"), 0,		{ 1.812500, 1, 1.312500, 0 } }, /*7500K*/

  { "OLYMPUS", "E-300", N_("Incandescent"), -7,	{ 1.179688, 1, 2.125000, 0 } },
  { "OLYMPUS", "E-300", N_("Incandescent"), 0,	{ 1.140625, 1, 2.203125, 0 } },
  { "OLYMPUS", "E-300", N_("Incandescent"), 7,	{ 1.093750, 1, 2.273438, 0 } },
  { "OLYMPUS", "E-300", N_("Incandescent warm"), -7, { 1.382812, 1, 1.859375, 0 } },
  { "OLYMPUS", "E-300", N_("Incandescent warm"), 0, { 1.312500, 1, 1.906250, 0 } },
  { "OLYMPUS", "E-300", N_("Incandescent warm"), 7, { 1.257812, 1, 1.984375, 0 } },
  { "OLYMPUS", "E-300", N_("White fluorescent"), -7, { 2.109375, 1, 1.710938, 0 } },
  { "OLYMPUS", "E-300", N_("White fluorescent"), 0, { 1.976562, 1, 1.921875, 0 } },
  { "OLYMPUS", "E-300", N_("White fluorescent"), 7, { 1.804688, 1, 2.062500, 0 } },
  { "OLYMPUS", "E-300", N_("Neutral fluorescent"), -7, { 1.945312, 1, 1.445312, 0 } },
  { "OLYMPUS", "E-300", N_("Neutral fluorescent"), 0, { 1.820312, 1, 1.562500, 0 } },
  { "OLYMPUS", "E-300", N_("Neutral fluorescent"), 7, { 1.585938, 1, 1.945312, 0 } },
  { "OLYMPUS", "E-300", N_("Daylight fluorescent"), -7,	{ 2.203125, 1, 1.000000, 0 } },
  { "OLYMPUS", "E-300", N_("Daylight fluorescent"), 0, { 2.031250, 1, 1.328125, 0 } },
  { "OLYMPUS", "E-300", N_("Daylight fluorescent"), 7, { 1.765625, 1, 1.367188, 0 } },
  { "OLYMPUS", "E-300", N_("Daylight"), -7,	{ 1.835938, 1, 1.304688, 0 } },
  { "OLYMPUS", "E-300", N_("Daylight"), 0,	{ 1.789062, 1, 1.351562, 0 } },
  { "OLYMPUS", "E-300", N_("Daylight"), 7,	{ 1.726562, 1, 1.398438, 0 } },
  { "OLYMPUS", "E-300", N_("Cloudy"), -7,	{ 2.000000, 1, 1.156250, 0 } },
  { "OLYMPUS", "E-300", N_("Cloudy"), 0,	{ 1.890625, 1, 1.257812, 0 } },
  { "OLYMPUS", "E-300", N_("Cloudy"), 7,	{ 1.835938, 1, 1.304688, 0 } },
  { "OLYMPUS", "E-300", N_("Shade"), -7,	{ 2.179688, 1, 1.007812, 0 } },
  { "OLYMPUS", "E-300", N_("Shade"), 0,		{ 2.070312, 1, 1.109375, 0 } },
  { "OLYMPUS", "E-300", N_("Shade"), 7,		{ 1.945312, 1, 1.210938, 0 } },

  { "OLYMPUS", "E-500", N_("Daylight"), 0,	{ 1.898438, 1, 1.359375, 0 } }, /*5300K*/
  { "OLYMPUS", "E-500", N_("Cloudy"), 0,	{ 1.992188, 1, 1.265625, 0 } }, /*6000K*/
  { "OLYMPUS", "E-500", N_("Shade"), 0,		{ 2.148438, 1, 1.125000, 0 } }, /*7500K*/
  { "OLYMPUS", "E-500", N_("Incandescent"), 0,	{ 1.265625, 1, 2.195312, 0 } }, /*3000K*/
  { "OLYMPUS", "E-500", N_("White fluorescent"), 0, { 1.976562, 1, 1.914062, 0 } }, /*4000K*/
  { "OLYMPUS", "E-500", N_("Neutral fluorescent"), 0, { 1.828125, 1, 1.562500, 0 } }, /*4500K*/
  { "OLYMPUS", "E-500", N_("Daylight fluorescent"), 0, { 2.046875, 1, 1.359375, 0 } }, /*6600K*/

  { "OLYMPUS", "SP510UZ", N_("Daylight"), 0, { 1.656250, 1, 1.621094, 0 } },
  { "OLYMPUS", "SP510UZ", N_("Cloudy"), 0, { 1.789063, 1, 1.546875, 0 } },
  { "OLYMPUS", "SP510UZ", N_("Incandescent"), 0, { 1, 1.066667, 2.891667, 0 } },
  { "OLYMPUS", "SP510UZ", N_("White fluorescent"), 0, { 1.929688, 1, 1.562500, 0 } },
  { "OLYMPUS", "SP510UZ", N_("Neutral fluorescent"), 0, { 1.644531, 1, 1.843750, 0 } },
  { "OLYMPUS", "SP510UZ", N_("Daylight fluorescent"), 0, { 1.628906, 1, 2.210938, 0 } },

  { "Panasonic", "DMC-FZ30", N_("Daylight"), 0,	{ 1.757576, 1, 1.446970, 0 } },
  { "Panasonic", "DMC-FZ30", N_("Cloudy"), 0,	{ 1.943182, 1, 1.276515, 0 } },
  { "Panasonic", "DMC-FZ30", N_("Fluorescent"), 0, { 1.098485, 1, 2.106061, 0 } },
  { "Panasonic", "DMC-FZ30", N_("Flash"), 0,	{ 1.965909, 1, 1.303030, 0 } },

  { "Panasonic", "DMC-FZ50", N_("Daylight"), 0, { 2.095057, 1, 1.642586, 0 } },
  { "Panasonic", "DMC-FZ50", N_("Cloudy"), 0,	{ 2.319392, 1, 1.482890, 0 } },
  { "Panasonic", "DMC-FZ50", N_("Shade"), 0, { 2.463878, 1, 1.414449, 0 } },
  { "Panasonic", "DMC-FZ50", N_("Fluorescent"), 0, { 1.365019, 1, 2.311787, 0 } },
  { "Panasonic", "DMC-FZ50", N_("Flash"), 0,	{ 2.338403, 1, 1.338403, 0 } },

  /* It seemst that the *ist DL WB settings are not really presets. */
  { "PENTAX", "*ist DL", N_("Daylight"), 0,	{ 1.546875, 1, 1.007812, 0 } },
  { "PENTAX", "*ist DL", N_("Shade"), 0,	{ 1.933594, 1, 1.027344, 0 } },
  { "PENTAX", "*ist DL", N_("Cloudy"), 0,	{ 1.703125, 1, 1.003906, 0 } },
  { "PENTAX", "*ist DL", N_("Daylight fluorescent"), 0, { 2.593909, 1.299492, 1, 0 } },
  { "PENTAX", "*ist DL", N_("Neutral fluorescent"), 0, { 1.539062, 1, 1.003906, 0 } },
  { "PENTAX", "*ist DL", N_("White fluorescent"), 0, { 1.390625, 1, 1.117188, 0 } },
  { "PENTAX", "*ist DL", N_("Tungsten"), 0,	{ 1.000000, 1, 2.074219, 0 } },
  { "PENTAX", "*ist DL", N_("Flash"), 0,	{ 1.621094, 1, 1.027344, 0 } },

  /* It seemst that the *ist DS WB settings are not really presets. */
  { "PENTAX", "*ist DS", N_("Daylight"), 0,	{ 1.632812, 1, 1, 0 } },
  { "PENTAX", "*ist DS", N_("Shade"), 0,	{ 1.964844, 1, 1, 0 } },
  { "PENTAX", "*ist DS", N_("Cloudy"), 0,	{ 1.761719, 1, 1, 0 } },
  { "PENTAX", "*ist DS", N_("Daylight fluorescent"), 0, { 1.910156, 1, 1, 0 } },
  { "PENTAX", "*ist DS", N_("Neutral fluorescent"), 0, { 1.521569, 1.003922, 1, 0 } },
  { "PENTAX", "*ist DS", N_("White fluorescent"), 0, { 1.496094, 1, 1.023438, 0 } },
  { "PENTAX", "*ist DS", N_("Tungsten"), 0,	{ 1, 1, 2.027344, 0 } },
  { "PENTAX", "*ist DS", N_("Flash"), 0,	{ 1.695312, 1, 1, 0 } },

  { "PENTAX", "K10D", N_("Daylight"), 0, { 1.660156, 1.000000, 1.066406, 0 } },
  { "PENTAX", "K10D", N_("Shade"), 0, { 2.434783, 1.236715, 1.000000, 0 } },
  { "PENTAX", "K10D", N_("Cloudy"), 0, { 1.872428, 1.053498, 1.000000, 0 } },
  { "PENTAX", "K10D", N_("Daylight fluorescent"), 0, { 2.121094, 1.000000, 1.078125, 0 } },
  { "PENTAX", "K10D", N_("Neutral fluorescent"), 0, { 1.773438, 1.000000, 1.226562, 0 } },
  { "PENTAX", "K10D", N_("White fluorescent"), 0, { 1.597656, 1.000000, 1.488281, 0 } },
  { "PENTAX", "K10D", N_("Tungsten"), 0, { 1.000000, 1.000000, 2.558594, 0 } },
  { "PENTAX", "K10D", N_("Flash"), 0, { 1.664062, 1.000000, 1.046875, 0 } },

  { "PENTAX", "K100D", N_("Daylight"), 0,	{ 1.468750, 1, 1.023438, 0 } },
  { "PENTAX", "K100D", N_("Shade"), 0,	{ 1.769531, 1, 1, 0 } },
  { "PENTAX", "K100D", N_("Cloudy"), 0,	{ 1.589844, 1, 1, 0 } },
  { "PENTAX", "K100D", N_("Daylight fluorescent"), 0, { 1.722656, 1, 1.039063, 0 } },
  { "PENTAX", "K100D", N_("Neutral fluorescent"), 0, { 1.425781, 1, 1.160156, 0 } },
  { "PENTAX", "K100D", N_("White fluorescent"), 0, { 1.265625, 1, 1.414063, 0 } },
  { "PENTAX", "K100D", N_("Tungsten"), 0,	{ 1, 1.015873, 2.055556, 0 } },
  { "PENTAX", "K100D", N_("Flash"), 0,	{ 1.527344, 1, 1, 0 } },

  { "SAMSUNG", "GX-1S", N_("Daylight"), 0,	{ 1.574219, 1, 1.109375, 0 } },
  { "SAMSUNG", "GX-1S", N_("Shade"), 0,		{ 1.855469, 1, 1.000000, 0 } },
  { "SAMSUNG", "GX-1S", N_("Cloudy"), 0,	{ 1.664062, 1, 1.000000, 0 } },
  { "SAMSUNG", "GX-1S", N_("Daylight fluorescent"), 0, { 1.854251, 1.036437, 1, 0 } },
  { "SAMSUNG", "GX-1S", N_("Neutral fluorescent"), 0, { 1.574219, 1, 1.171875, 0 } },
  { "SAMSUNG", "GX-1S", N_("White fluorescent"), 0, { 1.363281, 1, 1.335938, 0 } },
  { "SAMSUNG", "GX-1S", N_("Tungsten"), 0,	{ 1.000000, 1, 2.226562, 0 } },
  { "SAMSUNG", "GX-1S", N_("Flash"), 0,		{ 1.609375, 1, 1.031250, 0 } },

  { "SONY", "DSLR-A100", N_("Daylight"), -3,	{ 1.601562, 1, 2.101562, 0 } },
  { "SONY", "DSLR-A100", N_("Daylight"), 0,	{ 1.746094, 1, 1.843750, 0 } },
  { "SONY", "DSLR-A100", N_("Daylight"), 3,	{ 1.914062, 1, 1.628906, 0 } },
  { "SONY", "DSLR-A100", N_("Shade"), -3,	{ 1.906250, 1, 1.843750, 0 } },
  { "SONY", "DSLR-A100", N_("Shade"), 0,	{ 2.070312, 1, 1.609375, 0 } },
  { "SONY", "DSLR-A100", N_("Shade"), 3,	{ 2.281250, 1, 1.429688, 0 } },
  { "SONY", "DSLR-A100", N_("Cloudy"), -3,	{ 1.691406, 1, 1.863281, 0 } },
  { "SONY", "DSLR-A100", N_("Cloudy"), 0,	{ 1.855469, 1, 1.628906, 0 } },
  { "SONY", "DSLR-A100", N_("Cloudy"), 3,	{ 2.023438, 1, 1.445312, 0 } },
  { "SONY", "DSLR-A100", N_("Tungsten"), -3,    { 1, 1.028112, 4.610442, 0 } },
  { "SONY", "DSLR-A100", N_("Tungsten"), 0,	{ 1.054688, 1, 3.917969, 0 } },
  { "SONY", "DSLR-A100", N_("Tungsten"), 3,	{ 1.164062, 1, 3.476562, 0 } },
  { "SONY", "DSLR-A100", N_("Fluorescent"), -2,	{ 1.058594, 1, 4.453125, 0 } },
  { "SONY", "DSLR-A100", N_("Fluorescent"), 0,	{ 1.718750, 1, 3.058594, 0 } },
  { "SONY", "DSLR-A100", N_("Fluorescent"), 3,	{ 2.238281, 1, 1.949219, 0 } },
  { "SONY", "DSLR-A100", N_("Fluorescent"), 4,	{ 1.992188, 1, 1.757812, 0 } },
  { "SONY", "DSLR-A100", N_("Flash"), -3,	{ 1.710938, 1, 1.988281, 0 } },
  { "SONY", "DSLR-A100", N_("Flash"), 0,	{ 1.859375, 1, 1.746094, 0 } },
  { "SONY", "DSLR-A100", N_("Flash"), 3,	{ 2.046875, 1, 1.542969, 0 } },

};

const int wb_preset_count = sizeof(wb_preset) / sizeof(wb_data);
