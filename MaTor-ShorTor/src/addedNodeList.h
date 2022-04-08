/*
 * addedNodeList.h
 *
 *  Created on: Apr 12, 2016
 *      Author: Tahleen
 */

#ifndef BRANCHES_BRANCHES_REFACTORING_SRC_ADDEDNODELIST_H_
#define BRANCHES_BRANCHES_REFACTORING_SRC_ADDEDNODELIST_H_
#include <string>
#include <map>
#include <vector>
#include <memory>

#include "relay.hpp"
#include "db_connection.hpp"
#include "utils.hpp"

#include "types/general_exception.hpp"
#include "types/symmetric_matrix.hpp"
class AddedNodeList {
public:
	AddedNodeList();
	virtual ~AddedNodeList();
	size_t getSize() const
	{
		return availableRelays.size();
	}
	const Relay& getRelay(size_t relayPos) const
	{
		return availableRelays[relayPos];
	}
protected:
	std::vector<Relay> availableRelays;
};

#endif /* BRANCHES_BRANCHES_REFACTORING_SRC_ADDEDNODELIST_H_ */
