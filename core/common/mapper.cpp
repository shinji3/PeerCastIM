#include <limits.h>
#include <stdlib.h>

#include "common.h"
#include "mapper.h"
#include "str.h"
#include "_string.h"

#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

FileSystemMapper::FileSystemMapper(const std::string& aVirtualPath, const std::string& aDocumentRoot)
    : virtualPath(aVirtualPath)
{
    char *dr = _fullpath(NULL, aDocumentRoot.c_str(), aDocumentRoot.length());
    if (!dr)
    {
        throw GeneralException(String::format("Document root `%s` inaccessible", aDocumentRoot.c_str()));
    }
    documentRoot = dr;
    free(dr);
}

std::string FileSystemMapper::toLocalFilePath(const std::string& vpath)
{
    if (virtualPath == vpath ||
        !str::is_prefix_of(virtualPath + "/", vpath))
        return "";

    auto filePath = str::replace_prefix(vpath, virtualPath, documentRoot);

    char* p;
    p = _fullpath(NULL, filePath.c_str(), filePath.length());

    if (p == NULL)
    {
        LOG_ERROR("Cannot resolve path %s", filePath.c_str());
        return "";
    }

    return p;
}
