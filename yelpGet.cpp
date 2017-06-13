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

#define YELP_SEARCH_SUCCESS 0
#define YELP_ERROR_UNINITIALIZED -1
#define YELP_SEARCH_NO_RESPONSE -2
#define YELP_NO_MATCHING_DATA -3

//#define DEBUG_SEARCH



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
    if (numResultsToSearch == 0) numResults = 3;
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
    if (result.size() != 0) {
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


        } else {
            std::cerr << "ERROR: unexpected token retrieval response" << std::endl;
        }

    } else {
        std::cerr << "ERROR: no result received from token authentication" << std::endl;
    }
}

//Searches for the provided search term and attempts to find a complete match, partial matches will be added later
//Position of result returned as a size_t type passed in by reference

int yelpAPI::yelpSearch(const std::string& content, std::pair<double, double> coordinates, unsigned numResultsToSearch) {
    if (instPtr) {
        std::vector<searchResult> searchResults;
        std::string contentSimplified = simplifyString(content);


        previousSearch.JSONfile = instPtr->getSearchResponse(content, std::make_pair(43.6543, -79.3860), numResultsToSearch);


        if (previousSearch.JSONfile.empty() == false) {
            //Creating a json document using rapidjson in order to extract information more easily
            rapidjson::Document document;
            bool businessFound = false;
            document.Parse(previousSearch.JSONfile.c_str());

            const rapidjson::Value& a = document["businesses"];
            assert(a.IsArray());
            //Loops through JSON result, extracts all business names and stores them in vector, while also checking if there is an exact match
            for (rapidjson::SizeType i = 0; i < a.Size(); i++) { // Uses SizeType instead of size_t
                searchResult tempResult;
                if (a[i].HasMember("name")) {
                    std::string theName = a[i]["name"].GetString();
                    if (contentSimplified == simplifyString(theName)) {
                        previousSearch.businessIndex = i;
                        previousSearch.businessName = theName;
                        businessFound = true;
                        break;
                    } else {
                        tempResult.businessName = theName;
                        tempResult.businessIndex = i;
                        searchResults.push_back(tempResult);
                    }
                }
            }

            //If the business has not been found, alert the user to choose one of the search results
            //Eventually it might be a good idea to store pairs of search results and user searches
            //in a txt file in order to allow the program to do this part automatically for some searches

            if (businessFound == false) {
                if (partialMatchesVerbose == false) {
                    previousSearch.JSONfile.clear();
                    return YELP_NO_MATCHING_DATA;
                }
                int resultNumber = 1;
                std::cout << "No matching venues, choose one of the following or type -1 to exit" << std::endl;

                for (unsigned j = 0; j < searchResults.size(); j++) {
                    std::cout << resultNumber << ". " << searchResults[j].businessName << std::endl;
                    resultNumber++;
                }
                std::string userInput;
                std::cout << "Number: ";
                std::getline(std::cin, userInput);

                //Getting user input, converting it to an integer (if its valid), and checking to see if its in range
                if (isInteger(userInput)) {
                    int userInputInt = stoi(userInput);
                    if (userInputInt > 0 && userInputInt <= static_cast<int> (searchResults.size())) {
                        previousSearch.businessName = searchResults[userInputInt - 1].businessName;
                        previousSearch.businessIndex = searchResults[userInputInt - 1].businessIndex;
                        return YELP_SEARCH_SUCCESS;
                    }
                }

                previousSearch.JSONfile.clear();
                return YELP_NO_MATCHING_DATA;
            }
        }

        return YELP_SEARCH_SUCCESS;
    } else {
        return YELP_ERROR_UNINITIALIZED;
    }
}


//Initializes curl, must be called before using any other functions

bool yelpAPI::init() {
    if (!instPtr) {
        instPtr = std::unique_ptr<curl_boo>(new curl_boo);
    }

    partialMatchesEnabled = true;
    partialMatchesVerbose = true;
    return instPtr->isSuccessful;
}

void yelpAPI::close() {
    instPtr.reset();
}

//Checks if previous search is identical using structure

bool yelpAPI::previousSearchIdentical(const std::string& content, const std::pair<double, double>& coordinates) {
    if (content == previousSearch.businessName || content == "") {
        if (coordinates.first == previousSearch.searchedCoordinates.first && coordinates.second == previousSearch.searchedCoordinates.second) {
            return true;
        }
    }

    //If the previous search does not match, update the prevous search info and return false
    previousSearch.businessName = content;
    previousSearch.searchedCoordinates.first = coordinates.first;
    previousSearch.searchedCoordinates.second = coordinates.second;
    previousSearch.JSONfile.clear();
    return false;
}

//Returns the business name of the previous search by reference if it exists, otherwise it prints an error

int yelpAPI::yelpGetName(std::string& name) {
    if (previousSearch.JSONfile.empty() == false) {
        name = previousSearch.businessName;
        return YELP_SEARCH_SUCCESS;
    } else {
        std::cerr << "ERROR: No JSON file in cache, use yelpSearch(...) first" << std::endl;
        return YELP_NO_MATCHING_DATA;
    }
}

int yelpAPI::yelpGetAddress(std::string& resultAddress) {



    if (previousSearch.JSONfile.empty() == false) {
        rapidjson::Document document;
        document.Parse(previousSearch.JSONfile.c_str());

        //The business list in the JSON is an array
        if (document.HasMember("businesses") == true) {
            const rapidjson::Value& a = document["businesses"];

            std::string streetAddress = "";
            std::string city = "";
            std::string provinceOrState = "";
            //Extracting the address, this is stored inside a object, inside the array
            //If statements required to ensure that each key exists in the object
            if (a[previousSearch.businessIndex].HasMember("location")) {
                if (a[previousSearch.businessIndex]["location"].HasMember("address1")) streetAddress = a[previousSearch.businessIndex]["location"]["address1"].GetString();
                if (a[previousSearch.businessIndex]["location"].HasMember("city")) city = a[previousSearch.businessIndex]["location"]["city"].GetString();
                if (a[previousSearch.businessIndex]["location"].HasMember("state")) provinceOrState = a[previousSearch.businessIndex]["location"]["state"].GetString();
                resultAddress = streetAddress + ", " + city + ", " + provinceOrState;
                return YELP_SEARCH_SUCCESS;
            }
        }

        return YELP_NO_MATCHING_DATA;
    } else {
        std::cerr << "ERROR: No JSON file in cache, use yelpSearch(...) first" << std::endl;
        return YELP_NO_MATCHING_DATA;
    }
}

//Returns rating (out of 5 stars) and the number of reviews by reference

int yelpAPI::yelpGetRating(double& ratingOutOfFive, int& numberOfReviews) {

    if (previousSearch.JSONfile.empty() == false) {
        rapidjson::Document document;
        document.Parse(previousSearch.JSONfile.c_str());

        
        if (document.HasMember("businesses") == true) {
            //Checking business array at search result index for rating and review count information, if they exist
            const rapidjson::Value& a = document["businesses"];
             if (a[previousSearch.businessIndex].HasMember("rating") == true && a[previousSearch.businessIndex].HasMember("review_count")){
                 ratingOutOfFive = a[previousSearch.businessIndex]["rating"].GetDouble();
                 numberOfReviews = a[previousSearch.businessIndex]["review_count"].GetInt();
                 
                 return YELP_SEARCH_SUCCESS;
             }
        }

        return YELP_NO_MATCHING_DATA;

    } else {
        std::cerr << "ERROR: No JSON file in cache, use yelpSearch(...) first" << std::endl;
        return YELP_NO_MATCHING_DATA;
    }



    return YELP_SEARCH_SUCCESS;
}

//Returns cost as string, Value is one of $, $$, $$$ and $$$$.

int yelpAPI::yelpGetCost(std::string &cost) {

    if (previousSearch.JSONfile.empty() == false) {
        rapidjson::Document document;
        document.Parse(previousSearch.JSONfile.c_str());       
        if (document.HasMember("businesses") == true) {
            const rapidjson::Value& a = document["businesses"];
             if (a[previousSearch.businessIndex].HasMember("price") == true){
                 cost = a[previousSearch.businessIndex]["price"].GetString();               
                 return YELP_SEARCH_SUCCESS;
             }
        }

        return YELP_NO_MATCHING_DATA;

    } else {
        std::cerr << "ERROR: No JSON file in cache, use yelpSearch(...) first" << std::endl;
        return YELP_NO_MATCHING_DATA;
    }


    return YELP_SEARCH_SUCCESS;
}

//Retrieves other information from the last search and returns it as a string
//This will only work if the data you are looking for exists, and if it is a string, int or double in the JSON file
//this will not work if the data you are looking for is part of an sub-object (e.g. key = 'title' in the categories object)

int yelpAPI::yelpGetOther(const std::string& content, const std::string& dataToFind, std::pair<double, double> coordinates, unsigned numResultsToSearch, std::string & otherData) {




    return YELP_SEARCH_SUCCESS;
}


//Enables partial matches for when searching by name
//For now enabling this will only ignore capitalization and punctuation in the search by default
//If partial verbose is enabled, the program will print the top 5 results and allow the user to choose if a match is not found

void yelpAPI::enablePartialMatches(bool enablePartial, bool partialVerbose) {
    partialMatchesEnabled = enablePartial;
    partialMatchesVerbose = partialVerbose;

}

std::string yelpAPI::getAccessToken() {
    return theAccessToken;
}

//Turns a string into all lowercase and removes certain words

std::string simplifyString(std::string inputString) {
    std::string outputString = "";
    char character;
    for (unsigned i = 0; i < inputString.size(); i++) {

        character = inputString[i];

        if (character == '&') outputString = outputString + "and";
        else if (character == '-') outputString = outputString + ' ';
        else if (character != '\'' && character != ',' && character != '.') {
            outputString = outputString + static_cast<char> (tolower(character));
        }

    }

    return outputString;
}

//Checks if string is an integer

bool isInteger(const std::string & s) {
    if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false;

    char * p;
    strtol(s.c_str(), &p, 10);

    return (*p == 0);
}