#ifndef _CMAKE_SCAN_FILE_HELPER_H_
#define _CMAKE_SCAN_FILE_HELPER_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>

class FileHelper
{
public:
    static std::string GetFileLocation(const std::string &file_)
    {
        size_t pos = file_.find_last_of("/");
        if (std::string::npos != pos)
        {
            return file_.substr(0, pos + 1);
        }
        return file_ + "/";
    }

    static int WriteFile(const std::string &file_, const std::string &data)
    {
        std::ofstream ofs;
        ofs.open(file_);
        if (ofs.is_open() == false)
        {
            std::cout << file_ << " open file failed" << std::endl;
            return -1;
        }

        ofs << data;

        ofs.close();
        return 0;
    }

    static int Chdir(std::string path)
    {
        return chdir(path.c_str());
    }

    static int DeleteFile(std::string path)
    {
        DIR *dir;
        dirent *dir_info;
        std::string file_path;
        if (IsFile(path))
        {
            remove(path.c_str());
            return 0;
        }
        if (IsDir(path))
        {
            if ((dir = opendir(path.c_str())) == NULL)
                return -1;
            while ((dir_info = readdir(dir)) != NULL)
            {
                if(file_path.back() == '/')
                    file_path = path + "/" + path;
                if (strcmp(path.c_str(), ".") == 0 || strcmp(path.c_str(), "..") == 0)
                    continue;
                if(0 != DeleteFile(file_path))
                {
                    return -1;
                }
                if(0 != rmdir(file_path.c_str()))
                {
                    return -1;
                }
            }
        }
        return 0;
    }

    static bool IsFile(std::string path)
    {
        struct stat statbuf;
        if(lstat(path.c_str(), &statbuf) ==0)
        return S_ISREG(statbuf.st_mode) != 0;//判断文件是否为常规文件
        return false;
    }

    static bool IsDir(std::string path)
    {
        if (path.empty())
        {
            return 0;
        }
        struct stat st;
        if (0 != stat(path.c_str(), &st))
        {
            return 0;
        }
        if (S_ISDIR(st.st_mode))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    static int CopyFile(std::string sourcePath, std::string destPath)
    {
        FILE *pIn = NULL;
        FILE *pOut = NULL;

        if ((pIn = fopen(sourcePath.c_str(), "r")) == NULL)
        {
            std::cout << "Open File " << sourcePath << " Failed...\n";
            return -1;
        }
        if ((pOut = fopen(destPath.c_str(), "w")) == NULL)
        {
            std::cout << "Create [" << destPath << "] File Failed...\n";
            return -1;
        }

        while (!feof(pIn))
        {
            fputc(fgetc(pIn), pOut);
        }

        if (0 != fclose(pOut) || 0 != fclose(pIn))
        {
            std::cout << "close file failed\n";
            return -1;
        }
        return 0;
    }

    static int CopyFolder(std::string sourcePath, std::string destPath)
    {
        if (!HasFile(sourcePath))
        {
            std::cout << "file not exist: " << sourcePath << std::endl;
            return -1;
        }
        if (!HasFile(destPath) && CreateDir(destPath))
        {
            std::cout << "create dir failed: " << destPath << std::endl;
            return -1;
        }
        struct dirent *filename = NULL;
        std::string path = sourcePath;
        if (sourcePath.back() != '/')
        {
            sourcePath += "/";
        }
        if (destPath.back() != '/')
        {
            destPath += "/";
        }

        DIR *dp = opendir(path.c_str());
        while (filename = readdir(dp))
        {
            std::string fileSourceFath = sourcePath;

            std::string fileDestPath = destPath;

            fileSourceFath += filename->d_name;
            fileDestPath += filename->d_name;
            if (IsDir(fileSourceFath.c_str()))
            {
                if (strncmp(filename->d_name, ".", 1) && strncmp(filename->d_name, "..", 2))
                {
                    if (0 != CopyFolder(fileSourceFath, fileDestPath))
                    {
                        std::cout << "CopyFolder failed\n";
                        return -1;
                    }
                }
            }
            else
            {
                if (0 != CopyFile(fileSourceFath, fileDestPath))
                {
                    std::cout << "CopyFolder failed\n";
                    return -1;
                }
            }
        }
        return 0;
    }

    static bool HasFile(std::string dirName_)
    {
        return (access(dirName_.c_str(), F_OK) == 0);
    }

    static bool HasRelativePathFile(const std::string &file)
    {
        char buf[128] = {0};
        std::string path;
        if (getcwd(buf, sizeof(buf)) == buf)
        {
            path = std::string(buf) + "/" + file;
            if (HasFile(path))
            {
                return true;
            }
        }
        std::cout << "access failed: " << path << std::endl;
        return false;
    }

    static bool GetExecuteDir(std::string &dir)
    {
        char buf[128] = {0};
        if (getcwd(buf, sizeof(buf)) == buf)
        {
            dir = buf;
            return true;
        }
        return false;
    }

    static int CreateDir(std::string dirName_)
    {
        if (!dirName_.empty() && dirName_.back() != '/')
        {
            dirName_ += "/";
        }

        size_t pos = dirName_.find("/");
        while (std::string::npos != pos)
        {
            size_t len = pos + 1 > dirName_.length() ? std::string::npos : pos + 1;
            std::string dir = dirName_.substr(0, len);

            if (access(dir.c_str(), F_OK) != 0)
            {
                if (mkdir(dir.c_str(), 0755) == -1)
                {
                    std::cout << dir << " mkdir error\n";
                    return -1;
                }
            }
            pos = dirName_.find("/", pos + 1);
        }

        return 0;
    }

    //获取特定格式的文件名
    static void GetAllSuffixFiles(const std::string &path, const std::string &suffix, std::vector<std::string> &files)
    {
        if (access(path.c_str(), F_OK) != 0)
        {
            std::cout << "can not stat path: " << path << std::endl;
            return;
        }

        DIR *dir;
        struct dirent *ptr;
        dir = opendir(path.c_str());
        while ((ptr = readdir(dir)) != NULL)
        {
            std::string file(ptr->d_name);
            if (file.size() >= suffix.size() && 0 == file.compare(file.size() - suffix.size(), suffix.size(), suffix))
            {
                std::cout << "locate file " << path + "/" + file << std::endl;
                files.emplace_back(path + "/" + file);
            }
        }
        closedir(dir);
        return;
    }
};
#endif