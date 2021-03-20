#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "xml/parser.h"
#include "parseGPX.h"
using namespace std;
using namespace GPS;
using namespace XML;

namespace GPX
{
void ContainsAttribute(Element element) {
    if (! element.containsAttribute("lat")) {
        throw domain_error("Missing 'lat' attribute.");
    }
    if (! element.containsAttribute("lon")) {
        throw domain_error("Missing 'lon' attribute.");
    }
}

//magic literals = changing for loops from numbers to named variables
std::vector<GPS::RoutePoint> parseRoute(std::string source, bool isFileName) {
    int num=0, firstCharNotSpace, lastCharNotSpace, totalSubElements, skipped=0;
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
        if (element.getName() != "gpx") {
            throw domain_error("Missing 'gpx' element.");
        }
        if (! element.containsSubElement("rte")) {
            throw domain_error("Missing 'rte' element.");
        }
        element = element.getSubElement("rte");

        if (! element.containsSubElement("rtept")) {
            throw domain_error("Missing 'rtept' element.");
        }
        totalSubElements = element.countSubElements("rtept");
        element1 = element.getSubElement("rtept");

        ContainsAttribute(element1);
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

        while (num+skipped < totalSubElements) {
            element1 = element.getSubElement("rtept",num+skipped);
            ContainsAttribute(element1);

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
std::vector<GPS::TrackPoint> parseTrack(std::string source, bool isFileName) {
    int num=0,firstCharNotSpace,lastCharNotSpace,total,skipped;
    string lat,lon,el,name,time;
    tm t;
    ostringstream oss,oss2;
    istringstream iss;
    std::vector<TrackPoint> result;
    Element ele = SelfClosingElement("",{}), temp = ele, temp2 = ele, ele2 = ele; // Work-around because there's no public constructor in Element.
    Position startPos(0,0), prevPos = startPos, nextPos = startPos; // Same thing but for Position.
    if (isFileName) {
        ifstream fs(source);
        if (! fs.good()) {
            throw invalid_argument("Error opening source file '" + source + "'.");
        }
        oss << "Source file '" << source << "' opened okay." << endl;
        while (fs.good()) {
            getline(fs, name); // Using name as temporary variable as we don't need it until later
            oss2 << name << endl;
        }
        source = oss2.str();
    }

    ele = Parser(source).parseRootElement();
    if (ele.getName() != "gpx") {
        throw domain_error("Missing 'gpx' element.");
    }
    if (! ele.containsSubElement("trk")) {
        throw domain_error("Missing 'trk' element.");
    }

    ele = ele.getSubElement("trk");
    if (! ele.containsSubElement("trkseg")) {
        if (! ele.containsSubElement("trkpt")) {
            throw domain_error("Missing 'trkpt' element.");
        }
        total = ele.countSubElements("trkpt");
        temp = ele.getSubElement("trkpt");
        //ContainsAttribute(temp);
        if (! temp.containsAttribute("lat")) {
            throw domain_error("Missing 'lat' attribute.");
        }
        if (! temp.containsAttribute("lon")) {
            throw domain_error("Missing 'lon' attribute.");
        }
        lat = temp.getAttribute("lat");
        lon = temp.getAttribute("lon");
        if (temp.containsSubElement("ele")) {
            temp2 = temp.getSubElement("ele");
            el = temp2.getLeafContent();
            startPos = Position(lat,lon,el);
            result.push_back({startPos,name,t});
            oss << "Position added: " << endl; // << startPos.toString() << endl; // Need to update since removing toString()
            ++num;
        } else {
            startPos = Position(lat,lon);
            result.push_back({startPos,name,t});
            oss << "Position added: " << endl; // << startPos.toString() << endl; // Need to update since removing toString()
            ++num;
        }
        if (temp.containsSubElement("name")) {
            temp2 = temp.getSubElement("name");
            name = temp2.getLeafContent();
            firstCharNotSpace = name.find_first_not_of(' ');
            lastCharNotSpace = name.find_last_not_of(' ');
            name = (firstCharNotSpace == -1) ? "" : name.substr(firstCharNotSpace,lastCharNotSpace-firstCharNotSpace+1);
        } else {
            name = ""; // Fixed bug by adding this.
        }
        result.back().name = name;
        if (! temp.containsSubElement("time")) {
            throw domain_error("Missing 'time' element.");
        }
        temp2 = temp.getSubElement("time");
        time = temp2.getLeafContent();
        iss.str(time);
        iss >> std::get_time(&t,"%Y-%m-%dT%H:%M:%SZ");
        if (iss.fail()) {
            throw std::domain_error("Malformed date/time content: " + time);
        }
        result.back().dateTime = t;
        prevPos = result.back().position, nextPos = result.back().position;
        skipped = 0;

        while (num+skipped < total) {
            temp = ele.getSubElement("trkpt",num+skipped);
            if (! temp.containsAttribute("lat")) {
                throw domain_error("Missing 'lat' attribute.");
            }
            if (! temp.containsAttribute("lon")) {
                throw domain_error("Missing 'lon' attribute.");
            }
            lat = temp.getAttribute("lat");
            lon = temp.getAttribute("lon");
            if (temp.containsSubElement("ele")) {
                temp2 = temp.getSubElement("ele");
                el = temp2.getLeafContent();
                nextPos = Position(lat,lon,el);
            } else {
                nextPos = Position(lat,lon);
            }
            if (! temp.containsSubElement("time")) {
                throw domain_error("Missing 'time' element.");
            }
            temp2 = temp.getSubElement("time");
            time = temp2.getLeafContent();
            iss.str(time);
            iss >> std::get_time(&t,"%Y-%m-%dT%H:%M:%SZ");
            if (iss.fail()) {
                throw std::domain_error("Malformed date/time content: " + time);
            }
            if (temp.containsSubElement("name")) {
                temp2 = temp.getSubElement("name");
                name = temp2.getLeafContent();
                firstCharNotSpace = name.find_first_not_of(' ');
                lastCharNotSpace = name.find_last_not_of(' ');
                name = (firstCharNotSpace == -1) ? "" : name.substr(firstCharNotSpace,lastCharNotSpace-firstCharNotSpace+1);
            } else {
                name = ""; // Fixed bug by adding this.
            }
            result.push_back({nextPos,name,t});
            oss << "Position added: " << endl; // << nextPos.toString() << endl; // Need to update since removing toString()
            oss << " at time: " << std::put_time(&t,"%c") << endl;
            ++num;
            prevPos = nextPos;
        }
    }
    else {
        for (unsigned int segNum = 0; segNum < ele.countSubElements("trkseg"); ++segNum) {
            ele2 = ele.getSubElement("trkseg",segNum);
            if (! ele2.containsSubElement("trkpt")) {
                throw domain_error("Missing 'trkpt' element.");
            }
            total = ele2.countSubElements("trkpt");
            skipped = -num; // Setting skipped to start at -num (rather than 0) cancels any points accumulated from previous segments
            // We have to set it here, rather than just before the loop, because num may increment in the next if-statement
            if (segNum == 0) {
                temp = ele2.getSubElement("trkpt");
                if (! temp.containsAttribute("lat")) {
                    throw domain_error("Missing 'lat' attribute.");
                }
                if (! temp.containsAttribute("lon")) {
                    throw domain_error("Missing 'lon' attribute.");
                }
                lat = temp.getAttribute("lat");
                lon = temp.getAttribute("lon");
                if (temp.containsSubElement("ele")) {
                    temp2 = temp.getSubElement("ele");
                    el = temp2.getLeafContent();
                    startPos = Position(lat,lon,el);
                    result.push_back({startPos,name,t});
                    oss << "Position added: " << endl; // << startPos.toString() << endl; // Need to update since removing toString()
                    ++num;
                }
                else {
                    startPos = Position(lat,lon);
                    result.push_back({startPos,name,t});
                    oss << "Position added: " << endl; // << startPos.toString() << endl; // Need to update since removing toString()
                    ++num;
                }
                if (temp.containsSubElement("name")) {
                    temp2 = temp.getSubElement("name");
                    name = temp2.getLeafContent();
                    firstCharNotSpace = name.find_first_not_of(' ');
                    lastCharNotSpace = name.find_last_not_of(' ');
                    name = (firstCharNotSpace == -1) ? "" : name.substr(firstCharNotSpace,lastCharNotSpace-firstCharNotSpace+1);
                } else {
                    name = ""; // Fixed bug by adding this.
                }
                result.back().name = name;
                if (! temp.containsSubElement("time")) {
                    throw domain_error("Missing 'time' element.");
                }
                temp2 = temp.getSubElement("time");
                time = temp2.getLeafContent();
                iss.str(time);
                iss >> std::get_time(&t,"%Y-%m-%dT%H:%M:%SZ");
                if (iss.fail()) {
                    throw std::domain_error("Malformed date/time content: " + time);
                }
                result.back().dateTime = t;
            }
            prevPos = result.back().position, nextPos = result.back().position;
            while (num+skipped < total) {
                temp = ele2.getSubElement("trkpt",num+skipped);
                if (! temp.containsAttribute("lat")) {
                    throw domain_error("Missing 'lat' attribute.");
                }
                if (! temp.containsAttribute("lon")) {
                    throw domain_error("Missing 'lon' attribute.");
                }
                lat = temp.getAttribute("lat");
                lon = temp.getAttribute("lon");
                if (temp.containsSubElement("ele")) {
                    temp2 = temp.getSubElement("ele");
                    el = temp2.getLeafContent();
                    nextPos = Position(lat,lon,el);
                } else {
                    nextPos = Position(lat,lon);
                }

                if (! temp.containsSubElement("time")) {
                    throw domain_error("Missing 'time' element.");
                }
                temp2 = temp.getSubElement("time");
                time = temp2.getLeafContent();
                iss.str(time);
                iss >> std::get_time(&t,"%Y-%m-%dT%H:%M:%SZ");
                if (iss.fail()) throw std::domain_error("Malformed date/time content: " + time);
                if (temp.containsSubElement("name")) {
                    temp2 = temp.getSubElement("name");
                    name = temp2.getLeafContent();
                    firstCharNotSpace = name.find_first_not_of(' ');
                    lastCharNotSpace = name.find_last_not_of(' ');
                    name = (firstCharNotSpace == -1) ? "" : name.substr(firstCharNotSpace,lastCharNotSpace-firstCharNotSpace+1);
                } else {
                    name = ""; // Fixed bug by adding this.
                }
                result.push_back({nextPos,name,t});
                oss << "Position added: " << endl; // << nextPos.toString() << endl; // Need to update since removing toString()
                oss << " at time: " << std::put_time(&t,"%c") << endl;
                ++num;
                prevPos = nextPos;
            }
        }
    }
    oss << num << " positions added." << endl;
    return result;
}
}
