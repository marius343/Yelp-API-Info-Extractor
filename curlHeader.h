/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   curlHeader.h
 * Author: marius
 *
 * Created on June 13, 2017, 2:08 PM
 */

#include "curl/curl.h"
#include "yelpGet.h"

#ifndef CURLHEADER_H
#define CURLHEADER_H

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




#endif /* CURLHEADER_H */

