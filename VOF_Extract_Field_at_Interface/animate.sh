#!/bin/bash

# run this script in the directory where the png files generated by 
# ExtractAtInterface are dumped. Note this requires mencoder, which you can
# get from apt by 'sudo apt install mencoder'. Mencoder is used to 
# transcode a set of png files into an avi movie.

# change w (width), h (height), fps (frames per second,) o (output name) as needed.

# create file list
ls -lrt *.png | awk '{print $9}' > file_list

# uses mencoder to animate png frames into avi
mencoder mf://@file_list -mf w=800:h=600:fps=5:type=png -ovc copy -oac copy -o output.avi
