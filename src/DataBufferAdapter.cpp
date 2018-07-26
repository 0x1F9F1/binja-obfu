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

#include "DataBufferAdapter.h"

InputDataBufferAdapater::InputDataBufferAdapater(DataBuffer& buffer)
    : buffer_(std::addressof(buffer))
    , offset_(0)
    , error_(bitsery::ReaderError::NoError)
{ }

void InputDataBufferAdapater::read(uint8_t* data, size_t size)
{
    if ((offset_ + size) <= buffer_->GetLength())
    {
        std::memcpy(data, buffer_->GetDataAt(offset_), size);
    }
    else
    {
        if (error() == bitsery::ReaderError::NoError)
            setError(bitsery::ReaderError::DataOverflow);

        std::memset(data, 0, size);
    }

    offset_ += size;
}

void InputDataBufferAdapater::setError(bitsery::ReaderError error)
{
    error_ = error;
}

bitsery::ReaderError InputDataBufferAdapater::error() const
{
    return error_;
}

bool InputDataBufferAdapater::isCompletedSuccessfully() const
{
    return (error_ == bitsery::ReaderError::NoError) && (offset_ == buffer_->GetLength());
}

OutputDataBufferAdapater::OutputDataBufferAdapater(DataBuffer& buffer)
    : buffer_(std::addressof(buffer))
    , local_size_(0)
{ }

void OutputDataBufferAdapater::write(const uint8_t *data, const size_t size)
{
    if ((size + local_size_) > local_buffer_.size())
    {
        flush();
    }

    if (size < local_buffer_.size())
    {
        std::memcpy(&local_buffer_[local_size_], data, size);

        local_size_ += size;
    }
    else
    {
        buffer_->Append(data, size);
    }
}

void OutputDataBufferAdapater::flush()
{
    if (local_size_)
    {
        buffer_->Append(local_buffer_.data(), local_size_);

        local_size_ = 0;
    }
}

size_t OutputDataBufferAdapater::writtenBytesCount() const
{
    return buffer_->GetLength();
}
