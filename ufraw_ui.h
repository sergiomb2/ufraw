/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * ufraw_ui.h - Common definitions for UFRaw's GUI.
 * Copyright 2004-2016 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _UFRAW_UI_H
#define _UFRAW_UI_H

#define raw_his_size 320
#define live_his_size 256
#define max_scale 20

typedef struct {
    GtkLabel *labels[5];
    int num;
    int format;
    gboolean zonep; /* non-zero to display value/zone */
} colorLabels;

typedef enum { render_default, render_overexposed, render_underexposed
             } RenderModeType;

typedef enum { options_button, cancel_button, ok_button, save_button,
               gimp_button, delete_button, num_buttons
             } ControlButtons;

typedef enum { spot_cursor, crop_cursor,
               left_cursor, right_cursor, top_cursor, bottom_cursor,
               top_left_cursor, top_right_cursor, bottom_left_cursor, bottom_right_cursor,
               move_cursor, cursor_num
             } CursorType;

enum { base_curve, luminosity_curve };

/* All the "global" information is here: */
typedef struct {
    ufraw_data *UF;
    conf_data *rc;
    int raw_his[raw_his_size][4];
    int TypeComboMap[num_types];
    GdkPixbuf *PreviewPixbuf;
    GdkCursor *Cursor[cursor_num];
    /* Remember the Gtk Widgets that we need to access later */
    GtkWidget *Controls, *PreviewWidget, *RawHisto, *LiveHisto;
    GtkLabel *DarkFrameLabel;
    GtkWidget *BaseCurveWidget, *CurveWidget, *BlackLabel;
    GtkComboBox *BaseCurveCombo, *CurveCombo,
                *ProfileCombo[profile_types], *BitDepthCombo, *TypeCombo;
    GtkWidget *GrayscaleButtons[6];
    GtkTable *SpotTable;
    GtkTable *GrayscaleMixerTable;
    GtkLabel *GrayscaleMixerColor;
    GtkLabel *SpotPatch;
    GtkLabel *HotpixelCount;
    colorLabels *SpotLabels, *AvrLabels, *DevLabels, *OverLabels, *UnderLabels;
    GtkToggleButton *AutoExposureButton, *AutoBlackButton;
    GtkToggleButton *AutoCropButton, *LockAspectButton;
    GtkWidget *AutoCurveButton;
    GtkWidget *ResetGammaButton, *ResetLinearButton;
    GtkWidget *ResetExposureButton, *ResetSaturationButton;
    GtkWidget *ResetThresholdButton;
    GtkWidget *ResetHotpixelButton;
#ifdef UFRAW_CONTRAST
    GtkWidget *ResetContrastButton;
#endif
    GtkWidget *ResetBlackButton, *ResetBaseCurveButton, *ResetCurveButton;
    GtkWidget *ResetGrayscaleChannelMixerButton;
    GtkWidget *ResetDespeckleButton;
    GtkWidget *SaveButton;
    GtkWidget *ControlButton[num_buttons];
    guint16 ButtonMnemonic[num_buttons];
    GtkProgressBar *ProgressBar;
    GtkSpinButton *CropX1Spin;
    GtkSpinButton *CropY1Spin;
    GtkSpinButton *CropX2Spin;
    GtkSpinButton *CropY2Spin;
    GtkSpinButton *ShrinkSpin;
    GtkSpinButton *HeightSpin;
    GtkSpinButton *WidthSpin;
    /* We need the adjustments for update_scale() */
    GtkAdjustment *GammaAdjustment;
    GtkAdjustment *LinearAdjustment;
    GtkAdjustment *ExposureAdjustment;
    GtkAdjustment *ThresholdAdjustment;
    GtkAdjustment *HotpixelAdjustment;
    GtkAdjustment *SaturationAdjustment;
#ifdef UFRAW_CONTRAST
    GtkAdjustment *ContrastAdjustment;
#endif
    GtkWidget *LightnessHueSelectNewButton;
    GtkTable *LightnessAdjustmentTable[max_adjustments];
    GtkAdjustment *LightnessAdjustment[max_adjustments];
    GtkWidget *ResetLightnessAdjustmentButton[max_adjustments];
    GtkWidget *LightnessHueSelectButton[max_adjustments];
    GtkWidget *LightnessHueRemoveButton[max_adjustments];
    GtkAdjustment *CropX1Adjustment;
    GtkAdjustment *CropY1Adjustment;
    GtkAdjustment *CropX2Adjustment;
    GtkAdjustment *CropY2Adjustment;
    GtkAdjustment *ZoomAdjustment;
    GtkAdjustment *ShrinkAdjustment;
    GtkAdjustment *HeightAdjustment;
    GtkAdjustment *WidthAdjustment;
    GtkAdjustment *RotationAdjustment;
    GtkWidget *ResetRotationAdjustment;
    GtkAdjustment *GrayscaleMixers[3];
    GtkAdjustment *DespeckleWindowAdj[4];
    GtkAdjustment *DespeckleDecayAdj[4];
    GtkAdjustment *DespecklePassesAdj[4];
    GtkToggleButton *DespeckleLockChannelsButton;
    GtkToggleButton *ChannelSelectButton[4];
    int ChannelSelect;
#ifdef HAVE_LENSFUN
    /* The GtkEntry with camera maker/model name */
    GtkWidget *CameraModel;
    /* The menu used to choose camera - either full or limited by search criteria */
    GtkWidget *CameraMenu;
    /* The GtkEntry with lens maker/model name */
    GtkWidget *LensModel;
    /* The menu used to choose lens - either full or limited by search criteria */
    GtkWidget *LensMenu;
    /* The lens fix notebook distortion page */
    GtkWidget *LensDistortionTable, *LensDistortionDesc;
    /* The lens fix notebook TCA page */
    GtkWidget *LensTCATable, *LensTCADesc;
    /* The lens fix notebook vignetting page */
    GtkWidget *LensVignettingTable, *LensVignettingDesc;
    /* The hbox containing focal, aperture, distance combos */
    GtkWidget *LensParamBox;
#endif /* HAVE_LENSFUN */
    long(*SaveFunc)();
    RenderModeType RenderMode;
    /* Current subarea index (0-31). If negative, rendering has stopped */
    int RenderSubArea;
    /* Some actions update the progress bar while working, but meanwhile we
     * want to freeze all other actions. After we thaw the dialog we must
     * call update_scales() which was also frozen. */
    gboolean FreezeDialog;
    /* Since the event-box can be larger than the preview pixbuf we need: */
    gboolean PreviewButtonPressed, SpotDraw;
    int SpotX1, SpotY1, SpotX2, SpotY2;
    CursorType CropMotionType;
    int DrawnCropX1, DrawnCropX2, DrawnCropY1, DrawnCropY2;
    double shrink, height, width;
    gboolean OptionsChanged;
    int PageNum;
    int PageNumSpot;
    int PageNumGray;
    int PageNumLensfun;
    int PageNumLightness;
    int PageNumCrop;
    int HisMinHeight;
    int UnnormalizedOrientation;
    /* Original aspect ratio (0) or actual aspect ratio */
    /* The aspect ratio entry field */
    GtkEntry *AspectEntry;
    /* Output base filename (without the path) */
    GtkEntry *OutFileEntry;
    /* Mouse coordinates in previous frame (used when dragging crop area) */
    int OldMouseX, OldMouseY;
    int OverUnderTicker;
    /* The event source number when the highlight blink function is enabled. */
    guint BlinkTimer;
    guint DrawCropID;
} preview_data;

/* Response can be any unique positive integer */
#define UFRAW_RESPONSE_DELETE 1
#define UFRAW_NO_RESPONSE 2

/* These #defines are not very elegant, but otherwise things get tooo long */
#define CFG data->UF->conf
#define RC data->rc
#define Developer data->UF->developer
#define CFG_cameraCurve (CFG->BaseCurve[camera_curve].m_numAnchors>0)

/* Start the render preview refresh thread for invalid layers in background */
void render_preview(preview_data *data);

void lens_fill_interface(preview_data *data, GtkWidget *page);

GtkWidget *table_with_frame(GtkWidget *box, char *label, gboolean expand);

GtkWidget *notebook_page_new(GtkNotebook *notebook, char *text, char *icon);

GtkWidget *stock_icon_button(const gchar *stock_id,
                             const char *tip, GCallback callback, void *data);
void ufnumber_adjustment_scale(UFObject *obj,
                               GtkTable *table, int x, int y, const char *label, const char *tip);

#endif /* _UFRAW_UI_H */
