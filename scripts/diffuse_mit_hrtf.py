"""Loads the Diffused MIT KEMAR HRTF dataset included with this repository and converts it to a .hrtf file.  Pass the path to the data as the parameter to this script."""
import numpy
import scipy.io.wavfile as wavfile
from glob import glob
import sys
import os.path
import re
import itertools
import struct

if len(sys.argv) != 3:
	print "Usage: mit_hrtf.py <directory> <output_file>"
	exit()

root_path = sys.argv[1]
output_file = sys.argv[2]
all_wavefiles = glob(root_path + '/*/*.wav')
print "Enumerated wave files:", len(all_wavefiles), "found."
print "Reading data."

read_wavefiles = dict()

for i in all_wavefiles:
	name = os.path.split(i)[1]
	read_wavefiles[name] = wavfile.read(i)

print "Performing basic sanity checks: expected 2-channel, 44100 HZ files."
samplerate = 44100
hrir_length = None

for i, j in read_wavefiles.iteritems():
	if j[0] != samplerate:
		print "File", i, "is not", samplerate, "HZ."
		exit()
	if j[1].shape[1] != 2:
		print "File", i, "is not a stereo HRIR datapoint."
		exit()
	if hrir_length is None:
		hrir_length = j[1].shape[0]
	elif hrir_length != j[1].shape[0]:
		print "File", i, "has a different length; expected", hrir_length, "got", j[0].shape[0]

print "Sanity checks passed.  Continuing."
print "Separating channels."

#we can discard samplerates at this point.
for i, j in dict(read_wavefiles).iteritems():
	arr = j[1]
	#Note: this is numpy slicing syntax.
	read_wavefiles[i] = (arr[:, 0], arr[:, 1])

print "Extracting angles."

hrirs = dict()
pattern = re.compile(r"H(-{0,1}\d+)e(\d+)a\.wav")

for i, j in read_wavefiles.iteritems():
	extracted = pattern.match(i)
	elevation = extracted.group(1)
	azimuth = extracted.group(2)
	elevation = int(elevation)
	azimuth = int(azimuth)
	hrirs[(elevation, azimuth)] = j

print "Expanding 180-degree stereo responses to 360-degree mono responses."
for i, j in dict(hrirs).iteritems():
	#0 and 180 are special: we average these instead.
	if i[1] in set([0, 180]):
		response = [(a+b)/2.0 for a, b in zip(j[0], j[1])]
		hrirs[i] = response
		continue
	#otherwise, we have two responses.  The first is at the current angle, and is taken as the right channel.
	#the second is 360 minus the current angle, and is the left channel from this file.
	hrirs[i] = list(j[1]) #the first response.
	hrirs[(i[0], 360-i[1])] = list(j[0]) #the second response.

print "Converting to normalized floating-point values."
all_data = itertools.chain.from_iterable(hrirs.itervalues())
max_sample = max(all_data)

#we have to reset the iterator.
all_data = itertools.chain.from_iterable(hrirs.itervalues())
min_sample = min(all_data)
normalization_factor = max([abs(max_sample), abs(min_sample)])
print "Normalization factor is", normalization_factor

for i, j in dict(hrirs).iteritems():
	hrirs[i] = [k/float(normalization_factor) for k in j]

print "Data normalized.  Preparing to write responses."

#compute the number of elevations:
elev_list = list(set([i[0] for i in hrirs.iterkeys()]))
number_of_elevations = len(elev_list)
elev_list.sort() #from least to greatest, per the hrir datafile format.

#get min and max elevation.
minimum_elevation = min(elev_list)
maximum_elevation = max(elev_list)

#now, compute the azimuths per elevation, and create a list of them.
azimuth_list = []
for elev in elev_list:
	azimuth_list.append(len([key for key in hrirs if key[0] == elev]))

print "Elevations:", number_of_elevations
print "Minimum elevation:", minimum_elevation
print "Maximum elevation:", maximum_elevation
print "Elevation resolution:", (maximum_elevation-minimum_elevation)/float(number_of_elevations-1) #-1 because there is also a ring of elevations at 0 that doesn't show up when you just subtract.
print "Azimuth counters:"
print azimuth_list

print "Data is okay and formatted."
print "Packing data."

endianness_marker = 1

#we need to sort the responses.
hrir_keys = hrirs.keys()
#it happens that the built-in tuple sorting actually already does what we want.
hrir_keys.sort()

#build a list representing the file itself.
file_data = itertools.chain(
[endianness_marker],
[samplerate],
[len(hrirs)],
[number_of_elevations],
[minimum_elevation],
[maximum_elevation],
azimuth_list,
[hrir_length],
[j for i in hrir_keys for j in hrirs[i]])

#make this into a really, really big list.
file_data = list(file_data)

expected_size = 4*len(file_data)
print "Size of resulting file:", expected_size/1024.0, "kb"


format_string = "=" #native endianness and standard sizes.
format_string += "ii" #magic endianness marker and samplerate.
format_string += "iiii" #number of responses, number of elevations, min elevation, max elevation.
format_string += str(len(azimuth_list)) + "i" #the azimuth counters.
format_string += "i" #the length of each response.
format_string += str(len(hrirs)*hrir_length) + "f" #the responses themselves.

#pack with struct module.
writing_string = struct.pack(format_string, *file_data)

if len(writing_string) != expected_size:
	print "Error: expected size", expected_size, "but file is only", len(writing_string)
	exit()

print "Data packed.  Writing..."

with open(output_file, "wb") as f:
	f.write(writing_string)
print "Done."
