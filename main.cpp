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

    int resultSuccess;
    double longitude = 0.0, latitude = 0.0;
    std::pair<double, double> actualCoordinates;
    unsigned numResultsToSearch = 5;

    //The API must be re-initialized after retrieving the access token (for now at least)
    yelpAPI::init();
    yelpAPI::retreiveAccessToken();
    yelpAPI::close();


    yelpAPI::init();

    while (true) {
        int numberOfReviews;
        double ratingOutOfFive;
        std::string name = "", price = "", resultAddress = "";


        std::cout << "Enter Search term: ";
        std::getline(std::cin, userInput);
        if (userInput == "end" || userInput == "\0") break;
        //DOing the search and retrieving all the information
        yelpAPI::yelpSearch(userInput, std::make_pair(longitude, latitude), numResultsToSearch);
        yelpAPI::yelpGetName(name);
        yelpAPI::yelpGetAddress(resultAddress);
        yelpAPI::yelpGetRating(ratingOutOfFive, numberOfReviews);
        if(yelpAPI::yelpGetCost(price) != 0 ) price = "?";

        //Printing out result (for testing only)
        std::cout << "\n" << name << "\n" << resultAddress << std::endl;
        std::cout << ratingOutOfFive << "/5 stars (" << numberOfReviews << " reviews)" << std::endl;
        std::cout << "Price: " << price << std::endl;
        std::cout << std::endl;
    }


    return 0;
}

