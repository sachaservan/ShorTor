#include "utils.hpp"

#include <sstream>
#include <iomanip> 

std::vector<std::string> split(const std::string& str, char delim)
{
	std::stringstream stringStream(str);
	std::string temp;
	std::vector<std::string> parts;
	
	while(std::getline(stringStream, temp, delim))
		parts.push_back(temp);	
    return parts;
}

std::string b64toHex(std::string b64)
{
	const std::string keyStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    int chr1, chr2, chr3;
    char enc1, enc2, enc3, enc4;
    int i = 0;

    while (b64.length() % 4) b64 = b64 + "="; // add missing padding if neccessary
    
	std::stringstream ss;
	while (i < b64.length())
	{
		enc1 = keyStr.find(b64[i++]);
		enc2 = keyStr.find(b64[i++]);
		enc3 = keyStr.find(b64[i++]);
		enc4 = keyStr.find(b64[i++]);

		chr1 = (enc1 << 2) | (enc2 >> 4);
		chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
		chr3 = ((enc3 & 3) << 6) | enc4;
		
		ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << chr1;
		if (enc3 != 64)
			ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << chr2;
		if (enc4 != 64)
			ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << chr3;
	}
	return ss.str();
}

bool findToken(const std::vector<std::string>& tokens, std::string searchItem)
{
	for(int i = 0; i < tokens.size(); ++i)
		if(tokens[i] == searchItem)
			return true;
	return false;
}

double distance(double lat1, double long1, double lat2, double long2)
{
	// auxilliary constants
	const double PI = M_PI;
	const double RADIUS = 6371; // earth radius in kilometers
	
	// precomputing differences and converting values to radians
	double dLat = (lat2 - lat1) / 180. * PI;
	double dLon = (long2 - long1) / 180.0f * PI;	
	double latRad1 = lat1 / 180. * PI;
	double latRad2 = lat2 / 180. * PI;
	
	// applying haversine formula
	double a = sin(dLat/2) * sin(dLat/2) + sin(dLon/2) * sin(dLon/2) * cos(latRad1) * cos(latRad2);
	double c = 2. * atan2(sqrt(a), sqrt(1. - a));
	
	return RADIUS * c;
}
