// Copyright (C) 2018 Brick
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "BinaryNinja.h"

#include <bitsery/details/adapter_utils.h>

#include <array>

class InputDataBufferAdapater
{
protected:
    DataBuffer * buffer_;
    size_t offset_;
    bitsery::ReaderError error_;

public:
    using TValue = uint8_t;
    using TIterator = TValue*;

    InputDataBufferAdapater(DataBuffer& buffer);

    void read(uint8_t* data, size_t size);
    void setError(bitsery::ReaderError error);

    bitsery::ReaderError error() const;
    bool isCompletedSuccessfully() const;
};

class OutputDataBufferAdapater
{
protected:
    DataBuffer * buffer_;
    size_t current_size_;

    size_t calculate_growth(const size_t new_size) const;

public:
    using TValue = uint8_t;
    using TIterator = TValue*;

    OutputDataBufferAdapater(DataBuffer& buffer);

    void write(const uint8_t *data, const size_t size);
    void flush();

    size_t writtenBytesCount() const;
};
