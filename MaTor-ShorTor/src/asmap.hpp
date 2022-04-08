#ifndef ASMAP_HPP
#define ASMAP_HPP

#include <memory>
#include <string>
#include <set>
#include <vector>
#include <map>
#include <functional>

class Consensus;
class Relay;
class SenderSpec;
class RecipientSpec;

class ASMap : public std::enable_shared_from_this<ASMap>
{
public:
  ASMap(const Consensus& consensus,
    std::shared_ptr<SenderSpec> senderSpecA,
    std::shared_ptr<SenderSpec> senderSpecB,
    std::shared_ptr<RecipientSpec> recipientSpecA,
    std::shared_ptr<RecipientSpec> recipientSpecB,
    std::string network_file);
  std::set<std::string>& aspath_nodes(size_t nodeA, size_t nodeB);
  std::set<std::string>& aspath_endpoint(size_t nodeID, std::string endpoint_IP);
  bool endpoint_exists(std::string endpoint_IP);
  void print_statistics();

private:
  std::vector<std::vector<std::set<std::string> > > nodesets;
  std::map<std::string, std::vector<std::set<std::string> > > endpointsets;
};


#endif
