/// \file
#ifdef __cplusplus
//#include "opencv2/core/core.hpp" //core opencv
//#include "opencv2/highgui/highgui.hpp" //for video writer
typedef struct randconns_info randconns_info;
namespace cv
{
    class VideoWriter;
}
///generic class for outputting an object.
///you probably want to inhereit from this for a new output method
class Output
{
    int interval; /// <- how often to output data
    int idx;      /// <- used to store a prefix for the output file
    public:
        Output(int intervalin,int idxin) {interval=intervalin;idx=idxin;};
        virtual void DoOutput() {};
        int GetInterval() const {return interval;};
        int GetIdx() const {return idx;};
        virtual ~Output() {};
};
/// outputs to series of pictures
class PNGoutput : public Output
{
    int count=0;
    protected:
        const tagged_array* data;
        const overlaytext* overlay;
    public:
        PNGoutput(int,int,const tagged_array*,const char* const );
        void DoOutput() ;
};
class GUIoutput : public PNGoutput
{
    const char* winname;
    public:
        GUIoutput(int,int,const tagged_array*, const char* const,const char* const);
        void DoOutput();
};
class SingleFileOutput : public Output
{
    protected:
        FILE* f;
    public:
        SingleFileOutput(int,int );
        virtual void DoOutput() {};
};
class VidOutput: public Output //This class probably needs a destructor to end the video.  Vlc will probably handle the file just fine though.
{
    const tagged_array* data;
    const overlaytext* overlay;
    cv::VideoWriter* writer;
    public:
        VidOutput(int,int,const tagged_array*,const char* const);
        void DoOutput();
};
class TextOutput : public SingleFileOutput
{
    const tagged_array* data;
    public:
        TextOutput(int,int,const tagged_array* );
        void DoOutput() ;
};
class ConsoleOutput: public Output
{
    const tagged_array* data;
    public:
        ConsoleOutput(int,int,const tagged_array*);
        void DoOutput();
};
class SpikeOutput: public SingleFileOutput
{
    const lagstorage* data;
    public:
        SpikeOutput(int,int,const lagstorage*);
        void DoOutput();
};
extern "C" {
#endif
typedef struct output_parameters output_parameters;
void DoOutputs(const unsigned int time);
void MakeOutputs(const output_parameters* const m);
void CleanupOutputs();
extern on_off showimages;
#ifdef MATLAB
#include "../matlab_includes.h"
#include "../output.h"
void outputExtraThings(mxArray* plhs[],int nrhs,const mxArray* prhs[]);
mxArray* outputToMxArray (const output_s input);
#endif
#ifdef __cplusplus
	}
#endif
