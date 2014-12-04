//  Created by Alejandro Isaza on 2014-06-26.
//  Copyright (c) 2014 Venture Media Labs. All rights reserved.

#include "PartGeometry.h"
#include "ScoreGeometry.h"
#include <mxml/SpanFactory.h>

namespace mxml {

ScoreGeometry::ScoreGeometry(const dom::Score& score, bool naturalSpacing) : _score(score) {
    SpanFactory spanFactory(_score, naturalSpacing);
    _spans = spanFactory.build();

    coord_t offset = 0;
    for (auto& part : _score.parts()) {
        std::unique_ptr<PartGeometry> geom(new PartGeometry(*part, _spans));
        geom->setHorizontalAnchorPointValues(0, 0);
        geom->setVerticalAnchorPointValues(0, 0);
        geom->setLocation({0, offset});
        offset += geom->size().height;
        addGeometry(std::move(geom));
    }
    
    setBounds(subGeometriesFrame());
}

} // namespace