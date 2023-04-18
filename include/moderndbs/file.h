#ifndef INCLUDE_MODERNDBS_FILE_H_
#define INCLUDE_MODERNDBS_FILE_H_

#include <cstdint>
#include <memory>


namespace moderndbs {

///
/// Block-wise file API for C++.
///
class File {
public:
    /// File mode (read or write)
    enum Mode { READ, WRITE };

    File() = default;
    File(const File&) = default;
    File(File&&) = default;
    File& operator=(const File&) = default;
    File& operator=(File&&) = default;
    virtual ~File() = default;

    /// Returns the `Mode` this file was opened with.
    [[nodiscard]] virtual Mode get_mode() const = 0;

    /// Returns the current size of the file in bytes.
    /// Is not thread-safe w.r.t concurrent calls to `resize()`.
    [[nodiscard]] virtual size_t size() const = 0;

    /// Resizes the file to `new_size`. If `new_size` is smaller than `size()`,
    /// the file is cut off at the end. Otherwise zero bytes are appended at
    /// the end.
    /// Is not thread-safe.
    virtual void resize(size_t new_size) = 0;

    /// Reads a block of the file. `offset + size` must not be larger than
    /// `size()`.
    /// Is thread-safe w.r.t concurrent calls to `read_block()` and
    /// `write_block()`.
    /// @param[in]  offset The offset in the file from which the block should
    ///                    be read.
    /// @param[in]  size   The size of the block.
    /// @param[out] block  A pointer to memory where the block is written to.
    ///                    Must be able to hold at least `size` bytes.
    virtual void read_block(size_t offset, size_t size, char* block) = 0;

    /// Reads a block of the file and returns it.
    [[nodiscard]] std::unique_ptr<char[]> read_block(size_t offset, size_t size) { // NOLINT(cppcoreguidelines-avoid-c-arrays)
        auto block = std::make_unique<char[]>(size);
        read_block(offset, size, block.get());
        return block;
    }

    /// Writes a block to the file. `offset + size` must not be larger than
    /// `size()`. If you want to write past the end of the file, use
    /// `resize()` first.
    /// This function must not be used when the file was opened in `READ` mode.
    /// Is thread-safe w.r.t concurrent calls to `read_block()` and
    /// `write_block()`.
    /// @param[in] block  A pointer to memory that will be written to the
    ///                   file. Must hold at least `size` bytes.
    /// @param[in] offset The offset in the file at which the block should be
    ///                   written.
    /// @param[in] size   The size of the block.
    virtual void write_block(const char* block, size_t offset, size_t size) = 0;

    /// Opens a file with the given mode. Existing files are never overwritten.
    /// @param[in] filename Path to the file.
    /// @param[in] mode     `Mode` that should be used to open the file.
    [[nodiscard]] static std::unique_ptr<File> open_file(const char* filename, Mode mode);

    /// Opens a temporary file in `WRITE` mode. The file will be deleted
    /// automatically after use.
    [[nodiscard]] static std::unique_ptr<File> make_temporary_file();
};

class PosixFile
 : public File {
private:
    Mode mode;
    int fd;
    size_t cached_size;
    
    [[nodiscard]] size_t read_size() const;

public:
    PosixFile(Mode mode, int fd, size_t size);
    PosixFile(const char* filename, Mode mode);
    PosixFile(const PosixFile&) = delete;
    PosixFile(PosixFile&&) = delete;
    PosixFile& operator=(const PosixFile&) = delete;
    PosixFile& operator=(PosixFile&&) = delete;

    ~PosixFile() override;

    [[nodiscard]] Mode get_mode() const override;

    [[nodiscard]] size_t size() const override;

    void resize (size_t new_size) override;

    void read_block(size_t offset, size_t, char* block) override;

    void write_block (const char* block, size_t offset, size_t size) override;
 };

}  // namespace moderndbs

#endif
