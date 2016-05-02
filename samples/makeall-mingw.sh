#!/bin/sh
# Build all sample
#
cc=gcc
cp=cp
rm=rm
#outExt=".out"
outExt=".exe"
libsrc="textscreen.c"
opt="-Wall -lm"
samples="
  interrupt
  life
  rectbench
  resize
  sample
  scroll
  showkeycode
  waveview
  bindump
  hello
"

${cp} ../textscreen.* .
for src in ${samples}
do
  inFile=${src}.c
  outFile=${src}${outExt}
  echo "${cc} ${inFile} ${libsrc} ${opt} -o ${outFile}"
  ${cc} ${inFile} ${libsrc} ${opt} -o ${outFile}
  echo "done."
done
${rm} textscreen.*

