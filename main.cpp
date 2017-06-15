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


//For printing out bold text to the terminal

std::ostream& bold_on(std::ostream& os) {
    return os << "\e[1m";
}

std::ostream& bold_off(std::ostream& os) {
    return os << "\e[0m";
}

/*
 * 
 */
int main(int argc, char** argv) {
    std::string userInput = "\0";
    double longitude = 43.6543, latitude = -79.3860;

    unsigned numResultsToSearch = 5;

    //The API must be re-initialized after retrieving the access token (for now at least)
    yelpAPI::init();
    yelpAPI::retreiveAccessToken();
    yelpAPI::close();


    yelpAPI::init();

    while (true) {
        int numberOfReviews;
        double ratingOutOfFive;
        std::string name = "", price = "", resultAddress = "", category = "", otherValue = "";
        std::pair<double, double> actualCoordinates;


        std::cout << "Enter search term: ";
        std::getline(std::cin, userInput);
        if (userInput == "end" || userInput == "\0") break;
        //DOing the search and retrieving all the information
        int result = yelpAPI::yelpSearch(userInput, std::make_pair(longitude, latitude), numResultsToSearch);
        if (result == 0) {
            yelpAPI::yelpGetName(name);
            yelpAPI::yelpGetAddress(resultAddress);
            yelpAPI::yelpGetCategories(category);
            yelpAPI::yelpGetRating(ratingOutOfFive, numberOfReviews);
            yelpAPI::yelpGetCoordinates(actualCoordinates);
            //yelpAPI::yelpGetOther("phone", otherValue);
            if (yelpAPI::yelpGetCost(price) != 0) price = "?";

            //Printing out result (for testing only)
            std::cout << "\n" << bold_on << name << bold_off << "\n" << "Category: " << category << std::endl;
            std::cout << resultAddress << std::endl;
            std::cout << ratingOutOfFive << "/5 stars (" << numberOfReviews << " reviews)" << std::endl;
            std::cout << "Price: " << price << std::endl;
            std::cout << actualCoordinates.first << ", " << actualCoordinates.second << std::endl;
            //std::cout << otherValue << std::endl;           
        }
        std::cout << std::endl;
    }


    return 0;
}

