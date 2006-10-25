#!/bin/sh

#
# ./generate_schemas PREFIX OUTPUT
#
# generate the part of the schema required for the mime type into OUTPUT.
# location of the ufraw-batch executable should be PREFIX/bin
#
# based on rawthumb: http://www.chauveau-central.net/rawthumb/

# Reminder: in the thumbnailer command, the following token are replaced:
#
#  %s is the thumbnail size
#  %u is the gnome VFS url of the raw image file
#      file://home/bob/image.raw
#  %i is the Unix file name of the raw image file
#      /home/bob/image.raw
#  %o is the Unix file name of the thumbnail (without .png extension)
#       /home/bob/.thumbnail/arq25w5rwer6

do_mime () {

    MIME=$1 ; shift

    cat <<EOF >> $SCHEMAS
    <schema>
      <key>/schemas/desktop/gnome/thumbnailers/$MIME/enable</key>
      <applyto>/desktop/gnome/thumbnailers/$MIME/enable</applyto>
      <owner>ufraw</owner>
      <type>bool</type>
      <default>true</default>
      <locale name="C">
        <short></short>
        <long></long>
      </locale>
    </schema>

    <schema>
      <key>/schemas/desktop/gnome/thumbnailers/$MIME/command</key>
      <applyto>/desktop/gnome/thumbnailers/$MIME/command</applyto>
      <owner>ufraw</owner>
      <type>string</type>
      <default>$PREFIX/bin/ufraw-batch --embedded-image --out-type=png --size=%s %i --overwrite --silent --output=%o</default>
      <locale name="C">
        <short></short>
        <long></long>
      </locale>
    </schema>

EOF

}

#
#
#

if [ $# != "2" ] ; then
  echo "usage: generate_schemas prefix output"
  echo "generate schemas for thumbnailing raw images."
  exit 1
fi

PREFIX=$1 ; shift
SCHEMAS=$1 ; shift

echo "<gconfschemafile>" > $SCHEMAS
echo "   <schemalist>" >> $SCHEMAS

do_mime application@x-ufraw
do_mime image@x-dcraw
do_mime image@x-adobe-dng
do_mime image@x-canon-crw
do_mime image@x-canon-cr2
do_mime image@x-fuji-raf
do_mime image@x-kodak-dcr
do_mime image@x-minolta-mrw
do_mime image@x-nikon-nef
do_mime image@x-olympus-orf
do_mime image@x-panasonic-raw
do_mime image@x-pentax-pef
do_mime image@x-sigma-x3f
do_mime image@x-sony-srf
do_mime image@x-sony-sr2
do_mime image@x-sony-arw

echo "   </schemalist>" >> $SCHEMAS
echo "</gconfschemafile>" >> $SCHEMAS

exit 0
