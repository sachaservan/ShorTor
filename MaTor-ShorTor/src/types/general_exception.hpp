#ifndef GENERAL_EXCEPTION_HPP
#define GENERAL_EXCEPTION_HPP

/** @file */

#include <exception>
#include <string>

/**
 * Custom exception throwing (adds filename and exception line).
 * @param ex exception class
 * @param args exception class arguments.
 */

#define throw_exception(ex, ...) do{throw ex(__VA_ARGS__, __FILE__, __LINE__); }while(false)

/**
 * Virtual class for creating custom exceptions.
 */
class general_exception : public std::exception
{
	public:
		/**
		 * Constructs exception instance.
		 * @param err exception message.
		 * @param file name of the file file in which exception has occured.
		 * @param line line at which exception has occured.
		 */
		general_exception(const std::string& err, const std::string& file, int line)
			: message(err), file(file), line(line) { commit_message(); }

		/**
		 * Destructor that cannot throw exception (required by some compilers).
		 */
		virtual ~general_exception() throw () { }

		/**
		 * Function called when this exception is thrown.
		 * @return error message.
		 */
		virtual const char* what() const throw()
		{
			return what_buffer.c_str();
		}

		/**
		 * Commits current tag, message, line and file values, generating exception message for what.
		 * Always call it in the deriving class's constructors to ensure correct tags.
		 */
		virtual void commit_message() throw()
		{
			what_buffer = (getTag() + ": " + message + "\n\tat " + std::to_string(line) + " in " + file);
		}

		/**
		 * @return enum reason for exception.
		 */
		virtual int why() const = 0;

	protected:
		std::string what_buffer; /**< what() const char* buffer, makes sure it's c_str is not freed between the calls. */
		std::string message; /**< Exception message. */
		std::string file; /**< Name of the file in which exception has occured. */
		int line; /**< Line at which exception has occured. */

		/**
		 * @return Exception class tag.
		 */
		virtual const std::string& getTag() const
		{
			const static std::string tag = "general_exception";
			return tag;
		}
};

#endif
