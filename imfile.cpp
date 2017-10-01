/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "imfile.h"

#include <cstdio>
#include <cstdarg>

class ImInputFileStreamStd final : public ImInputStream
{
public:
    ~ImInputFileStreamStd()
    {
        if (m_file != nullptr)
        {
            ::fclose(m_file);
        }
    }

    bool open(const char* path, const char* mode)
    {
        m_file = ::fopen(path, mode);
        if (m_file != nullptr)
        {
            ::fseek(m_file, 0, SEEK_END);
            m_size = ::ftell(m_file);
            ::fseek(m_file, 0, SEEK_SET);
        }

        return m_file != nullptr;
    }

    unsigned size() const override
    {
        return m_size;
    }

    unsigned read(void* buffer, unsigned count) override
    {
        return (unsigned)::fread(buffer, 1, count, m_file);
    }

private:
    FILE* m_file = nullptr;
    unsigned m_size = 0;
};

class ImOutputFileStreamStd final : public ImOutputStream
{
public:
    ~ImOutputFileStreamStd()
    {
        if (m_file != nullptr)
        {
            ::fclose(m_file);
        }
    }

    bool create(const char* path, const char* mode)
    {
        m_file = ::fopen(path, mode);
        return m_file != nullptr;
    }

    unsigned write(const void* buffer, unsigned count) override
    {
        return (unsigned)::fwrite(buffer, 1, count, m_file);
    }

    unsigned format(const char* fmt, ...) override
    {
        va_list argList;
        va_start(argList, fmt);
        unsigned result = ::vfprintf(m_file, fmt, argList);
        va_end(argList);

        return result;
    }

private:
    FILE* m_file = nullptr;
};

ImInputStream* ImFile::open(const char* path, const char* mode) const
{
    auto stream = new ImInputFileStreamStd();
    if (stream->open(path, mode))
    {
        return stream;
    }

    delete stream;
    return nullptr;
}

ImOutputStream* ImFile::create(const char* path, const char* mode) const
{
    auto stream = new ImOutputFileStreamStd();
    if (stream->create(path, mode))
    {
        return stream;
    }

    delete stream;
    return nullptr;
}
