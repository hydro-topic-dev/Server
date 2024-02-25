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
    concept folder_files_iterator =
        std::forward_iterator<Iter> &&
        std::same_as<std::decay_t<typename Iter::value_type>, std::unique_ptr<FileBase>>;

    class FileBase
    {
    public:
        constexpr virtual ~FileBase() = default;
        constexpr FileBase(const std::string& name);
        constexpr FileBase(std::string&& name) noexcept;

        constexpr std::string name() const noexcept;
        constexpr void rename(const std::string& new_name);
        constexpr void rename(std::string&& new_name) noexcept;

        template <typename T>
        constexpr const T& to_actually_type() const noexcept
        requires std::is_base_of_v<FileBase, T>;

        template <typename T>
        constexpr T& to_actually_type() noexcept
        requires std::is_base_of_v<FileBase, T>;
    private:
        std::string m_name;
    };

    class File : public FileBase
    {
    public:
        File() = delete;
        constexpr File(const File&) = default;
        constexpr File(const std::string& name, const std::string& content);
        constexpr File(const std::string& name, std::string&& content);
        constexpr File(std::string&& name, const std::string& content);
        constexpr File(std::string&& name, std::string&& content) noexcept;

        constexpr std::string content() const noexcept;
        constexpr void change_content(const std::string& new_content);
        constexpr void change_content(std::string&& new_content) noexcept;
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

        constexpr const reference operator*() const;
        constexpr reference operator*();
        constexpr const pointer operator->() const;
        constexpr pointer operator->();
    };

    class Folder : public FileBase
    {
    private:
        using container_of_file = std::unordered_set<std::unique_ptr<FileBase>>;
    public:
        using iterator = FolderIteratorBase<container_of_file::iterator>;
        using const_iterator = FolderIteratorBase<container_of_file::const_iterator>;
    private:
        void copy_files_from_folder(const container_of_file& files);
        const FileBase& search_file(const std::string& name) const;
        FileBase& search_file(const std::string& name);
    public:
        // What to do with files with the same name
        enum class HowToHandleFilesWithTheSameName
        {
            overwrite,
            throw_exception
        };

        Folder() = delete;
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
        decltype(auto)
        add(FileType&& file, HowToHandleFilesWithTheSameName how_to_handle_files_with_the_same_name)
        requires std::is_base_of_v<FileBase, std::decay_t<FileType>>;
        bool remove(const std::string& name) noexcept;
    private:
        container_of_file m_files;
        Folder* m_parent = nullptr;
    };

    class FileSystem
    {
    private:
        const Folder& entry_path(const Folder& folder, const std::filesystem::path& path) const;
        Folder& entry_path(Folder& folder, const std::filesystem::path& path);
    public:
        FileSystem();
        FileSystem(const FileSystem&) = default;

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
        Folder m_root;
        std::filesystem::path m_active_path;
        Folder* m_active_folder;
    };

    constexpr FileBase::FileBase(const std::string& name) : m_name(name) {}

    constexpr FileBase::FileBase(std::string&& name) noexcept : m_name(move(name)) {}

    constexpr File::File(const std::string& name, const std::string& content)
        : FileBase(name)
        , m_content(content)
    {}

    constexpr server::File::File(const std::string& name, std::string&& content)
        : FileBase(name)
        , m_content(move(content))
    {}

    constexpr server::File::File(std::string&& name, const std::string& content)
        : FileBase(move(name))
        , m_content(content)
    {}

    constexpr server::File::File(std::string&& name, std::string&& content) noexcept
        : FileBase(move(name))
        , m_content(move(content))
    {}

    constexpr std::string FileBase::name() const noexcept
    {
        return m_name;
    }

    constexpr void FileBase::rename(const std::string& new_name)
    {
        m_name = new_name;
    }

    constexpr void FileBase::rename(std::string&& new_name) noexcept
    {
        using std::move;
        m_name = new_name;
    }

    template <typename T>
    constexpr const T& FileBase::to_actually_type() const noexcept
    requires std::is_base_of_v<FileBase, T>
    {
        return static_cast<const T&>(*this);
    }

    template <typename T>
    constexpr T& FileBase::to_actually_type() noexcept
    requires std::is_base_of_v<FileBase, T>
    {
        return static_cast<T&>(*this);
    }

    constexpr std::string File::content() const noexcept
    {
        return m_content;
    }

    constexpr void File::change_content(const std::string& new_content)
    {
        m_content = new_content;
    }

    constexpr void File::change_content(std::string&& new_content) noexcept
    {
        using std::move;
        m_content = move(new_content);
    }

    template <folder_files_iterator Iter>
    constexpr const FolderIteratorBase<Iter>::reference FolderIteratorBase<Iter>::operator*() const
    {
        return *(*static_cast<const original_type&>(*this));
    }

    template <folder_files_iterator Iter>
    constexpr FolderIteratorBase<Iter>::reference FolderIteratorBase<Iter>::operator*()
    {
        return *(*static_cast<original_type&>(*this));
    }

    template <folder_files_iterator Iter>
    constexpr const FolderIteratorBase<Iter>::pointer FolderIteratorBase<Iter>::operator->() const
    {
        return &(*(*this));
    }

    template <folder_files_iterator Iter>
    constexpr FolderIteratorBase<Iter>::pointer FolderIteratorBase<Iter>::operator->()
    {
        return &(*(*this));
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
    decltype(auto) Folder::add(
        FileType&& file,
        HowToHandleFilesWithTheSameName how_to_handle_files_with_the_same_name
    )
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
            switch (how_to_handle_files_with_the_same_name) {
            case server::Folder::HowToHandleFilesWithTheSameName::overwrite:
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
            case server::Folder::HowToHandleFilesWithTheSameName::throw_exception:
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
