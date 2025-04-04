
#ifndef  _EOKAS_BASE_FILE_H_
#define  _EOKAS_BASE_FILE_H_

#include "./header.h"
#include "./stream.h"

namespace eokas {
    /*
    =================================================================
    == FileStream
    =================================================================
    */
    /*
    FileOpenMode
    r	: readonly nocreate
    w	: writeonly
    a	: append
    r+	: read write nocreate
    w+	: read write
    a+	: read append
    rb, wb, ab, rb+, wb+, ab+   binary
    */
    class FileStream : public Stream {
    public:
        FileStream(FILE* handle);
        FileStream(const String& path, const String& mode);
        virtual ~FileStream();
    
    public:
        virtual bool open() override;
        virtual void close() override;
        virtual bool isOpen() const override;
        virtual bool readable() const override;
        virtual bool writable() const override;
        virtual bool eos() const override;
        virtual size_t pos() const override;
        virtual size_t size() const override;
        virtual size_t read(void* data, size_t size) override;
        virtual size_t write(void* data, size_t size) override;
        virtual bool seek(int offset, int origin = SEEK_CUR) override;
        virtual void flush() override;
    
    public:
        FILE* handle() const;
    
    private:
        FILE* mHandle;
        String mPath;
        String mMode;
    };
    
    /*
    =================================================================
    == FileInfo
    =================================================================
    */
    struct FileInfo {
        String name;
        u64_t size;
        bool isFile;
        bool isHidden;
        time_t atime;
        time_t mtime;
        time_t ctime;
    };
    
    using FileList = std::list<FileInfo>;
    
    /*
    =================================================================
    == File system interface
    =================================================================
    */
    class File {
    public:
        using FileInfoPredicate = std::function<bool(const FileInfo& info)>;
        using FileNamePredicate = std::function<bool(const String& name)>;
        
        static FileStream open(const String& path, const String& mode);
        static bool readText(const String& path, String& content);
        static bool readData(const String& path, void* data, size_t size);
        static bool writeText(const String& path, String& content);
        static bool writeData(const String& path, void* data, size_t size);
        static bool exists(const String& path);
        static bool isFile(const String& path);
        static bool isFolder(const String& path);
        static FileList listFileInfos(const String& path, FileInfoPredicate predicate = FileInfoPredicate());
        static StringList listFileNames(const String& path, FileNamePredicate predicate = FileNamePredicate());
        static StringList listFolderNames(const String& path, FileNamePredicate predicate = FileNamePredicate());
        static FileList glob(const String& path, const String& pattern);
        static String absolutePath(const String& path);
        static String basePath(const String& path);
        static String fileName(const String& path);
        static String fileNameWithoutExtension(const String& path);
        static String fileExtension(const String& path);
        static String combinePath(const String& path1, const String& path2);
    };
    
    /*
    =================================================================
    == Process interface
    =================================================================
    */
    class Process {
    public:
        static FileStream open(const String& command, const String& mode);
        static u32_t getPID();
        static String executingPath();
        static String workingPath();
    };
}

#endif//_EOKAS_BASE_FILE_H_
