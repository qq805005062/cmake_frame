#include "MessageFilesImpl.h"

MessageFiles* MessageFilesFactory::New()
{
    return new MessageFilesImpl();
}

void MessageFilesFactory::Destroy(MessageFiles* files)
{
    if (files)
    {
        delete files;
        files = 0;
    }
}

