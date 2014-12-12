ffmpeg -i out.avi -b:v 90000k  -trellis 1 -lmax 4200*QP2LAMBDA out2.avi
