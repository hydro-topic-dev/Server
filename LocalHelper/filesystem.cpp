#include "filesystem.h"

#include <queue>
using std::bad_cast;
using std::invalid_argument;
using std::make_unique;
using std::move;
using std::next;
using std::prev;
using std::queue;
using std::reference_wrapper;
using std::runtime_error;
using std::string;
using std::unique_ptr;
using std::unordered_set;
using std::vector;
namespace filesystem = std::filesystem;
namespace ranges = std::ranges;

void server::Folder::copy_files_from_folder(const container_of_file& files)
{
    for (const auto& e : files) {
        const auto& type = typeid(*e);
        assert(type == typeid(File) || type == typeid(Folder));
        if (type == typeid(File)) {
            m_files.insert(make_unique<File>(static_cast<const File&>(*e)));
        } else {
            m_files.insert(make_unique<Folder>(static_cast<const Folder&>(*e)));
        }
    }
}

server::Folder::Folder(const Folder& right) : FileBase(right.name())
{
    copy_files_from_folder(right.m_files);
}

server::Folder::Folder(const string& name) : FileBase(name) {}

server::Folder::Folder(string&& name) : FileBase(move(name)) {}

const server::FileBase& server::Folder::search_file(const string& name) const
{
    for (const auto& file : m_files) {
        if (file->name() == name) {
            return *file;
        }
    }

    throw invalid_argument{ "Unknown filename." };
}

server::FileBase& server::Folder::search_file(const string& name)
{
    for (const auto& file : m_files) {
        if (file->name() == name) {
            return *file;
        }
    }

    throw invalid_argument{ "Unknown filename." };
}

server::Folder& server::Folder::operator=(const Folder& right)
{
    copy_files_from_folder(right.m_files);
    return *this;
}

bool server::Folder::has_file(const string& name) const noexcept
{
    for (const auto& file : m_files) {
        if (file->name() == name) {
            return true;
        }
    }

    return false;
}

const server::File& server::Folder::get_file(const string& name) const
{
    try {
        return dynamic_cast<const File&>(search_file(name));
    } catch (bad_cast) {
        throw runtime_error{ "The name does not refer to a file." };
    }
}

server::File& server::Folder::get_file(const string& name)
{
    try {
        return dynamic_cast<File&>(search_file(name));
    } catch (bad_cast) {
        throw runtime_error{ "The name does not refer to a file." };
    }
}

const server::Folder& server::Folder::get_folder(const string& name) const
{
    try {
        return dynamic_cast<const Folder&>(search_file(name));
    } catch (bad_cast) {
        throw runtime_error{ "The name does not refer to a folder." };
    }
}

server::Folder& server::Folder::get_folder(const string& name)
{
    try {
        return dynamic_cast<Folder&>(search_file(name));
    } catch (bad_cast) {
        throw runtime_error{ "The name does not refer to a folder." };
    }
}

bool server::Folder::remove(const string& name) noexcept
{
    auto iter = m_files.begin();
    auto end = m_files.end();
    for (; iter != end; ++iter) {
        if ((*iter)->name() == name) {
            m_files.erase(iter);
            return true;
        }
    }

    return false;
}

const server::Folder&
server::FileSystem::entry_path(const Folder& folder, const filesystem::path& path) const
{
    const Folder* now = &folder;
    for (const auto& subdir : path) {
        if (subdir == ".") {
            continue;
        } else if (subdir == "..") {
            now = &(now->get_parent());
        } else [[unlikely]] if (subdir == "/") {
            now = &m_root;
        } else [[likely]] if (!subdir.empty()) {
            now = &(now->get_folder(subdir.generic_string()));
        }
    }

    return *now;
}

server::Folder& server::FileSystem::entry_path(Folder& folder, const filesystem::path& path)
{
    Folder* now = &folder;
    for (const auto& subdir : path) {
        if (subdir == ".") {
            continue;
        } else if (subdir == "..") {
            now = &(now->get_parent());
        } else [[unlikely]] if (subdir == "/") {
            now = &m_root;
        } else [[likely]] if (!subdir.empty()) {
            now = &(now->get_folder(subdir.generic_string()));
        }
    }

    return *now;
}

server::FileSystem::FileSystem() : m_root("/"), m_active_folder(&m_root) {}

const server::File& server::FileSystem::get_file(const filesystem::path& path) const
{
    const Folder& folder = get_folder(path.parent_path());
    return folder.get_file(path.filename().generic_string());
}

server::File& server::FileSystem::get_file(const filesystem::path& path)
{
    Folder& folder = get_folder(path.parent_path());
    return folder.get_file(path.filename().generic_string());
}

const server::Folder& server::FileSystem::get_folder(const filesystem::path& path) const
{
    return entry_path(*m_active_folder, path);
}

server::Folder& server::FileSystem::get_folder(const filesystem::path& path)
{
    return entry_path(*m_active_folder, path);
}

bool server::FileSystem::remove(const filesystem::path& path)
{
    Folder& folder = get_folder(path.parent_path());
    return folder.remove(path.filename().generic_string());
}

vector<reference_wrapper<const server::File>> server::FileSystem::search_file(const string& name
) const
{
    vector<reference_wrapper<const File>> found_files;
    queue<reference_wrapper<const Folder>> unvisited_folders;
    unvisited_folders.emplace(m_root);

    while (!unvisited_folders.empty()) {
        for (const auto& file : unvisited_folders.front().get()) {
            if (typeid(file) == typeid(Folder)) {
                unvisited_folders.push(file.to_actually_type<Folder>());
            } else if (file.name() == name) {
                found_files.emplace_back(file.to_actually_type<File>());
            }
        }

        unvisited_folders.pop();
    }

    return found_files;
}

vector<reference_wrapper<server::File>> server::FileSystem::search_file(const string& name)
{
    vector<reference_wrapper<File>> found_files;
    queue<reference_wrapper<Folder>> unvisited_folders;
    unvisited_folders.emplace(m_root);

    while (!unvisited_folders.empty()) {
        for (auto& file : unvisited_folders.front().get()) {
            if (typeid(file) == typeid(Folder)) {
                unvisited_folders.emplace(file.to_actually_type<Folder>());
            } else if (file.name() == name) {
                found_files.emplace_back(file.to_actually_type<File>());
            }
        }

        unvisited_folders.pop();
    }

    return found_files;
}
