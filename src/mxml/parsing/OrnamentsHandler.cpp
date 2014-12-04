//  Created by Alejandro Isaza on 2014-06-03.
//  Copyright (c) 2014 Venture Media Labs. All rights reserved.

#include "OrnamentsHandler.h"

namespace mxml {

using dom::Ornaments;
using lxml::QName;

static const char* kTrillMarkTag = "trill-mark";
static const char* kMordentTag = "mordent";
static const char* kInvertedMordentTag = "inverted-mordent";
static const char* kTurnTag = "turn";
static const char* kInvertedTurnTag = "invertedTurn";

void OrnamentsHandler::startElement(const lxml::QName& qname, const AttributeMap& attributes) {
    _result = Ornaments();
}

lxml::RecursiveHandler* OrnamentsHandler::startSubElement(const lxml::QName& qname) {
    if (strcmp(qname.localName(), kTrillMarkTag) == 0)
        return &_emptyPlacementHandler;
    else if (strcmp(qname.localName(), kMordentTag) == 0)
        return &_mordentHandler;
    else if (strcmp(qname.localName(), kInvertedMordentTag) == 0)
        return &_mordentHandler;
    else if (strcmp(qname.localName(), kTurnTag) == 0)
        return &_turnHandler;
    else if (strcmp(qname.localName(), kInvertedTurnTag) == 0)
        return &_turnHandler;
    return 0;
}

void OrnamentsHandler::endSubElement(const lxml::QName& qname, RecursiveHandler* parser) {
    if (strcmp(qname.localName(), kTrillMarkTag) == 0)
        _result.setTrillMark(presentOptional(_emptyPlacementHandler.result()));
    else if (strcmp(qname.localName(), kMordentTag) == 0)
       _result.setMordent(presentOptional(_mordentHandler.result()));
    else if (strcmp(qname.localName(), kInvertedMordentTag) == 0)
        _result.setInvertedMordent(presentOptional(_mordentHandler.result()));
    else if (strcmp(qname.localName(), kTurnTag) == 0)
        _result.setTurn(presentOptional(_turnHandler.result()));
    else if (strcmp(qname.localName(), kInvertedTurnTag) == 0)
        _result.setInvertedTurn(presentOptional(_turnHandler.result()));
}

} // namespace