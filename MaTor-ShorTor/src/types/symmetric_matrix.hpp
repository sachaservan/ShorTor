#ifndef SYMMETRIC_MATRIX_HPP
#define SYMMETRIC_MATRIX_HPP

/** @file */

#include <vector>
#include <exception>
#include <cstddef>

#include "general_exception.hpp"

// matrix access without range checks
// uncomment to improve access performance
// #define FAST_SYMMETRIC_MATRIX

/**
 * Exceptions thrown by symmetric matrix.
 */
class symmetric_matrix_exception : public general_exception
{
	public:
		/**
		 * Exception reasons.
		 */
		enum reason
		{
			DIAGONAL_NOT_ALLOWED, /**< Diagonal access when it was disallowed. */
			OUT_OF_BOUNDS /**< Accessed row or column out of matrix bounds. */			
		};
		
		// constructors
		/**
		 * Constructs exception instance.
		 * @param why reason of throwing exception
		 * @param row row number.
		 * @param col column number.
		 * @param file name of the file file in which exception has occured.
		 * @param line line at which exception has occured.
		 */
		symmetric_matrix_exception(reason why, int row, int col, const char* file, int line) : general_exception("", file, line)
		{
			reasonWhy = why;
			switch(why)
			{
				case DIAGONAL_NOT_ALLOWED:
					this->message = "accessing [" + std::to_string(row) + "][" + std::to_string(col) + "] not allowed.";
					break;
				case OUT_OF_BOUNDS:
					this->message = "cell [" + std::to_string(row) + "][" + std::to_string(col) + "] out of bounds.";
					break;
				default:
					this->message = "unknown reason for [" + std::to_string(row) + "][" + std::to_string(col) + "]";
			}
			commit_message();
		}

		/**
		 * @copydoc general_exception::~general_exception()
		 */
		virtual ~symmetric_matrix_exception() throw () { }
	
		virtual int why() const { return reasonWhy; }
	
	protected:
		reason reasonWhy; /**< Exception reason. */
		
		virtual const std::string& getTag() const 
		{
			const static std::string tag = "symmetric_matrix_exception";
			return tag;
		}
};

/**
 * std-like iterator going through the pairs in the array.
 * Goes in order: (1, 0); (2, 0); (2, 1); (3, 0); (3, 1); (3, 2) ...
 * Ignores undefined entries.
 */
template<typename POINTER_TYPE, typename TYPE>
class SymmetricMatrixIterator
{
	private:
		typedef SymmetricMatrixIterator<POINTER_TYPE, TYPE> thistype; /**< Typename of this class. */
		template<typename T>
		friend class SymmetricMatrix;
		
		typedef TYPE& REF_TYPE;
		
	protected:
		size_t i /** Current iterator row.*/, j; /**< Current iterator column.*/
		POINTER_TYPE parent; /**< Accessed instance pointer. */
	
		/**
		 * Creates iterator at specified position.
		 * @param i row number
		 * @param j column number
		 * @param parent accessed instance pointer
		 */
		SymmetricMatrixIterator(size_t i, size_t j, POINTER_TYPE parent) : i(i), j(j), parent(parent) { }
		
	public:
		/**
		 * Copy constructor.
		 * @param t source iterator
		 */
		SymmetricMatrixIterator(const thistype& t) : i(t.i), j(t.j), parent(t.parent) { }
		
		/**
		 * Post-increment operator. Ignores undefined cells.
		 * @return post-incremented iterator.
		 */
		thistype& operator++()
		{
			++j;
			if(parent->allowDiagonal){
				if(i < j)
				{
					j = 0; ++i;
				}
			}else{
				if(i <= j)
				{
					j = 0; ++i;
				}
			}
			if(i == parent->matrixSize) return *this;
			if(parent->defined(i, j)) return *this;
			return ++(*this);
		}
		/**
		 * Pre-increment operator. Ignores undefined cells.
		 * @return pre-incremented iterator.
		 */
		thistype operator++(int) {thistype tmp(*this); operator++(); return tmp;}
		// suppressing doxygen "uncommented" warnings
		//! {
		bool operator==(const thistype& rhs) {return i==rhs.i && j==rhs.j;}
		//! } //! {
		bool operator!=(const thistype& rhs) {return !(*this == rhs);}
		//! }

		REF_TYPE operator*() {return parent->matrix[i][j].value;}
};

/**
 * Small struct providing easy access to elements of the array with
 * double [] operator.
 */
template<typename POINTER_TYPE, typename TYPE>
struct SymmetricMatrixAccessor
{
	POINTER_TYPE parent; /**< The instance accessor is accessing. */
	int i; /**< Accessor row in instance. */
	
	template<typename T>
	friend class SymmetricMatrix;
	
	struct SymmetricMatrixRowIterator : public std::iterator<std::random_access_iterator_tag, TYPE>
	{		
		POINTER_TYPE parent;
		size_t j, i;
		SymmetricMatrixRowIterator(POINTER_TYPE parent, size_t i, size_t j) : parent(parent), i(i), j(j) { }
				
		SymmetricMatrixRowIterator &operator++()
        {
            ++j;
            return *this;
        }

        SymmetricMatrixRowIterator operator++(int)
        {
            SymmetricMatrixRowIterator tmp(*this);
            operator++();
            return tmp;
        }

        SymmetricMatrixRowIterator &operator--()
        {
            --j;
            return *this;
        }

        SymmetricMatrixRowIterator operator--(int)
        {
            SymmetricMatrixRowIterator tmp(*this);
            operator--();
            return tmp;
        }

        template <typename integer, typename std::enable_if<std::is_integral<integer>::value>::type>
        SymmetricMatrixRowIterator operator+=(integer n)
        {
            j+=n;
            return *this;
        }

        template <typename integer, typename std::enable_if<std::is_integral<integer>::value>::type>
        SymmetricMatrixRowIterator operator-=(integer n)
        {
            j-=n;
            return *this;
        }

        template <typename integer, typename std::enable_if<std::is_integral<integer>::value>::type>
        SymmetricMatrixRowIterator operator+(integer n)
        {
            SymmetricMatrixRowIterator it(*this);
            return it += n;
        }

        template <typename integer, typename std::enable_if<std::is_integral<integer>::value>::type>
        SymmetricMatrixRowIterator operator-(integer n)
        {
            SymmetricMatrixRowIterator it(*this);
            return it -= n;
        }

        ptrdiff_t operator-(SymmetricMatrixRowIterator b)
        {
            return j - b.j;
        }

        bool operator==(const SymmetricMatrixRowIterator &rhs) const { return j == rhs.j; }
        bool operator!=(const SymmetricMatrixRowIterator &rhs) const { return j != rhs.j; }
		bool operator<(const SymmetricMatrixRowIterator &rhs) const
		{
			return j < rhs.j;
		}
        bool operator>(const SymmetricMatrixRowIterator &rhs) const
		{
			return j > rhs.j;
		}
        bool operator<=(const SymmetricMatrixRowIterator &rhs) const { return !(*this > rhs); }
        bool operator>=(const SymmetricMatrixRowIterator &rhs) const { return !(*this < rhs); }

        TYPE& operator*() { return parent->get(i,j); }
        const TYPE& operator*() const { return parent->get(i,j); }

        TYPE& operator[] (ptrdiff_t n)
        {
            return (*this) + n;
        }
	};
	
	/**
	 * Creates accessor for the instance for specified row.
	 * @param i row number
	 * @param parent accessed instance.
	 */
	SymmetricMatrixAccessor(int i, POINTER_TYPE parent) : i(i), parent(parent) { }
	
	/**
	 * Accesses chosen cell in a row.
	 * @param j column number
	 * @return reference to the cell.
	 */
	TYPE& operator [] (int j) { return parent->get(i, j); }
	
	SymmetricMatrixRowIterator begin() const
	{
		return SymmetricMatrixRowIterator(parent, i, 0);
	}
	
	SymmetricMatrixRowIterator end() const
	{
		return SymmetricMatrixRowIterator(parent, i, parent->size());
	}
};

/**
 * This class provides a very easy interface for arrays N x N, where [i][j] == [j][i].
 * May be used for creating symmetric matrices or inderected graphs (not all elements have to be defined).
 */
template<typename T>
class SymmetricMatrix
{
	private:
		template<typename POINTER_TYPE, typename TYPE>
		friend class SymmetricMatrixIterator;
		typedef SymmetricMatrix<T> thistype; /**< Typename of this class. */
	
	protected:		
		/**
		 * This structure describes particular "cell" in a matrix.
		 */
		struct cell
		{
			public:
				bool defined; /**< Is cell value defined (for structures such as graphs)? */
				T value; /**< Value stored in a cell. */
				/**
				 * Constructs undefined cell.
				 */
				cell() : defined(false), value(0) { }
		};
		
		std::vector<std::vector<cell> > matrix; /**< Symmetric matrix structure. */
		size_t matrixSize; /**< Number of rows / number of columns. */
		bool allowDiagonal; /**< Are diagonal cells allowed? */
	
	public:
		typedef SymmetricMatrixIterator<thistype*, T> iterator; /**< Symmetric Matrix iterator. */
		typedef SymmetricMatrixIterator<const thistype*, const T> const_iterator; /**< Symmetric Matrix constant iterator. */
		typedef SymmetricMatrixAccessor<thistype*, T> accessor; /**< Symmetric Matrix cells accessor. */
		typedef SymmetricMatrixAccessor<const thistype*, const T> const_accessor; /**< Symmetric Matrix constant cells accessor. */
	
		/**
		 * Constructs instance of order size x size.
		 * @param size number of rows / number of columns.
		 * @param allowDiagonal are diagonal cells allowed?
		 */
		SymmetricMatrix(size_t size, bool allowDiagonal = false) : matrix(size), matrixSize(size), allowDiagonal(allowDiagonal)
		{
			if(allowDiagonal)
				for(int i=0; i < matrixSize; ++i)
					matrix[i] = std::vector<cell>(i+1);
			else
				for(int i=0; i < matrixSize; ++i)
					matrix[i] = std::vector<cell>(i);
		}
		
		/**
		 * Checks whether such pair is defined in the matrix.
		 * @param i row number.
		 * @param j column number.
		 * @return true, if cell was defined.
		 */
		bool defined(int i, int j) const
		{
			#ifndef FAST_SYMMETRIC_MATRIX
			if(i >= matrixSize || j >= matrixSize)
				return false;
			#endif
			
			if(i > j)
				return matrix[i][j].defined;
			if(i < j)
				return matrix[j][i].defined;
			if(allowDiagonal)
				return matrix[i][i].defined;
			return false;
		}
		
		/**
		 * Returns reference to the item at [i][j], marking it as defined.
		 * Accessing [i][i] when not allowed or accessing index out of bounds will result in SymmetricMatrixException throw.
		 * @param i row number.
		 * @param j column number.
		 * @throw symmetric_matrix_exception
		 * @return item at [i][j].
		 */
		T& get(int i, int j)
		{
			#ifndef FAST_SYMMETRIC_MATRIX
			if(i >= matrixSize || j >= matrixSize)
				throw_exception(symmetric_matrix_exception, symmetric_matrix_exception::OUT_OF_BOUNDS, i, j);
			#endif
			
			if(i > j)
				return matrix[i][j].defined = true, matrix[i][j].value;
			if(i < j)
				return matrix[j][i].defined = true, matrix[j][i].value;
			if(allowDiagonal)
				return matrix[i][i].defined = true, matrix[i][i].value;
			throw_exception(symmetric_matrix_exception, symmetric_matrix_exception::DIAGONAL_NOT_ALLOWED, i, j);
		}
		
		/**
		 * Returns const reference to the item at [i][j], without marking it as defined
		 * if it hasn't been defined yet. Accessing [i][i] when not allowed
		 * or accessing index out of bounds will result in SymmetricMatrixException throw.
		 * @param i row number.
		 * @param j column number.
		 * @throw symmetric_matrix_exception
		 * @return item at [i][j].
		 */
		const T& get(int i, int j) const
		{
			#ifndef FAST_SYMMETRIC_MATRIX
			if(i >= matrixSize || j >= matrixSize)
				throw_exception(symmetric_matrix_exception, symmetric_matrix_exception::OUT_OF_BOUNDS, i, j);
			#endif
			
			if(i > j)
				return matrix[i][j].value;
			if(i < j)
				return matrix[j][i].value;
			if(allowDiagonal)
				return matrix[i][i].value;
			throw_exception(symmetric_matrix_exception, symmetric_matrix_exception::DIAGONAL_NOT_ALLOWED, i, j);
		}
		
		/**
		 * @return number of rows / number of columns.
		 */
		size_t size() const
		{
			return matrixSize;
		}
		
		/**
		 * @return number of defined cells.
		 */
		size_t count() const
		{
			size_t counter = 0;
			for(auto& it : *this)
				++counter;
			return counter;
		}
		
		/**
		 * Wrapper for get(i, j) function (non-const), using a [i][j] syntax
		 * @param i row number
		 * @return matrix row accessor 
		 */
		accessor operator [] (int i)
		{
			return accessor(i, this);
		}
		
		/**
		 * Wrapper for get(i, j) const function, using a [i][j] syntax
		 * @param i row number
		 * @return matrix row constant accessor 
		 */
		const_accessor operator [] (int i) const
		{
			return const_accessor(i, this);
		}
		
		/**
		 * Returns iterator to the first (allready defined!) element in the array, 
		 * or the end() if the array is empty.
		 */
		iterator begin()
		{
			iterator zero = iterator(0, 0, this);
			if(!(allowDiagonal && defined(0, 0)))
			{				
				++zero;
				return zero;
			}
			return zero;
		}
		
		/**
		 * Returns iterator to the artificial element after the last in the
		 * array (just like for the std::iterator)
		 */
		iterator end()
		{
			return iterator(matrixSize, 0, this);
		}
		
		/**
		 * Returns iterator to the first (allready defined!) element in the array, 
		 * or the end() if the array is empty.
		 */
		const_iterator begin() const
		{
			const_iterator zero = const_iterator(0, 0, this);
			if(!(allowDiagonal && defined(0, 0)))
			{				
				++zero;
				return zero;
			}
			return zero;
		}
		
		/**
		 * Returns iterator to the artificial element after the last in the
		 * array (just like for the std::iterator)
		 */
		const_iterator end() const
		{
			return const_iterator(matrixSize, 0, this);
		}
};

#endif
