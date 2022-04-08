#ifndef UTILS_HPP
#define UTILS_HPP

/** @file */

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MATOR_USE_LOG_TAGS
#define MATOR_LOCK_PARALLEL_CODE

#define STRINGIFY__(x) #x
#define STRINGIFY_(x) STRINGIFY__(x)

#ifndef SOURCE_DIRECTORY_
#define SOURCE_DIRECTORY ""
#else
#define SOURCE_DIRECTORY STRINGIFY_(SOURCE_DIRECTORY_)
#endif

constexpr size_t SOURCE_DIRECTORY_OFFSET = sizeof(SOURCE_DIRECTORY); /**< Number of characters in full source directory path name.*/

/**
 * @enum logLevel
 * Logging levels.
 */
enum logLevel
{
	LOG_SILENT = 0, /**< No logging. Logs tagged with this enum will not be printed. */
	LOG_STANDARD = 1, /**< Standard logging. Logs tagged with this enum will be always printed if MATor is not at silent (0) logging level. */
	LOG_VERBOSE = 2 /**< Optional, verbose logging. Logs tagged with this enum wiil be printed at verbose (2) logging level. */
};

constexpr logLevel LOG_LEVEL = LOG_SILENT; /**< MATor logging level. */

using weight_t = double;
using probability_t = double;

/**
 * Logging macro
 * @param level log level (enum or int)
 * @param m message
 * @see logLevel
 */
#define clog(level, m) { if(LOG_LEVEL > 0 && level <= LOG_LEVEL) std::cout << m; }
/**
 * Logging with new line macro
 * @param level log level (enum or int)
 * @param m message
 * @see logLevel
 */
#ifdef MATOR_USE_LOG_TAGS
#ifdef SOURCE_DIRECTORY_
#define clogn(level, m) clog(level, "[" << std::string(__FILE__).substr(SOURCE_DIRECTORY_OFFSET) << "]\t" << m << std::endl)
#else
#define ___FILE___ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define clogn(level, m) clog(level, "[" << ___FILE___ << "]\t" << m << std::endl)
#endif
#else
#define clogn(level, m) clog(level, m << std::endl)
#endif

/**
 * Logging standard macro
 * @param m message
 */
#define clogs(m) clog(LOG_STANDARD, m)

/**
 * Logging standard with new line macro
 * @param m message
 */
#define clogsn(m) clogn(LOG_STANDARD, m)

/**
 * Logging verbose macro
 * @param m message
 */
#define clogv(m) clog(LOG_VERBOSE, m)

/**
 * Logging verbose with new line macro
 * @param m message
 */
#define clogvn(m) clogn(LOG_VERBOSE, m)

/**
 * Logs error message even if MATor is in silent mode.
 * @param m error message
 */
#define clogerr(m) { std::cout << m; }

/**
 * Logs error message with new line even if MATor is in silent mode.
 * @param m error message
 */
#define clogerrn(m) clogerr(m << std::endl)

/**
 * Splits string using specified delimiter.
 * If no delimiter is specified, whitespace is used by default.
 * @param str input string
 * @param delim delimiter
 * @return vector of input's parts.
 */
std::vector<std::string> split(const std::string& str, char delim = ' ');

/**
 * Converts base64 string to hex string.
 * @param b64 string in base64 format (no trailing "=" required)
 * @return converted hex string.
 */
std::string b64toHex(std::string b64);

/**
 * Finds specified token in tokens vector.
 * @param tokens tested vector
 * @param searchItem token to be found
 * @return true iff token was found.
 */
bool findToken(const std::vector<std::string>& tokens, std::string searchItem);

/**
 * Computes the distance between points specified by geographic coordinates
 * @param lat1 Latitude of the first point in degrees.
 * @param long1 Longitude of the first point in degrees.
 * @param lat2 Latitude of the second point in degrees.
 * @param long2 Longitude of the second point in degrees.
 * @return distance in kilometers.
 */
double distance(double lat1, double long1, double lat2, double long2);

/**
 * Initialized parameters for time measurement.
 * @param _start starting measure time hook
 * @param _stop stopping measure time hook
 */
#define initMeasure(_start, _stop) std::chrono::system_clock::time_point _start; std::chrono::system_clock::time_point _stop
/**
 * Assigns time measure to the hook.
 * @param _hook time measure hook
 */
#define makeMeasure(_hook) _hook = std::chrono::system_clock::now()
/**
 * Computes time duration between two time points in microseconds.
 * @param _start startting measure time
 * @param _stop stopping measure time
 */
#define measureTime(_start, _stop) std::chrono::duration_cast<std::chrono::milliseconds>(_stop - _start).count()




// ===== ASSERTIONS =====

class Assert {
public:
	Assert(bool cond) : assertion(cond) {}
	~Assert() {
		if (!assertion) {
			std::cerr << std::endl;
			abort();
		}
	}
	template <class T>
	inline Assert& operator<<(const T& message) {
		if (!assertion)
			std::cerr << message;
		return *this;
	}
private:
	bool assertion;
};

class NullAssert {
public:
	NullAssert(bool cond) {}
	template <class T>
	inline NullAssert& operator<<(const T& message) { return *this; }
};

#ifndef NDEBUG
#define myassert(cond) Assert(cond) << "Assertion failure at " << __FILE__ << ":" << __LINE__ << " -- "
#define release_assert(cond) Assert(cond) << "Fatal error at at " << __FILE__ << ":" << __LINE__ << " -- "
#else
#define myassert(cond) NullAssert(cond)
#define release_assert(cond) Assert(cond) << "Fatal error -- "
#endif



#define UNREACHABLE do { release_assert(false) << "UNREACHABLE"; exit(3); } while(0)
//#define NOT_IMPLEMENTED do { release_assert(false) << "NOT IMPLEMENTED"; exit(3); } while(0)
#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define NOT_IMPLEMENTED do { std::cerr << "Fatal error at at " << __FILE__ << ":" << __LINE__ << " -- " << "NOT IMPLEMENTED"; throw std::runtime_error("NOT IMPLEMENTED (" __FILE__ ":" STRINGIZE(__LINE__) ")"); } while(0)


#endif
