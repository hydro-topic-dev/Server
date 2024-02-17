#pragma once
#ifndef FILE_SYSTEM_H_
#  define FILE_SYSTEM_H_
#  include <cassert>
#  include <cstdlib>

#  include <algorithm>
#  include <filesystem>
#  include <memory>
#  include <ranges>
#  include <stdexcept>
#  include <string>
#  include <type_traits>
#  include <unordered_set>
#  include <utility>
#  include <variant>
#  include <vector>

namespace server
{
    template <typename Container>
    concept container = requires {
        typename Container::value_type;
        requires std::ranges::range<Container>;
    };

    class FileBase
    {
    public:
        virtual ~FileBase() = default;
        FileBase(const std::string& name);
        FileBase(std::string&& name) noexcept;

        inline std::string name() const noexcept;
        inline void rename(const std::string& new_name);
        inline void rename(std::string&& new_name) noexcept;
    private:
        std::string m_name;
    };

    class File : public FileBase
    {
    public:
        File() = delete;
        File(const File&) = default;
        File(File&&) noexcept = default;
        File(const std::string& name, const std::string& content);
        File(const std::string& name, std::string&& content);
        File(std::string&& name, const std::string& content);
        File(std::string&& name, std::string&& content) noexcept;

        File& operator=(const File&) = default;
        File& operator=(File&&) noexcept = default;

        inline std::string content() const noexcept;
        inline void change_content(const std::string& new_content);
        inline void change_content(std::string&& new_content) noexcept;
    private:
        std::string m_content;
    };

    class Folder : public FileBase
    {
    private:
        template <container Container>
        void copy_files(Container files)
        requires std::is_base_of_v<FileBase, typename Container::value_type>;
        void copy_files_from_folder(const std::unordered_set<std::unique_ptr<FileBase>>& files);
        const FileBase& find_file(const std::string& name) const;
        FileBase& find_file(const std::string& name);
    public:
        // How duplicate files are handled
        enum class ProcessingSameNameFiles
        {
            overwrite,
            throw_exception
        };

        Folder() = delete;
        Folder(Folder&&) noexcept = default;
        Folder(const Folder& right);
        Folder(const std::string& name);
        Folder(std::string&& name);
        template <container Container>
        Folder(const std::string& name, Container files)
        requires std::is_base_of_v<FileBase, typename Container::value_type>;

        Folder& operator=(const Folder& right);
        Folder& operator=(Folder&&) = default;

        inline ProcessingSameNameFiles get_way_of_handling_same_name_files() const;
        inline void change_way_of_handling_same_name_files(ProcessingSameNameFiles processing
        ) noexcept;
        inline bool has_parent() const noexcept;
        inline const Folder& get_parent() const noexcept;
        inline Folder& get_parent() noexcept;
        inline void set_parent(Folder& folder) noexcept;
        bool has_file(const std::string& name) const noexcept;
        const File& get_file(const std::string& name) const;
        File& get_file(const std::string& name);
        const Folder& get_folder(const std::string& name) const;
        Folder& get_folder(const std::string& name);
        template <typename FileType>
        decltype(auto) add(FileType&& file)
        requires std::is_base_of_v<FileBase, std::decay_t<FileType>>;
        bool remove(const std::string& name) noexcept;
    private:
        std::unordered_set<std::unique_ptr<FileBase>> m_files;
        Folder* m_parent = nullptr;
        ProcessingSameNameFiles m_processing_same_name_files =
            ProcessingSameNameFiles::throw_exception;
    };

    class FileSystem
    {
    private:
        const Folder& entry_path(const Folder& folder, const std::filesystem::path& path) const;
        Folder& entry_path(Folder& folder, const std::filesystem::path& path);
    public:
        FileSystem();
        FileSystem(const FileSystem&) = default;
        FileSystem(FileSystem&&) noexcept = default;
        ~FileSystem() = default;

        const File& get_file(const std::filesystem::path& path) const;
        File& get_file(const std::filesystem::path& path);
        const Folder& get_folder(const std::filesystem::path& path) const;
        Folder& get_folder(const std::filesystem::path& path);
        template <typename FileType>
        decltype(auto) create(FileType&& file, const std::filesystem::path& path)
        requires std::is_base_of_v<FileBase, std::decay_t<FileType>>;
        bool remove(const std::filesystem::path& path);
        inline void change_directory(const std::filesystem::path& path);
        inline std::filesystem::path get_working_directory() const noexcept;
        std::vector<std::reference_wrapper<const File>> find_file(const std::string& name) const;
        std::vector<std::reference_wrapper<File>> find_file(const std::string& name);
    private:
        std::filesystem::path m_active_path;
        Folder* m_active_folder;
        Folder m_root;
    };

    inline std::string FileBase::name() const noexcept
    {
        return m_name;
    }

    inline void FileBase::rename(const std::string& new_name)
    {
        m_name = new_name;
    }

    inline void FileBase::rename(std::string&& new_name) noexcept
    {
        using std::move;
        m_name = new_name;
    }

    inline std::string File::content() const noexcept
    {
        return m_content;
    }

    inline void File::change_content(const std::string& new_content)
    {
        m_content = new_content;
    }

    inline void File::change_content(std::string&& new_content) noexcept
    {
        using std::move;
        m_content = move(new_content);
    }

    template <container Container>
    inline void Folder::copy_files(Container files)
    requires std::is_base_of_v<FileBase, typename Container::value_type>
    {
        using std::make_unique;
        using std::same_as;

        for (const auto& file : files) {
            if constexpr (same_as<FileBase, typename Container::value_type>) {
                const auto& type = typeid(file);
                assert(type == typeid(File) || type == typeid(Folder));
                if (type == typeid(File)) {
                    m_files.insert(make_unique<File>(static_cast<const File&>(file)));
                } else {
                    m_files.insert(make_unique<Folder>(static_cast<const Folder&>(file)));
                }
            } else {
                m_files.insert(make_unique<typename Container::value_type>(file));
            }
        }
    }

    template <container Container>
    Folder::Folder(const std::string& name, Container container)
    requires std::is_base_of_v<FileBase, typename Container::value_type>
        : FileBase(name)
    {
        copy_files(container);
    }

    inline Folder::ProcessingSameNameFiles
    server::Folder::get_way_of_handling_same_name_files() const
    {
        return m_processing_same_name_files;
    }

    inline void Folder::change_way_of_handling_same_name_files(ProcessingSameNameFiles processing
    ) noexcept
    {
        m_processing_same_name_files = processing;
    }

    inline bool Folder::has_parent() const noexcept
    {
        return m_parent != nullptr;
    }

    inline const Folder& Folder::get_parent() const noexcept
    {
        return *m_parent;
    }

    inline Folder& Folder::get_parent() noexcept
    {
        return *m_parent;
    }

    inline void Folder::set_parent(Folder& folder) noexcept
    {
        m_parent = &folder;
    }

    template <typename FileType>
    decltype(auto) Folder::add(FileType&& file)
    requires std::is_base_of_v<FileBase, std::decay_t<FileType>>
    {
        using std::abort;
        using std::decay_t;
        using std::forward;
        using std::make_unique;
        using std::runtime_error;
        using real_type = decay_t<FileType>;

        [[likely]] if (!has_file(file.name())) {
            auto& added_file =
                *(*(m_files.insert(make_unique<real_type>(forward<FileType>(file))).first));
            return static_cast<real_type&>(added_file);
        } else {
            switch (m_processing_same_name_files) {
            case server::Folder::ProcessingSameNameFiles::overwrite:
                {
                    const auto& type = typeid(real_type);
                    assert(type == typeid(File) || type == typeid(Folder));
                    FileBase* found_file;
                    if (type == typeid(File)) {
                        found_file = &get_file(file.name());
                    } else {
                        found_file = &get_folder(file.name());
                    }

                    static_cast<real_type&>(*found_file) = forward<FileType>(file);
                    return static_cast<real_type&>(*found_file);
                }
            case server::Folder::ProcessingSameNameFiles::throw_exception:
                throw runtime_error{ "A file with the same name exists" };
            default:
                assert(false);
                abort();
            }
        }
    }

    template <typename FileType>
    decltype(auto) FileSystem::create(FileType&& file, const std::filesystem::path& path)
    requires std::is_base_of_v<FileBase, std::decay_t<FileType>>
    {
        using std::forward;
        return get_folder(path.parent_path()).add(forward<FileType>(file));
    }

    inline void FileSystem::change_directory(const std::filesystem::path& path)
    {
        m_active_folder = &get_folder(path);
        m_active_path = path;
    }

    inline std::filesystem::path FileSystem::get_working_directory() const noexcept
    {
        return m_active_path;
    }
}  // namespace server
#endif  // !FILE_SYSTEM_H_
