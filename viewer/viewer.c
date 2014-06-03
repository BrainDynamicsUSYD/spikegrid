#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include "cv.h"
#include "highgui.h"

// initialize global variables
const int maxjobs = 1000;
const int rows = 9;
const int cols = 9;
char* winname = "viewer0";
char** dirnames;
void mousecb(int event, int x,int y,int dummy,void* dummy2) 
{
    if (event==CV_EVENT_LBUTTONDOWN)
    {
        int idx = (x/100)*cols+(y/100);
        printf("%s\n",dirnames[idx]);
    }
}
int main() {
    //scan directory
    dirnames = malloc(sizeof(char*)*maxjobs);
    int dirno=0;
    DIR* d = opendir(".");
    struct dirent* dir;
    if (d)
    {
        while ((dir=readdir(d)) != NULL)
        {
            if (!strncmp(dir->d_name,"job-",4))
            {
                dirnames[dirno]=malloc(strlen(dir->d_name));
                strcpy(dirnames[dirno],dir->d_name);
                dirno++;
            }
        }
    }
    //set up video inputs
    cvNamedWindow(winname,CV_WINDOW_AUTOSIZE);
    cvMoveWindow(winname,0,0);
    cvSetMouseCallback(winname,mousecb,0);
    int size;
    CvCapture* caps[rows*cols];
    int vididx = 0;
    int iterations = 0;
    while (vididx < dirno)
    {
        int vcount=0;
        for (int i=0;i<rows*cols && vididx< dirno ;i++)
        {
            char buf[100];
            sprintf(buf,"%s/test.avi",dirnames[vididx]);
            caps[i] = cvCreateFileCapture(buf);
            size = (int)cvGetCaptureProperty(caps[i],CV_CAP_PROP_FRAME_HEIGHT);
            vididx++;
            vcount++;
        }
        IplImage* frame;
        IplImage* dispimage = cvCreateImage(cvSize(size*rows,size*cols),8,3);
        // display video frame by frame
        while(1) {
            for (int i=0;i<rows*cols && i<vcount;i++)
            {
                frame=cvQueryFrame(caps[i]);
                cvSetImageROI(dispimage,cvRect((i/cols)*size,(i%cols)*size,size,size));
                if (!frame) goto done;
                cvResize(frame,dispimage,CV_INTER_LINEAR);
                cvResetImageROI(dispimage);
            }
            cvShowImage(winname,dispimage);
                int c = cvWaitKey(33);
                if (c==27) break;
        }
done:
        // free memory
        for (int i=0;i<rows*cols && i < vididx;i++)
        {
            cvReleaseCapture( &caps[i] );
        }
        iterations++;
    }
    return(0);
}
