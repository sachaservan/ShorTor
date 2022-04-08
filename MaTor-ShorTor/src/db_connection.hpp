#ifndef DB_CONNECTION_HPP
#define DB_CONNECTION_HPP

/** @file */

#include <vector>
#include <string>
#include <sqlite3.h>

#include "types/general_exception.hpp"
#include "utils.hpp"

typedef std::vector<std::string> Row; /**< Row of a query result. All data types set to string. */
typedef std::vector<Row> QueryResult; /**< Result of a database query as vector of rows. */
typedef QueryResult *QueryResultP; /**< Pointer to database query result. */

/**
 * Thrown if an error in database connection occurs.
 */
class db_exception : public general_exception
{
	public:
		/**
		 * Exception reasons.
		 */
		enum reason
		{
			DB_CREATION, /**< Database creation failure. */
			DB_OPEN, /**< Opening database failure. */
			DB_SQL_ERROR /**< SQL query error. */
		};
		
		// constructors
		/**
		 * Constructs exception instance.
		 * @param why reason of throwing exception
		 * @param file name of the file file in which exception has occured.
		 * @param line line at which exception has occured.
		 */
		db_exception(reason why, const char* file, int line) : general_exception("", file, line), reasonWhy(why)
		{
			switch(why)
			{
				case DB_CREATION:
					this->message = "the process creating database has failed.";			
					break;
				default:
					this->message = "unknown reason.";
					break;
			}
			commit_message();
		}

		/**
		 * Constructs exception instance with a parameter.
		 * @param why reason of throwing exception
		 * @param p parameter
		 * @param file name of the file file in which exception has occured.
		 * @param line line at which exception has occured.
		 */
		db_exception(reason why, int p, const char* file, int line) : general_exception("", file, line), reasonWhy(why)
		{
			switch(why)
			{
				case DB_OPEN:
					this->message = "opening database failed; SQLite returned code: " + std::to_string(p);			
					break;		
				default:
					this->message = "unknown reason.";
					break;
			}
			commit_message();
		}

				/**
		 * Constructs exception instance with SQL error.
		 * @param why reason of throwing exception
		 * @param errorCode SQL error code
		 * @param errorMessage SQL error message
		 * @param file name of the file file in which exception has occured.
		 * @param line line at which exception has occured.
		 */
		db_exception(reason why, int errorCode, const std::string& errorMessage, const char* file, int line) : general_exception("", file, line), reasonWhy(why)
		{
			switch(why)
			{
				case DB_SQL_ERROR:
					this->message = "SQL query error: code " + std::to_string(errorCode) + ", [" + errorMessage + "]";			
					break;		
				default:
					this->message = "unknown reason.";
					break;
			}
			commit_message();
		}

		/**
		 * @copydoc general_exception::~general_exception()
		 */
		virtual ~db_exception() throw () { }

		virtual int why() const { return reasonWhy; }
	
	protected:
		reason reasonWhy; /**< Exception reason. */
		
		virtual const std::string& getTag() const 
		{
			const static std::string tag = "db_exception";
			return tag;
		}
};

/**
 * Class provides database functionalities and structures.
 */ 
class DBConnection
{
	public:
		// constructors
		/**
		 * Initializes Database connection.
		 * @param DBPath .sqlite database file name.
		 * @param consensusDate date of the consensus in YYYY-MM-DD hh:mm:ss format.
		 * @throw db_exception
		 */
		DBConnection(const std::string& DBPath, const std::string& consensusDate);
		
		/**
		 * Freeing database handle.
		 */
		~DBConnection();
		
		// functions
		/**
		 * Performs SQL query for relays' fingerprints accessed by iterator.
		 * @param begin start of the interval.
		 * @param end end of the interval.
		 * @return nodes additional information.
		 */
		QueryResult readRelaysInfo(const std::vector<std::string>::iterator& begin, const std::vector<std::string>::iterator& end) const;
		
		/**
		 * Performs SQL query to read relays families status for specified consensus date.
		 * @return pairs of nodes considered as families.
		 */
		QueryResult readFamilies() const;
		
	private:
		// functions
		/**
		 * Executes database query and handles database errors.
		 * @param query database query.
		 * @return query result.
		 */
		QueryResult executeSQLiteQuery(const std::string& query) const;
		
		// variables
		std::string consensusDate; /**< Consensus valid after date */
		sqlite3* databaseHandler; /**< Handler for SQLite database connection. */
};

#endif
