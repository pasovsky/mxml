//  Created by Alejandro Isaza on 2014-05-05.
//  Copyright (c) 2014 Venture Media Labs. All rights reserved.

#include "TieGeometryFactory.h"
#include "ChordGeometry.h"
#include "MeasureGeometry.h"
#include "NoteGeometry.h"

namespace mxml {

const coord_t TieGeometryFactory::kTieSpacing = 2;

TieGeometryFactory::TieGeometryFactory(const PartGeometry& partGeometry) : _partGeometry(partGeometry) {
    
}

vector<unique_ptr<TieGeometry>>&& TieGeometryFactory::buildTieGeometries(const vector<unique_ptr<Geometry>>& geometries) {
    _tieGeometries.clear();
    _tieStartGeometries.clear();
    _slurStartGeometries.clear();
    createGeometries(geometries);
    return std::move(_tieGeometries);
}

void TieGeometryFactory::createGeometries(const vector<unique_ptr<Geometry>>& geometries) {
    for (auto& geom : geometries) {
        if (auto measure = dynamic_cast<MeasureGeometry*>(geom.get())) {
            createGeometries(measure->geometries());
        } else if (auto chordGeom = dynamic_cast<ChordGeometry*>(geom.get())) {
            createGeometriesFromNotes(chordGeom->notes());
        } else if (auto noteGeom = dynamic_cast<NoteGeometry*>(geom.get())) {
            createGeometryFromNote(*noteGeom);
        }
    }
}

void TieGeometryFactory::createGeometriesFromNotes(const vector<unique_ptr<NoteGeometry>>& notes) {
    for (auto& note : notes) {
        createGeometryFromNote(*note);
    }
}

void TieGeometryFactory::createGeometriesFromNotes(const vector<NoteGeometry*>& notes) {
    for (auto& note : notes) {
        createGeometryFromNote(*note);
    }
}

void TieGeometryFactory::createGeometryFromNote(const NoteGeometry& noteGeometry) {
    const dom::Note& note = noteGeometry.note();
    if (!note.notations().isPresent())
        return;
    
    const dom::Notations& notations = note.notations();
    
    for (auto& tie : notations.ties()) {
        auto key = std::make_pair(note.staff(), note.pitch().value());
        if (tie.type() == dom::TYPE_START) {
            _tieStartGeometries[key] = &noteGeometry;
        } else if (tie.type() == dom::TYPE_STOP) {
            auto startGeom = _tieStartGeometries.find(key);
            if (startGeom != _tieStartGeometries.end()) {
                _tieGeometries.push_back(std::move(buildTieGeometry(startGeom->second, &noteGeometry, tie.placement())));
                _tieStartGeometries.erase(startGeom);
            }
        }
    }
    
    for (auto& slur : notations.slurs()) {
        auto key = std::make_pair(note.staff(), slur.number());
        if (slur.type() == dom::TYPE_START) {
            _slurStartGeometries[key] = &noteGeometry;
        } else if (slur.type() == dom::TYPE_STOP) {
            auto startGeom = _slurStartGeometries.find(key);
            if (startGeom != _slurStartGeometries.end()) {
                _tieGeometries.push_back(std::move(buildSlurGeometry(startGeom->second, &noteGeometry, slur.placement())));
                _slurStartGeometries.erase(startGeom);
            }
        }
    }
}

std::unique_ptr<TieGeometry> TieGeometryFactory::buildTieGeometry(const NoteGeometry* start, const NoteGeometry* stop, const dom::Optional<dom::Placement>& placement) {
    std::unique_ptr<TieGeometry> tieGeom(new TieGeometry);
    
    Point startLocation;
    Point stopLocation;
    
    startLocation.x = start->frame().max().x;
    stopLocation.x = stop->frame().min().x;
    
    if (!placement.isPresent()) {
        coord_t startStaffY = startLocation.y - _partGeometry.staffOrigin(start->note().staff());
        coord_t stopStaffY = stopLocation.y - _partGeometry.staffOrigin(stop->note().staff());
        coord_t avgy = (startStaffY + stopStaffY) / 2;
        if (avgy < PartGeometry::staffHeight()/2)
            tieGeom->setPlacement(absentOptional(dom::PLACEMENT_ABOVE));
        else
            tieGeom->setPlacement(absentOptional(dom::PLACEMENT_BELOW));
    }
    
    if (tieGeom->placement().value() == dom::PLACEMENT_BELOW) {
        startLocation.y = start->frame().max().y;
        stopLocation.y = stop->frame().max().y;
    } else {
        startLocation.y = start->frame().min().y;
        stopLocation.y = stop->frame().min().y;
    }
    
    tieGeom->setStartLocation(_partGeometry.convertFromGeometry(startLocation, start->parentGeometry()));
    tieGeom->setStopLocation(_partGeometry.convertFromGeometry(stopLocation, stop->parentGeometry()));
    
    return tieGeom;
}

std::unique_ptr<TieGeometry> TieGeometryFactory::buildSlurGeometry(const NoteGeometry* start, const NoteGeometry* stop, const dom::Optional<dom::Placement>& placement) {
    std::unique_ptr<TieGeometry> tieGeom(new TieGeometry);
    
    Point startLocation = start->location();
    Point stopLocation = stop->location();
    
    startLocation.x += kTieSpacing;
    stopLocation.x -= kTieSpacing;
    
    if (!placement.isPresent()) {
        coord_t startStaffY = startLocation.y - _partGeometry.staffOrigin(start->note().staff());
        coord_t stopStaffY = stopLocation.y - _partGeometry.staffOrigin(stop->note().staff());
        coord_t avgy = (startStaffY + stopStaffY) / 2;
        if (avgy < PartGeometry::staffHeight()/2)
            tieGeom->setPlacement(absentOptional(dom::PLACEMENT_ABOVE));
        else
            tieGeom->setPlacement(absentOptional(dom::PLACEMENT_BELOW));
    }
    
    const ChordGeometry& startChordGeom = static_cast<const ChordGeometry&>(*start->parentGeometry());
    const ChordGeometry& stopChordGeom = static_cast<const ChordGeometry&>(*stop->parentGeometry());
    
    Rect startNotesFrame = startChordGeom.notesFrame();
    Rect stopNotesFrame = stopChordGeom.notesFrame();
    
    if (tieGeom->placement().value() == dom::PLACEMENT_BELOW) {
        startLocation.y = startNotesFrame.max().y + kTieSpacing;
        stopLocation.y = stopNotesFrame.max().y + kTieSpacing;
    } else {
        startLocation.y = startNotesFrame.min().y - kTieSpacing;
        stopLocation.y = stopNotesFrame.min().y - kTieSpacing;
    }
    
    // Avoid collisions with beamed sets
    if (!start->note().beams().empty() && start->note().beams().front().type() != dom::Beam::TYPE_END) {
        ChordGeometry* chordGeom = (ChordGeometry*)start->parentGeometry();
        Rect stemFrame = chordGeom->stem()->frame();
        
        if (tieGeom->placement().value() == dom::PLACEMENT_BELOW && start->note().stem() == dom::STEM_DOWN) {
            startLocation.x = stemFrame.min().x + StemGeometry::kNoFlagWidth;
            startLocation.y = stemFrame.max().y + 2*kTieSpacing;
        } else if (tieGeom->placement().value() == dom::PLACEMENT_ABOVE && start->note().stem() == dom::STEM_UP) {
            startLocation.x = stemFrame.max().x - StemGeometry::kNoFlagWidth;
            startLocation.y = stemFrame.min().y - 2*kTieSpacing;
        }
    }
    if (!stop->note().beams().empty() && stop->note().beams().front().type() != dom::Beam::TYPE_BEGIN) {
        ChordGeometry* chordGeom = (ChordGeometry*)stop->parentGeometry();
        Rect stemFrame = chordGeom->stem()->frame();
        
        if (tieGeom->placement().value() == dom::PLACEMENT_BELOW && stop->note().stem() == dom::STEM_DOWN) {
            stopLocation.x = stemFrame.min().x + StemGeometry::kNoFlagWidth;
            stopLocation.y = stemFrame.max().y + 2*kTieSpacing;
        } else if (tieGeom->placement().value() == dom::PLACEMENT_ABOVE && stop->note().stem() == dom::STEM_UP) {
            stopLocation.x = stemFrame.max().x - StemGeometry::kNoFlagWidth;
            stopLocation.y = stemFrame.min().y - 2*kTieSpacing;
        }
    }

    tieGeom->setStartLocation(_partGeometry.convertFromGeometry(startLocation, start->parentGeometry()));
    tieGeom->setStopLocation(_partGeometry.convertFromGeometry(stopLocation, stop->parentGeometry()));
    
    return tieGeom;
}

} // namespace mxml