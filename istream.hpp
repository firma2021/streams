#pragma once

#include <algorithm>
#include <optional>


class buf_istream : public istream
{
public:
    explicit buf_istream(istream& source, std::ptrdiff_t buffer_size = 1024) : _source(source), _buffer(buffer_size) {}

private:
    gsl::span<gsl::byte> _read(gsl::span<gsl::byte> s) override
    {
        auto original_span = s;
        if (_eof) { return s.first(0); }
        std::ptrdiff_t bytes_delivered = 0;
        //While the caller still wants bytes...
        while (s.size() > 0)
        {
            //If the buffer is empty, fill it.
            if (_available.size() <= 0)
            {
                _available = _buffer;
                _available = _source.read(_available);
                if (_available.size() < _buffer.size())
                {
                    //If we didn't fill the buffer..
                    _eof = true;
                }
            }
            //Copy from buffer to caller.
            auto to_copy = std::min(s.size(), _available.size());
            std::copy_n(_available.begin(), to_copy, s.begin());
            _available = _available.subspan(to_copy);
            s = s.subspan(to_copy);
            bytes_delivered += to_copy;
            if (_eof) { break; }
        }
        return original_span.first(bytes_delivered);
    }
};

class span_istream : public istream
{
public:
    explicit span_istream(gsl::span<const gsl::byte> s) : _available(s) {}

private:
    gsl::span<gsl::byte> _read(gsl::span<gsl::byte> s) override
    {
        auto nbytes = std::min(s.size(), _available.size());
        std::copy_n(_available.begin(), nbytes, s.begin());
        _available = _available.subspan(nbytes);
        return s.first(nbytes);
    }

    gsl::span<const gsl::byte> _available;
};

//unget_istream
//Enable arbitrary amounts of unget for any istream.
//The data you unget doesn't even have to be the same as what you read.
//You don't even have to have read data previously.
class unget_istream : public istream
{
public:
    explicit unget_istream(istream& source) : _source(source) {}

    void unget(gsl::span<const gsl::byte> s) { std::copy(s.crbegin(), s.crend(), std::back_inserter(_buffer)); }

private:
    gsl::span<gsl::byte> _read(gsl::span<gsl::byte> s) override
    {
        auto original_span = s;
        std::ptrdiff_t bytes_given = 0;
        if (!_buffer.empty())
        {
            std::ptrdiff_t buf_size = _buffer.size();
            auto to_copy = std::min(s.size(), buf_size);
            std::copy_n(_buffer.crbegin(), to_copy, s.begin());
            _buffer.resize(buf_size - to_copy);
            s = s.subspan(to_copy);
            bytes_given = to_copy;
        }
        if (s.size() > 0)
        {
            auto span_read = _source.read(s);
            bytes_given += span_read.size();
        }
        return original_span.first(bytes_given);
    }

    istream& _source;
    std::vector<gsl::byte> _buffer;
};


} // namespace streams
