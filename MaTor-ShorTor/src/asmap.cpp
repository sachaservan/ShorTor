#include <vector>
#include <set>
#include <iostream>
#include <fstream>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include "asmap.hpp"
#include "sender_spec.hpp"
#include "recipient_spec.hpp"
#include "utils.hpp"
#include "consensus.hpp"

using size_type = std::size_t;

static void buildIDMap(std::map<std::string,int> & IDMap, const Consensus& consensus)
{
	IDMap.clear();
	for(int i=0; i < consensus.getSize(); i++)
	{
		if(IDMap.find(consensus.getRelay(i).getAddress()) != IDMap.end())
			clogsn("twice: [" << ((std::string) consensus.getRelay(i).getAddress()) << "]");
		IDMap[consensus.getRelay(i).getAddress()] = i;

	}
}

inline std::string trim_right_copy(
  const std::string& s,
  const std::string& delimiters = " \f\n\r\t\v" )
{
  return s.substr( 0, s.find_last_not_of( delimiters ) + 1 );
}


ASMap::ASMap(const Consensus& consensus,
  std::shared_ptr<SenderSpec> senderSpecA,
  std::shared_ptr<SenderSpec> senderSpecB,
  std::shared_ptr<RecipientSpec> recipientSpecA,
  std::shared_ptr<RecipientSpec> recipientSpecB,
  std::string network_file)
{

  size_t size = consensus.getSize();
  clogsn("Creating AS Map from file " << network_file << std::endl);

  std::map<std::string,int> IDMap;
	buildIDMap(IDMap,consensus);

  // Create sets for all the routes between nodes
  this->nodesets.resize(size);
	for(size_t i = 0; i < size; ++i)
		this->nodesets[i].resize(size);


  // Create a map of the sets for all the routes between nodes and endpoints
  this->endpointsets.emplace((std::string)senderSpecA->address, std::vector<std::set<std::string> >(size));
  this->endpointsets.emplace((std::string)senderSpecB->address, std::vector<std::set<std::string> >(size));
  this->endpointsets.emplace((std::string)recipientSpecA->address, std::vector<std::set<std::string> >(size));
  this->endpointsets.emplace((std::string)recipientSpecB->address, std::vector<std::set<std::string> >(size));


  // read the network_file
  std::filebuf fb;

	std::ofstream debugfile;
	debugfile.open("debug.txt");

  clogsn("reading file " << network_file);
  unsigned int ctr = 0;
  if (fb.open (network_file,std::ios::in))
  {
    std::istream is(&fb);

		int countknown=0;

    int n1=0;
    int n2=0;
    char buf[999];
                std::string t0;
                std::string t1;
                std::string t2;
    while (is)
    {
			// parse the network_file and fill the sets accordingly
      if(ctr % 100000 == 0)
        clogsn("." << std::flush);
      ctr++;
      getline(is, t0, '\t');
      getline(is, t1, '\t');
      getline(is, t2);
			t2.erase(std::remove(t2.begin(), t2.end(), '\n'), t2.end());

      // Check whether the line is about an endpoint
      if ( this->endpointsets.find(t0) == this->endpointsets.end() && this->endpointsets.find(t1) == this->endpointsets.end())
      {
        // If no endpoint, check whether both IP addresses are known Tor nodes
				//myassert(IDMap.find(t0) != IDMap.end());
				//myassert(IDMap.find(t1) != IDMap.end());
        if(IDMap.find(t0) == IDMap.end())
					continue;
        if(IDMap.find(t1) == IDMap.end())
          continue;
        //knownroutes[IDMap[t0]][IDMap[t1]] = true;
        //knownroutes[IDMap[t1]][IDMap[t0]] = true;
        // store ASes (in t2) in the respective set
        boost::char_separator<char> sep(" ");
        boost::tokenizer< boost::char_separator<char> > tokens(t2, sep);
				int counttokens = 0;
        BOOST_FOREACH (const std::string t, tokens)
        {
					counttokens++;
          this->nodesets[IDMap[t0]][IDMap[t1]].insert(trim_right_copy(t));
					this->nodesets[IDMap[t1]][IDMap[t0]].insert(trim_right_copy(t));
        }

				countknown++;
      } else {
        // If endpoint, get endpoint IP and node IP
        std::string endpoint;
        std::string nodestr;
        if ( this->endpointsets.find(t0) != this->endpointsets.end())
        {
          endpoint = t0;
          nodestr = t1;
        }else{
					if ( this->endpointsets.find(t1) == this->endpointsets.end())
						continue;
          endpoint = t1;
          nodestr = t0;
        }
        // Check whether the node IP exists
        if(IDMap.find(nodestr) == IDMap.end())
				{
					continue;
				}
        // store ASes (in t2) in the respective set
        boost::char_separator<char> sep(" ");
        boost::tokenizer< boost::char_separator<char> > tokens(t2, sep);
				int counttokens = 0;
        BOOST_FOREACH (const std::string t, tokens)
        {
					counttokens++;
          this->endpointsets[endpoint][IDMap[nodestr]].insert(trim_right_copy(t));
        }
				countknown++;
      }

    }
    fb.close();
    clogsn("");
		debugfile << countknown << std::endl;

		// Fix the nodes that have the same IP
		for(size_t i =0; i < size; i++)
		{
			for(size_t j =0; j < size; j++)
			{
				if(consensus.getRelay(i).getAddress() == consensus.getRelay(j).getAddress())
				{
					// The nodes are the same, so we should combine the AS sets. This is okay, because we considered exactly one of the nodes in the IDMap
					for(size_t k =0; k < size; k++)
					{
						this->nodesets[i][k].insert(this->nodesets[j][k].begin(), this->nodesets[j][k].end());
						this->nodesets[k][i].insert(this->nodesets[k][j].begin(), this->nodesets[k][j].end());
					}
					this->endpointsets[senderSpecA->address][i].insert(this->endpointsets[senderSpecA->address][j].begin(),this->endpointsets[senderSpecA->address][j].end());
					this->endpointsets[senderSpecB->address][i].insert(this->endpointsets[senderSpecB->address][j].begin(),this->endpointsets[senderSpecB->address][j].end());
					this->endpointsets[recipientSpecA->address][i].insert(this->endpointsets[recipientSpecA->address][j].begin(),this->endpointsets[recipientSpecA->address][j].end());
					this->endpointsets[recipientSpecB->address][i].insert(this->endpointsets[recipientSpecB->address][j].begin(),this->endpointsets[recipientSpecB->address][j].end());
				}
			}
		}

}else{
	//Could not find network_file
	debugfile << "network_file not found!" << std::endl;
}
debugfile.close();
}


std::set<std::string>& ASMap::aspath_nodes(size_t nodeA, size_t nodeB)
{
  return nodesets[nodeA][nodeB];
}

bool ASMap::endpoint_exists(std::string endpoint_IP)
{
	return this->endpointsets.find(endpoint_IP) != this->endpointsets.end();
}


std::set<std::string>& ASMap::aspath_endpoint(size_t nodeID, std::string endpoint_IP)
{
  // find the correct item in the map
	assert(this->endpoint_exists(endpoint_IP));
  return this->endpointsets[endpoint_IP][nodeID];
}
void ASMap::print_statistics()
{
  // initialize counters
  int ctr_emptysets_nodes = 0;
  int ctr_emptysets_endpoints = 0;
  int sum_ASes_nodes = 0;
  int sum_ASes_endpoints = 0;
  // iterate over all sets and increment counters

  // print some statistics about the node sets

}
