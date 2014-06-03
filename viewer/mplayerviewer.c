#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
const unsigned int maxjobs = 1000;
const unsigned int mplayers = 9;
int main()
{
    char** dirnames = malloc(sizeof(char*)*maxjobs);
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

    //create mplayer instances
    FILE** mplayer = malloc(sizeof(FILE*)*mplayers);
    for(unsigned int i=0;i<mplayers && i<dirno;i++)
    {
        char buf[100];
        sprintf(buf,"mplayer -fstype -fullscreen -really-quiet -slave %s/test.avi -playing-msg TEST\n",dirnames[i]);
        mplayer[i] = popen(buf,"w");
    }
    //add files to playlist
    for (unsigned int i=mplayers;i<dirno;i++)
    {
        char buf[100];
        sprintf(buf,"get_file_name\nloadfile %s/test.avi 1\n get_file_name\n",dirnames[i]);
        printf("running %s\n",buf);
        fwrite(buf,sizeof(char),strlen(buf),mplayer[i%mplayers]);
    }
    //send EOF
    for(unsigned int i=0;i<mplayers && i<dirno;i++)
    {
        char c = EOF;
        fwrite(&c,sizeof(char),1,mplayer[i]);
    }
    return 0;
}
