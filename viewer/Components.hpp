#pragma once

#include <vector>
#include <utility>

#include "DescriptionComponent.hpp"
#include "LabelComponent.hpp"
#include "ShapeComponent.hpp"
#include "LocationComponent.hpp"
#include "FDGVertexComponent.hpp"
#include "FDGEdgeComponent.hpp"
#include "InterfaceEdgeComponent.hpp"


//  Components are stored in vectors.
//  A component entry with ID==0 is an empty position.
//  Otherwise, nonempty components are ordered in the vector ascending by ID.
//  To add a component, push it onto the end of the array.
//  To remove a component, set its ID to 0.
//  TODO: *Something* will come along and remove the empty entries.
//  TODO: This scheme works only when a component is inserted at creation time.
//      Inserting after creation time makes it possible that its ID is less than
//      the current end of the vector.

struct Components {
    std::vector<DescriptionComponent> description_components;
    std::vector<LabelComponent> label_components;
    std::vector<LocationComponent> location_components;
    std::vector<ShapeComponent> shape_components;
    std::vector<FDGVertexComponent> fdg_vertex_components;
    std::vector<FDGEdgeComponent> fdg_edge_components;
    std::vector<InterfaceEdgeComponent> interface_edge_components;


    //  Write a description of all entities to stdout.
    //
    void describe_entities() const;


    //  Returns an entity's component instance given the component table and an entity ID.
    //  Throws if the enity doesn't have this component.
    //
    template<class T>
    T& get(int entity_id, std::vector<T>& table)
    {
        for (auto& c : table)
            if (c.entity_id == entity_id)
                return c;
        throw std::runtime_error(
                std::string("Components::get(): entity ID ")
                + std::to_string(entity_id)
                + " not found");
        //  TODO: Make this efficient.
    }

    template<class T>
    T* find(int entity_id, std::vector<T>& table)
    {
        for (auto& c : table)
            if (c.entity_id == entity_id)
                return &c;
        return nullptr;
    }


    template<class T, class U>
    class Join {
    public:
        Join(std::vector<T>& t, std::vector<U>& u) : t(t), u(u) {};
        class iterator {
            public:
                iterator(size_t t_ix, size_t u_ix, std::vector<T>& t, std::vector<U>& u) : t_ix(t_ix), u_ix(u_ix), t(t), u(u) {};
                std::pair<T&,U&> operator *() { return std::pair<T&,U&>(t[t_ix], u[u_ix]); }
                bool operator ==(const iterator& rhs) const { return t_ix == rhs.t_ix && u_ix == rhs.u_ix; }
                bool operator !=(const iterator& rhs) const { return t_ix != rhs.t_ix || u_ix != rhs.u_ix; }
                iterator& operator ++() {
                    ++t_ix;
                    ++u_ix;
                    for (;;) {
                        if (t_ix >= t.size() || u_ix >= u.size()) {
                            t_ix = t.size();
                            u_ix = u.size();
                            return *this;
                        }
                        if (t[t_ix].entity_id == u[u_ix].entity_id)
                            return *this;
                        if (t[t_ix].entity_id < u[u_ix].entity_id)
                            ++t_ix;
                        else if (u[u_ix].entity_id < t[t_ix].entity_id)
                            ++u_ix;
                    }
                }
            private:
                size_t t_ix;
                size_t u_ix;
                std::vector<T>& t;
                std::vector<U>& u;
        };
        iterator begin() { return ++iterator(-1, -1, t, u); }
        iterator end() { return iterator(t.size(), u.size(), t, u); }
    private:
        std::vector<T>& t;
        std::vector<U>& u;
    };
};
