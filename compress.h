//
//
//
//
//
#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <stdio.h>
#include "lz4.h"

static constexpr int COMPRESS_STREAM_ELE_SIZE = 204800;

template <typename element_type>
class Buffer
{
    char *buff;
    int size;

    char *compressed_buff;
    int compressed_size;
    int compressed_capacity;

public:
    Buffer()
    {
        size = sizeof(element_type) * COMPRESS_STREAM_ELE_SIZE;
        buff = (char *)malloc(size);
        compressed_capacity = LZ4_COMPRESSBOUND(size);
        compressed_buff = (char *)malloc(compressed_capacity);
    }
    ~Buffer()
    {
        free(buff);
        free(compressed_buff);
    }

    bool save(std::ofstream &file)
    {
        file.write((char *)&compressed_size, sizeof(compressed_size));
        file.write(compressed_buff, compressed_size);
        return file.good();
    }

    bool restore(std::ifstream &file)
    {
        file.read((char *)&compressed_size, sizeof(compressed_size));
        file.read(compressed_buff, compressed_size);
        return file.good();
    }

    bool compress(LZ4_stream_t *stream, size_t element_size)
    {
        compressed_size = LZ4_compress_fast_continue(stream, buff, compressed_buff, element_size * sizeof(element_type), compressed_capacity, 1);
        if (compressed_size <= 0)
        {
            std::cerr << "Error: LZ4_compress_fast_continue return " << compressed_size << "\n";
            return false;
        }
        return true;
    }

    bool decompress(LZ4_streamDecode_t *stream, size_t element_size)
    {

        int bytes = LZ4_decompress_safe_continue(stream, compressed_buff, buff, compressed_size, size);
        if (bytes <= 0 || bytes != element_size * sizeof(element_type))
        {
            std::cerr << "Error: LZ4_compress_fast_continue return " << compressed_size << "\n";
            return false;
        }
        return true;
    }

    element_type &operator[](size_t n)
    {
        return ((element_type *)buff)[n];
    }
};

template <typename element_type>
class Compress
{
    Buffer<element_type> buff[2];
    uint8_t current_buff;

    LZ4_stream_t *stream;
    LZ4_streamDecode_t *decomp_stream;

public:
    Compress(bool is_compress = true)
    {
        current_buff = 0;
        if (is_compress)
        {
            stream = LZ4_createStream();
            decomp_stream = nullptr;
        }
        else
        {
            stream = nullptr;
            decomp_stream = LZ4_createStreamDecode();
        }
    }

    bool save(std::ofstream &file)
    {
        return buff[current_buff].save(file);
    }

    bool restore(std::ifstream &file)
    {
        return buff[current_buff].restore(file);
    }

    bool compress(std::ofstream &file, size_t element_size)
    {
        return buff[current_buff].compress(file, stream, element_size);
    }

    bool decompress(std::ifstream &file, size_t element_size)
    {
        return buff[current_buff].decompress(file, decomp_stream, element_size);
    }

    void switch_buff()
    {
        current_buff = (current_buff + 1) % 2;
    }

    element_type &operator[](const size_t &n)
    {
        return buff[current_buff][n];
    }
};
