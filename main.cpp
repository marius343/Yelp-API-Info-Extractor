/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: marius
 *
 * Created on June 12, 2017, 9:49 AM
 */

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
#include "yelpGet.h"

using namespace std;

/*
 * 
 */
int main(int argc, char** argv) {
 std::string userInput = "\0";
    yelpAPI::init();
    int resultSuccess, popularity;

    
   yelpAPI::enablePartialMatches(true, true);
    
    double longitude = 0.0, latitude = 0.0;
    std::pair<double, double> actualCoordinates;
    unsigned numResultsToSearch = 5;
    std::string additionalParamters = "", resultAddress = "", additionalData = "";
    
    yelpAPI::retreiveAccessToken();
    yelpAPI::close();
    yelpAPI::init();
    
    while(true){
     std::cout << "Enter Search term: ";
     std::getline(std::cin, userInput);    
     if(userInput == "end" || userInput == "\0") break;
    yelpAPI::yelpGetAddress(userInput, std::make_pair(longitude, latitude), numResultsToSearch, resultAddress);
    
    
    }
    
    /*

    while (userInput != "end") {
        std::cout << "Enter Search term: ";
        std::getline(std::cin, userInput);
        if (userInput == "end") break;
        resultSuccess = yelpAPI::yelpGetAddress(userInput, std::make_pair(longitude, latitude), numResultsToSearch, resultAddress);
        std::cout << resultAddress << std::endl;
        if (resultSuccess == 0) {
            resultSuccess = yelpAPI::yelpGetRating(userInput, std::make_pair(longitude, latitude), numResultsToSearch, popularity);
            if (resultSuccess == 0) std::cout << "Number Of Check-ins: " << popularity << std::endl;
            else std::cout << "ERROR: " << resultSuccess << std::endl;
        } else {
            std::cout << "ERROR: " << resultSuccess << std::endl;
        }
        
        if(resultSuccess == 0){
            resultSuccess = yelpAPI::yelpGetCost(userInput, std::make_pair(longitude, latitude), numResultsToSearch, actualCoordinates);
            if (resultSuccess == 0) std::cout << std::fixed << std::setprecision(14) << "Coordinates:  " << actualCoordinates.first << ", " << actualCoordinates.second << std::endl;
            else std::cout << "ERROR: " << resultSuccess << std::endl;
        }
        
        std::cout<< std::endl;
        
        if (resultSuccess == 0) {
            
            std::string secondUserInput = "";
            while (secondUserInput != "no") {
                std::cout << "Find additional data?: ";
                std::getline(std::cin, secondUserInput);
                if (secondUserInput == "no" || secondUserInput == "No") break;
                resultSuccess = yelpAPI::yelpGetOther(userInput, secondUserInput, std::make_pair(longitude, latitude), numResultsToSearch, additionalData);

                if (resultSuccess == 0) {
                    std::cout << secondUserInput << ": " << additionalData << std::endl;
                } else {
                    std::cout << "The data specified does not exist!" << std::endl;
                }

                additionalData.clear();
            }
        }

        std::cout << "\n" << std::endl;
        resultAddress.clear();
    }
*/
    return 0;
}

