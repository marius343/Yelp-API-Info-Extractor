
/* 
 * File:   yelpGet.h
 * Author: marius
 *
 * Created on June 12, 2017, 9:50 AM
 */

#ifndef YELPGET_H
#define YELPGET_H

#include <string>
#include "rapidjson/document.h"

//Error Codes
#define YELP_SEARCH_SUCCESS 0
#define YELP_ERROR_UNINITIALIZED -1
#define YELP_SEARCH_NO_RESPONSE -2
#define YELP_NO_MATCHING_DATA -3


//Client public and secret ID, current IDs are temporary development IDS. They should be changed for the final version
const std::string clientID = "client_id=TY3Cij8USYvx48B0twBzCA";
const std::string clientSecretID = "client_secret=zG2JD7UuufNPqhQ5ctcf2wAVZuBXhAAjkZjm7vMRdIz7vIvB7fe9p7aXXPNO5vxz";

//Structure for storing a previous search query and result, good for avoiding duplicate API calls for identical info

struct searchParameters {
    std::string businessName;
    std::string JSONfile;
    std::pair <double, double> searchedCoordinates;
    rapidjson::SizeType businessIndex;

};

struct searchResult {
    std::string businessName;
    rapidjson::SizeType businessIndex;
};

class yelpAPI {
    
private:
    static std::string theAccessToken;

    //Stores previous search parameters to avoid having to do the search again
    static searchParameters previousSearch;
    //Partial matches means the program will attempt to find a match automatically even if strings aren't identical
    //if verbose is also enabled, the program will prompt the user to make a choice, both of these are enabled by default
    static bool partialMatchesEnabled, partialMatchesVerbose;
public:

    static bool init();
    static void enablePartialMatches(bool enablePartial, bool partialVerbose);
    static void close();
    static void retreiveAccessToken();
    static std::string getAccessToken();
    static int yelpSearch(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch);
    static std::string formYelpSearchAPIRequest(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch);

    //Search result accessors
    static int yelpGetName(std::string& name);
    static int yelpGetAddress(std::string& resultAddress);
    static int yelpGetRating(double& ratingOutOfFive, int& numberOfReviews);
    static int yelpGetCost(std::string &cost);
    static int yelpGetCoordinates(std::pair<double, double> &coordinates);
    static int yelpGetCategories(std::string &category);

    //Retrieves other information, returns as string
    static int yelpGetOther(const std::string& keyToFind, std::string &result);

};

//Helper functions
size_t writeDataToString(char *ptr, size_t size, size_t nmemb, void *stream);
std::string percentEncode(std::string raw);
std::string simplifyString(std::string inputString);
bool isInteger(const std::string & s);


#endif /* YELPGET_H */

