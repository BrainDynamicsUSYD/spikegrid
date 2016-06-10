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
        Output(int intervalin,int idxin) {interval=intervalin;idx=idxin;}
        virtual void DoOutput_() {}
        virtual void update();
        void DoOutput();
        int GetInterval() const {return interval;}
        int GetIdx() const {return idx;}
        virtual ~Output() {}
};

class TAOutput : public Output
{
    const output_s* out;
    protected:
        const tagged_array* data;
    public:
        TAOutput(int,int,const output_s*);
        void update();
};
/// outputs to series of pictures
class PNGoutput : public TAOutput
{
    int count=0;
    protected:
        const overlaytext* overlay;
    public:
        PNGoutput(int,int,const output_s*,const char* const );
        void DoOutput_() ;
};
class GUIoutput : public PNGoutput
{
    char* winname;
    public:
        GUIoutput(int,int,  const output_s*, const char* const,const char* const);
        void DoOutput_();
};
class SingleFileOutput : public Output
{
    protected:
        FILE* f;
    public:
        SingleFileOutput(int,int );
        virtual void DoOutput_() {}
};
class VidOutput: public TAOutput //This class probably needs a destructor to end the video.  Vlc will probably handle the file just fine though.
{
    const overlaytext* overlay;
    cv::VideoWriter* writer;
    public:
        VidOutput(int,int,const output_s*,const char* const);
        void DoOutput_();
};
class TextOutput : public SingleFileOutput //maybe this should inherit from TAoutput - unclear here
{
    const output_s* out;
    const tagged_array* data;
    public:
        TextOutput(int,int,const output_s* );
        void DoOutput_() ;
};
class ConsoleOutput: public TAOutput
{
    public:
        ConsoleOutput(int,int,const output_s*);
        void DoOutput_();
};
class SpikeOutput: public SingleFileOutput
{
    const simplestorage* data;
    public:
        SpikeOutput(int,int,const simplestorage*);
        void DoOutput_();
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
