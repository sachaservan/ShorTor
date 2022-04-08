#include "db_connection.hpp"
#include "utils.hpp"

#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>

DBConnection::DBConnection(const std::string& DBPath, const std::string& consensusDate) : consensusDate(consensusDate)
{
	if(!std::ifstream(DBPath))
	{
		clogsn("Running: ./mator-db " << consensusDate.substr(0, 7) << " -n -o " << DBPath);
		
		int scriptResult;
		if(LOG_LEVEL || true)
			scriptResult = system(("./mator-db " + consensusDate.substr(0, 7) + " -n -o " + DBPath).c_str());
		else // running script in quite mode
			scriptResult = system(("./mator-db " + consensusDate.substr(0, 7) + " -n -o -q " + DBPath).c_str());
		
		if(scriptResult) // execution returned non-zero status
			throw_exception(db_exception, db_exception::DB_CREATION);
	}
	
	int openResult = sqlite3_open_v2(DBPath.c_str(), &databaseHandler, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, NULL);
	if(openResult) // opening database returned non-zero status 
	{
		sqlite3_close(databaseHandler); // handler should be freed anyway
		throw_exception(db_exception, db_exception::DB_OPEN, openResult);
	}
}

DBConnection::~DBConnection()
{
	sqlite3_close(databaseHandler);
}

QueryResult DBConnection::readRelaysInfo(const std::vector<std::string>::iterator& begin, const std::vector<std::string>::iterator& end) const
{
	std::ostringstream inFingerprints;
	for (auto it = begin; it != end; ++it) 
	{
		if (it != begin) 
		{
			inFingerprints << ",";
		}
		inFingerprints << "\"" << *it << "\"";
	}
	std::string query = "SELECT nodes.fingerprint, descriptors.bandwidth_avg, descriptors.platform, descriptors.exit_policy, "
		"geoip.country, geoip.lat, geoip.long, geoip.as_number, geoip.as_name FROM descriptors "
		"JOIN geoip ON descriptors.address = geoip.ip "
		"JOIN nodes ON nodes.id = descriptors.node_id "
		"WHERE descriptors.start_time <= \"" + consensusDate
		+ "\" AND descriptors.end_time > \"" + consensusDate 
		+ "\" AND nodes.fingerprint IN (" + inFingerprints.str() + ");";
	
	return executeSQLiteQuery(query);
}

QueryResult DBConnection::readFamilies() const
{
	std::string query = "SELECT nA.fingerprint, nB.fingerprint FROM families JOIN nodes AS nA ON nA.id = ida JOIN nodes AS nB ON nB.id = idb WHERE start_time <= \""
		+ consensusDate + "\" AND end_time > \"" + consensusDate + "\"";
	return executeSQLiteQuery(query);
}

QueryResult DBConnection::executeSQLiteQuery(const std::string& query) const
{
	sqlite3_stmt *statement = 0;
	// compiling SQL query.
	// using sqlite3_prepare instead of sqlite3_prepare_v2 to avoid playing with error messages.
	int prepareResult = sqlite3_prepare(databaseHandler, query.c_str(), -1, &statement, NULL);
	if(prepareResult != SQLITE_OK)
		throw_exception(db_exception, db_exception::DB_SQL_ERROR, sqlite3_errcode(databaseHandler), std::string(sqlite3_errmsg(databaseHandler)));
	
	QueryResult queryResult;
	int stepResult = sqlite3_step(statement);
	int columns = sqlite3_column_count(statement);
	
	while (stepResult != SQLITE_DONE)       
	{
		switch(stepResult)
		{
			case SQLITE_ROW:
				{
					Row row;
					for (int i = 0; i < columns; ++i) 
					{
						const char* cc = (const char*)sqlite3_column_text(statement, i);
						row.push_back(cc ? std::string(cc) : ""); // if result is not null, then parse it and push, push empty string otherwise.
					}
					queryResult.push_back(row);
				}
				break;
				
			case SQLITE_ERROR: // if an error has occured, throw exception with detailed message
				{
					std::string errorMessage(sqlite3_errmsg(databaseHandler));
					int errorCode = sqlite3_reset(statement);
					sqlite3_finalize(statement);
					throw_exception(db_exception, db_exception::DB_SQL_ERROR, errorCode, errorMessage);
				}
				break;
			
			default: // SQLITE_BUSY
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				break;
		}
		
		stepResult = sqlite3_step(statement);
	}
	sqlite3_finalize(statement);
	return queryResult;
}
