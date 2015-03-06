//  Created by Alejandro Isaza on 2014-05-12.
//  Copyright (c) 2014 Venture Media Labs. All rights reserved.

#include "SpanDirectionGeometry.h"
#include <mxml/dom/Wedge.h>
#include <mxml/dom/Pedal.h>

#include <algorithm>
#include <cassert>

namespace mxml {

using namespace dom;

const coord_t SpanDirectionGeometry::kLineWidth = 1;

SpanDirectionGeometry::SpanDirectionGeometry(const Direction* start, const Point& startLocation, const Direction* stop, const Point& stopLocation)
: PlacementGeometry(start ? start->placement() : stop ? stop->placement() : dom::absentOptional(Placement::Above),
                    start ? start->staff() : stop ? stop->staff() : dom::absentOptional(1)),
  _startDirection(start),
  _startLocation(startLocation),
  _stopDirection(stop),
  _stopLocation(stopLocation),
  _continuation(false)
{
    const dom::DirectionType* type = nullptr;
    if (start)
        type = start->type();
    else if (stop)
        type = stop->type();

    setHorizontalAnchorPointValues(0, 0);
    
    Size size;
    size.width = stopLocation.x - startLocation.x;
    
    if (const Wedge* startWedge = dynamic_cast<const Wedge*>(type)) {
        const Wedge* stopWedge = dynamic_cast<const Wedge*>(type);
        size.height = 2*kLineWidth + std::max(startWedge->spread(), stopWedge->spread());
    }
    setSize(size);
}

} // namespace mxml
