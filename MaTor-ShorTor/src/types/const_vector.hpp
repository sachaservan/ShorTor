//
// Created by Marcin Slowik on 08.09.15.
//

#ifndef CONST_VECTOR_HPP
#define CONST_VECTOR_HPP

/** @file */

#include <memory>

/**
 * Wrapper for dynamically allocated arrays, automatically invokes constructors and destructors.
 * @param T type of held objects.
 * @param Alloc allocator type used for allocating memory for objects, std::allocator<T> by default.
 */
template<typename T, typename Alloc=std::allocator<T>>
class const_vector : protected Alloc
{
public:
    // First template argument
    typedef T value_type;
    // Second template argument
    typedef Alloc allocator_type;
    // Reference type, equivalent to T&
    typedef value_type &reference;
    // Const reference type, equivalent to const T&
    typedef const value_type &const_reference;
    // Pointer type of the corresponding <allocator_type>.
    typedef typename std::allocator_traits<allocator_type>::pointer pointer;
    // Const pointer type of the corresponding <allocator_type>.
    typedef typename std::allocator_traits<allocator_type>::const_pointer const_pointer;

    // A random access iterator (pointer)
    typedef pointer iterator;
    // A const random access iterator (const_pointer)
    typedef const_pointer const_iterator;
    // A signed integral type representing the differences between the iterators.
    typedef typename std::iterator_traits<iterator>::difference_type difference_type;
    // An unsigned integral type representing the size of the container.
    typedef size_t size_type;


protected:
    // Pointer to the beginning of the array
    pointer p_begin;
    /* Pointer to the place in memory after the last in the array
     *
     * Equivalent to p_begin + [array size]
     */
    pointer p_end;

    /* Hidden constructor, allocates space in memory, but does not invoke constructors. For use by <make_indexed>.
     * @size Number of elements to allocate.
     * @null nullptr to distinguish the constructors.
     */
    const_vector(size_t size, std::nullptr_t null) : Alloc(), p_begin(this->allocate(size)),
                                                     p_end(p_begin + size) { }

public:
    /* Default constructor, allocates <size> objects and invokes their default constructors.
     * @size Number of elements to allocate and initialize.
     * @alloc Allocator object to use.
     */
    const_vector(size_t size, Alloc alloc = Alloc()) :
            Alloc(alloc),
            p_begin(alloc.allocate(size)),
            p_end(p_begin + size)
    {
        while (size--)
            alloc.construct(&p_begin[size]);
    }

    /* Forward-constructor, allocates <size> objects and invokes their constructors using the parameters in <args>.
     * @Args Types of parameters used in the derived constructors.
     * @size Number of elements to allocate and initialize.
     * @args Arguments to pass to each and every element's constructor.
     */
    template<typename... Args>
    const_vector(size_t size, const Args&... args) :
            Alloc(),
            p_begin(this->allocate(size)),
            p_end(p_begin + size)
    {
        while (size--)
            this->construct(&p_begin[size], args...);
    }

    // Copy constructor, deleted.
    const_vector(const const_vector<T, Alloc> &other) = delete;

    // Move constructor, moves the pointers
    const_vector(const_vector<T, Alloc> &&other) : Alloc(other),
                                                   p_begin(other.p_begin),
                                                   p_end(other.p_end)
    {
        // mark the boundaries as moved/inaccessible
        other.p_begin = nullptr;
        other.p_end = nullptr;
    }

    // Class destructor, invokes appropriate destructor for each element.
    ~const_vector(void)
    {
        if(p_begin == nullptr || p_end == nullptr) return; // the content has been already moved.
        for (reference p : (*this))
            this->destroy(this->address(p));
        this->deallocate(p_begin, p_end - p_begin);
        return;
    }

    /* Return the iterator to the first element of the underlying array.
     *
     * @return Pointer to the first element of the vector.
     */
    iterator begin() { return p_begin; }

    /* Return the iterator to the first element of the underlying array.
     *
     * @return Pointer to the first element of the vector.
     */
    const_iterator begin() const { return p_begin; }

    /* Return the iterator to the element after the last of the underlying array.
     *
     * @return Pointer to the end element of the vector.
     */
    iterator end() { return p_end; }

    /* Return the iterator to the element after the last of the underlying array.
     *
     * @return Pointer to the end element of the vector.
     */
    const_iterator end() const { return p_end; }

    /* Return the iterator to the first element of the underlying array.
     *
     * @return Pointer to the first element of the vector.
     */
    const_iterator cbegin() const { return p_begin; }

    /* Return the iterator to the element after the last of the underlying array.
     *
     * @return Pointer to the end element of the vector.
     */
    const_iterator cend() const { return p_end; }

    /* Count the number of elements in the vector.
     *
     * @return Number of elements in the vector.
     */
    size_type size() const { return p_end - p_begin; }

    /* Count the number of elements in the vector. Since the size cannot be changed, max_size = size.
     *
     * @return Number of elements in the vector.
     */
    size_type max_size() { return p_end - p_begin; }

    /* Count the number of elements in the vector. Since the size cannot be changed, capacity = size.
     *
     * @return Number of elements in the vector.
     */
    size_type capacity() { return p_end - p_begin; }

    /* Check, whether the vector has been created empty.
     *
     * @return True, if <size> == 0, false otherwise.
     */
    bool empty() { return !(p_end - p_begin); }


    /* Indexed access operator.
     * @index Index of the element in the underlying array.
     *
     * @return Reference to the corresponding object.
     */
    reference operator[](size_t index) { return p_begin[index]; }

    /* Indexed access operator.
     * @index Index of the element in the underlying array.
     *
     * @return Reference to the corresponding object.
     */
    const_reference operator[](size_t index) const { return p_begin[index]; }

    /* Indexed access method.
     * @index Index of the element in the underlying array.
     *
     * @return Reference to the corresponding object.
     */
    reference at(size_t index) { return p_begin[index]; }

    /* Indexed access method.
     * @index Index of the element in the underlying array.
     *
     * @return Reference to the corresponding object.
     */
    const_reference at(size_t index) const { return p_begin[index]; }

    /* Access the first element of the vector.
     *
     * Will cause undefined behaviour, if the vector is empty.
     *
     * @return Reference to the first element of the vector.
     */
    reference front(size_t index) { return p_begin[0]; }

    /* Access the first element of the vector.
     *
     * Will cause undefined behaviour, if the vector is empty.
     *
     * @return Reference to the first element of the vector.
     */
    const_reference front(size_t index) const { return p_begin[0]; }

    /* Access the last element of the vector.
     *
     * Will cause undefined behaviour, if the vector is empty.
     *
     * @return Reference to the last element of the vector.
     */
    reference back(size_t index) { return p_end[-1]; }

    /* Access the last element of the vector.
     *
     * Will cause undefined behaviour, if the vector is empty.
     *
     * @return Reference to the last element of the vector.
     */
    const_reference back(size_t index) const { return p_end[-1]; }

    /* Access the raw dynamic array pointer.
     *
     * @return Pointer to the underlying data structure. Equivalent to <begin>, since iterator == pointer.
     */
    pointer data() { return p_begin; }

    /* Access the raw dynamic array pointer.
     *
     * @return Pointer to the underlying data structure. Equivalent to <begin>, since iterator == pointer.
     */
    const_pointer data() const { return p_begin; }

    /* Retrieve the underlying allocator type.
     *
     * May cause undefined behaviour, if the allocator_type has not been properly designed.
     *
     * @return An instance of the underlying allocator object.
     */
    allocator_type get_allocator() const { return static_cast<allocator_type>(*this); }

    /* Create a new <const_vector>, passing arguments to the constructor, along with items indices.
     * @Args Types of parameters used in the derived constructors.
     * @size Number of elements to allocate and initialize.
     * @args Arguments to pass to each and every element's constructor.
     *
     * Invoke each element's constructor in the following way: T(index, <args>), where index is the item's index
     * in the array.
     * The objects are currently created in order from last to first, but this behaviour is not guaranteed in future.
     *
     * @return New object of type const_vector, ready to move-construct. Note, that copy constructor is unavailable.
     */
    template<typename... Args>
    static const_vector<T, Alloc> make_indexed(size_t size, Args&... args)
    {
        const_vector<T, Alloc> result(size, nullptr);
        while (size--)
            result.construct(&(result.p_begin[size]), size, args...);
        return result;
    };


};

#endif // CONST_VECTOR_HPP
