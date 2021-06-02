#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <iterator>

#include <iostream>

template <typename T>
class RawMemory {
public:
    /////_CONSTRUCTORS_/////////////////////////////////////
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        :buffer_(Allocate(capacity)),
        capacity_(capacity) {
    }

    RawMemory(const RawMemory& other) = delete;
    RawMemory(RawMemory&& other) noexcept {
        Swap(other);
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }
    /////_СOPY & ASSIGN FOR EQUAL_/////////////////////////////////////
    
    RawMemory& operator=(const RawMemory& rhs) = delete;    
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (buffer_ != nullptr) {
            Deallocate(buffer_);
        }
        buffer_ = nullptr;
        Swap(rhs);
        return *this;
    }

    /////_OVERLOADS_/////////////////////////////////////
    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_); // it's allowed to get the address next the last element
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    /////_ADDITIONAL METHODS_/////////////////////////////////////
    void Swap(RawMemory& other) {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }
    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    T* buffer_ = nullptr;
    size_t capacity_ = 0;

    static T* Allocate(size_t n) { // Reserve space in memory
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept { // Free space in memory
        operator delete(buf);
    }
};



template <typename T>
class Vector {
public:
    /////_CONSTRUCTORS_/////////////////////////////////////
    Vector() = default;

    Vector(size_t n)
        :data_(n)
        , size_(n)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), n);
    }

    Vector(const Vector& other)
        :data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
    {
        Swap(other);
    }

    ~Vector()
    {
        std::destroy_n(data_.GetAddress(), size_);
    }

    /////_ITERATORS_/////////////////////////////////////
    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }
    const_iterator cend() const noexcept {
        return end();
    }

    iterator operator+(size_t n) noexcept {
        return data_.GetAddress() + size_;
    }

    /////_OVERLOADS_/////////////////////////////////////
    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) { // another vector is BIGGER
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else { // another vector is LESS
                std::copy_n(rhs.begin(), std::min(size_, rhs.size_), begin());
                if (size_ > rhs.size_) {  
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_); // free unused elements
                }
                else { 
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_); // copy elements with memory initialization in range [size_ + 1, rhs.size_)
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }
    Vector& operator=(Vector&& rhs) noexcept {        
        Swap(rhs);        
        return *this;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    /////_METHODS FOR SIZE/CAPACITY_/////////////////////////////////////
    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    size_t Size() const noexcept {
        return size_;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) { // no need to reassign heap
            return;
        }
        RawMemory<T> new_data(new_capacity); // new heap creation
        TransferDataToNewHeap(data_.GetAddress(), size_, new_data.GetAddress());
        std::destroy_n(data_.GetAddress(), size_); // destroy old values in heap
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (size_ < new_size) {
            if (new_size > data_.Capacity()) { // generate new heap if current heap is small
                Reserve(new_size);
            }
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_); // initialize new elements by default
        }
        else { // remove excess elements
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }
        size_ = new_size;
    }


    /////_METHODS FOR ELEMENTS MODIFICATION_/////////////////////////////////////
    //////////_LAST ELEMENT_//////////////////////////////////////////////////////
    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
            TransferDataToNewHeap(data_.GetAddress(), size_, new_data.GetAddress());
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else {
            new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
        }
        return data_[size_++];
    }

    void PushBack(const T& value) {
        EmplaceBack(value);
    }
    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    void PopBack() noexcept {
        assert(size_ > 0);        
        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;       
    }

    //////////_ANY POS OF ELEMENT_//////////////////////////////////////////////////////
    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        size_t pos_count = std::distance(this->cbegin(), pos);
        if (this->cend() == pos) { // if pos point to hte end of vector
            EmplaceBack(std::forward<Args>(args)...);
        }
        else if (size_ == data_.Capacity()) { // need new heap            
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data.GetAddress() + pos_count) T(std::forward<Args>(args)...); // create new value in heap
            TransferDataToNewHeap(data_.GetAddress(), pos_count, new_data.GetAddress());
            size_t dist_to_end = std::distance(pos, cend()); // qty of leemnts after desired pos
            TransferDataToNewHeap(data_.GetAddress() + pos_count, dist_to_end, new_data.GetAddress() + pos_count + 1);
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            ++size_;
        }
        else {
            T tmp(std::forward<Args>(args)...);
            std::uninitialized_move_n(end() - 1, 1, end()); // move last element right on uninitilized address 
            std::move_backward(begin() + pos_count, end() - 1, end()); // shift elements after pos right on one step
            *(data_.GetAddress() + pos_count) = std::move(tmp); // move tmp value to pos vector element
            ++size_;
        }
        return begin() + pos_count;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }
    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>) {
        if (cend() == pos) {
            PopBack(); // remove last element
            return end();
        }
        else {
            size_t pos_count = std::distance(cbegin(), pos);
            std::move(begin() + pos_count + 1, end(), begin() + pos_count); // move all elemnets right to pos to -1 index
            PopBack(); // remove empty element
            return begin() + pos_count;
        }
    }

    /////_OTHER_//////////////////////////////////////////////////////////////////
    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    void TransferDataToNewHeap(iterator src, size_t size, iterator dst) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) { // move or copy old values to new heap
            std::uninitialized_move_n(src, size, dst);
        }
        else {
            std::uninitialized_copy_n(src, size, dst);
        }
    }
};
