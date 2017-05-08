/****************************************************************************
* Italian National Research Council                                         *
* Institute for Applied Mathematics and Information Technologies, Genoa     *
* IMATI-GE / CNR                                                            *
*                                                                           *
* Author: Marco Livesu (marco.livesu@gmail.com)                             *
*                                                                           *
* Copyright(C) 2016                                                         *
* All rights reserved.                                                      *
*                                                                           *
* This file is part of CinoLib                                              *
*                                                                           *
* CinoLib is free software; you can redistribute it and/or modify           *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 3 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
****************************************************************************/
#include <cinolib/meshes/hexmesh/hexmesh.h>
#include <cinolib/quality.h>
#include <cinolib/timer.h>
#include <cinolib/io/read_write.h>
#include <cinolib/common.h>

#include <float.h>
#include <map>
#include <set>

namespace cinolib
{

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
Hexmesh<M,V,E,F,C>::Hexmesh(const std::vector<vec3d> & verts,
                            const std::vector<u_int> & cells)
: verts(verts)
, cells(cells)
{
    init();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
Hexmesh<M,V,E,F,C>::Hexmesh(const std::vector<double> & coords,
                            const std::vector<u_int>  & cells)
{
    this->verts = vec3d_from_serialized_xyz(coords);
    this->cells = cells;
    init();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
Hexmesh<M,V,E,F,C>::Hexmesh(const char * filename)
{
    timer_start("load hexmesh");

    load(filename);
    init();

    //print_quality_statistics();

    timer_stop("load hexmesh");
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
void Hexmesh<M,V,E,F,C>::load(const char * filename)
{
    timer_start("Load Hexmesh");

    clear();
    std::vector<double> coords;

    std::string str(filename);
    std::string filetype = str.substr(str.size()-4,4);

    if (filetype.compare("mesh") == 0 ||
        filetype.compare("MESH") == 0)
    {
        std::vector<uint> dummy; // not used here
        read_MESH(filename, coords, dummy, cells);
    }
    else if (filetype.compare(".vtu") == 0 ||
             filetype.compare(".VTU") == 0)
    {
        std::vector<uint> dummy; // not used here
        read_VTU(filename, coords, dummy, cells);
    }
    else if (filetype.compare(".vtk") == 0 ||
             filetype.compare(".VTK") == 0)
    {
        std::vector<uint> dummy; // not used here
        read_VTK(filename, coords, dummy, cells);
    }
    else
    {
        std::cerr << "ERROR : " << __FILE__ << ", line " << __LINE__ << " : load() : file format not supported yet " << endl;
        exit(-1);
    }

    verts = vec3d_from_serialized_xyz(coords);

    logger << num_cells() << " hexahedra read" << endl;
    logger << num_verts() << " vertices  read" << endl;

    this->mesh_data().filename = std::string(filename);

    timer_stop("Load Hexmesh");
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
void Hexmesh<M,V,E,F,C>::save(const char * filename) const
{
    timer_start("Save Hexmesh");

    std::vector<double> coords = serialized_xyz_from_vec3d(verts);

    std::string str(filename);
    std::string filetype = str.substr(str.size()-4,4);

    if (filetype.compare("mesh") == 0 ||
        filetype.compare("MESH") == 0)
    {
        std::vector<uint> dummy; // empty
        write_MESH(filename, coords, dummy, cells);
    }
    else if (filetype.compare(".vtu") == 0 ||
             filetype.compare(".VTU") == 0)
    {
        std::vector<uint> dummy; // empty
        write_VTU(filename, coords, dummy, cells);
    }
    else if (filetype.compare(".vtk") == 0 ||
             filetype.compare(".VTK") == 0)
    {
        std::vector<uint> dummy; // empty
        write_VTK(filename, coords, dummy, cells);
    }
    else
    {
        std::cerr << "ERROR : " << __FILE__ << ", line " << __LINE__ << " : write() : file format not supported yet " << endl;
        exit(-1);
    }

    timer_stop("Save Hexmesh");
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
void Hexmesh<M,V,E,F,C>::clear()
{
    bb.reset();
    //
    verts.clear();
    edges.clear();
    faces.clear();
    cells.clear();
    v_on_srf.clear();
    e_on_srf.clear();
    //
    M tmp;
    m_data = tmp;;
    v_data.clear();
    e_data.clear();
    f_data.clear();
    c_data.clear();
    //
    v2v.clear();
    v2e.clear();
    v2f.clear();
    v2c.clear();
    e2f.clear();
    e2c.clear();
    f2e.clear();
    f2f.clear();
    f2c.clear();
    c2e.clear();
    c2f.clear();
    c2c.clear();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
void Hexmesh<M,V,E,F,C>::init()
{
    update_bbox();
    update_interior_adjacency();
    update_surface_adjacency();

    v_data.resize(num_verts());
    e_data.resize(num_edges());
    c_data.resize(num_cells());
    f_data.resize(num_faces());

    update_q_normals();
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
void Hexmesh<M,V,E,F,C>::update_bbox()
{
    bb.reset();
    for(uint vid=0; vid<num_verts(); ++vid)
    {
        vec3d v = vert(vid);
        bb.min = bb.min.min(v);
        bb.max = bb.max.max(v);
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
void Hexmesh<M,V,E,F,C>::update_interior_adjacency()
{
    timer_start("Build interior adjacency");

    v2v.clear(); v2v.resize(num_verts());
    v2e.clear(); v2e.resize(num_verts());
    v2c.clear(); v2c.resize(num_verts());
    c2c.clear(); c2c.resize(num_cells());
    c2e.clear(); c2e.resize(num_cells());

    std::map<ipair,std::vector<uint>> e2c_map;
    for(uint cid=0; cid<num_cells(); ++cid)
    {
        uint cid_ptr = cid * verts_per_cell();
        uint vids[8] = { cells.at(cid_ptr+0), cells.at(cid_ptr+1), cells.at(cid_ptr+2), cells.at(cid_ptr+3),
                         cells.at(cid_ptr+4), cells.at(cid_ptr+5), cells.at(cid_ptr+6), cells.at(cid_ptr+7) };

        for(uint vid=0; vid<verts_per_cell(); ++vid)
        {
            v2c.at(vids[vid]).push_back(cid);
        }
        for(uint eid=0; eid<edges_per_cell(); ++eid)
        {
            ipair e = unique_pair(vids[HEXA_EDGES[eid][0]], vids[HEXA_EDGES[eid][1]]);
            e2c_map[e].push_back(cid);
        }
    }

    edges.clear();
    e2c.clear();
    e2c.resize(e2c_map.size());

    std::set<ipair> cell_pairs;

    uint fresh_id = 0;
    for(auto e2c_it : e2c_map)
    {
        ipair e    = e2c_it.first;
        uint  eid  = fresh_id++;
        uint  vid0 = e.first;
        uint  vid1 = e.second;

        edges.push_back(vid0);
        edges.push_back(vid1);

        v2v.at(vid0).push_back(vid1);
        v2v.at(vid1).push_back(vid0);

        v2e.at(vid0).push_back(eid);
        v2e.at(vid1).push_back(eid);

        std::vector<uint> cids = e2c_it.second;
        for(uint i=0; i<cids.size(); ++i)
        {
            uint cid = cids.at(i);

            c2e.at(cid).push_back(eid);
            e2c.at(eid).push_back(cid);

            for(uint j=i+1; j<cids.size(); ++j)
            {
                uint nbr = cids.at(j);
                if (cell_shared_face(cid,nbr) != -1)
                {
                    ipair p = unique_pair(cid,nbr);
                    if (DOES_NOT_CONTAIN(cell_pairs, p))
                    {
                        cell_pairs.insert(p);
                        c2c.at(cid).push_back(nbr);
                        c2c.at(nbr).push_back(cid);
                        assert(c2c.at(cid).size() <= faces_per_cell());
                        assert(c2c.at(nbr).size() <= faces_per_cell());
                    }
                }
            }
        }
    }

    logger << num_verts() << "\tvertices"  << endl;
    logger << num_cells() << "\thexahedra" << endl;
    logger << num_edges() << "\tedges"     << endl;

    timer_stop("Build interior adjacency");
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
void Hexmesh<M,V,E,F,C>::update_surface_adjacency()
{
    timer_start("Build Surface");

    typedef std::vector<uint> face;
    std::map<face,std::pair<uint,uint>> f2c_map;

    for(uint cid=0; cid<num_cells(); ++cid)
    {
        uint cid_ptr = cid * verts_per_cell();
        for(uint fid=0; fid<faces_per_cell(); ++fid)
        {
            face f;
            f.push_back(cells.at(cid_ptr + HEXA_FACES[fid][0]));
            f.push_back(cells.at(cid_ptr + HEXA_FACES[fid][1]));
            f.push_back(cells.at(cid_ptr + HEXA_FACES[fid][2]));
            f.push_back(cells.at(cid_ptr + HEXA_FACES[fid][3]));
            sort(f.begin(), f.end());
            if (CONTAINS(f2c_map,f)) f2c_map.erase(f);
            else                     f2c_map[f] = std::make_pair(cid,fid);
        }
    }

    v2f.clear(); v2f.resize(num_verts());
    e2f.clear(); e2f.resize(num_edges());
    c2f.clear(); c2f.resize(num_cells());

    faces.clear();
    f2c.clear(); f2c.resize(f2c_map.size());
    f2e.clear(); f2e.resize(f2c_map.size());
    v_on_srf.resize(num_verts(), false);
    e_on_srf.resize(num_edges(), false);

    uint fresh_id = 0;
    for(auto f2c_it : f2c_map)
    {
        uint cid     = f2c_it.second.first;
        uint f       = f2c_it.second.second;
        uint cid_ptr = cid * verts_per_cell();
        uint vid0    = cells.at(cid_ptr + HEXA_FACES[f][0]);
        uint vid1    = cells.at(cid_ptr + HEXA_FACES[f][1]);
        uint vid2    = cells.at(cid_ptr + HEXA_FACES[f][2]);
        uint vid3    = cells.at(cid_ptr + HEXA_FACES[f][3]);

        faces.push_back(vid0);
        faces.push_back(vid1);
        faces.push_back(vid2);
        faces.push_back(vid3);

        v_on_srf.at(vid0) = true;
        v_on_srf.at(vid1) = true;
        v_on_srf.at(vid2) = true;
        v_on_srf.at(vid3) = true;

        v2f.at(vid0).push_back(fresh_id);
        v2f.at(vid1).push_back(fresh_id);
        v2f.at(vid2).push_back(fresh_id);
        v2f.at(vid3).push_back(fresh_id);

        c2f.at(cid).push_back(fresh_id);
        f2c.at(fresh_id) = cid;

        for(uint eid : adj_c2e(cid))
        {
            uint eid0  = edge_vert_id(eid, 0);
            uint eid1  = edge_vert_id(eid, 1);
            bool has_0 = (eid0 == vid0 || eid0 == vid1 || eid0 == vid2 || eid0 == vid3);
            bool has_1 = (eid1 == vid0 || eid1 == vid1 || eid1 == vid2 || eid1 == vid3);
            if (has_0 && has_1)
            {
                e2f.at(eid).push_back(fresh_id);
                f2e.at(fresh_id).push_back(eid);
            }
        }

        ++fresh_id;
    }

    f2f.clear(); f2f.resize(num_faces());
    for(uint eid=0; eid<num_edges(); ++eid)
    {
        for(uint fid1 : e2f.at(eid))
        for(uint fid2 : e2f.at(eid))
        {
            if (fid1 != fid2) f2f.at(fid1).push_back(fid2);
        }

        if (!e2f.at(eid).empty()) e_on_srf.at(eid) = true;
    }

    timer_stop("Build Surface");

    logger << faces.size()/verts_per_face() << " quads" << endl;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
void Hexmesh<M,V,E,F,C>::update_q_normals()
{
    for(uint fid=0; fid<num_faces(); ++fid)
    {
        vec3d v0 = face_vert(fid,0);
        vec3d v1 = face_vert(fid,1);
        vec3d v2 = face_vert(fid,2);

        vec3d u = v1 - v0;    u.normalize();
        vec3d v = v2 - v0;    v.normalize();
        vec3d n = u.cross(v); n.normalize();

        face_data(fid).normal = n;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
int Hexmesh<M,V,E,F,C>::cell_shared_face(const uint cid0, const uint cid1) const
{
    for(uint f=0; f<faces_per_cell(); ++f)
    {
        if (cell_contains_vert(cid1, cell_vert_id(cid0, HEXA_FACES[f][0])) &&
            cell_contains_vert(cid1, cell_vert_id(cid0, HEXA_FACES[f][1])) &&
            cell_contains_vert(cid1, cell_vert_id(cid0, HEXA_FACES[f][2])) &&
            cell_contains_vert(cid1, cell_vert_id(cid0, HEXA_FACES[f][3])) )
        {
            return f;
        }
    }
    return -1;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
bool Hexmesh<M,V,E,F,C>::cell_contains_vert(const uint cid, const uint vid) const
{
    for(uint i=0; i<verts_per_cell(); ++i)
    {
        if (cell_vert_id(cid,i) == vid) return true;
    }
    return false;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
vec3d Hexmesh<M,V,E,F,C>::cell_centroid(const uint cid) const
{
    vec3d c(0,0,0);
    for(uint off=0; off<verts_per_cell(); ++off)
    {
        c += cell_vert(cid,off);
    }
    c /= static_cast<double>(verts_per_cell());
    return c;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
vec3d Hexmesh<M,V,E,F,C>::elem_centroid(const uint cid) const
{
    return cell_centroid(cid);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
uint Hexmesh<M,V,E,F,C>::cell_vert_id(const uint cid, const uint off) const
{
    uint cid_ptr = cid * verts_per_cell();
    return cells.at(cid_ptr + off);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
vec3d Hexmesh<M,V,E,F,C>::cell_vert(const uint cid, const uint off) const
{
    return verts.at(cell_vert_id(cid,off));
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
vec3d Hexmesh<M,V,E,F,C>::face_centroid(const uint fid) const
{
    vec3d c(0,0,0);
    for(uint off=0; off<verts_per_face(); ++off) c += face_vert(fid,off);
    c /= static_cast<double>(verts_per_face());
    return c;
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
uint Hexmesh<M,V,E,F,C>::face_vert_id(const uint fid, const uint off) const
{
    uint fid_ptr = fid * verts_per_face();
    return faces.at(fid_ptr + off);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
vec3d Hexmesh<M,V,E,F,C>::face_vert(const uint fid, const uint off) const
{
    return verts.at(face_vert_id(fid,off));
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
uint Hexmesh<M,V,E,F,C>::edge_vert_id(const uint eid, const uint off) const
{
    uint eid_ptr = 2*eid;
    return edges.at(eid_ptr + off);
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
vec3d Hexmesh<M,V,E,F,C>::edge_vert(const uint eid, const uint off) const
{
    return verts.at(edge_vert_id(eid,off));
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
void Hexmesh<M,V,E,F,C>::elem_show_all()
{
    for(uint cid=0; cid<num_cells(); ++cid)
    {
        cell_data(cid).visible = true;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
void Hexmesh<M,V,E,F,C>::face_set_color(const Color & c)
{
    for(uint fid=0; fid<num_faces(); ++fid)
    {
        face_data(fid).color = c;
    }
}

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

template<class M, class V, class E, class F, class C>
CINO_INLINE
void Hexmesh<M,V,E,F,C>::cell_set_color(const Color & c)
{
    for(uint cid=0; cid<num_cells(); ++cid)
    {
        cell_data(cid).color = c;
    }
}

}
