/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   yelpGet.h
 * Author: marius
 *
 * Created on June 12, 2017, 9:50 AM
 */

#ifndef YELPGET_H
#define YELPGET_H

#include <string>

//Error Codes
#define YELP_SEARCH_SUCCESS 0
#define YELP_ERROR_UNINITIALIZED -1
#define YELP_SEARCH_NO_RESPONSE -2
#define YELP_NO_MATCHING_DATA -3

//Structure for storing a previous search query and result, good for avoiding duplicate API calls for identical info

struct searchParameters {
    std::string yelpName;
    std::string JSONfile;
    std::pair <double, double> searchedCoordinates;
    size_t stringStartingPosition;
    size_t stringEndingPosition;

};

struct searchResult {
    std::string yelpName;
    size_t stringStartPosition;
    size_t stringEndPosition;
};

class yelpAPI {
private:
    static std::string theAccessToken;

    //Stores previous search parameters to avoid having to do the search again
    static searchParameters previousSearch;
    static bool partialMatchesEnabled, partialMatchesVerbose;
 
    static int yelpSearch(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch, size_t &stringPosition, size_t &endingPos, std::string& JSONresult);
public:

    static bool init();
    static void enablePartialMatches(bool enablePartial, bool partialVerbose);
    static void close();
    static void retreiveAccessToken();
    static std::string getAccessToken();
    static std::string formYelpSearchAPIRequest(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch);
    static bool previousSearchIdentical(const std::string& content, const std::pair<double, double>& coordinates);

    /*
     *Next three functions take in a search query (venue name) and return an address and the number of check-ins respectivly
     *Coordinates (Latitude, longitude) are required, and should be coordinates closest to venue, if not specified (inputed as 0,0) they will default to downtown Toronto
     *Num results to search specifies how many results the API will search through (will default to 5 if input is 0), program will get slower the higher the number
     *Additional parameters (empty string will be ignored) should follow foursquare API conventions (See: https://developer.foursquare.com/docs/venues/search)
     *Int returned is an error code, see the section at the top of this file
     */
    static int yelpGetAddress(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch, std::string& resultAddress);
    static int yelpGetRating(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch, int& popularity);
    static int yelpGetCost(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch, std::pair<double, double>& actualCoordinates);

    //Same as functions above, but returns other information from the API
    //See example return info at: https://developer.foursquare.com/docs/explore#req=venues/search%3Fll%3D43.654300,-79.386000%26query%3DCN%2620Tower%26limit%3D5
    static int yelpGetOther(const std::string& content, const std::string& dataToFind, std::pair<double, double> coordinates, unsigned numResultsToSearch, std::string& otherData);

};

//Helper functions
size_t writeDataToString(char *ptr, size_t size, size_t nmemb, void *stream);
std::string percentEncode(std::string raw);


#endif /* YELPGET_H */

