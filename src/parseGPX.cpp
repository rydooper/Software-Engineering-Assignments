#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "xml/parser.h"
#include "parseGPX.h"
#include <assert.h>
using namespace std;
using namespace GPS;
using namespace XML;

GPX::ParseData::ParseData(std::string source, bool isFileName) {
    //assert(source != "");
    this->source = source;
    this->isFileName = isFileName;
}

bool GPX::ParseData::ContainsAttribute(XML::Element element) {
    bool exceptionThrown = false;
    if (! element.containsAttribute("lat")) {
        throw domain_error("Missing 'lat' attribute.");
        exceptionThrown = true;
    }
    if (! element.containsAttribute("lon")) {
        throw domain_error("Missing 'lon' attribute.");
        exceptionThrown = true;
    }
    return exceptionThrown;
}

bool GPX::ParseData::ContainsSubElement(XML::Element element, string inputString) {
    bool exceptionThrown = false;
    if (! element.containsSubElement(inputString)) {
        throw domain_error("Missing '" + inputString + "' element.");
        exceptionThrown = true;
    }
    return exceptionThrown;
}

bool GPX::ParseData::GetName(XML::Element element, string inputString) {
    bool exceptionThrown = false;
    if (element.getName() != inputString) {
        throw domain_error("Missing '" + inputString + "' element.");
        exceptionThrown = true;
    }
    return exceptionThrown;
}

std::vector<GPS::RoutePoint> GPX::ParseData::parseRoute() {
    int num=0, firstCharNotSpace, lastCharNotSpace, totalSubElements;
    string latitude, longitude, elevation, name, lineFromFile;
    ostringstream positionsStream, fileReadStream;
    std::vector<RoutePoint> parsedResult;

    Element element = SelfClosingElement("",{}), element1 = element, element2 = element; // Work-around because there's no public constructor in Element.
    Position startPos(0,0), prevPos = startPos, nextPos = startPos; // Same thing but for Position.
    try {
        if (isFileName == true) { //checks if the file is a file name
            ifstream fs(source);
            if (!fs.good()) {
                throw invalid_argument("Error opening source file '" + source + "'.");
            }
            while (!fs.eof()) {
                getline(fs, lineFromFile);
                fileReadStream << lineFromFile << endl;
            }
            source = fileReadStream.str();
        }

        element = Parser(source).parseRootElement();
        assert(GetName(element, "gpx") == false);
        assert(ContainsSubElement(element, "rte") == false);

        element = element.getSubElement("rte");

        assert(ContainsSubElement(element, "rtept") == false);
        totalSubElements = element.countSubElements("rtept");
        element1 = element.getSubElement("rtept");

        bool exceptionTest = GPX::ParseData::ContainsAttribute(element1);
        assert(exceptionTest == false);
        latitude = element1.getAttribute("lat");
        longitude = element1.getAttribute("lon");

        if (element1.containsSubElement("ele")) {
            element2 = element1.getSubElement("ele");
            elevation = element2.getLeafContent();
            Position startPos = Position(latitude,longitude,elevation);
            parsedResult.push_back({startPos,""});
            positionsStream << "Position added: " << endl;
            ++num;
        } else {
            Position startPos = Position(latitude,longitude);
            parsedResult.push_back({startPos,""});
            positionsStream << "Position added: " << endl;
            ++num;
        }
        if (element1.containsSubElement("name")) {
            element2 = element1.getSubElement("name");
            name = element2.getLeafContent();
            firstCharNotSpace = name.find_first_not_of(' ');
            lastCharNotSpace = name.find_last_not_of(' ');
            name = (firstCharNotSpace == -1) ? "" : name.substr(firstCharNotSpace,lastCharNotSpace-firstCharNotSpace+1);
        } else {
            name = ""; // Fixed bug by adding this.
        }
        parsedResult.front().name = name;
        prevPos = parsedResult.back().position, nextPos = parsedResult.back().position;

        while (num < totalSubElements) {        //skipped removed as it didn't change in while and added 0 to num (making it redundant)
            element1 = element.getSubElement("rtept",num);
            bool exceptionTest = GPX::ParseData::ContainsAttribute(element1);
            assert(exceptionTest == false);

            latitude = element1.getAttribute("lat");
            longitude = element1.getAttribute("lon");
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
                firstCharNotSpace = name.find_first_not_of(' ');
                lastCharNotSpace = name.find_last_not_of(' ');
                name = (firstCharNotSpace == -1) ? "" : name.substr(firstCharNotSpace,lastCharNotSpace-firstCharNotSpace+1);
            } else {
                name = ""; // Fixed bug by adding this.
            }
            parsedResult.push_back({nextPos,name});
            positionsStream << "Position added: " << endl;
            ++num;
            prevPos = nextPos;
        }
        positionsStream << num << " positions added." << endl;

    } catch (std::string error) {
        throw std::domain_error(error);
    }
    return parsedResult;
}

std::vector<GPS::TrackPoint> GPX::ParseData::parseTrack() {
    int num=0, firstCharNotSpace, lastCharNotSpace, total, skipped=0;
    string latitude, longitude, elevation, name, time, lineFromFile;
    const string trksegString = "trkseg";
    const string trkString = "trk";
    const string trkptString = "trkpt";

    tm timeStruct;  //struct for time
    ostringstream positionStream, fileReadStream;
    istringstream timeStream;
    std::vector<TrackPoint> parsedResult;

    Element element = SelfClosingElement("",{}), element1 = element, element2 = element, element3 = element; // Work-around because there's no public constructor in Element.
    Position startPos(0,0), prevPos = startPos, nextPos = startPos; // Same thing but for Position.
    try {
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
        if (! element.containsSubElement(trksegString)) {
            assert(ContainsSubElement(element, trkptString) == false);

            total = element.countSubElements(trkptString);
            element1 = element.getSubElement(trkptString);
            bool exceptionTest = GPX::ParseData::ContainsAttribute(element1);
            assert(exceptionTest == false);

            latitude = element1.getAttribute("lat");
            longitude = element1.getAttribute("lon");
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

                firstCharNotSpace = name.find_first_not_of(' ');
                lastCharNotSpace = name.find_last_not_of(' ');
                name = (firstCharNotSpace == -1) ? "" : name.substr(firstCharNotSpace,lastCharNotSpace-firstCharNotSpace+1);
            } else {
                name = ""; // Fixed bug by adding this.
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
            prevPos = parsedResult.back().position, nextPos = parsedResult.back().position;

            while (num+skipped < total) {
                element1 = element.getSubElement(trkptString,num+skipped);
                bool exceptionTest = GPX::ParseData::ContainsAttribute(element1);
                assert(exceptionTest == false);

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

                    firstCharNotSpace = name.find_first_not_of(' ');
                    lastCharNotSpace = name.find_last_not_of(' ');
                    name = (firstCharNotSpace == -1) ? "" : name.substr(firstCharNotSpace,lastCharNotSpace-firstCharNotSpace+1);
                } else {
                    name = ""; // Fixed bug by adding this.
                }

                parsedResult.push_back({nextPos,name,timeStruct});

                positionStream << "Position added: " << endl;
                positionStream << " at time: " << std::put_time(&timeStruct,"%c") << endl;
                ++num;
                prevPos = nextPos;
            }
        } else {
            unsigned int segmentNum;

            for (segmentNum=0; segmentNum < element.countSubElements(trksegString); ++segmentNum) {
                element3 = element.getSubElement(trksegString, segmentNum);
                assert(ContainsSubElement(element3, trkptString) == false);

                total = element3.countSubElements(trkptString);
                skipped = -num; // Setting skipped to start at -num (rather than 0) cancels any points accumulated from previous segments
                // We have to set it here, rather than just before the loop, because num may increment in the next if-statement
                if (segmentNum == 0) {
                    element1 = element3.getSubElement(trkptString);
                    bool exceptionTest = GPX::ParseData::ContainsAttribute(element1);
                    assert(exceptionTest == false);

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

                        firstCharNotSpace = name.find_first_not_of(' ');
                        lastCharNotSpace = name.find_last_not_of(' ');
                        name = (firstCharNotSpace == -1) ? "" : name.substr(firstCharNotSpace,lastCharNotSpace-firstCharNotSpace+1);
                    } else {
                        name = ""; // Fixed bug by adding this.
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
                    bool exceptionTest = GPX::ParseData::ContainsAttribute(element1);
                    assert(exceptionTest == false);

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

                        firstCharNotSpace = name.find_first_not_of(' ');
                        lastCharNotSpace = name.find_last_not_of(' ');
                        name = (firstCharNotSpace == -1) ? "" : name.substr(firstCharNotSpace,lastCharNotSpace-firstCharNotSpace+1);
                    } else {
                        name = ""; // Fixed bug by adding this.
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
        return parsedResult;
    } catch (std::string error) {
        throw std::domain_error(error);
    }
}
