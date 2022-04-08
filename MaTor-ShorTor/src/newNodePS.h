/*
 * newNodePS.h
 *
 *  Created on: Apr 12, 2016
 *      Author: Tahleen
 */

#ifndef BRANCHES_BRANCHES_REFACTORING_SRC_NEWNODEPS_H_
#define BRANCHES_BRANCHES_REFACTORING_SRC_NEWNODEPS_H_

#include <memory>
#include <set>

#include "path_selection_standard.hpp"
#include "relay.hpp"
#include "consensus.hpp"
#include "sender_spec.hpp"
#include "recipient_spec.hpp"
#include "path_selection_spec.hpp"
#include "utils.hpp"
#include "types/symmetric_matrix.hpp"
#include "types/work_manager.hpp"
#include "addedNodeList.h"

class newNodePS : public PathSelectionStandard {

public:
	newNodePS(	std::shared_ptr<SenderSpec> senderSpec,
				std::shared_ptr<RecipientSpec> recipientSpec,
				std::shared_ptr<TorLikeSpec> pathSelectionSpec,
				std::shared_ptr<RelationshipManager> relationshipManager,
				const Consensus& consensus) :
					PathSelectionStandard(senderSpec, recipientSpec, pathSelectionSpec, relationshipManager, consensus)
					{}
	virtual ~newNodePS();
	void setExitBW();
	void setOtherBW();
	double getExitBW(size_t index);
	double getOtherBW(size_t index);

protected:
	static AddedNodeList addedNodeList;
	static int size=addedNodeList.getSize();
	static std::unique_ptr<SymmetricMatrix<bool>> relations;
	weight_t flagSumG, flagSumE, flagSumD, flagSumN, exitSumG, exitSumE, exitSumD, exitSumN;
	std::vector<weight_t> relatedSum1G, relatedSum1E, relatedSum1D,relatedSum1N;
	SymmetricMatrix<weight_t> relatedSum2G, relatedSum2E, relatedSum2D, relatedSum2N;
	weight_t factorsGforG[size][size][size], factorsEforG[size][size][size], factorsDforG[size][size][size], factorsNforG[size][size][size],
	factorsGforE[size][size][size], factorsEforE[size][size][size], factorsDforE[size][size][size], factorsNforE[size][size][size],
	factorsGforM[size][size][size], factorsEforM[size][size][size], factorsDforM[size][size][size], factorsNforM[size][size][size];
	double exitBW[size + consensus.getSize()], otherBW[size + consensus.getSize()];

};

#endif /* BRANCHES_BRANCHES_REFACTORING_SRC_NEWNODEPS_H_ */
