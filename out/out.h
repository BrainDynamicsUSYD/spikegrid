#ifdef __cplusplus
class Output
{
    int interval;
    int idx;
    public:
        Output(int intervalin,int idxin) {interval=intervalin;idx=idxin;};
        virtual void DoOutput() {};
        int GetInterval() const {return interval;};
        int GetIdx() const {return idx;};
};
class PNGoutput : public Output
{
    int count=0;
    const tagged_array* data;
    public:
        PNGoutput(int,int,const tagged_array* );
        void DoOutput() ;
};
class TextOutput : public Output
{
    const tagged_array* data;
    protected:
        FILE* f;
    public:
        TextOutput(int,int,const tagged_array* );
        virtual void DoOutput() ;
};
class ConsoleOutput: public Output
{
    const tagged_array* data;
    public:
        ConsoleOutput(int,int,const tagged_array*);
        virtual void DoOutput();
};
class SpikeOutput: public TextOutput
{
    const lagstorage* data;
    public:
        SpikeOutput(int,int,const lagstorage*);
        virtual void DoOutput();
};
extern "C" {
#endif
typedef struct output_parameters output_parameters;
void DoOutputs(const unsigned int time);
void MakeOutputs(const output_parameters* const m);
void CleanupOutputs();
#ifdef __cplusplus
	}
#endif
