//
// Created by Fabian on 01.05.17.
//
#include "Adjacency.hpp"
#include "gl/Geometry.hpp"
#include "core/Timer.hpp"

namespace
{
inline uint64_t pack(uint64_t a, uint64_t b){ return (a << 32) | b; }
inline uint64_t swizzle(uint64_t a){ return ((a & 0xFFFFFFFF) << 32) | (a >> 32); }
}

namespace kinski{ namespace gl{

std::vector<HalfEdge> compute_half_edges(gl::GeometryPtr the_geom)
{
    Stopwatch timer;
    timer.start();
    std::vector<HalfEdge> ret(3 * the_geom->faces().size());
    std::unordered_map<uint64_t, HalfEdge*> edge_table;

    HalfEdge *edge = ret.data();

    for(const Face3& face : the_geom->faces())
    {
        // create the half-edge that goes from C to A:
        edge_table[pack(face.c, face.a)] = edge;
        edge->index = face.a;
        edge->next = edge + 1;
        ++edge;

        // create the half-edge that goes from A to B:
        edge_table[pack(face.a, face.b)] = edge;
        edge->index = face.b;
        edge->next = edge + 1;
        ++edge;

        // create the half-edge that goes from B to C:
        edge_table[pack(face.b, face.c)] = edge;
        edge->index = face.c;
        edge->next = edge - 2;
        ++edge;
    }

    // populate the twin pointers by iterating over the edge_table
    int boundaryCount = 0;

    for(auto &e : edge_table)
    {
        HalfEdge *current_edge = e.second;

        // try to find twin edge in map
        auto it = edge_table.find(swizzle(e.first));

        if(it != edge_table.end())
        {
            HalfEdge *twin_edge = it->second;
            twin_edge->twin = current_edge;
            current_edge->twin = twin_edge;
        }
        else{ ++boundaryCount; }
    }

    // Now that we have a half-edge structure, it's easy to create adjacency info:
    if(boundaryCount > 0)
    {
        LOG_DEBUG << "mesh is not watertight. contains " << boundaryCount << " boundary edges.";
    }
    LOG_DEBUG << "half-edge computation took " << timer.time_elapsed() * 1000.0 << " ms";
    return ret;
}

}}

