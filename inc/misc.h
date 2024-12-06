#ifndef MISC_H
#define MISC_H

#include <string>
#include <3ds.h>

#include "ui.h"

namespace misc
{
    void setPC();

    void clearStepHistory(void *a);

    void clearSoftwareLibraryAndPlayHistory(void *a);

    void clearSharedIconCache(void *a);

    void clearHomeMenuIconCache(void *a);

    void resetDemoPlayCount(void *a);

    void clearGameNotes(void *a);

    void removeSoftwareUpdateNag(void *a);
}

#endif
