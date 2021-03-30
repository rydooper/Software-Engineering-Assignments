#include <sstream> //Lines 1-7 includes the libraries and header files that are utilised in the program
#include <fstream>
#include <iomanip>
#include <iostream>
#include "xml/parser.h"
#include "parseGPX.h"
#include <assert.h>
using namespace std; //Lines 8-10 declare the namespaces that are used in the program, which is beneficial due to the frequency of their use
using namespace GPS;
using namespace XML;

GPX::ParseData::ParseData(std::string source, bool isFileName, int num, int firstCharNotDelimiterIndex, int lastCharNotDelimiterIndex, std::string LatAttributeString, std::string LongAttributeString) { //public constructor of the ParseData class in the GPX namespace
    this->source = source; //Lines 13-19 initialising the ParseData class attributes in the object
    this->isFileName = isFileName;
    this->num = num;
    this->firstCharNotDelimiterIndex = firstCharNotDelimiterIndex;
    this->lastCharNotDelimiterIndex = lastCharNotDelimiterIndex;
    this->LatAttributeString = LatAttributeString;
    this->LongAttributeString = LongAttributeString;
}

bool GPX::ParseData::ContainsAttribute(XML::Element element, std::string LatAttributeString, std::string LongAttributeString) { //checks if the element contains the attribute string, if not then a domain_error is thrown
    bool exceptionThrown = false;
    if (! element.containsAttribute(LatAttributeString)) { //the reason why the attribute strings are parameters is in case further developments change the attribute strings (makes the function modular)
        throw domain_error("Missing '"+ LatAttributeString +"' attribute.");
        exceptionThrown = true;
    }
    if (! element.containsAttribute(LongAttributeString)) {
        throw domain_error("Missing '"+ LongAttributeString +"' attribute.");
        exceptionThrown = true;
    }
    return exceptionThrown; //returns whether the element object contains the two strings or not
}

bool GPX::ParseData::ContainsSubElement(XML::Element element, string inputString) { //checks if the element object contains the sub element string
    bool exceptionThrown = false;
    if (! element.containsSubElement(inputString)) {
        throw domain_error("Missing '" + inputString + "' element.");
        exceptionThrown = true;
    }
    return exceptionThrown; //returns whether the element object contains the sub element string or not and if an exception was thrown
}

bool GPX::ParseData::GetName(XML::Element element, string inputString) { //checks if the getName() function contains the input string or not
    bool exceptionThrown = false;
    if (element.getName() != inputString) {
        throw domain_error("Missing '" + inputString + "' element.");
        exceptionThrown = true;
    }
    return exceptionThrown; //returns whether an exception was thrown due to the getName() function not returning the inputString
}

std::vector<GPS::RoutePoint> GPX::ParseData::parseRoute() { //Parses GPX data containing a route
    int totalSubElements; //Lines 54-58 declare the variables to be used in the function
    string latitude, longitude, elevation, name, lineFromFile;
    ostringstream positionsStream, fileReadStream;
    std::vector<RoutePoint> parsedResult;
    const char Delimiter = ' ';

    Element element = SelfClosingElement("",{}), element1 = element, element2 = element; //Declares the element object and assigns values to the attributes
    Position startPos(0,0), prevPos = startPos, nextPos = startPos; //Declares the Position object and assigns values to the attributes
    try { //tries the run the following code, any errors will be caught in the catch
        if (isFileName == true) { //checks if the source data is provided from a file
            ifstream fs(source);
            if (!fs.good()) {
                throw invalid_argument("Error opening source file '" + source + "'.");
            }
            while (!fs.eof()) { //reads in each line of data from the file, adding it to a stringstream object
                getline(fs, lineFromFile);
                fileReadStream << lineFromFile << endl;
            }
            source = fileReadStream.str();
        }

        element = Parser(source).parseRootElement();
        assert(GetName(element, "gpx") == false); //asserts whether the getName() function in the element object returns "gpx" or not
        assert(ContainsSubElement(element, "rte") == false); //asserts whether the element object contains the "rte" sub element or not

        element = element.getSubElement("rte");

        assert(ContainsSubElement(element, "rtept") == false);
        totalSubElements = element.countSubElements("rtept"); //returns the number of sub elements instances in the element object
        element1 = element.getSubElement("rtept");

        assert(ContainsAttribute(element1, LatAttributeString, LongAttributeString) == false);
        latitude = element1.getAttribute(LatAttributeString);
        longitude = element1.getAttribute(LongAttributeString);

        if (element1.containsSubElement("ele")) { //if the element object contains the sub element, then the data will be parsed and then added to the parsedResult vector
            element2 = element1.getSubElement("ele");
            elevation = element2.getLeafContent();
            Position startPos = Position(latitude,longitude,elevation);
            parsedResult.push_back({startPos,""});
            positionsStream << "Position added: " << endl;
            ++num;
        } else { //if not, then the position will be pushed back to the parsedResult vector straight away
            Position startPos = Position(latitude,longitude);
            parsedResult.push_back({startPos,""});
            positionsStream << "Position added: " << endl;
            ++num;
        }
        if (element1.containsSubElement("name")) {
            element2 = element1.getSubElement("name");
            name = element2.getLeafContent();
            firstCharNotDelimiterIndex = name.find_first_not_of(Delimiter);
            lastCharNotDelimiterIndex = name.find_last_not_of(Delimiter);
            name = (firstCharNotDelimiterIndex == -1) ? "" : name.substr(firstCharNotDelimiterIndex,lastCharNotDelimiterIndex-firstCharNotDelimiterIndex+1);
        } else {
            name = ""; // Assigning the name variable as an empty string fixes a bug
        }
        parsedResult.front().name = name;
        prevPos = parsedResult.back().position, nextPos = parsedResult.back().position;

        while (num < totalSubElements) { //iterates through the counted sub elements, parsing the next position
            element1 = element.getSubElement("rtept",num);
            assert(ContainsAttribute(element1, LatAttributeString, LongAttributeString) == false);

            latitude = element1.getAttribute(LatAttributeString);
            longitude = element1.getAttribute(LongAttributeString);
            if (element1.containsSubElement("ele")) {
                element2 = element1.getSubElement("ele");
                elevation = element2.getLeafContent();
                nextPos = Position(latitude,longitude,elevation);
            } else {
                nextPos = Position(latitude,longitude);
            }
            if (element1.containsSubElement("name")) {
                element2 = element1.getSubElement("name");
                name = element2.getLeafContent();
                firstCharNotDelimiterIndex = name.find_first_not_of(Delimiter); //returns the index of the first character in the name that is not a delimiter character
                lastCharNotDelimiterIndex = name.find_last_not_of(Delimiter); //returns the index of the last character in the name that is not a delimiter character
                name = (firstCharNotDelimiterIndex == -1) ? "" : name.substr(firstCharNotDelimiterIndex,lastCharNotDelimiterIndex-firstCharNotDelimiterIndex+1); //parses the name out of the input between the delimiter character indexes
            } else {
                name = ""; // Assigning the name variable as an empty string fixes a bug
            }
            parsedResult.push_back({nextPos,name});
            positionsStream << "Position added: " << endl;
            ++num;
            prevPos = nextPos;
        }
        positionsStream << num << " positions added." << endl;

    } catch (std::string error) { //if any errors are thrown during the function, then the error is caught and handled
        throw std::domain_error(error);
    }
    return parsedResult; //returns the vector of parsed points
}

std::vector<GPS::TrackPoint> GPX::ParseData::parseTrack() { //Parses GPX data containing a track
    int total, skipped=0; //Lines 150-155 declare the variables to be used in this function
    string latitude, longitude, elevation, name, time, lineFromFile;
    const string trksegString = "trkseg"; //constant variables of sub elements to be retrieved and counted from the element object (Lines 152-155)
    const string trkString = "trk";
    const string trkptString = "trkpt";
    const char Delimiter = ' ';

    tm timeStruct;  //struct for time
    ostringstream positionStream, fileReadStream;
    istringstream timeStream;
    std::vector<TrackPoint> parsedResult;

    Element element = SelfClosingElement("",{}), element1 = element, element2 = element, element3 = element; // Declares the element object and assigns values to the attributes
    Position startPos(0,0), prevPos = startPos, nextPos = startPos; // Declares the Position object and assigns values to the attributes
    try { //tries the run the following code, any errors will be caught in the catch
        if (isFileName == true) {
            ifstream fs(source);
            if (! fs.good()) {
                throw invalid_argument("Error opening source file '" + source + "'.");
            }
            positionStream << "Source file '" << source << "' opened okay." << endl;

            while (! fs.eof()) {
                getline(fs, lineFromFile);
                fileReadStream << lineFromFile << endl;
            }
            source = fileReadStream.str();
        }

        element = Parser(source).parseRootElement();

        assert(GetName(element, "gpx") == false);
        assert(ContainsSubElement(element, trkString) == false);

        element = element.getSubElement(trkString);
        if (! element.containsSubElement(trksegString)) { //if element does not contain the string, then it will parse the data and adds it to the position stream object
            assert(ContainsSubElement(element, trkptString) == false);

            total = element.countSubElements(trkptString);
            element1 = element.getSubElement(trkptString);
            assert(ContainsAttribute(element1, LatAttributeString, LongAttributeString) == false);

            latitude = element1.getAttribute(LatAttributeString);
            longitude = element1.getAttribute(LongAttributeString);
            if (element1.containsSubElement("ele")) {
                element2 = element1.getSubElement("ele");
                elevation = element2.getLeafContent();

                startPos = Position(latitude,longitude,elevation);
                parsedResult.push_back({startPos,name,timeStruct});
                positionStream << "Position added: " << endl;
                ++num;
            } else {
                startPos = Position(latitude,longitude);
                parsedResult.push_back({startPos,name,timeStruct});
                positionStream << "Position added: " << endl;
                ++num;
            }
            if (element1.containsSubElement("name")) {
                element2 = element1.getSubElement("name");
                name = element2.getLeafContent();

                firstCharNotDelimiterIndex = name.find_first_not_of(Delimiter);
                lastCharNotDelimiterIndex = name.find_last_not_of(Delimiter);
                name = (firstCharNotDelimiterIndex == -1) ? "" : name.substr(firstCharNotDelimiterIndex,lastCharNotDelimiterIndex-firstCharNotDelimiterIndex+1);
            } else {
                name = ""; // Assigning the name variable to an empty string fixes bug
            }

            parsedResult.back().name = name;
            assert(ContainsSubElement(element1, "time") == false);

            element2 = element1.getSubElement("time");
            time = element2.getLeafContent();

            timeStream.str(time);
            timeStream >> std::get_time(&timeStruct,"%Y-%m-%dT%H:%M:%SZ"); //reads out time data from time stream and changes the datetime format
            if (timeStream.fail()) {
                throw std::domain_error("Malformed date/time content: " + time); //throws an error if the datetime data cannot be formatted correctly
            }

            parsedResult.back().dateTime = timeStruct; //adds the timeStruct object to the parsedResult vector
            prevPos = parsedResult.back().position, nextPos = parsedResult.back().position;

            while (num+skipped < total) {
                element1 = element.getSubElement(trkptString,num+skipped);
                assert(ContainsAttribute(element1, LatAttributeString, LongAttributeString) == false);

                latitude = element1.getAttribute(LatAttributeString);
                longitude = element1.getAttribute(LongAttributeString);
                if (element1.containsSubElement("ele")) {
                    element2 = element1.getSubElement("ele");
                    elevation = element2.getLeafContent();

                    nextPos = Position(latitude,longitude,elevation);
                } else {
                    nextPos = Position(latitude,longitude);
                }

                assert(ContainsSubElement(element1, "time") == false); //asserts whether the "time" sub-element is in the element object or not

                element2 = element1.getSubElement("time");
                time = element2.getLeafContent();

                timeStream.str(time);
                timeStream >> std::get_time(&timeStruct,"%Y-%m-%dT%H:%M:%SZ");
                if (timeStream.fail()) {
                    throw std::domain_error("Malformed date/time content: " + time); //throws an error if the datetime data cannot be formatted correctly
                }
                if (element1.containsSubElement("name")) {
                    element2 = element1.getSubElement("name");
                    name = element2.getLeafContent();

                    firstCharNotDelimiterIndex = name.find_first_not_of(Delimiter); //returns the index of the first instance of a delimiter character
                    lastCharNotDelimiterIndex = name.find_last_not_of(Delimiter); //returns the index of the last instance of a delimiter character
                    name = (firstCharNotDelimiterIndex == -1) ? "" : name.substr(firstCharNotDelimiterIndex,lastCharNotDelimiterIndex-firstCharNotDelimiterIndex+1); //parses the name from the delimiter indexes
                } else {
                    name = ""; // Assigning the name variable to an empty string fixes bug
                }

                parsedResult.push_back({nextPos,name,timeStruct}); //pushes the next position, the name and the time data struct to the parsedResult vector

                positionStream << "Position added: " << endl; //adds text to string stream object
                positionStream << " at time: " << std::put_time(&timeStruct,"%c") << endl;
                ++num;
                prevPos = nextPos;
            }
        } else {
            unsigned int segmentNum;

            for (segmentNum=0; segmentNum < element.countSubElements(trksegString); ++segmentNum) { //iterates through the counted sub-elements, parsing data and pushing it to the parsedResult vector
                element3 = element.getSubElement(trksegString, segmentNum);
                assert(ContainsSubElement(element3, trkptString) == false);

                total = element3.countSubElements(trkptString);
                skipped = -num; // Setting skipped to start at -num (rather than 0) cancels any points accumulated from previous segments
                // We have to set it here, rather than just before the loop, because num may increment in the next if-statement
                if (segmentNum == 0) {
                    element1 = element3.getSubElement(trkptString);
                    assert(ContainsAttribute(element1, LatAttributeString, LongAttributeString) == false);

                    latitude = element1.getAttribute("lat");
                    longitude = element1.getAttribute("lon");
                    if (element1.containsSubElement("ele")) {
                        element2 = element1.getSubElement("ele");
                        elevation = element2.getLeafContent();

                        startPos = Position(latitude,longitude,elevation);
                        parsedResult.push_back({startPos,name,timeStruct});
                        positionStream << "Position added: " << endl;
                        ++num;
                    }
                    else {
                        startPos = Position(latitude,longitude);
                        parsedResult.push_back({startPos,name,timeStruct});
                        positionStream << "Position added: " << endl;
                        ++num;
                    }
                    if (element1.containsSubElement("name")) {
                        element2 = element1.getSubElement("name");
                        name = element2.getLeafContent();

                        firstCharNotDelimiterIndex = name.find_first_not_of(Delimiter);
                        lastCharNotDelimiterIndex = name.find_last_not_of(Delimiter);
                        name = (firstCharNotDelimiterIndex == -1) ? "" : name.substr(firstCharNotDelimiterIndex,lastCharNotDelimiterIndex-firstCharNotDelimiterIndex+1);
                    } else {
                        name = ""; // Assigning the name variable to an empty string fixes bug
                    }

                    parsedResult.back().name = name;
                    assert(ContainsSubElement(element1, "time") == false);

                    element2 = element1.getSubElement("time");
                    time = element2.getLeafContent();

                    timeStream.str(time);
                    timeStream >> std::get_time(&timeStruct,"%Y-%m-%dT%H:%M:%SZ");
                    if (timeStream.fail()) {
                        throw std::domain_error("Malformed date/time content: " + time);
                    }
                    parsedResult.back().dateTime = timeStruct;
                }
                prevPos = parsedResult.back().position, nextPos = parsedResult.back().position;

                int numSkippedTotal;
                while (num+skipped < total) {
                    numSkippedTotal = num + skipped;
                    element1 = element3.getSubElement(trkptString,numSkippedTotal);
                    assert(ContainsAttribute(element1, LatAttributeString, LongAttributeString) == false);

                    latitude = element1.getAttribute("lat");
                    longitude = element1.getAttribute("lon");
                    if (element1.containsSubElement("ele")) {
                        element2 = element1.getSubElement("ele");
                        elevation = element2.getLeafContent();

                        nextPos = Position(latitude,longitude,elevation);
                    } else {
                        nextPos = Position(latitude,longitude);
                    }

                    assert(ContainsSubElement(element1, "time") == false);

                    element2 = element1.getSubElement("time");
                    time = element2.getLeafContent();

                    timeStream.str(time);
                    timeStream >> std::get_time(&timeStruct,"%Y-%m-%dT%H:%M:%SZ");

                    if (timeStream.fail()) {
                        throw std::domain_error("Malformed date/time content: " + time);
                    }
                    if (element1.containsSubElement("name")) {
                        element2 = element1.getSubElement("name");
                        name = element2.getLeafContent();

                        firstCharNotDelimiterIndex = name.find_first_not_of(Delimiter);
                        lastCharNotDelimiterIndex = name.find_last_not_of(Delimiter);
                        name = (firstCharNotDelimiterIndex == -1) ? "" : name.substr(firstCharNotDelimiterIndex,lastCharNotDelimiterIndex-firstCharNotDelimiterIndex+1);
                    } else {
                        name = ""; // Assigning the name variable to an empty string fixes bug
                    }
                    parsedResult.push_back({nextPos, name, timeStruct});
                    positionStream << "Position added: " << endl;
                    positionStream << " at time: " << std::put_time(&timeStruct,"%c") << endl;
                    ++num;
                    prevPos = nextPos;
                }
            }
        }
        positionStream << num << " positions added." << endl;
        return parsedResult; //returns the parsedResult vector from the function

    } catch (std::string error) { //catches and handles any errors that were thrown in the function
        throw std::domain_error(error);
    }
}
