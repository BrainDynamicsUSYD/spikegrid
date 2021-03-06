#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include "cv.h"
#include "highgui.h"

// initialize global variables
const int maxjobs = 1000;
const int rows = 3;
const int cols = 3;
const int gridsize = 100;
char* winname = "viewer0";
char** dirnames;
int upto=0;
IplImage* dispimage;
int printimagecount=0;
void mousecb(int event, int x,int y,int dummy,void* dummy2)
{
    if (event==CV_EVENT_LBUTTONDOWN)
    {
        int idx = (x/gridsize)*cols+(y/gridsize);
        printf("%s\n",dirnames[idx+upto]);
    }
    else if (event==CV_EVENT_RBUTTONDOWN)
    {
        char buf[gridsize];
        sprintf(buf,"still-%i.png",printimagecount);
        cvSaveImage(buf,dispimage,NULL);
        printimagecount++;
    }
}
int compare(const void* a, const void* b)
{
    const char * ca = *(const char**)a;
    const char * cb = *(const char**)b;
    int ia = atoi(ca+4);
    int ib = atoi(cb+4);
    return ia-ib;
}
int main(int argc,char** argv) {
    //scan directory
    dirnames = malloc(sizeof(char*)*maxjobs);
    int dirno=0;
    DIR* d = opendir("."); // Possible candidate
    struct dirent* dir;
    if (d)
    {
        while ((dir=readdir(d)) != NULL)
        {
            if (!strncmp(dir->d_name,"job-",4)) //check dirname matches pattern
            {   //store dirname
                dirnames[dirno]=malloc(strlen(dir->d_name));
                strcpy(dirnames[dirno],dir->d_name);
                dirno++;
            }
        }
    }
    printf("Found all directories");
    qsort(dirnames,dirno,sizeof(char*),compare);
    //set up window
    if (argc==1)
    {
    cvNamedWindow(winname,CV_WINDOW_AUTOSIZE);
    cvMoveWindow(winname,0,0);
    cvSetMouseCallback(winname,mousecb,0);
    }
    int size;
    CvCapture* caps[rows*cols];
    int vididx = 0;
    int iterations = 0;
    int fourcc = CV_FOURCC('H','F','Y','U') /*use huffyuv codec (same used to encode input videos) */;
    const char* fnameout = "out.avi";
    CvVideoWriter* vidwrite=cvCreateVideoWriter(
            fnameout,fourcc,60.0 /*fps*/,cvSize(300*rows,300*cols),1 /*color*/);
    while (vididx < dirno)
    {
        int vcount=0;
        upto = vididx;
        //open video files for viewing
        for (int i=0;i<rows*cols && vididx< dirno ;i++)
        {
            char buf[100];
            sprintf(buf,"%s/test.avi",dirnames[vididx]);
            caps[i] = cvCreateFileCapture(buf);
            if (caps[i]){size = (int)cvGetCaptureProperty(caps[i],CV_CAP_PROP_FRAME_HEIGHT);}
            else {printf("init capture failed for %s - maybe test.avi doesn't exist?\n",buf);}
            vididx++;
            vcount++;
        }
        if (size==0) {printf("No videos found\n");return(0);}
        IplImage* frame;
        dispimage = cvCreateImage(cvSize(size*rows,size*cols),8,3);
        // display video frame by frame
        int c = 0;
        while(1)
        {
            c++;
            if (c%100 == 0) {printf("%i\n",c);}
            for (int i=0; i<vcount;i++)
            {
                if  (caps[i]) //only process frames where capture succeeded
                {            //if the capture failed, just leave the square blank.
                     frame=cvQueryFrame(caps[i]);
                    if (!frame)
                    {
                        goto done; //here we have assumed once one video is done they all are, and we bail
                    }
                    cvSetImageROI(dispimage,cvRect((i/cols)*size,(i%cols)*size,size,size));
                    cvResize(frame,dispimage,CV_INTER_LINEAR);
                    cvResetImageROI(dispimage);
                }
            }
            int c=0;
            if (argc==1)
            {
                cvShowImage(winname,dispimage);
                c = cvWaitKey(10);
            }
            if (c==27) break;
            cvWriteFrame(vidwrite,dispimage);
        }
done:
        // free memory
        for (int i=0; i < vcount;i++)
        {
            cvReleaseCapture( &caps[i] );
        }
        iterations++;
    }
    cvReleaseVideoWriter(&vidwrite);
    return(0);
}
