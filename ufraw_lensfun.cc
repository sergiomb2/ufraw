/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_lensfun.cc - define lensfun UFObject settings.
 * Copyright 2004-2010 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ufraw.h"
#include <glib/gi18n.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_LENSFUN
namespace UFRaw {

class Lensfun : public UFGroup {
public:
    Lensfun();
    void Interpolate(const lfLens *lens);
};

extern "C" { UFName ufModel = "Model"; }

extern "C" { UFName ufTCA = "TCA"; }
class TCA : public UFArray {
public:
    TCA() : UFArray(ufTCA,
	    lfLens::GetTCAModelDesc(LF_TCA_MODEL_NONE, NULL, NULL)) {
	for (lfTCAModel model = LF_TCA_MODEL_NONE; ;
		model = lfTCAModel(model+1)) {
	    const lfParameter **params;
	    const char *model_name =
		    lfLens::GetTCAModelDesc(model, NULL, &params);
	    if (model_name == NULL)
		break; // End of loop.
	    UFGroup &Model = *(new UFGroup(ufModel, model_name));
	    (*this) << &Model;
	    assert(params != NULL);
	    for (int i = 0; params[i] != NULL; i++)
		Model << new UFNumber(params[i]->Name, params[i]->Min,
			params[i]->Max, params[i]->Default);
	}
    }
    void Event(UFEventType type) {
	if (type != uf_value_changed)
	    return UFObject::Event(type);
	ufraw_data *uf = ufraw_image_get_data(this);
	if (uf == NULL)
	    return UFObject::Event(type);
	if (uf->conf->lens == NULL)
	    return UFObject::Event(type);
	if (uf->conf->lens->CalibTCA != NULL)
	    while (uf->conf->lens->CalibTCA[0] != NULL)
		uf->conf->lens->RemoveCalibTCA(0);
	lfLensCalibTCA calib;
	calib.Model = static_cast<lfTCAModel>(Index());
	calib.Focal = uf->conf->focal_len;
	const lfParameter **params;
	lfLens::GetTCAModelDesc(calib.Model, NULL, &params);
	if (params != NULL) {
	    UFGroup &Model = (*this)[StringValue()];
	    for (int i = 0; params[i] != NULL; i++) {
		UFNumber &Param = Model[params[i]->Name];
		calib.Terms[i] = Param.DoubleValue();
	    }
	}
	uf->conf->lens->AddCalibTCA(&calib);
	ufraw_invalidate_tca_layer(uf);
	return UFObject::Event(type);
    }
    void Interpolate(const lfLens *lens) {
	ufraw_data *uf = ufraw_image_get_data(this);
	if (uf == NULL)
	    return;
	lfLensCalibTCA calib;
	if (!lens->InterpolateTCA(uf->conf->focal_len, calib)) {
	    SetIndex(0);
	    return;
	}
	SetIndex(calib.Model - LF_TCA_MODEL_NONE);
	const lfParameter **params;
	lfLens::GetTCAModelDesc(calib.Model, NULL, &params);
	if (params != NULL) {
	    UFGroup &Model = (*this)[StringValue()];
	    for (int i = 0; params[i] != NULL; i++) {
		UFNumber &Param = Model[params[i]->Name];
		Param.Set(calib.Terms[i]);
	    }
	}
    }
};

extern "C" { UFName ufVignetting = "Vignetting"; }
class Vignetting : public UFArray {
public:
    Vignetting() : UFArray(ufVignetting,
	    lfLens::GetVignettingModelDesc(LF_VIGNETTING_MODEL_NONE,
		    NULL, NULL)) {
	for (lfVignettingModel model = LF_VIGNETTING_MODEL_NONE; ;
		model = lfVignettingModel(model+1)) {
	    const lfParameter **params;
	    const char *model_name =
		    lfLens::GetVignettingModelDesc(model, NULL, &params);
	    if (model_name == NULL)
		break; // End of loop.
	    UFGroup &Model = *(new UFGroup(ufModel, model_name));
	    (*this) << &Model;
	    assert(params != NULL);
	    for (int i = 0; params[i] != NULL; i++)
		Model << new UFNumber(params[i]->Name, params[i]->Min,
			params[i]->Max, params[i]->Default);
	}
    }
    void Event(UFEventType type) {
	if (type != uf_value_changed)
	    return UFObject::Event(type);
	ufraw_data *uf = ufraw_image_get_data(this);
	if (uf == NULL)
	    return UFObject::Event(type);
	if (uf->conf->lens == NULL)
	    return UFObject::Event(type);
	if (uf->conf->lens->CalibVignetting != NULL)
	    while (uf->conf->lens->CalibVignetting[0] != NULL)
		uf->conf->lens->RemoveCalibVignetting(0);
	lfLensCalibVignetting calib;
	calib.Model = static_cast<lfVignettingModel>(Index());
	calib.Focal = uf->conf->focal_len;
	calib.Aperture = uf->conf->aperture;
	calib.Distance = uf->conf->subject_distance;
	const lfParameter **params;
	lfLens::GetVignettingModelDesc(calib.Model, NULL, &params);
	if (params != NULL) {
	    UFGroup &Model = (*this)[StringValue()];
	    for (int i = 0; params[i] != NULL; i++) {
		UFNumber &Param = Model[params[i]->Name];
		calib.Terms[i] = Param.DoubleValue();
	    }
	}
	uf->conf->lens->AddCalibVignetting(&calib);
	ufraw_invalidate_layer(uf, ufraw_first_phase);
	return UFObject::Event(type);
    }
    void Interpolate(const lfLens *lens) {
	ufraw_data *uf = ufraw_image_get_data(this);
	if (uf == NULL)
	    return;
	lfLensCalibVignetting calib;
	if (!lens->InterpolateVignetting(uf->conf->focal_len,
		uf->conf->aperture, uf->conf->subject_distance, calib)) {
	    SetIndex(0);
	    return;
	}
	SetIndex(calib.Model - LF_VIGNETTING_MODEL_NONE);
	const lfParameter **params;
	lfLens::GetVignettingModelDesc(calib.Model, NULL, &params);
	if (params != NULL) {
	    UFGroup &Model = (*this)[StringValue()];
	    for (int i = 0; params[i] != NULL; i++) {
		UFNumber &Param = Model[params[i]->Name];
		Param.Set(calib.Terms[i]);
	    }
	}
    }
};

extern "C" { UFName ufDistortion = "Distortion"; }
class Distortion : public UFArray {
public:
    // The elements of Distortion are distortion models:
    // UFGroup &Model = (*this)[StringValue()];
    // The elements of Model are the model's parameters:
    // UFNumber Param = Model[params[i].Name];
    // In XML it would look something like this:
    // <Distortion Index='PanoTools lens model'>
    //   <Model Label='5th order polynomial'>
    //     <k1>0.0100</k1>
    //     <k2>-0.0100</k2>
    //   </Model>
    //   <Model Label='PanoTools lens model'>
    //     <a>0.0010</a>
    //     <b>0.0023</b>
    //   </Model>
    // </Distortion>
    Distortion() : UFArray(ufDistortion,
	    lfLens::GetDistortionModelDesc(LF_DIST_MODEL_NONE, NULL, NULL)) {
	for (lfDistortionModel model = LF_DIST_MODEL_NONE; ;
		model = lfDistortionModel(model+1)) {
	    const lfParameter **params;
	    const char *model_name =
		    lfLens::GetDistortionModelDesc(model, NULL, &params);
	    if (model_name == NULL)
		break; // End of loop.
	    UFGroup &Model = *(new UFGroup(ufModel, model_name));
	    (*this) << &Model;
	    assert(params != NULL);
	    for (int i = 0; params[i] != NULL; i++)
		Model << new UFNumber(params[i]->Name, params[i]->Min,
			params[i]->Max, params[i]->Default);
	}
    }
    void Event(UFEventType type) {
	if (type != uf_value_changed)
	    return UFObject::Event(type);
	ufraw_data *uf = ufraw_image_get_data(this);
	if (uf == NULL)
	    return UFObject::Event(type);
	if (uf->conf->lens == NULL)
	    return UFObject::Event(type);
	if (uf->conf->lens->CalibDistortion != NULL)
	    while (uf->conf->lens->CalibDistortion[0] != NULL)
		uf->conf->lens->RemoveCalibDistortion(0);
	lfLensCalibDistortion calib;
	calib.Model = static_cast<lfDistortionModel>(Index());
	calib.Focal = uf->conf->focal_len;
	const lfParameter **params;
	lfLens::GetDistortionModelDesc(calib.Model, NULL, &params);
	if (params != NULL) {
	    UFGroup &Model = (*this)[StringValue()];
	    for (int i = 0; params[i] != NULL; i++) {
		UFNumber &Param = Model[params[i]->Name];
		calib.Terms[i] = Param.DoubleValue();
	    }
	}
	uf->conf->lens->AddCalibDistortion(&calib);
	ufraw_invalidate_layer(uf, ufraw_transform_phase);
	return UFObject::Event(type);
    }
    void Interpolate(const lfLens *lens) {
	ufraw_data *uf = ufraw_image_get_data(this);
	if (uf == NULL)
	    return;
	lfLensCalibDistortion calib;
	if (!lens->InterpolateDistortion(uf->conf->focal_len, calib)) {
	    SetIndex(0);
	    return;
	}
	SetIndex(calib.Model - LF_DIST_MODEL_NONE);
	const lfParameter **params;
	lfLens::GetDistortionModelDesc(calib.Model, NULL, &params);
	if (params != NULL) {
	    UFGroup &Model = (*this)[StringValue()];
	    for (int i = 0; params[i] != NULL; i++) {
		UFNumber &Param = Model[params[i]->Name];
		Param.Set(calib.Terms[i]);
	    }
	}
    }
};

extern "C" { UFName ufLensfun = "Lensfun"; }
Lensfun::Lensfun() : UFGroup(ufLensfun) {
    *this
	<< new TCA
	<< new Vignetting
	<< new Distortion
    ;
}

void Lensfun::Interpolate(const lfLens *lens) {
    static_cast<TCA &>((*this)[ufTCA]).Interpolate(lens);
    static_cast<Vignetting &>((*this)[ufVignetting]).Interpolate(lens);
    static_cast<Distortion &>((*this)[ufDistortion]).Interpolate(lens);
}

extern "C" {
void ufraw_lensfun_interpolate(UFObject *lensfun, const lfLens *lens) {
    Lensfun &Lensfun = dynamic_cast<UFRaw::Lensfun &>(*lensfun);
    Lensfun.Interpolate(lens);
}

void ufraw_lensfun_init(ufraw_data *uf)
{
    /* Load lens database only once */
    static lfDatabase *lensdb = NULL;
    if (lensdb == NULL) {
	lensdb = lf_db_new();
	lf_db_load(lensdb);
    }
    uf->conf->lensdb = lensdb;

    /* Create a default lens & camera */
    uf->conf->lens = lf_lens_new();
    uf->conf->camera = lf_camera_new();
    uf->conf->cur_lens_type = LF_UNKNOWN;

    /* Set lens and camera from EXIF info, if possible */
    if (uf->conf->real_make[0] || uf->conf->real_model[0]) {
	const lfCamera **cams = lf_db_find_cameras(uf->conf->lensdb,
		uf->conf->real_make, uf->conf->real_model);
	if (cams != NULL) {
	    lf_camera_copy(uf->conf->camera, cams[0]);
	    lf_free(cams);
	}
    }
    if (strlen(uf->conf->lensText) > 0) {
	const lfLens **lenses = lf_db_find_lenses_hd(uf->conf->lensdb,
		uf->conf->camera, NULL, uf->conf->lensText, 0);
	if (lenses != NULL) {
	    lf_lens_copy(uf->conf->lens, lenses[0]);
	    lf_free(lenses);
	}
    }
    UFGroup &Image = *uf->conf->ufobject;
    UFGroup &Lensfun = Image[ufLensfun];
    if (uf->conf->lensfunMode == lensfun_default) {
	if (uf->LoadingID) {
	    Lensfun[ufTCA].Event(uf_value_changed);
	    Lensfun[ufVignetting].Event(uf_value_changed);
	    Lensfun[ufDistortion].Event(uf_value_changed);
	    return;
	}
	uf->conf->lensfunMode = lensfun_auto;
    }
    if (uf->conf->lensfunMode == lensfun_none) {
	Lensfun[ufTCA].Reset();
	Lensfun[ufVignetting].Reset();
	Lensfun[ufDistortion].Reset();
    } else {
	static_cast<UFRaw::Lensfun &>(Lensfun).Interpolate(uf->conf->lens);
    }
}

}

} // namespace UFRaw
#endif // HAVE_LENSFUN
