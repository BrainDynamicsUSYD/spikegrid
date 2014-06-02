echo job-* | tr ' ' '\n' | parallel "ffmpeg -y -f image2  -i {}/%d.png  {}/test.avi"
