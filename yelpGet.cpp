//Foursquare API file, includes some basic functions to extract information
//Created May 24 2017 by Marius, based on Bing API file created by another author

#include "yelpGet.h"
#include <curl/curl.h>
#include <memory>
#include <exception>
#include <unordered_set>
#include <sstream>
#include <string>
#include <iomanip>
#include <iostream>
#include <vector>
#include <ctype.h>

#define FOURSQUARE_SEARCH_SUCCESS 0
#define FOURSQUARE_ERROR_UNINITIALIZED -1
#define FOURSQUARE_SEARCH_NO_RESPONSE -2
#define FOURSQUARE_NO_MATCHING_VENUE_DATA -3

#define DEBUG_SEARCH



#ifdef DEBUG_SEARCH
bool isVerboseOn = true;
#else
bool isVerboseOn = false;
#endif


//Global Variables


//Client public and secret ID, current IDs are temporary development IDS. They should be changed for the final version
const std::string clientID = "client_id=TY3Cij8USYvx48B0twBzCA";
const std::string clientSecretID = "client_secret=zG2JD7UuufNPqhQ5ctcf2wAVZuBXhAAjkZjm7vMRdIz7vIvB7fe9p7aXXPNO5vxz";

//Static member definition
std::string yelpAPI::theAccessToken;
searchParameters yelpAPI::previousSearch;
bool yelpAPI::partialMatchesEnabled, yelpAPI::partialMatchesVerbose;

//Encodes user input so that it matches the proper format for URLs (Not

std::string percentEncode(std::string raw) {
    static const std::unordered_set<char> percentEncoding_reservedCharSet{
        '!', '#', '$', '&', '\'', '(', ')', '*', '+', ',', '/', ':', ';', '=', '?', '@', '[', ']',
        ' ', '"'};
    //*****************************************************
    std::stringstream tmp;
    tmp << std::hex << std::setfill('0');
    for (auto itor = raw.begin(); itor != raw.end(); itor++) {
        if (percentEncoding_reservedCharSet.find(*itor) != percentEncoding_reservedCharSet.end()) {
            tmp << '%' << std::setw(2) << static_cast<unsigned> (*itor);
        } else {
            tmp << *itor;
        }
    }
    return tmp.str();
}


//Determines what the HTTPS URL should be for a given user request

std::string yelpAPI::formYelpSearchAPIRequest(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch) {
    static const std::string foursquareAPIURL = "https://api.yelp.com/v3/";
    unsigned numResults;
    if (numResultsToSearch == 0) numResults = 5;
    else numResults = numResultsToSearch;
    std::string searchContent = "term=" + percentEncode(content);
    std::string latitude = "&latitude=" + std::to_string(coordinates.first);
    std::string longitude = "&longitude=" + std::to_string(coordinates.second);

    std::string request = "businesses/search?" + searchContent + latitude + longitude + "&limit=" + std::to_string(numResults);

    /*Request parameters:
    Return only webpages: &responseFilter=Webpages
    Return only 1 result: &count=1
    JSON output format: This is automatic now
     Return only English results: &setLang=ENG
     */

#ifdef DEBUG_SEARCH
    std::cout << foursquareAPIURL + request << std::endl;
#endif

    return foursquareAPIURL + request;
}

size_t writeDataToString(char *ptr, size_t size, size_t nmemb, void *stream) {
    ((std::string*)stream)->append(ptr, size * nmemb);
    return size*nmemb;
}

//**********************************************************************************

//Curl structure for HTTPS requests

struct curl_boo {
    CURL* searchHandle;
    bool isInit;
    bool isSuccessful;

    curl_boo() : searchHandle(nullptr), isInit(false), isSuccessful(false) {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK) {
            isInit = true;
            searchHandle = curl_easy_init();
            if (searchHandle != NULL) {
                isSuccessful = true;
                curl_easy_setopt(searchHandle, CURLOPT_WRITEFUNCTION, writeDataToString);
                curl_easy_setopt(searchHandle, CURLOPT_FAILONERROR, 1l);
                curl_easy_setopt(searchHandle, CURLOPT_NOSIGNAL, 1l);

            }
        }
    }

    ~curl_boo() {
        if (isInit) {
            if (isSuccessful) {
                curl_easy_cleanup(searchHandle);
            }
            curl_global_cleanup();
        }
    }

    //Gets the search results by conducting an HTTP request with a custom header

    std::string getSearchResponse(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch) {
        std::string result;
        std::string fullRequest = yelpAPI::formYelpSearchAPIRequest(content, coordinates, numResultsToSearch);
        
        //Creating request header structure
        struct curl_slist *theHeader = NULL;
        std::string accessToken = "authorization: Bearer " + yelpAPI::getAccessToken();
        //Required header info access token
        theHeader = curl_slist_append(theHeader, accessToken.c_str());
        std::cout << accessToken << std::endl;
        
        curl_easy_setopt(searchHandle, CURLOPT_URL, fullRequest.c_str());
        curl_easy_setopt(searchHandle, CURLOPT_HTTPHEADER, theHeader);
        curl_easy_setopt(searchHandle, CURLOPT_WRITEDATA, &result);
        CURLcode retValue = curl_easy_perform(searchHandle);

#ifdef DEBUG_SEARCH
        std::cout << std::endl;
        std::cout << "Result is: \n" << result << " (" << retValue << ")" << std::endl;
        std::cout << std::endl;
#endif

        if (retValue != CURLE_OK) {
            result.clear();
        }
        return result;
    }

    std::string getAccessTokenResponse(std::string theURL) {
        std::string result;
        std::string fullRequest = theURL;

        std::string postFields = "grant_type=client_credentials&" + clientID + "&" + clientSecretID;

        curl_easy_setopt(searchHandle, CURLOPT_URL, fullRequest.c_str());
        curl_easy_setopt(searchHandle, CURLOPT_POSTFIELDS, postFields.c_str());
        curl_easy_setopt(searchHandle, CURLOPT_WRITEDATA, &result);

        CURLcode retValue = curl_easy_perform(searchHandle);

#ifdef DEBUG_SEARCH
        std::cout << std::endl;
        std::cout << "Result is: \n" << result << std::endl;
        std::cout << std::endl;
#endif

        if (retValue != CURLE_OK) {
            result.clear();
        }

        return result;
    }
};

static std::unique_ptr<curl_boo> instPtr;

//Retrieves the Access token at the beginning of the program

void yelpAPI::retreiveAccessToken() {
    static const std::string yelpOAUTH = "https://api.yelp.com/oauth2/token";
    std::string result = instPtr->getAccessTokenResponse(yelpOAUTH);
    //Finding and extracting access token
    std::string theToken = "", toFind = "\"access_token\": \"";
    auto position = result.find(toFind);

    position = position + toFind.size();
    if (position != std::string::npos) {
        char currentCharacter = result.at(position);
        while (position != std::string::npos) {
            if (currentCharacter != '\"') theToken = theToken + currentCharacter;
            else break;
            position++;
            currentCharacter = result.at(position);
        }

        theAccessToken = theToken;
        std::cout << theToken << std::endl;

    } else {
        std::cerr << "ERROR: unexpected token retrieval response" << std::endl;
    }


}

//Searches for the provided search term and attempts to find a complete match, partial matches will be added later
//Position of result returned as a size_t type passed in by reference

int yelpAPI::yelpSearch(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch, size_t &stringPosition, size_t &endingPos, std::string& JSONresult) {
    if (instPtr) {
        std::vector<searchResult> searchResults;
        std::string contentSimplified;

        
        JSONresult = instPtr->getSearchResponse(content, std::make_pair(43.6543, -79.3860), numResultsToSearch);
        
    }
    return FOURSQUARE_SEARCH_SUCCESS;
}


//Initializes curl, must be called before using any other functions

bool yelpAPI::init() {
    if (!instPtr) {
        instPtr = std::unique_ptr<curl_boo>(new curl_boo);
    }
    partialMatchesEnabled = false;
    partialMatchesVerbose = false;
    return instPtr->isSuccessful;
}

void yelpAPI::close() {
    instPtr.reset();
}

//Checks if previous search is identical using structure

bool yelpAPI::previousSearchIdentical(const std::string& content, const std::pair<double, double>& coordinates) {
    if (content == previousSearch.yelpName || content == "") {
        if (coordinates.first == previousSearch.searchedCoordinates.first && coordinates.second == previousSearch.searchedCoordinates.second) {
            return true;
        }
    }

    //If the previous search does not match, update the prevous search info and return false
    previousSearch.yelpName = content;
    previousSearch.searchedCoordinates.first = coordinates.first;
    previousSearch.searchedCoordinates.second = coordinates.second;
    previousSearch.JSONfile.clear();
    return false;
}

int yelpAPI::yelpGetAddress(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch, std::string& resultAddress) {
    int resultSuccess;



    if (previousSearchIdentical(content, coordinates) == false) {
        resultSuccess = yelpSearch(content, coordinates, numResultsToSearch, previousSearch.stringStartingPosition, previousSearch.stringEndingPosition, previousSearch.JSONfile);
    } else {
        resultSuccess = 0;
    }

    if (resultSuccess == 0) {

    }
    return resultSuccess;
}

int yelpAPI::yelpGetRating(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch, int& popularity) {

    int resultSuccess;

    if (previousSearchIdentical(content, coordinates) == false) {
        resultSuccess = yelpSearch(content, coordinates, numResultsToSearch, previousSearch.stringStartingPosition, previousSearch.stringEndingPosition, previousSearch.JSONfile);
    } else {
        resultSuccess = 0;
    }

    if (resultSuccess == 0) {
 
    }

    return resultSuccess;

}

int yelpAPI::yelpGetOther(const std::string& content, const std::string& dataToFind, std::pair<double, double> coordinates, unsigned numResultsToSearch, std::string& otherData) {
    int resultSuccess;

    if (previousSearchIdentical(content, coordinates) == false) {
        resultSuccess = yelpSearch(content, coordinates, numResultsToSearch, previousSearch.stringStartingPosition, previousSearch.stringEndingPosition, previousSearch.JSONfile);
    } else {
        resultSuccess = 0;
    }

    if (resultSuccess == 0) {
       
    }

    return resultSuccess;
}

int yelpAPI::yelpGetCost(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch, std::pair<double, double>& actualCoordinates) {
    int resultSuccess;
    std::string tempLat = "", tempLong = "";

    if (previousSearchIdentical(content, coordinates) == false) {
        resultSuccess = yelpSearch(content, coordinates, numResultsToSearch, previousSearch.stringStartingPosition, previousSearch.stringEndingPosition, previousSearch.JSONfile);
    } else {
        resultSuccess = 0;
    }

    if (resultSuccess == 0) {
        
    }

    return resultSuccess;
}

//Enables partial matches for when searching by name
//For now enabling this will only ignore capitalization and punctuation in the search by default
//If partial verbose is enabled, the program will print the top 5 results and allow the user to choose if a match is not found

void yelpAPI::enablePartialMatches(bool enablePartial, bool partialVerbose) {
    partialMatchesEnabled = enablePartial;
    partialMatchesVerbose = partialVerbose;

}

std::string yelpAPI::getAccessToken(){
    return theAccessToken;
}