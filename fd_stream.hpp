#include "stream.hpp"
#include <unistd.h>

class fd_istream
{
    int fd;
    shared_ptr<void> close_guard {nullptr, [](void* p) { close(fd); }};

    size_t get_count {};
    bool eof {false};

public:
    explicit fd_istream(string_view path) : fd {open(path.c_str())}
    {
        if (fd == -1) { throw std::system_error {errno, std::system_category()}; }
    }

    int get() { return fd; }
    size_t gcount() const { return get_count; }

    void read(span<char> bytes)
    {
        while (size(bytes) > 0)
        {
            auto ret = ::read(fd, data(bytes), size(bytes));
            if (ret == -1) { throw std::system_error {errno, std::system_category()}; }
            if (ret == 0)
            {
                eof = true;
                break;
            }
            get_count += ret;
            bytes = bytes.subspan(get_count);
        }
    }
};
