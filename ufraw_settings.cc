/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_settings.cc - define all UFObject settings.
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

namespace UFRaw {

// Shortcut for UFString definitions
#define UF_STRING(Class, Name, String, ...) \
    extern "C" { UFName Name = String; } \
    class Class : public UFString { \
    public: \
	Class() : UFString(Name, __VA_ARGS__) { } \
    }
// We could have also used a template:
/*
template <const char *name>
    class UF_STRING : public UFString {
    public:
	UF_STRING() : UFString(name) { }
    };

typedef UF_STRING<ufCameraMake> UFCameraMake;
*/

// Shortcut for UFNumber definitions
#define UF_NUMBER(Class, Name, String, ...) \
    extern "C" { UFName Name = String; } \
    class Class : public UFNumber { \
    public: \
	Class() : UFNumber(Name, __VA_ARGS__) { } \
    }

class Image : public UFGroup {
private:
public:
    // uf should be private
    ufraw_data *uf;
    explicit Image(ufraw_data *data = NULL);
    void SetUFRawData(ufraw_data *data);
    void SetWB(const char *mode = NULL);
    void Message(const char *Format, ...) const {
	if (Format == NULL)
	    return;
	va_list ap;
	va_start(ap, Format);
	char *message = g_strdup_vprintf(Format, ap);
	va_end(ap);
	ufraw_message(UFRAW_ERROR, "%s: %s\n", Name(), message);
	g_free(message);
    }
};

static Image &ParentImage(UFObject *obj) {
    return static_cast<Image &>(obj->Parent());
}

//UF_STRING(CameraMake, ufCameraMake, "Make", "");

extern "C" { UFName ufWB = "WB"; }
class WB : public UFString {
public:
    WB() : UFString(ufWB, uf_camera_wb) { }
    void Event(UFEventType type) {
	// spot_wb is a temporary value, that would be changed in SetWB()
	if (!this->IsEqual(uf_spot_wb))
	    UFObject::Event(type);
    }
    void OriginalValueChangedEvent() {
	/* Keep compatibility with old numbers from ufraw-0.6 */
	int i;
	if (sscanf(StringValue(), "%d", &i) == 1) {
	    switch (i) {
		case -1: Set(uf_spot_wb); break;
		case 0: Set(uf_manual_wb); break;
		case 1: Set(uf_camera_wb); break;
		case 2: Set(uf_auto_wb); break;
		case 3: Set("Incandescent"); break;
		case 4: Set("Fluorescent"); break;
		case 5: Set("Direct sunlight");break;
		case 6: Set("Flash"); break;
		case 7: Set("Cloudy"); break;
		case 8: Set("Shade"); break;
		default: Set("");
	    }
	}
	if (HasParent())
	    ParentImage(this).SetWB();
    }
    // Output XML block even if IsDefault().
    std::string XML(const char *indent) const {
	char *value = g_markup_escape_text(StringValue(), -1);
	std::string str = (std::string)indent +
		"<" + Name() + ">" + value + "</" + Name() + ">\n";
	g_free(value);
	return str;
    }
};

extern "C" { UFName ufWBFineTuning = "WBFineTuning"; }
class WBFineTuning : public UFNumber {
public:
    WBFineTuning() : UFNumber(ufWBFineTuning, -9, 9, 0, 0, 1, 1) { }
    void OriginalValueChangedEvent() {
	if (!HasParent())
	    return;
	UFString &wb = ParentImage(this)[ufWB];
	if (wb.IsEqual(uf_auto_wb) || wb.IsEqual(uf_camera_wb))
	    /* Prevent recalculation of Camera/Auto WB on WBTuning events */
	    Set(0.0);
	else
	    ParentImage(this).SetWB();
    }
    // Output XML block even if IsDefault().
    std::string XML(const char *indent) const {
	char *value = g_markup_escape_text(StringValue(), -1);
	std::string str = (std::string)indent +
		"<" + Name() + ">" + value + "</" + Name() + ">\n";
	g_free(value);
	return str;
    }
};

extern "C" { UFName ufTemperature = "Temperature"; }
class Temperature : public UFNumber {
public:
    Temperature() : UFNumber(ufTemperature, 2000, 15000, 6500, 0, 50, 200) { }
    void OriginalValueChangedEvent() {
	if (HasParent())
	    ParentImage(this).SetWB(uf_manual_wb);
    }
};

extern "C" { UFName ufGreen = "Green"; }
class Green : public UFNumber {
public:
    Green() : UFNumber(ufGreen, 0.2, 2.5, 1.0, 3, 0.01, 0.05) { };
    void OriginalValueChangedEvent() {
	if (HasParent())
	    ParentImage(this).SetWB(uf_manual_wb);
    }
};

extern "C" { UFName ufChannelMultipliers = "ChannelMultipliers"; }
class ChannelMultipliers : public UFNumberArray {
public:
    ChannelMultipliers() : UFNumberArray(ufChannelMultipliers, 4,
	    0.100, 99.000, 1.0, 3, 0.001, 0.001) { };
    void Event(UFEventType type) {
	if (type != uf_value_changed)
	    return UFObject::Event(type);
	if (!HasParent())
	    return UFObject::Event(type);
	Image &image = ParentImage(this);
	if (image.uf == NULL)
	    return UFObject::Event(type);
	/* Normalize chanMul so that min(chanMul) will be 1.0 */
	double min = Maximum();
	for (int c = 0; c < image.uf->colors; c++)
	    if (DoubleValue(c) < min)
		min = DoubleValue(c);
	assert(min > 0.0);
	double chanMulArray[4] = { 1.0, 1.0, 1.0, 1.0 };
	for (int c = 0; c < image.uf->colors; c++)
	    chanMulArray[c] = DoubleValue(c) / min;
	Set(chanMulArray);
	
	if (image.uf->conf->autoExposure == enabled_state)
	    image.uf->conf->autoExposure = apply_state;
	if (image.uf->conf->autoBlack == enabled_state)
	    image.uf->conf->autoBlack = apply_state;

	UFObject::Event(type);
    }
    void OriginalValueChangedEvent() {
	if (HasParent())
	    ParentImage(this).SetWB(uf_spot_wb);
    }
    // Output XML block even if IsDefault().
    std::string XML(const char *indent) const {
	char *value = g_markup_escape_text(StringValue(), -1);
	std::string str = (std::string)indent +
		"<" + Name() + ">" + value + "</" + Name() + ">\n";
	g_free(value);
	return str;
    }
};

// ufRawImage is short for 'raw image processing parameters'.
extern "C" { UFName ufRawImage = "Image"; }
Image::Image(ufraw_data *data) : UFGroup(ufRawImage) {
    *this
	<< new WB
	<< new WBFineTuning
	<< new Temperature
	<< new Green
	<< new ChannelMultipliers
    ;
    SetUFRawData(data);
}

void Image::SetWB(const char *mode) {
    UFString &wb = (*this)[ufWB];
    if (wb.IsEqual(uf_manual_wb) || wb.IsEqual(uf_camera_wb) ||
	wb.IsEqual(uf_auto_wb) || wb.IsEqual(uf_spot_wb)) {
	if (!Has(ufWBFineTuning))
	    *this << new WBFineTuning;
	UFNumber &wbTuning = (*this)[ufWBFineTuning];
	wbTuning.Set(0.0);
    }
    // While loading rc/cmd/conf data we do not want to alter the raw data.
    if (uf == NULL)
	return;
    if (uf->rgbMax == 0) { // Raw file was not loaded yet.
	if (!wb.IsEqual(uf_manual_wb) && !wb.IsEqual(uf_manual_wb))
	    uf->WBDirty = true; // ChannelMultipliers should be calculated later
	return;
    }
    if (mode != NULL)
	wb.Set(mode);
    ufraw_set_wb(uf);
    if (wb.IsEqual(uf_spot_wb))
	wb.Set(uf_manual_wb);
}

void Image::SetUFRawData(ufraw_data *data) {
    uf = data;
    if (uf == NULL)
	return;
    const wb_data *lastPreset = NULL;
    uf->wb_presets_make_model_match = FALSE;
    char model[max_name];
    if (strcmp(uf->conf->make, "MINOLTA") == 0 &&
	(strncmp(uf->conf->model, "ALPHA", 5) == 0 ||
	 strncmp(uf->conf->model, "MAXXUM", 6) == 0)) {
	/* Canonize Minolta model names (copied from dcraw) */
	g_snprintf(model, max_name, "DYNAX %s",
		uf->conf->model+6+(uf->conf->model[0] == 'M'));
    } else {
	g_strlcpy(model, uf->conf->model, max_name);
    }
    UFString &wb = (*this)[ufWB];
    UFTokenList &tokens = wb.GetTokens();
    for (int i = 0; i<wb_preset_count; i++) {
	if (strcmp(wb_preset[i].make, "") == 0) {
	    /* Common presets */
	    tokens.push_back(wb_preset[i].name);
	} else if (strcmp(wb_preset[i].make, uf->conf->make) == 0 &&
		   strcmp(wb_preset[i].model, model) == 0) {
	    /* Camera specific presets */
	    uf->wb_presets_make_model_match = TRUE;
	    if (lastPreset == NULL ||
		strcmp(wb_preset[i].name, lastPreset->name) != 0) {
		tokens.push_back(wb_preset[i].name);
	    }
	    lastPreset = &wb_preset[i];
	}
    }
}

extern "C" { UFName ufRawResources = "Resources"; }
class Resources : public UFGroup {
public:
    Resources(): UFGroup(ufRawResources) {
	*this << new Image;
    }
};

class CommandLineImage : public UFGroup {
public:
    CommandLineImage(): UFGroup(ufRawImage) { }
    void OriginalValueChangedEvent() {
	if (Has(ufTemperature) || Has(ufGreen)) {
	    if (Has(ufWB)) {
		UFString &wb = (*this)[ufWB];
		if (!wb.IsEqual(uf_manual_wb)) {
		    ufraw_message(UFRAW_WARNING,
			    _("--temperature and --green options override "
			    "the --wb=%s option."), wb.StringValue());
		}
	    } else {
		*this << new WB;
	    }
	    (*this)[ufWB].Set(uf_manual_wb);
	} else {
	    if (Has(ufWB)) {
		// We don't have green or temperature so this must be from --wb
		UFString &wb = (*this)[ufWB];
		if (wb.IsEqual("camera"))
		    wb.Set(uf_camera_wb);
		else if (wb.IsEqual("auto"))
		    wb.Set(uf_auto_wb);
		else
		    Throw(_("'%s' is not a valid white balance setting."),
			    wb.StringValue());
	    }
	}
    }
};

extern "C" { UFName ufCommandLine = "CommandLine"; }
class CommandLine : public UFGroup {
public:
    CommandLine(): UFGroup(ufCommandLine) {
	*this << new CommandLineImage;
    }
    void Message(const char *Format, ...) const {
	if (Format == NULL)
	    return;
	va_list ap;
	va_start(ap, Format);
	char *message = g_strdup_vprintf(Format, ap);
	va_end(ap);
	ufraw_message(UFRAW_ERROR, "%s: %s\n", Name(), message);
	g_free(message);
    }
};

} // namespace UFRaw

extern "C" {

UFObject *ufraw_image_new() {
    return new UFRaw::Image;
}

void ufraw_image_set_data(UFObject *obj, struct ufraw_struct *uf) {
    dynamic_cast<UFRaw::Image *>(obj)->SetUFRawData(uf);
}

UFObject *ufraw_resources_new() {
    return new UFRaw::Resources;
}

UFObject *ufraw_command_line_new() {
    return new UFRaw::CommandLine;
}

} // extern "C"