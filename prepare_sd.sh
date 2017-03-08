#!/bin/bash

SD_CARD=/media/florian/HACKBERT
SOURCE=/home/florian/Desktop/Hörbert

echo "Converting input files"
cd ${SOURCE}
for i in {1..9}; do
   cd $i
   j=1;
   for file in *.mp3; do
      sox "$file" -c 1 -e signed-integer -r 22050 -b 16 "$j.wav"
      echo "$j: $file" >> toc.txt
      j=`echo "$j + 1" | bc`
   done;
   rm -f *.mp3
   cd ..
done

if [ ! -d "$SD_CARD" ]; then
   echo "${SD_CARD} does not exist. Exiting."
   exit 0
fi

echo "Do you really want to delete all folders on ${SD_CARD}? Type YES"
answer="NO"
read answer
if [ "$answer" = "YES" ]; then
   echo "Deleting all directories on ${SD_CARD} ..."
   rm -rf ${SD_CARD}/{1,2,3,4,5,6,7,8,9}
   echo "Copying all files to ${SD_CARD}"
   cp -r ${SOURCE}/{1,2,3,4,5,6,7,8,9} ${SD_CARD}
   echo "Done. Umounting ${SD_CARD}"
   sudo umount ${SD_CARD}
   echo "SD Card is now save to remove. Exiting."
else
   echo "Aborting."
   exit 0
fi

exit 0
