#ifndef PARSEGPX_H_201220
#define PARSEGPX_H_201220

#include <string>
#include <vector>

#include "xml/parser.h"
#include "points.h"

namespace GPX
{
    class ParseData { //Contains the parsing methods and attributes to be used throughout the program
        public: //the methods (functions) that will be used in the program
            ParseData(std::string source, bool isFileName, int num, int firstCharNotDelimiterIndex, int lastCharNotDelimiterIndex, std::string LatAttributeString, std::string LongAttributeString); //ParseData public construcutor to be called whenever the class is needed
            bool ContainsAttribute(XML::Element element, std::string LatAttributeString, std::string LongAttributeString); //returns whether the element object contains the input string attribute
            bool ContainsSubElement(XML::Element element, std::string inputString); //returns whether the element object contains the input string sub element
            bool GetName(XML::Element element, std::string inputString); //returns whether the data returned from the getName() function matches the input string
            std::vector<GPS::RoutePoint> parseRoute(); //Parses GPX data containing a route
            std::vector<GPS::TrackPoint> parseTrack(); //Parses GPX data containing a track

        private: //the attributes (variables) that will be used in the program
            std::string source; //The source data can be provided as a string, or from a file; which one is determined by the bool variable "isFileName"
            bool isFileName;
            int num=0;
            int firstCharNotDelimiterIndex;
            int lastCharNotDelimiterIndex;
            std::string LatAttributeString;
            std::string LongAttributeString;
    };
}

#endif
