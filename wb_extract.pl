#!/usr/bin/env perl
# $Id: wb_extract.pl,v 1.9 2014/09/16 02:15:18 nkbj Exp $

# This program helps to prepare white balance preset lines for
# wb_presets.c.  To add a new camera, take exposures with every white
# balance preset setting in the natural order.  Natural is either
# lower color temperature to higher, or the order in which the camera
# scrolls through the setting (usually these are close).  Named
# settings like "DirectSunlight" come first, and then "Kelvin
# settings".  For each named setting, the entire range of adjustments
# should be obtained in numerical order; a typical set is -3 -2 -1 0 1
# 2 3.  If you do not have time to produce all the values, just doing
# the fine tune 0 named values is quite useful.  You should also note
# the firmware and revision level in a comment if that's reasonably
# easy to do.

use warnings;
use strict;

if (@ARGV < 1) {
  print "White balance extraction tool for UFRaw\n";
  print "By Colin Bennett 24 June 2006\n";
  print "Usage: $0 FILES...\n";
  exit(0);
}

# debug:
#my ($lastmul1, $lastmul2, $lastmul3) = (0,0,0);

my($const_WB);
$const_WB = 0;

my($print_fw);
$print_fw = 0;

for my $file (@ARGV) {
  if ($file eq "-const") {
    $const_WB = 1;
    next
  } elsif ($file eq "-printfw") {
    $print_fw = 1;
    next
  }

  my ($make, $model, $fw_version, $wbname, $wbfinetune, $wbfinetune_1, $mulred, $mulgreen, $mulblue);
  $mulgreen = 1;  # default value for green balance
  $wbfinetune = $wbfinetune_1 = 0, $mulred = -1, $mulblue = -1;  # avoid warnings about uninitialized vars

  open(EXIFTOOL, "exiftool -s -t -Model -CanonModelID -SonyModelID -FirmwareVersion -Software -ColorBalance1 -RedBalance -BlueBalance -WhiteBalance -WBShiftAB -WBBracketValueAB -\"WB_RGGB*\" -ColorTemperature $file|") 
  or die "can't open $file: $!";
  while (my $line = <EXIFTOOL>) {
    $line =~ /([^\t]+)\t(.*)/ or next;
    my ($field, $value) = ($1, $2);
    # debug: show field
    # print $field . "\n";
    if ($field eq "Model") {
      ($make, $model) = split(/ +/, $value);
      # exiftool's -Make and -Model are correct, but this should not
      # cause any regression
      if ($make eq "X-E1" || $make eq "X-Pro1") {
        $model = $make;
        $make = "FUJIFILM";
      }
    } elsif ($field eq "CanonModelID") {
      $model = $value;
    } elsif ($field eq "SonyModelID") {
      $model = $value;
    } elsif ($field eq "FirmwareVersion") {
      $fw_version = $value;
    } elsif ($field eq "Software") {
      $fw_version = $value;
    } elsif ($field eq "ColorBalance1") {   # Field for D200
      my $mul_unknown;
      ($mulred, $mulblue, $mulgreen, $mul_unknown) = split(/ +/, $value);
    } elsif ($field eq "RedBalance") {   # Field for D70 and X-E1 (red)
      $mulred = $value;
    } elsif ($field eq "BlueBalance") {   # Field for D70 and X-E1 (blue)
      $mulblue = $value;
    } elsif ($field eq "WhiteBalance") {
      if (($model eq "X-E1" || $model eq "X-Pro1") && $value eq "Unknown (0x600)") {
        $wbname = "Underwater";
      } else {
        $wbname = $value;
      }
    } elsif ($field eq "WhiteBalanceFineTune") {
      $wbfinetune = $value;
    } elsif ($field eq "WBShiftAB") {
      $wbfinetune = $value;
    } elsif ($field eq "WBBracketValueAB") {
      $wbfinetune_1 = $value;
    } elsif ($field eq "WB_RGGBLevels") {
      my ($mul_tmp1, $mul_tmp2, $mul_tmp3, $mul_tmp4) = split(/ +/, $value);
      $mulred = ($mul_tmp1 / $mul_tmp2);
      $mulblue = ($mul_tmp4 / $mul_tmp3);
    } elsif ($field eq "ColorTemperature" && $wbname eq "Kelvin" ) { # Field for X-E1
      $wbname = sprintf "%dK", $value;
    } elsif ($field =~ /WB_RGGBLevels/) { # Get embedded whitebalance values
      $field =~ s/^WB_RGGBLevels//; # Truncate for whitebalance-name
      my ($mul_tmp1, $mul_tmp2, $mul_tmp3, $mul_tmp4) = split(/ +/, $value);
      
      my $tmp_mulred = ($mul_tmp1 / $mul_tmp2);
      my $tmp_mulblue = ($mul_tmp4 / $mul_tmp3);
      
      my $result;
      $result = sprintf "  { \"%s\", \"%s\", ", $make, $model;
      $result .= sprintf "\"%s\", ", $field;
      $result .= sprintf "%d,", 0;
      $result .= " " while length($result) < 48;
      $result .= "{ $tmp_mulred, 0, $tmp_mulblue, 0 } },";
  
      print $result, "\n";
    }
  }
  close EXIFTOOL;

  # Fix names for consistency across Nikon cameras (D70 and D2X use "Direct sunlight")
  $wbname =~ s/^Sunny$/Direct sunlight/;

  # Fix names for wb_presets.c
  if ($make eq "FUJIFILM") {
    if ($model eq "X-E1" || $model eq "X-Pro1") {
      # The manual calls it "Shade", but exiftool shows it as "Cloudy".
      $wbname =~ s/^Cloudy$/Shade/;
    }
    $wbname =~ s/^Daylight Fluorescent$/DaylightFluorescent/;
    $wbname =~ s/^Day White Fluorescent$/WarmWhiteFluorescent/;
    $wbname =~ s/^White Fluorescent$/CoolWhiteFluorescent/;
  }

  if ($print_fw eq 1) {
    printf "  /* $make $model Firmware Version $fw_version */\n";
  }
 
  # Format and print the line
  my $result;
  $result = sprintf "  { \"%s\", \"%s\", ", $make, $model;
  if ($const_WB eq 1 && $wbname !~ "K") {
    $result .= sprintf "%s, ", $wbname;
  } else {
    $result .= sprintf "\"%s\", ", $wbname;
  }
  $result .= sprintf "%d,", $wbfinetune+$wbfinetune_1;
  $result .= " " while length($result) < 48;
  $result .= "{ $mulred, $mulgreen, $mulblue, 0 } },";

  # debug: add deltas
  #$result .= sprintf "/* dm1=%.4f dm2=%.4f dm3=%.4f */", $mul1-$lastmul1, $mul2-$lastmul2, $mul3-$lastmul3;
  #$lastmul1 = $mul1; 
  #$lastmul2 = $mul2; 
  #$lastmul3 = $mul3; 
 
  print $result, "\n";
}

# sample output:
#  { "NIKON", "D200", "Incandescent", 0,		{ 1.148438, 1, 2.398438, 0 } },

# sample exiftool output:
#cdb@gamma D200-wb $ exiftool -s -t -ColorBalance1 -WhiteBalance -WhiteBalanceFineTune dsc_0254.nef
#ColorBalance1   1.234375 2.136719 1 1
#WhiteBalance    Incandescent
#WhiteBalanceFineTune    -3
