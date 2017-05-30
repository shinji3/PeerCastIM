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

using namespace std;
using namespace str;

FileSystemMapper::FileSystemMapper(const string& aVirtualPath, const string& aDocumentRoot)
    : virtualPath(aVirtualPath)
{
    char *dr = _fullpath(NULL, aDocumentRoot.c_str(), _MAX_PATH);
    if (!dr)
    {
        throw GeneralException(String::format("Document root `%s` inaccessible", aDocumentRoot.c_str()));
    }
    documentRoot = dr;
    free(dr);
}

string FileSystemMapper::toLocalFilePath(const string& vpath)
{
    return toLocalFilePath(vpath, {}).first;
}

pair<string,string> FileSystemMapper::resolvePath(const string& rawPath, const vector<string>& langs)
{
    // if there's a language neutral version, return it
    if (realPath(rawPath) != "")
        return make_pair(realPath(rawPath), "");

    // otherwise, try each of the extensions
    for (auto ext : langs)
    {
        auto r = realPath(rawPath + "." + ext);
        if (r != "")
            return make_pair(r, ext);
    }

    // default to the English version if there is one
    auto r = realPath(rawPath + ".en");
    if (r != "")
        return make_pair(r, "en");

    return make_pair("", "");
}

string FileSystemMapper::realPath(const string& path)
{
    char resolvedPath[_MAX_PATH];
    char *p = _fullpath(resolvedPath, path.c_str(), _MAX_PATH);

    if (!p)
        return "";
    else
        return resolvedPath;
}

pair<string,string> FileSystemMapper::toLocalFilePath(const string& vpath, const vector<string>& langs)
{
    if (virtualPath == vpath || !is_prefix_of(virtualPath + "/", vpath))
        return make_pair("", "");

    auto filePath = replace_prefix(vpath, virtualPath, documentRoot);

    string resolvedPath, resolvedLang;
    tie(resolvedPath, resolvedLang) = resolvePath(filePath, langs);

    if (resolvedPath == "")
    {
        LOG_ERROR("Cannot resolve path %s", filePath.c_str());
        return make_pair("", "");
    }

    // �f�B���N�g���g���o�[�T���`�F�b�N
    if (documentRoot == resolvedPath || !is_prefix_of(documentRoot, resolvedPath))
    {
        LOG_ERROR("Possible directory traversal attack!");
        return make_pair("", "");
    }

    return make_pair(resolvedPath, resolvedLang);
}
