#!/usr/bin/env perl
# $Id: wb_extract.pl,v 1.3 2008/01/22 12:15:26 lexort Exp $

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

for my $file (@ARGV) {
  my ($make, $model, $wbname, $wbfinetune, $mulred, $mulgreen, $mulblue);

  $mulgreen = 1;  # default value for green balance

  open(EXIFTOOL, "exiftool -s -t -Model -ColorBalance1 -RedBalance -BlueBalance -WhiteBalance -WhiteBalanceFineTune $file|") 
    or die "can't open $file: $!";
  while (my $line = <EXIFTOOL>) {
    $line =~ /([^\t]+)\t(.*)/ or next;
    my ($field, $value) = ($1, $2);
    if ($field eq "Model") {
      ($make, $model) = split(/ +/, $value);
    } elsif ($field eq "ColorBalance1") {   # Field for D200
      my $mul_unknown;
      ($mulred, $mulblue, $mulgreen, $mul_unknown) = split(/ +/, $value);
    } elsif ($field eq "RedBalance") {   # Field for D70 (red)
      $mulred = $value;
    } elsif ($field eq "BlueBalance") {   # Field for D70 (blue)
      $mulblue = $value;
    } elsif ($field eq "WhiteBalance") {
      $wbname = $value;
    } elsif ($field eq "WhiteBalanceFineTune") {
      $wbfinetune = $value;
    }
  }
  close EXIFTOOL;

  # Fix names for consistency across Nikon cameras (D70 and D2X use "Direct sunlight")
  $wbname =~ s/^Sunny$/Direct sunlight/;
  
  # Format and print the line
  my $result;
  $result = sprintf "  { \"%s\", \"%s\", \"%s\", %d,", $make, $model, $wbname, $wbfinetune;
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
