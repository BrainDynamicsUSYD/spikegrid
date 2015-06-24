#include <map>
#include "outputtable.h"
extern "C" {
#include "../output.h"
}
std::map<std::string,output_s> outt;
void CreateOutputtable (const output_s out)
{
    outt.insert(std::make_pair(std::string(out.name),out));
}

output_s GetOutputByName(const char* const name)
{
    if (outt.find(std::string(name)) != outt.end())
    {
        return outt.at(std::string(name));
    }
    else
    {
        printf("failed to return something\n");
        //do something when outputtable not found
    }
}
