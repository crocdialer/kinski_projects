//
// Created by Fabian on 01.05.17.
//

#ifndef KINSKIGL_ADJACENCY_H
#define KINSKIGL_ADJACENCY_H

#include "gl/gl.hpp"

namespace kinski{ namespace gl{

struct HalfEdge
{
    //! Vertex index at the end of this half-edge
    uint32_t index;

    //! Oppositely oriented adjacent half-edge
    HalfEdge* twin = nullptr;

    //! Next half-edge around the face
    HalfEdge* next = nullptr;
};

std::vector<HalfEdge> compute_half_edges(gl::GeometryPtr the_geom);

}}

#endif //KINSKIGL_ADJACENCY_H
