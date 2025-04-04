
#include "./io.h"
#include "./string.h"
#include <regex>

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
#include <Windows.h>
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace eokas {

#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32

    static void FileTimeToTimeT(FILETIME* ft, time_t* tt)
    {
        ULARGE_INTEGER uln;
        uln.LowPart = ft->dwLowDateTime;
        uln.HighPart = ft->dwHighDateTime;
        *tt = ((LONGLONG)(uln.QuadPart - 116444736000000000) / 10000000);
    }
    
#endif

    /*
    =================================================================
    == FileStream
    =================================================================
    */
    FileStream::FileStream(FILE* handle)
        :mHandle(handle)
        ,mPath()
        ,mMode(){
        
    }
    
    FileStream::FileStream(const String& path, const String& mode)
        :mHandle(nullptr)
        ,mPath(path)
        ,mMode(mode)
    {}
    
    FileStream::~FileStream()
    {
        this->close();
    }
    
    bool FileStream::open()
    {
        if (mHandle != nullptr)
        {
            fseek(mHandle, 0, SEEK_SET);
            return true;
        }
        mHandle = fopen(mPath.cstr(), mMode.cstr());
        return mHandle != nullptr;
    }
    
    void FileStream::close()
    {
        if(mHandle != nullptr)
        {
            fclose(mHandle);
            mHandle = nullptr;
        }
    }
    
    bool FileStream::isOpen() const
    {
        return mHandle != nullptr;
    }
    
    bool FileStream::readable() const
    {
        return mMode.startsWith("r");
    }
    
    bool FileStream::writable() const
    {
        return mMode.startsWith("w") || mMode.startsWith("a");
    }
    
    bool FileStream::eos() const
    {
        return feof(mHandle) != 0;
    }
    
    size_t FileStream::pos() const
    {
        return ftell(mHandle);
    }
    
    size_t FileStream::size() const
    {
        long pos = ftell(mHandle);
        fseek(mHandle, 0, SEEK_END);
        long len = ftell(mHandle);
        fseek(mHandle, pos, SEEK_SET);
        return len;
    }
    
    size_t FileStream::read(void* data, size_t size)
    {
        return fread(data, 1, size, mHandle);
    }
    
    size_t FileStream::write(void* data, size_t size)
    {
        return fwrite(data, 1, size, mHandle);
    }
    
    bool FileStream::seek(int offset, int origin)
    {
        return fseek(mHandle, offset, origin) == 0;
    }
    
    void FileStream::flush()
    {
        fflush(mHandle);
    }
    
    FILE* FileStream::handle() const
    {
        return mHandle;
    }
    
    /*
    =================================================================
    == File System Interface
    =================================================================
    */
    FileStream File::open(const eokas::String& path, const eokas::String& mode) {
        FILE* handle = fopen(path.cstr(), mode.cstr());
        return FileStream(handle);
    }
    
    bool File::readText(const String& path, String& content) {
        FileStream stream(path, "r");
        if(!stream.isOpen())
            return false;
        size_t size = stream.size();
        content = String(' ', size);
        stream.read((void*)content.cstr(), size);
        stream.close();
        return true;
    }
    
    bool File::readData(const String& path, void* data, size_t size) {
        FileStream stream(path, "rb");
        if(!stream.isOpen())
            return false;
        size = size <= stream.size() ? size : stream.size();
        stream.read(data, size);
        stream.close();
        return true;
    }
    
    bool File::writeText(const String& path, String& content) {
        FileStream stream(path, "w");
        if(!stream.isOpen())
            return false;
        stream.write((void*)content.cstr(), content.length());
        stream.close();
        return true;
    }
    
    bool File::writeData(const String& path, void* data, size_t size) {
        FileStream stream(path, "wb");
        if(!stream.isOpen())
            return false;
        stream.write(data, size);
        stream.close();
        return true;
    }
    
    bool File::exists(const String& path)
    {
    #if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        return _access(path.cstr(), 0) == 0;
    #else
        return access(path.cstr(), F_OK) == 0;
    #endif
    }
    
    bool File::isFile(const String& path)
    {
    #if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        DWORD attr = GetFileAttributesA(path.cstr());
        if (attr == INVALID_FILE_ATTRIBUTES)
            return false;
        return !(attr & FILE_ATTRIBUTE_DIRECTORY);
    #else
        if (access(path.cstr(), 0) == 0)
        {
            struct stat status;
            stat(path.cstr(), &status);
            return status.st_mode & S_IFREG;
        }
        return false;
    #endif
    }
    
    bool File::isFolder(const String& path)
    {
    #if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        DWORD attr = GetFileAttributesA(path.cstr());
        if (attr == INVALID_FILE_ATTRIBUTES)
            return false;
        return !!(attr & FILE_ATTRIBUTE_DIRECTORY);
    #else
        if (access(path.cstr(), 0) == 0)
        {
            struct stat status;
            stat(path.cstr(), &status);
            return status.st_mode & S_IFDIR; // FIXME: what about symlinks here?
        }
        return false;
    #endif
    }
    
    FileList File::listFileInfos(const String& path, FileInfoPredicate predicate)
    {
        FileList list;
    #if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        String search = File::combinePath(path, "*");
        WIN32_FIND_DATAA f;
        HANDLE h = FindFirstFileA(search.cstr(), &f);
        if (h != INVALID_HANDLE_VALUE)
        {
            do
            {
                FileInfo info;
                info.name = f.cFileName;
                ULARGE_INTEGER n;
                n.LowPart = f.nFileSizeLow;
                n.HighPart = f.nFileSizeHigh;
                info.size = n.QuadPart;
                info.isFile = !(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                info.isHidden = !!(f.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
                FileTimeToTimeT(&f.ftLastAccessTime, &info.atime);
                FileTimeToTimeT(&f.ftLastWriteTime, &info.mtime);
                FileTimeToTimeT(&f.ftCreationTime, &info.ctime);
                if(!predicate || predicate(info)) {
                    list.push_back(info);
                }
            } while (FindNextFileA(h, &f));
            FindClose(h);
        }
    #else
        dirent* ptr = nullptr;
        DIR* dir = opendir(path.cstr());
        while ((ptr = readdir(dir)) != nullptr)
        {
            const char* name = ptr->d_name;
            struct stat status;
            stat((path + "/" + name).cstr(), &status);
    
            FileInfo info;
            info.name = name;
            info.size = status.st_size;
            info.isFile = (status.st_mode & S_IFREG) != 0;
            info.isHidden = false;
            info.atime = status.st_atime;
            info.mtime = status.st_mtime;
            info.ctime = status.st_ctime;
    
            if(!predicate || predicate(info)) {
                list.push_back(info);
            }
        }
        closedir(dir);
    #endif
        return list;
    }
    
    StringList File::listFileNames(const eokas::String& path, FileNamePredicate predicate)
    {
        StringList list;
    #if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        String search = File::combinePath(path, "*");
        WIN32_FIND_DATAA f;
        HANDLE h = FindFirstFileA(search.cstr(), &f);
        if (h != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    const char* name = f.cFileName;
                    if(!predicate || predicate(name)) {
                        list.push_back(name);
                    }
                }
            } while (FindNextFileA(h, &f));
            FindClose(h);
        }
    #else
        dirent* ptr = nullptr;
        DIR* dir = opendir(path.cstr());
        while ((ptr = readdir(dir)) != nullptr)
        {
            const char* name = ptr->d_name;
            if(!predicate || predicate(name)) {
                list.push_back(name);
            }
        }
        closedir(dir);
    #endif
        return list;
    }
    
    StringList File::listFolderNames(const String& path, FileNamePredicate predicate)
    {
        StringList list;
    #if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        String search = File::combinePath(path, "*");
        WIN32_FIND_DATAA f;
        HANDLE h = FindFirstFileA(search.cstr(), &f);
        if (h != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    const char* name = f.cFileName;
                    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                        continue;
                    if(!predicate || predicate(name)) {
                        list.push_back(name);
                    }
                }
            } while (FindNextFileA(h, &f));
            FindClose(h);
        }
    #else
        dirent* ptr = nullptr;
        DIR* dir = opendir(path.cstr());
        while ((ptr = readdir(dir)) != nullptr)
        {
            const char* name = ptr->d_name;
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;
            if(!predicate || predicate(name)) {
                list.push_back(name);
            }
        }
        closedir(dir);
    #endif
        return list;
    }
    
    FileList File::glob(const eokas::String& path, const eokas::String& pattern) {
        auto predicate = [&](const FileInfo& info)->bool{
            std::string regex_pattern = std::regex_replace(pattern.cstr(), std::regex(R"(\.)"), R"(\.)");
            regex_pattern = std::regex_replace(regex_pattern, std::regex(R"(\*)"), R"(.*)");
            regex_pattern = std::regex_replace(regex_pattern, std::regex(R"(\?)"), R"(.)");
            std::regex regex_expr(regex_pattern, std::regex::icase);
            return std::regex_match(info.name.cstr(), regex_expr);
        };
        return File::listFileInfos(path, predicate);
    };
    
    String File::absolutePath(const String& path)
    {
    #if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        char resolved_path[MAX_PATH] = { 0 };
        _fullpath(resolved_path, path.cstr(), MAX_PATH);
        return resolved_path;
    #else
        char resolved_path[PATH_MAX] = { 0 };
        realpath(path.cstr(), resolved_path);
        return resolved_path;
    #endif
    }
    
    String File::basePath(const String& path)
    {
        size_t index1 = path.rfind('/');
        size_t index2 = path.rfind('\\');
        if (index1 == String::npos && index2 == String::npos)
            return "./";
        if (index1 == String::npos)
            return path.substr(0, index2);
        if (index2 == String::npos)
            return path.substr(0, index1);
        return path.substr(0, index1 < index2 ? index1 : index2);
    }
    
    String File::fileName(const String& path)
    {
        size_t index1 = path.rfind('/');
        size_t index2 = path.rfind('\\');
        if (index1 == String::npos && index2 == String::npos)
            return path;
        if (index1 == String::npos)
            return path.substr(index2);
        if (index2 == String::npos)
            return path.substr(index1);
        return path.substr(index1 < index2 ? index1 : index2);
    }
    
    String File::fileNameWithoutExtension(const String& path)
    {
        String name = File::fileName(path);
        size_t index = name.rfind('.');
        if (index == String::npos)
            return name;
        return name.substr(0, index);
    }
    
    String File::fileExtension(const String& path)
    {
        String name = File::fileName(path);
        size_t index = name.rfind('.');
        if (index == String::npos)
            return "";
        return name.substr(index);
    }
    
    String File::combinePath(const String& path1, const String& path2)
    {
        if(path1.isEmpty()) return path2;
        if(path2.isEmpty()) return path1;
        
        char end = path1.at(path1.length() - 1);
        if(end == '/' || end == '\\')
            return path1 + path2;
        
        return path1 + "/" + path2;
    }
    
    FileStream Process::open(const eokas::String& command, const eokas::String& mode) {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        FILE* handle = _popen(command.cstr(), mode.cstr());
        return FileStream(handle);
#else
        FILE* handle = popen(command.cstr(), mode.cstr());
        return FileStream(handle);
#endif
    }
    
    u32_t Process::getPID() {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        return GetCurrentProcessId();
#else
        return getpid();
#endif
    }
    
    String Process::executingPath() {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        char buf[MAX_PATH];
        DWORD ret = GetModuleFileNameA(NULL, buf, MAX_PATH);
        if (ret == 0)
            return "";
        return buf;
#else
        char buf[PATH_MAX];
        size_t ret = readlink("/proc/self/exe", buf, PATH_MAX - 1);
        if (ret <= 0 || (ret > PATH_MAX - 1))
            return "";
        buf[ret] = '\0';
        return buf;
#endif
    }
    
    String Process::workingPath() {
#if _EOKAS_OS == _EOKAS_OS_WIN64 || _EOKAS_OS == _EOKAS_OS_WIN32
        char buf[MAX_PATH] = { 0 };
        _getcwd(buf, MAX_PATH);
        return buf;
#else
        char buf[1024] = { 0 };
        getcwd(buf, 1024);
        return buf;
#endif
    }
}