#include "scenario.hpp"
#include "ps_tor.hpp"
#include "ps_distributor.hpp"
#include "ps_lastor.hpp"
#include "ps_uniform.hpp"
#include "ps_selektor.hpp"
#include "relationship_manager.hpp"

std::shared_ptr<PathSelection> Scenario::makePathSelection(
	std::shared_ptr<PathSelectionSpec> pathSelectionSpec,
	std::shared_ptr<SenderSpec> senderSpec,
	std::shared_ptr<RecipientSpec> recipientSpec,
	const Consensus& consensus)
{
	std::shared_ptr<RelationshipManager> relationship;
	
	switch((*pathSelectionSpec).getType())
	{
		case PS_TOR:
			relationship = std::make_shared<SubnetRelations>(consensus);
			return std::make_shared<PSTor>(std::dynamic_pointer_cast<PSTorSpec>(pathSelectionSpec), senderSpec, recipientSpec, relationship, consensus);
		
		case PS_DISTRIBUTOR:
			relationship = std::make_shared<SubnetRelations>(consensus);
			return std::make_shared<PSDistribuTor>(std::dynamic_pointer_cast<PSDistribuTorSpec>(pathSelectionSpec), senderSpec, recipientSpec, relationship, consensus);
		
		case PS_LASTOR:
			relationship = std::make_shared<SubnetRelations>(consensus);
			return std::make_shared<PSLASTor>(std::dynamic_pointer_cast<PSLASTorSpec>(pathSelectionSpec), senderSpec, recipientSpec, relationship, consensus);

		case PS_UNIFORM:
			relationship = std::make_shared<SubnetRelations>(consensus);
			return std::make_shared<PSUniform>(std::dynamic_pointer_cast<PSUniformSpec>(pathSelectionSpec), senderSpec, recipientSpec, relationship, consensus);

		case PS_SELEKTOR:
			relationship = std::make_shared<SubnetRelations>(consensus);
			return std::make_shared<PSSelektor>(std::dynamic_pointer_cast<PSSelektorSpec>(pathSelectionSpec), senderSpec, recipientSpec, relationship, consensus);

		case PS_AS_TOR:
			relationship = std::make_shared<ASRelations>(consensus, senderSpec->address, recipientSpec->address);
			return std::make_shared<PSTor>( std::dynamic_pointer_cast<PSTorSpec>(pathSelectionSpec), senderSpec, recipientSpec, relationship, consensus);
		
		case PS_AS_DISTRIBUTOR:
			relationship = std::make_shared<ASRelations>(consensus, senderSpec->address, recipientSpec->address);
			return std::make_shared<PSDistribuTor>(std::dynamic_pointer_cast<PSDistribuTorSpec>(pathSelectionSpec), senderSpec, recipientSpec, relationship, consensus);
		
		case PS_AS_LASTOR:
			relationship = std::make_shared<ASRelations>(consensus, senderSpec->address, recipientSpec->address);
			return std::make_shared<PSLASTor>(std::dynamic_pointer_cast<PSLASTorSpec>(pathSelectionSpec), senderSpec, recipientSpec, relationship, consensus);

		case PS_AS_UNIFORM:
			relationship = std::make_shared<ASRelations>(consensus, senderSpec->address, recipientSpec->address);
			return std::make_shared<PSUniform>(std::dynamic_pointer_cast<PSUniformSpec>(pathSelectionSpec), senderSpec, recipientSpec, relationship, consensus);

		case PS_AS_SELEKTOR:
			relationship = std::make_shared<ASRelations>(consensus, senderSpec->address, recipientSpec->address);
			return std::make_shared<PSSelektor>(std::dynamic_pointer_cast<PSSelektorSpec>(pathSelectionSpec), senderSpec, recipientSpec, relationship, consensus);

		default:
			return NULL;
	}
}
