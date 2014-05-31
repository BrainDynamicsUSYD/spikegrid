echo pics* | tr ' ' '\n' | parallel -j 8 DISPLAY=:0.0 mplayer -fps 100 {}/test.avi
