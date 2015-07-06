#include <map>
#include <stdio.h>
#include "outputtable.h"
std::map<std::string,output_s> outt;
std::map<std::string,overlaytext> overlaymap;
void CreateOutputtable (const output_s out)
{
    outt.insert(std::make_pair(std::string(out.name),out));
}
void CreateOverlay(const overlaytext overlay)
{
    overlaymap.insert(std::make_pair(std::string(overlay.name),overlay));
}
output_s* GetOutputByName(const char* const name)
{
    if (outt.find(std::string(name)) != outt.end())
    {
        return &outt.at(std::string(name));
    }
    else
    {
        printf("failed to return something - tried to get %s \n  This is probably catastrophic and as a result the program should quit",name);
        return NULL;
    }
}
overlaytext* GetOverlayByName(const char* const name)
{
    if (overlaymap.find(std::string(name)) != overlaymap.end())
    {
        return &overlaymap.at(std::string(name));
    }
    else
    {
        return NULL;
    }
}

void CleanupOutput()
{
    //TODO: massive hackery - but it should work fine
    outt = *new std::map<std::string,output_s>();
    overlaymap = *new std::map<std::string,overlaytext>();
}
