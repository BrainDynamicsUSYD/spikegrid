#ifdef __cplusplus
	extern "C" {
#endif
typedef struct output_parameters output_parameters;
void DoOutputs(const unsigned int time);
void MakeOutputs(const output_parameters* const m);
void CleanupOutputs();
#ifdef __cplusplus
	}
#endif
