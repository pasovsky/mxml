//  Created by Steve Hosking on 2015-01-20.
//  Copyright (c) 2015 Venture Media Labs. All rights reserved.

#include "VerticalResolver.h"

#include <mxml/geometry/AccidentalGeometry.h>
#include <mxml/geometry/ArticulationGeometry.h>
#include <mxml/geometry/BeamGeometry.h>
#include <mxml/geometry/CodaGeometry.h>
#include <mxml/geometry/ClefGeometry.h>
#include <mxml/geometry/SegnoGeometry.h>
#include <mxml/geometry/DotGeometry.h>
#include <mxml/geometry/EndingGeometry.h>
#include <mxml/geometry/FermataGeometry.h>
#include <mxml/geometry/KeyGeometry.h>
#include <mxml/geometry/LyricGeometry.h>
#include <mxml/geometry/NoteGeometry.h>
#include <mxml/geometry/OctaveShiftGeometry.h>
#include <mxml/geometry/OrnamentsGeometry.h>
#include <mxml/geometry/PedalGeometry.h>
#include <mxml/geometry/RestGeometry.h>
#include <mxml/geometry/SpanDirectionGeometry.h>
#include <mxml/geometry/StemGeometry.h>
#include <mxml/geometry/TieGeometry.h>
#include <mxml/geometry/TimeSignatureGeometry.h>
#include <mxml/geometry/WordsGeometry.h>

#include <mxml/Metrics.h>

#include <typeinfo>

static const int kMaxCollisionsPerGeometry = 10;

namespace mxml {
    
    VerticalTypeComparator::VerticalTypeComparator() {
        // This is the collision priority of different geometry types. Geometries with lower numbers are less likely to
        // be moved.
        typeOrder[std::type_index(typeid(ClefGeometry))] = 0;
        typeOrder[std::type_index(typeid(KeyGeometry))] = 0;
        typeOrder[std::type_index(typeid(TimeSignatureGeometry))] = 0;
        typeOrder[std::type_index(typeid(NoteGeometry))] = 1;
        typeOrder[std::type_index(typeid(DotGeometry))] = 1;
        typeOrder[std::type_index(typeid(AccidentalGeometry))] = 1;
        typeOrder[std::type_index(typeid(StemGeometry))] = 1;
        typeOrder[std::type_index(typeid(BeamGeometry))] = 1;
        typeOrder[std::type_index(typeid(TieGeometry))] = 1;
        typeOrder[std::type_index(typeid(EndingGeometry))] = 1;
        typeOrder[std::type_index(typeid(LyricGeometry))] = 2;
        typeOrder[std::type_index(typeid(ArticulationGeometry))] = 3;
        typeOrder[std::type_index(typeid(FermataGeometry))] = 3;
        typeOrder[std::type_index(typeid(RestGeometry))] = 4;
        typeOrder[std::type_index(typeid(SpanDirectionGeometry))] = 5;
        typeOrder[std::type_index(typeid(OrnamentsGeometry))] = 6;
        typeOrder[std::type_index(typeid(OctaveShiftGeometry))] = 7;
        typeOrder[std::type_index(typeid(PedalGeometry))] = 7;
        typeOrder[std::type_index(typeid(SegnoGeometry))] = 8;
        typeOrder[std::type_index(typeid(CodaGeometry))] = 8;
        typeOrder[std::type_index(typeid(WordsGeometry))] = 9;
    }
    
    bool VerticalTypeComparator::operator()(const Geometry* g1, const Geometry* g2) {
        auto& o1 = typeOrder[std::type_index(typeid(*g1))];
        auto& o2 = typeOrder[std::type_index(typeid(*g2))];
        if (o1 < o2)
            return true;
        if (o1 > o2)
            return false;
        
        // Order geometries of the same type by size. This is because usually short geometries are more closely related
        // to one particular note, and therefore should not be pushed out.
        return g1->size().width < g2->size().width;
    }
    
    VerticalResolver::VerticalResolver(const PartGeometry& partGeometry) : CollisionResolver(partGeometry) {
    }
    
    void VerticalResolver::resolveCollision(const CollisionPair& pair) {
        if (!canResolveCollision(pair))
            return;
        
        // Put the less movable geometry first, so that we move the other one
        Geometry* first = pair.firstGeometry();
        Geometry* second = pair.secondGeometry();
        
        if (!isImmovable(pair.firstGeometry()) && isImmovable(pair.secondGeometry()))
            std::swap(first, second);
        else if (!_geometryTypeComparator(pair.firstGeometry(), pair.secondGeometry()))
            std::swap(first, second);
        
        auto noteGeometry = dynamic_cast<NoteGeometry*>(first);
        auto restGeometry = dynamic_cast<RestGeometry*>(second);
        if (noteGeometry && restGeometry)
            resolveCollision(noteGeometry, restGeometry);
        resolveCollision(first, second);
    }
    
    void VerticalResolver::resolveCollision(const Geometry* g1, Geometry* g2) {
        Rect f1 = _partGeometry.convertFromGeometry(g1->frame(), g1->parentGeometry());
        Rect f2 = _partGeometry.convertFromGeometry(g2->frame(), g2->parentGeometry());
        
        // Only move things that are already outside the staves further away
        auto& part = _partGeometry.part();
        coord_t top = Metrics::staffOrigin(part, 1) - Metrics::stavesHeight(part)/2;
        coord_t bottom = Metrics::staffOrigin(part, part.staves()) - Metrics::stavesHeight(part)/2;
        if (f2.origin.y <= top)
            f2.origin.y = f1.origin.y - f2.size.height - 1;
        else if (f2.max().y >= bottom)
            f2.origin.y = f1.origin.y + f1.size.height + 1;
        else
            return;
        
        g2->setFrame(g2->parentGeometry()->convertFromRoot(f2));
        
        removeCollisions(g2);
        readdGeometry(g2);
    }
    
    void VerticalResolver::resolveCollision(const NoteGeometry* note, RestGeometry* rest) {
        Rect noteFrame = _partGeometry.convertFromGeometry(note->frame(), note->parentGeometry());
        Rect restFrame = _partGeometry.convertFromGeometry(rest->frame(), rest->parentGeometry());
        
        // Only move things that are already outside the staves further away
        if (note->note().stem() == dom::kStemUp)
            restFrame.origin.y = noteFrame.origin.y + noteFrame.size.height + 1;
        else if (note->note().stem() == dom::kStemDown)
            restFrame.origin.y = noteFrame.origin.y - restFrame.size.height - 1;
        
        rest->setFrame(rest->parentGeometry()->convertFromRoot(restFrame));
        
        removeCollisions(rest);
        readdGeometry(rest);
    }
    
    bool VerticalResolver::isImmovable(const Geometry* geometry) const {
        return (typeid(*geometry) == typeid(ClefGeometry) ||
                typeid(*geometry) == typeid(KeyGeometry) ||
                typeid(*geometry) == typeid(TimeSignatureGeometry) ||
                typeid(*geometry) == typeid(NoteGeometry) ||
                typeid(*geometry) == typeid(DotGeometry) ||
                typeid(*geometry) == typeid(AccidentalGeometry) ||
                typeid(*geometry) == typeid(StemGeometry) ||
                typeid(*geometry) == typeid(BeamGeometry) ||
                typeid(*geometry) == typeid(TieGeometry) ||
                typeid(*geometry) == typeid(EndingGeometry) ||
                typeid(*geometry) == typeid(LyricGeometry));
    }
    
    void VerticalResolver::readdGeometry(Geometry* geometry) {
        _collisionCount[geometry] += 1;
        if (_collisionCount[geometry] > kMaxCollisionsPerGeometry)
            return;
        
        // Re-add collisions involving rest
        addAllCollisions(geometry);
    }
    
}