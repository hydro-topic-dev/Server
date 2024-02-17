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

    class FileBase;

    template <typename Iter>
    concept folder_files_iterator = requires {
        requires std::forward_iterator<Iter> && std::same_as<
                                                    std::decay_t<typename Iter::value_type>,
                                                    std::unique_ptr<FileBase>>;
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

    template <folder_files_iterator Iter>
    class FolderIteratorBase : public Iter
    {
    public:
        using original_type = Iter;
        using value_type = FileBase;
        using reference = FileBase&;
        using pointer = FileBase*;

        using Iter::Iter;

        const reference operator*() const
        {
            return *(*static_cast<const original_type&>(*this));
        }

        reference operator*()
        {
            return *(*static_cast<original_type&>(*this));
        }

        const pointer operator->() const
        {
            return &(*(*this));
        }

        pointer operator->()
        {
            return &(*(*this));
        }
    };

    class Folder : public FileBase
    {
    private:
        using container_of_file = std::unordered_set<std::unique_ptr<FileBase>>;
    public:
        using iterator = FolderIteratorBase<container_of_file::iterator>;
        using const_iterator =
            FolderIteratorBase<container_of_file::const_iterator>;
    private:
        void copy_files_from_folder(const container_of_file& files);
        const FileBase& search_file(const std::string& name) const;
        FileBase& search_file(const std::string& name);
    public:
        // How duplicate files are handled
        enum class ProcessingSameNameFiles
        {
            overwrite,
            throw_exception
        };

        static inline ProcessingSameNameFiles get_way_of_handling_same_name_files() noexcept;
        static inline void change_way_of_handling_same_name_files(ProcessingSameNameFiles processing
        ) noexcept;

        Folder() = delete;
        Folder(Folder&&) noexcept = default;
        Folder(const Folder& right);
        Folder(const std::string& name);
        Folder(std::string&& name);

        Folder& operator=(const Folder& right);
        Folder& operator=(Folder&&) = default;

        inline const_iterator cbegin() const noexcept;
        inline const_iterator begin() const noexcept;
        inline iterator begin() noexcept;
        inline const_iterator cend() const noexcept;
        inline const_iterator end() const noexcept;
        inline iterator end() noexcept;
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
        container_of_file m_files;
        Folder* m_parent = nullptr;
        static inline ProcessingSameNameFiles m_processing_same_name_files =
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
        decltype(auto) create(FileType&& file, const std::filesystem::path& path = ".")
        requires std::is_base_of_v<FileBase, std::decay_t<FileType>>;
        bool remove(const std::filesystem::path& path);
        inline void change_directory(const std::filesystem::path& path);
        inline std::filesystem::path get_working_directory() const noexcept;
        std::vector<std::reference_wrapper<const File>> search_file(const std::string& name) const;
        std::vector<std::reference_wrapper<File>> search_file(const std::string& name);
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

    inline Folder::ProcessingSameNameFiles
    server::Folder::get_way_of_handling_same_name_files() noexcept
    {
        return m_processing_same_name_files;
    }

    inline void Folder::change_way_of_handling_same_name_files(ProcessingSameNameFiles processing
    ) noexcept
    {
        m_processing_same_name_files = processing;
    }

    inline Folder::const_iterator Folder::cbegin() const noexcept
    {
        return static_cast<const_iterator&&>(m_files.cbegin());
    }

    inline Folder::const_iterator Folder::begin() const noexcept
    {
        return cbegin();
    }

    inline Folder::iterator Folder::begin() noexcept
    {
        return static_cast<iterator&&>(m_files.begin());
    }

    inline Folder::const_iterator Folder::cend() const noexcept
    {
        return static_cast<const_iterator&&>(m_files.cend());
    }

    inline Folder::const_iterator Folder::end() const noexcept
    {
        return cend();
    }

    inline Folder::iterator Folder::end() noexcept
    {
        return static_cast<iterator&&>(m_files.end());
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
