#ifdef __cplusplus
extern "C" {
#endif
#include "../output.h"
void CreateOutputtable (const output_s out);
void CreateOverlay(const overlaytext overlay);
void CleanupOutput();
output_s GetOutputByName(const char* const name);
overlaytext* GetOverlayByName(const char* const name);
#ifdef __cplusplus
}
#endif
