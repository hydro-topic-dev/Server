#include <concepts>

#include <format>
#include <iostream>
#include <string>

#include "filesystem.h"
using namespace std;
using namespace server;

int main()
{
    FileSystem fs;
    fs.create(File{ "file1", "file1" });
    fs.create(Folder{ "folder1" });
    fs.change_directory("folder1");
    fs.create(File{ "file2", "file2" });
    fs.create(File{ "file3", "file3" }, "..");

    return 0;
}
