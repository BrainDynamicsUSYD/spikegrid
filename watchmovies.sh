echo job-* | tr ' ' '\n' | parallel -j 16 DISPLAY=:0.0 mplayer -fps 100 {}/test.avi
