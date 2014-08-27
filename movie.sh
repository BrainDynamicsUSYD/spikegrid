echo job-* | tr ' ' '\n' | parallel --progress --eta "ffmpeg -loglevel warning -y -f image2  -i {}/0-%d.png  -vcodec huffyuv {}/test.avi"
