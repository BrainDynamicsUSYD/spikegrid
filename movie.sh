echo job-* | tr ' ' '\n' | parallel --progress --eta "ffmpeg -loglevel warning -y -f image2  -i {}/%d.png  -vcodec huffyuv {}/test.avi"
