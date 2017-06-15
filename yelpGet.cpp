//Foursquare API file, includes some basic functions to extract information
//Created May 24 2017 by Marius, based on Bing API file created by another author

#include "yelpGet.h"
#include <curl/curl.h>
#include <memory>
#include <exception>
#include <string> 
#include <unordered_set>
#include <sstream>
#include <string>
#include <iomanip>
#include <iostream>
#include <vector>
#include <ctype.h>
#include "curlHeader.h"

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

//Pointer to a curl structure used for making calls to API
static std::unique_ptr<curl_boo> instPtr;

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

//Retrieves the Access token at the beginning of the program

void yelpAPI::retreiveAccessToken() {
    static const std::string yelpOAUTH = "https://api.yelp.com/oauth2/token";
    std::string result = instPtr->getAccessTokenResponse(yelpOAUTH);
   
     if (result.size() != 0) {
        rapidjson::Document document;
        document.Parse(result.c_str());

        if (document.HasMember("access_token")) {

            theAccessToken = document["access_token"].GetString();
            //std::cout << theAccessToken << std::endl;


        } else {
            std::cout << "ERROR: access_token field not found" << std::endl;
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


        previousSearch.JSONfile = instPtr->getSearchResponse(content, coordinates, numResultsToSearch);


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

                if (searchResults.size() > 1) {
                    int resultNumber = 1;
                    std::cout << "No matching businesses, choose one of the following or type -1 to exit" << std::endl;

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

                } else {
                    std::cout << "Error: 0 results found for search term" << std::endl;
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
            if (a[previousSearch.businessIndex].HasMember("rating") == true && a[previousSearch.businessIndex].HasMember("review_count")) {
                ratingOutOfFive = a[previousSearch.businessIndex]["rating"].GetDouble();
                numberOfReviews = a[previousSearch.businessIndex]["review_count"].GetInt();

                return YELP_SEARCH_SUCCESS;
            }
        }

        return YELP_NO_MATCHING_DATA;

    } else {
        std::cerr << "ERROR: No JSON file in cache, use yelpSearch(...) first" << std::endl;
    }

    return YELP_NO_MATCHING_DATA;
}

//Returns cost as string, Value is one of $, $$, $$$ and $$$$.

int yelpAPI::yelpGetCost(std::string &cost) {

    if (previousSearch.JSONfile.empty() == false) {
        rapidjson::Document document;
        document.Parse(previousSearch.JSONfile.c_str());
        if (document.HasMember("businesses") == true) {
            const rapidjson::Value& a = document["businesses"];
            if (a[previousSearch.businessIndex].HasMember("price") == true) {
                cost = a[previousSearch.businessIndex]["price"].GetString();
                return YELP_SEARCH_SUCCESS;
            }
        }

        return YELP_NO_MATCHING_DATA;

    } else {
        std::cerr << "ERROR: No JSON file in cache, use yelpSearch(...) first" << std::endl;
    }


    return YELP_NO_MATCHING_DATA;
}

//Returns coordinates by reference as a pair

int yelpAPI::yelpGetCoordinates(std::pair<double, double> &coordinates) {
    if (previousSearch.JSONfile.empty() == false) {
        rapidjson::Document document;
        document.Parse(previousSearch.JSONfile.c_str());

        //The business list in the JSON is an array
        if (document.HasMember("businesses") == true) {
            const rapidjson::Value& a = document["businesses"];
            //Extracting the address, this is stored inside a object, inside the array
            //If statements required to ensure that each key exists in the object
            if (a[previousSearch.businessIndex].HasMember("coordinates")) {
                if (a[previousSearch.businessIndex]["coordinates"].HasMember("latitude")) coordinates.first = a[previousSearch.businessIndex]["coordinates"]["latitude"].GetDouble();
                if (a[previousSearch.businessIndex]["coordinates"].HasMember("longitude")) coordinates.second = a[previousSearch.businessIndex]["coordinates"]["longitude"].GetDouble();
                return YELP_SEARCH_SUCCESS;
            }
        }

        return YELP_NO_MATCHING_DATA;
    } else {
        std::cerr << "ERROR: No JSON file in cache, use yelpSearch(...) first" << std::endl;
        return YELP_NO_MATCHING_DATA;
    }


    return YELP_NO_MATCHING_DATA;
}

//For now, returns the first category only (categories is an array). In the future might expand to return all categories
//Note, switch to key=alias if storing the category in a file, key=title is only for display purposes

int yelpAPI::yelpGetCategories(std::string &category) {
    if (previousSearch.JSONfile.empty() == false) {
        rapidjson::Document document;
        document.Parse(previousSearch.JSONfile.c_str());

        //The business list in the JSON is an array
        if (document.HasMember("businesses") == true) {
            const rapidjson::Value& a = document["businesses"];
            //Extracting the address, this is stored inside a object, inside the array
            //If statements required to ensure that each key exists in the object
            if (a[previousSearch.businessIndex].HasMember("categories")) {
                const rapidjson::Value& theCategoryArray = a[previousSearch.businessIndex]["categories"];

                //Use the alias if storing the category in a file
                //if(theCategoryArray[0].HasMember("alias")) category = theCategoryArray[0]["alias"].GetString();

                //Use title if printing out the category
                if (theCategoryArray[0].HasMember("title")) category = theCategoryArray[0]["title"].GetString();

                return YELP_SEARCH_SUCCESS;
            }
        }

        return YELP_NO_MATCHING_DATA;
    } else {
        std::cerr << "ERROR: No JSON file in cache, use yelpSearch(...) first" << std::endl;
        return YELP_NO_MATCHING_DATA;
    }




    return YELP_NO_MATCHING_DATA;
}

//Retrieves other information from the last search and returns it as a string
//This will only work if the data you are looking for exists, and if it is a string, int or double in the JSON file
//this will not work if the data you are looking for is part of an sub-object (e.g. key = 'title' in the categories object)

int yelpAPI::yelpGetOther(const std::string& keyToFind, std::string &result) {
    const char * theKey = keyToFind.c_str();
    if (previousSearch.JSONfile.empty() == false) {
        rapidjson::Document document;
        document.Parse(previousSearch.JSONfile.c_str());
        if (document.HasMember("businesses") == true) {
            const rapidjson::Value& a = document["businesses"];
            if (a[previousSearch.businessIndex].HasMember(theKey) == true) {
                //Checking data type and converting if necessary. 
                if (a[previousSearch.businessIndex][theKey].IsString())result = a[previousSearch.businessIndex][theKey].GetString();
                if (a[previousSearch.businessIndex][theKey].IsInt())result = std::to_string(a[previousSearch.businessIndex][theKey].GetInt());
                if (a[previousSearch.businessIndex][theKey].IsDouble())result = std::to_string(a[previousSearch.businessIndex][theKey].GetDouble());
                return YELP_SEARCH_SUCCESS;
            }
        }

        return YELP_NO_MATCHING_DATA;

    } else {
        std::cerr << "ERROR: No JSON file in cache, use yelpSearch(...) first" << std::endl;
    }


    return YELP_NO_MATCHING_DATA;


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