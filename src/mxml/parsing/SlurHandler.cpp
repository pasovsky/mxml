//  Created by Alejandro Isaza on 2014-04-23.
//  Copyright (c) 2014 Venture Media Labs. All rights reserved.

#include "SlurHandler.h"
#include <lxml/IntegerHandler.h>
#include <mxml/dom/InvalidDataError.h>

namespace mxml {

    using lxml::QName;

static const char* kNumberAttribute = "number";
static const char* kTypeAttribute = "type";
static const char* kPlacementAttribute = "placement";
static const char* kOrientationAttribute = "orientation";

void SlurHandler::startElement(const QName& qname, const AttributeMap& attributes) {
    auto number = attributes.find(kNumberAttribute);
    if (number != attributes.end())
        _result.setNumber(lxml::IntegerHandler::parseInteger(number->second));
    
    auto type = attributes.find(kTypeAttribute);
    if (type != attributes.end())
        _result.setType(typeFromString(type->second));
    
    auto placement = attributes.find(kPlacementAttribute);
    if (placement != attributes.end())
        _result.setPlacement(placementFromString(placement->second));
    
    auto orientation = attributes.find(kOrientationAttribute);
    if (orientation != attributes.end())
        _result.setOrientation(orientationFromString(orientation->second));
}

dom::StartStopContinue SlurHandler::typeFromString(const std::string& string) {
    if (string == "start")
        return dom::TYPE_START;
    else if (string == "continue")
        return dom::TYPE_CONTINUE;
    else if (string == "stop")
        return dom::TYPE_STOP;
    throw dom::InvalidDataError("Invalid slur type " + string);
}

dom::Placement SlurHandler::placementFromString(const std::string& string) {
    if (string == "above")
        return dom::PLACEMENT_ABOVE;
    else if (string == "below")
        return dom::PLACEMENT_BELOW;
    throw dom::InvalidDataError("Invalid slur placement " + string);
}

dom::Orientation SlurHandler::orientationFromString(const std::string& string) {
    if (string == "over")
        return dom::ORIENTATION_OVER;
    else if (string == "under")
        return dom::ORIENTATION_UNDER;
    throw dom::InvalidDataError("Invalid slur orientation " + string);
}

} // namespace mxml