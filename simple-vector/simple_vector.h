#pragma once

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include "array_ptr.h"
#include <iostream>
#include <algorithm>
#include <iterator>
#include <utility>

class ReserveProxyObj {
    public:
        ReserveProxyObj(size_t capacity) : capacity_(capacity)
        {
        }
        
        size_t GetCapacity(){
            return capacity_;
        }
        
    private:
        size_t capacity_;
    };
    

    ReserveProxyObj Reserve(size_t capacity_to_reserve) {
        ReserveProxyObj object(capacity_to_reserve);
        return object;
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : storage_(size), size_(size), capacity_(size) {
        if (size != 0){
            std::fill(begin(), end(), Type{});
        }
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) : storage_(size), size_(size), capacity_(size) {
        if (size != 0){
            std::fill(begin(), end(), value);
        }
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) : size_(init.size()), capacity_(init.size()) {
        if (init.size() != 0){
            ArrayPtr<Type> tmp(init.size());
            std::copy(init.begin(), init.end(), tmp.Get());
            this->storage_.swap(tmp);
        }
    }
    
    SimpleVector(const SimpleVector& other) {
         SimpleVector<Type> tmp;
         for (auto it = other.begin(); it != other.end(); ++it){
             tmp.PushBack(*it);
         }
         swap(tmp);
    }
    
    SimpleVector(SimpleVector&& other) {
        swap(other);
    }
    
    SimpleVector (ReserveProxyObj object) : SimpleVector() {
        Reserve(object.GetCapacity());
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs){            
            SimpleVector<Type> tmp(rhs);        
            swap(tmp);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs){                
            swap(rhs);
        }
        return *this;
    }
    
    void Reserve(size_t new_capacity){
        if (new_capacity > capacity_){
            ArrayPtr<Type> tmp(new_capacity);
            std::move(begin(), end(), tmp.Get());
            storage_.swap(tmp);
            capacity_ = new_capacity;
        }
    } 
    
    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        Insert(end(), item);
    }
    
     void PushBack(Type&& item) {
        Insert(end(), std::move(item));
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
   Iterator Insert(ConstIterator pos, const Type& value) {
       auto it = PrepareArrayForInsertion(pos);
       *it = value;
       return it;
    }

   Iterator Insert(ConstIterator pos, Type&& value) {
       auto it = PrepareArrayForInsertion(pos);
       *it = std::move(value);
       return it;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(size_ > 0);
            --size_;
    }

    //Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
       std::move(const_cast<SimpleVector<Type>::Iterator>(pos + 1), end(), const_cast<SimpleVector<Type>::Iterator>(pos));
       --size_;
       return const_cast<SimpleVector<Type>::Iterator>(pos);
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        storage_.swap(other.storage_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }
    

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return storage_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return storage_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_){
            throw std::out_of_range("Invalid index");
        }
        else {
            return storage_[index];
        }
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_){
            throw std::out_of_range("Invalid index");
        }
        else {
            return storage_[index];
        }
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        size_t previous_size = size_;
        if (new_size <= size_){
            size_ = new_size;
        }
        else if (new_size <= capacity_ && new_size > size_) {
            std::generate(begin() + previous_size, begin() + new_size, [](){ return Type{}; });
            size_ = new_size;
        }
        else {
            Reserve(std::max(new_size, capacity_ * 2));
            std::generate(begin() + previous_size, begin() + new_size, [](){ return Type{}; });
            size_ = new_size;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return &storage_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return &storage_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return &storage_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return &storage_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return &storage_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return &storage_[size_];
    }
    
    private:

    Iterator PrepareArrayForInsertion(ConstIterator pos){
       if (size_ < capacity_){
           auto it = std::move_backward(const_cast<SimpleVector<Type>::Iterator>(pos), end(), end() + 1);
           ++size_;
           return std::prev(it);
       }
       else {
          size_t new_capacity = capacity_ * 2;
          ArrayPtr<Type> tmp(std::max(new_capacity, static_cast<size_t>(1)));
          auto it = std::move(begin(), const_cast<SimpleVector<Type>::Iterator>(pos), tmp.Get());
          std::move(const_cast<SimpleVector<Type>::Iterator>(pos), end(), it + 1);
          storage_.swap(tmp);
          ++size_;
          capacity_ = std::max(new_capacity, static_cast<size_t>(1));
          return it;
       }
    }

    
    ArrayPtr<Type> storage_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs > rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs <= lhs;
}
