#include "streams/ostream.hpp"
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <system_error>
#include <type_traits>
#include <vector>
using namespace std;

struct InputStreamError
{};

struct OutputStreamError
{};

template <typename T>
class InputStream
{
    static_assert(is_base_of_v<InputStream, T>);

private:
    bool end_of_file {false};
    size_t gcount {};

    bool not_space(char c) { return c > ' '; }

    void skip_whitespaces()
    {
        while (char c = get())
        {
            if (not_space(c))
            {
                unget();
                return;
            }
        }
    }

public:
    [[nodiscard]]
    size_t get_count() const
    {
        return gcount;
    }

    int peek()
    {
        int ch = static_cast<T*>(this)->peek();
        if (ch == EOF) { end_of_file = true; }
        return ch;
    }

    InputStream& read(span<char> s)
    {
        gcount = static_cast<T*>(this)->read(s);
        if (gcount < s.size()) { end_of_file = true; }
        return *this;
    }

    int get()
    {
        gcount = 0;

        int ch = static_cast<T*>(this)->get();
        if (ch == EOF) { end_of_file = true; }
        else { gcount = 1; }

        return ch;
    }

    InputStream& get(char& c)
    {
        gcount = 0;

        int ch = static_cast<T*>(this)->get();
        if (ch != EOF)
        {
            gcount = 1;
            c = static_cast<char>(ch);
        }
        else { end_of_file = true; }

        return *this;
    }
    void unget() { return static_cast<T*>(this)->unget(); }

    InputStream& getline(const span<char> line, char delim = '\n')
    {
        if (line.empty()) { throw; }

        for (gcount = 0; gcount + 1 < line.size(); ++gcount)
        {
            int ch = get();
            if (ch == delim) { break; }
            if (ch == EOF)
            {
                end_of_file = true;
                break;
            }
            line[gcount++] = static_cast<char>(ch);
        }

        line[gcount] = '\0';

        return *this;
    }

    string getline(char delim = '\n')
    {
        string line;

        while (true)
        {
            int ch = get();
            if (ch == delim) { break; }
            if (ch == EOF)
            {
                end_of_file = true;
                break;
            }
            line.push_back(static_cast<char>(ch));
        }

        gcount = line.size();

        return line;
    }


    InputStream& ignore(size_t ignore_size = 1, int delim = -1)
    {
        gcount = 0;
        while (gcount < ignore_size)
        {
            int ch = get();
            if (ch == EOF)
            {
                end_of_file = true;
                break;
            }
            ++gcount;
            if (ch == delim) { break; }
        }

        return *this;
    }

    InputStream& operator>>(bool& b)
    {
        skip_whitespaces();

        int ch = get();

        if (ch == EOF) { throw; }

        if (ch == '0')
        {
            b = false;
            return *this;
        }
        if (ch == '1')
        {
            b = true;
            return *this;
        }

        if (ch == 't' || ch == 'T')
        {
            if (get() == 'r' && get() == 'u' && get() == 'e')
            {
                b = true;
                return *this;
            }
        }

        if (ch == 'f' || ch == 'F')
        {
            if (get() == 'a' && get() == 'l' && get() == 's' && get() == 'e')
            {
                b = false;
                return *this;
            }
        }

        throw;
    }

    InputStream& operator>>(char& c)
    {
        skip_whitespaces();

        int ch = get();
        if (ch == EOF) { throw; }
        c = static_cast<char>(ch);

        return *this;
    }

    InputStream& operator>>(span<char> s) // fixme: 补0 ?
    {
        skip_whitespaces();

        size_t i {};

        int ch = static_cast<T*>(this)->get();
        while (i < s.size() && ch != EOF && not_space(ch))
        {
            s[i++] = static_cast<char>(ch);
            ch = static_cast<T*>(this)->get();
        }

        return *this;
    }

    InputStream& operator>>(string& s)
    {
        skip_whitespaces();

        s.clear();

        int ch = static_cast<T*>(this)->get();
        while (ch != EOF && not_space(ch))
        {
            s.push_back(static_cast<char>(ch));
            ch = static_cast<T*>(this)->get();
        }

        return *this;
    }
};

template <typename T>
class OutputStream
{
    static_assert(is_base_of_v<OutputStream, T>);

private:
    auto put() { return static_cast<T*>(this)->put(); }
    void write(span<const char> s) { return static_cast<T*>(this)->write(s); }

    int base {10};
    optional<chars_format> fmt;
    optional<int> precision;

public:
    OutputStream& set_int_base(int b) { base = b; }
    OutputStream& set_float_precision(int p) { precision = p; }

    OutputStream& fixed_float() // f
    {
        fmt = chars_format::fixed;
        return *this;
    }
    OutputStream& scientific_float() // e
    {
        fmt = chars_format::scientific;
        return *this;
    }

    OutputStream& general_float() // g
    {
        fmt = chars_format::general;
        return *this;
    }

    OutputStream& hex_float() // a
    {
        fmt = chars_format::hex;
        return *this;
    }

    OutputStream& operator<<(span<const char> s)
    {
        write(s);

        return *this;
    }


    template <typename U>
        requires(integral<U>)
    OutputStream& operator<<(U num)
    {
        array<char, 64> buffer;

        auto res = to_chars(begin(buffer), end(buffer), num, base);
        if (res.ec != errc()) { throw std::system_error(std::make_error_code(res.ec)); }

        write({buffer, res.ptr - buffer});

        return *this;
    }

    template <typename U>
        requires(floating_point<U>)
    OutputStream& operator<<(U num)
    {
        array<char, 64> buffer;
        to_chars_result res;

        if (!fmt && !precision) { res = to_chars(begin(buffer), end(buffer), num); }
        else if (!precision) { res = to_chars(begin(buffer), end(buffer), num, *fmt); }
        else { res = to_chars(begin(buffer), end(buffer), num, *fmt, *precision); }

        if (res.ec != errc()) { throw std::system_error(std::make_error_code(res.ec)); }

        write({buffer, res.ptr - data(buffer)});

        return *this;
    }

    OutputStream& operator<<(bool b)
    {
        if (b) { write("true"); }
        else { write("false"); }

        return *this;
    }
};

class ISpanStream
{
private:
    span<const char> buf;

    int base {10};
    chars_format fmt {chars_format::general};

    static bool not_space(char c) { return c > ' '; }

    void skip_whitespaces()
    {
        size_t i {};
        for (; i < buf.size(); ++i)
        {
            if (not_space(buf[i])) { break; }
        }
        buf = buf.subspan(i);
    }

public:
    explicit ISpanStream(span<const char> s) : buf {s} {}

    size_t read(span<char> out_buf)
    {
        size_t n = min(buf.size(), out_buf.size());
        copy(buf.begin(), buf.begin() + n, out_buf.begin());
        buf = buf.subspan(n);
        return n;
    }

    ISpanStream& setbase(int b)
    {
        base = b;
        return *this;
    }
    ISpanStream& general_float()
    {
        fmt = chars_format::general;
        return *this;
    }
    ISpanStream& fixed_float()
    {
        fmt = chars_format::fixed;
        return *this;
    }
    ISpanStream& scientific_float()
    {
        fmt = chars_format::scientific;
        return *this;
    }
    ISpanStream& hex_float() // 不允许前缀 "0x" 或 "0X"; 不能与其它flag一起使用
    {
        fmt = chars_format::hex;
        return *this;
    }

    template <typename T>
        requires integral<T>
    ISpanStream& operator<<(T& int_val)
    {
        skip_whitespaces();

        auto res = from_chars(data(buf), data(buf) + size(buf), int_val, base);
        if (res.ec != errc {}) { throw std::system_error(std::make_error_code(res.ec)); }
        buf = buf.subspan(res.ptr - data(buf));
        return *this;
    }

    template <typename T>
        requires floating_point<T>
    ISpanStream& operator<<(T& float_val)
    {
        skip_whitespaces();

        auto res = from_chars(data(buf), data(buf) + size(buf), float_val, fmt);
        if (res.ec != errc {}) { throw std::system_error(std::make_error_code(res.ec)); }
        buf = buf.subspan(res.ptr - data(buf));
        return *this;
    }
};

template <typename InputHandler, size_t buffer_size = 8192>
class IBUfStream
{
private:
    InputHandler handler;
    vector<byte> buffer;
    span<byte> buffer_span;
    vector<byte> putback_buffer;
    bool eof {false};

public:
    explicit IBUfStream(InputHandler handler_) : handler(handler_), buffer(buffer_size) {}

    size_t read(span<byte> s)
    {
        if (eof) { return 0; }

        size_t bytes_delivered {};

        while (s.size() > 0)
        {
            if (!putback_buffer.empty())
            {
                size_t n = min(s.size(), putback_buffer.size());
                copy_n(putback_buffer.rbegin(), n, s.begin());
                putback_buffer.resize(putback_buffer.size() - n);
                s = s.subspan(n);
                bytes_delivered += n;
                if (s.empty()) { return bytes_delivered; }
            }

            if (buffer_span.empty())
            {
                buffer_span = buffer;
                size_t n = handler.read(buffer_span);
                if (n == 0)
                {
                    eof = true;
                    return bytes_delivered;
                }
            }

            size_t n = min(s.size(), buffer_span.size());
            copy_n(buffer_span.begin(), n, s.begin());

            buffer_span = buffer_span.subspan(n);
            bytes_delivered += n;
        }

        return bytes_delivered;
    }

    void putback(byte c) { putback_buffer.push_back(c); }
    void putback(span<const byte> s) { copy(s.rbegin(), s.rend(), back_inserter(putback_buffer)); }
};

class stdio_istream
{
    FILE* fp;
    bool eof {false};

public:
    explicit stdio_istream(FILE* fp_) : fp {fp_} {}

    [[nodiscard]]
    FILE* get() const
    {
        return fp;
    }

    size_t read(span<byte> s)
    {
        if (eof) { throw; }

        size_t bytes_read = fread(s.data(), 1, s.size(), fp);

        if (bytes_read != s.size() && ferror(fp) != 0) { throw runtime_error("Error reading from file."); }

        return bytes_read;
    }
};

inline stdio_istream stdin_stream {stdin};

class stdio_file_istream
{
    unique_ptr<FILE, int (*)(FILE*)> fp;

public:
    explicit stdio_file_istream(string_view path) : fp {fopen(path.data(), "r"), fclose}
    {
        if (fp == nullptr) { throw system_error {errno, system_category()}; }
    }

    [[nodiscard]]
    FILE* get() const
    {
        return fp.get();
    }

    size_t read(span<byte> s)
    {
        size_t bytes_read = fread(s.data(), 1, s.size(), fp.get());

        if (bytes_read != s.size() && ferror(fp.get()) != 0) { throw runtime_error("Error reading from file."); }

        return bytes_read;
    }

    [[nodiscard]]
    size_t size() const
    {
        // 保存当前位置
        long current_pos = ftell(fp.get());
        if (current_pos == -1) { throw system_error {errno, system_category()}; }

        // 移动到文件末尾
        if (fseek(fp.get(), 0, SEEK_END) == -1) { throw system_error {errno, system_category()}; }

        // 获取文件大小
        long file_size = ftell(fp.get());
        if (file_size == -1) { throw system_error {errno, system_category()}; }

        // 恢复原始位置
        if (fseek(fp.get(), current_pos, SEEK_SET) == -1) { throw system_error {errno, system_category()}; }

        return static_cast<size_t>(file_size);
    }

    // read_all() 函数实现
    template <typename T>
    void read_all(T& container)
    {
        // 获取文件大小
        size_t file_size = size();

        // 调整容器大小
        using value_type = typename T::value_type;
        container.resize(file_size / sizeof(value_type));

        if (!container.empty())
        {
            // 读取所有数据
            size_t bytes_read = fread(container.data(), 1, file_size, fp.get());

            // 检查是否读取成功
            if (bytes_read != file_size)
            {
                if (ferror(fp.get()) != 0) { throw runtime_error("Error reading from file."); }
                // 如果读取的字节数不足，调整容器大小
                container.resize(bytes_read / sizeof(value_type));
            }
        }
    }

    int seekg(long offset, int whence)
    {
        int ret = fseek(fp.get(), offset, whence);
        if (ret == -1) { throw system_error {errno, system_category()}; }
        return ret;
    }

    long tellg()
    {
        int ret = ftell(fp.get());
        if (ret == -1) { throw system_error {errno, system_category()}; }
        return ret;
    }
};

class stdio_ostream
{
    FILE* fp;

public:
    explicit stdio_istream(FILE* fp_) : fp {fp_} {}
    FILE* get() { return fp; }

    template <typename T>
    void write(span<const T> s)
    {
        if (fwrite(data(s), sizeof(T), size(s), fp) != size(s)) { throw system_error {errno, system_category()}; }
    }

    void flush() { fflush(fp); }
};

inline stdio_ostream stdouts {stdout};
inline stdio_ostream stderrs {stderr};

class stdio_file_ostream
{
    unique_ptr<FILE, int (*)(FILE*)> fp;

public:
    explicit stdio_istream(string_view path) : fp {fopen(data(path), "w"), fclose}
    {
        if (fp == nullptr) { throw std::system_error(errno, std::system_category()); }
    }
    FILE* get() { return fp.get(); }

    template <typename T>
    void write(span<const T> s)
    {
        if (fwrite(data(s), sizeof(T), size(s), fp) != size(s)) { throw system_error {errno, system_category()}; }
    }

    void flush() { fflush(fp); }
};

template <typename T>
class buf_ostream
{
    T buffer;

public:
    span<const char> view() { return buffer; }
    T& get() { return buffer; }

    void write(span<const char> s) { copy(begin(s), end(s), back_inserter(buffer)); }
};

class span_ostream
{
    span<char> free;

public:
    explicit span_ostream(span<char> s) : free {s} {}
    span<char> unused() { return free; }

    void write(span<const char> str)
    {
        if (size(free) < size(str)) { throw; }

        copy(begin(str), end(str), begin(free));
        free = free.subspan(size(str));
    }
};

template <typename T>
class buffered_ostream : public ostream
{
    vector<char> buffer;
    shared_ptr<void> flush_guard {nullptr, [](void* p) { flush(); }};

public:
    explicit buffered_ostream(size_t size = 8192) : { buffer.reserve(size); }

    void write(span<const char> bytes)
    {
        if (size(bytes) > buffer.capacity()) { static_cast<T*>(this)->write(bytes); }

        size_t available = buffer.capacity() - size(buffer);
        if (size(bytes) > available)
        {
            auto first = bytes.first(available);
            copy(begin(first), end(first), back_inserter(buffer));

            static_cast<T*>(this)->write(buffer);

            bytes = bytes.subspan(available);
        }

        copy(begin(bytes), end(bytes), back_inserter(buffer));
    }

    void flush() { static_cast<T*>(this)->write({data(buffer), size(buffer)}); }
};


// close 不会强制将未写入的数据刷新到磁盘
// 调用 close 只会将这些数据放入内核的缓冲区
class fd_ostream
{
    int fd {-1};
    shared_ptr<void> close_guard {nullptr, [](void* p) { close(fd) }};

public:
    fd_ostream(string_view path)
    {
        fd = open(data(path));
        if (fd == -1) { throw system_error {errno, system_category()}; }
    }

    int get() { return fd; }

    void write(span<const char> bytes)
    {
        while (size(bytes) > 0)
        {
            auto bytes_written = ::write(fd, data(bytes), size(bytes));
            if (bytes_written == -1) { throw system_error {errno, system_category()}; }
            bytes = bytes.subspan(bytes_written);
        }
    }

    void flush()
    {
        if (fsync(fd) == -1) { throw std::system_error {errno, std::system_category()}; }
    }
};
