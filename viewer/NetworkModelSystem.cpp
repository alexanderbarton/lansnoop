#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <string>
#include <fcntl.h>

#include "EventSerialization.hpp"
#include "Entities.hpp"
#include "Components.hpp"

#include "NetworkModelSystem.hpp"

#include "/home/abarton/debug.hpp"


constexpr float scatter_factor = 1.5f;


//  Returns a random value between -1.0 and 1.0.
static float rng()
{
    return (rand() - RAND_MAX/2) * 1.0f / (RAND_MAX/2);
}


static std::string bytes_to_mac(const std::string& bytes)
{
    std::stringstream sstream;
    sstream << std::hex << std::setfill('0');
    bool first = true;
    for (char c : bytes) {
        if (first)
            first = false;
        else
            sstream << ':';
        sstream << std::setw(2) << int((unsigned char)c);
    }
    std::string result = sstream.str();
    return result;
}


static std::string bytes_to_ip_address(const std::string& bytes)
{
    std::stringstream sstream;
    bool first = true;
    for (char c : bytes) {
        if (first)
            first = false;
        else
            sstream << '.';
        sstream << int((unsigned char)c);
    }
    std::string result = sstream.str();
    return result;
}


void NetworkModelSystem::receive(Components& components, const Lansnoop::Network& network)
{
    if (!network_to_entity_ids.count(network.id())) {
        int entity_id = generate_entity_id();
        std::string description("network ");
        description += std::to_string(network.id());
        components.description_components.push_back(DescriptionComponent(entity_id, description));
        components.location_components.push_back(LocationComponent(entity_id, 2*rng(), 2*rng(), 1.0f));
        // components.shape_components.push_back(ShapeComponent(entity_id, ShapeComponent::Shape::CYLINDER));
        components.fdg_vertex_components.push_back(FDGVertexComponent(entity_id));
        this->network_to_entity_ids[network.id()] = entity_id;
    }
}


void NetworkModelSystem::receive(Components& components, const Lansnoop::Interface& interface)
{
    if (!interface_to_entity_ids.count(interface.id())) {
        int network_entity_id = this->network_to_entity_ids.at(interface.network_id());
        const LocationComponent& location = components.get(network_entity_id, components.location_components);
        float x = 2*rng() + scatter_factor * location.x;
        float y = 2*rng() + scatter_factor * location.y;

        int entity_id = generate_entity_id();
        std::string description("interface ");
        description += bytes_to_mac(interface.address());
        components.description_components.push_back(DescriptionComponent(entity_id, description));
        components.label_components.push_back(LabelComponent(entity_id, bytes_to_mac(interface.address())));
        components.location_components.push_back(LocationComponent(entity_id, x, y, 1.0f));
        components.shape_components.push_back(ShapeComponent(entity_id, ShapeComponent::Shape::BOX));
        components.fdg_vertex_components.push_back(FDGVertexComponent(entity_id));
        components.fdg_edge_components.push_back(FDGEdgeComponent(entity_id, network_entity_id));
        components.interface_edge_components.push_back(InterfaceEdgeComponent(entity_id, network_entity_id));
        this->interface_to_entity_ids[interface.id()] = entity_id;
    }
    else {
        //  Check for changes to the assignment of this interface to a different network.
    }
}


void NetworkModelSystem::receive(Components& components, const Lansnoop::Traffic& traffic)
{
    for (const auto [id, count] : traffic.interface_packet_counts()) {
        long dp = count - this->interface_packet_counts[id];
        this->interface_packet_counts[id] = count;
        if (dp) {
            long entity_id = this->interface_to_entity_ids.at(id);
            InterfaceEdgeComponent& iec = components.get(entity_id, components.interface_edge_components);
            iec.glow += dp;
        }
    }
    for (const auto [id, count] : traffic.cloud_packet_counts()) {
        long dp = count - this->cloud_packet_counts[id];
        this->cloud_packet_counts[id] = count;
        if (dp) {
            long entity_id = this->cloud_to_entity_ids.at(id);
            InterfaceEdgeComponent& iec = components.get(entity_id, components.interface_edge_components);
            iec.glow += dp;
        }
    }
    for (const auto [id, count] : traffic.ipaddress_packet_counts()) {
        long dp = count - this->ipaddress_packet_counts[id];
        this->ipaddress_packet_counts[id] = count;
        if (dp) {
            long entity_id = this->ipaddress_to_entity_ids.at(id);
            InterfaceEdgeComponent& iec = components.get(entity_id, components.interface_edge_components);
            iec.glow += dp;
        }
    }
}


void NetworkModelSystem::receive(Components& components, const Lansnoop::IPAddress& ipaddress)
{
    //  New IPAddrInfo instance.
    //
    if (!ipaddress_to_entity_ids.count(ipaddress.id())) {
        int attached_entity_id;
        switch (ipaddress.attached_to_case()) {
            case Lansnoop::IPAddress::kInterfaceId: {
                auto it = this->interface_to_entity_ids.find(ipaddress.interface_id());
                if (it == this->interface_to_entity_ids.end())
                    throw std::invalid_argument("receive(IPAddress): attached interface not found");
                attached_entity_id = it->second;
                break;
            }
            case Lansnoop::IPAddress::kCloudId: {
                auto it = this->cloud_to_entity_ids.find(ipaddress.cloud_id());
                if (it == this->cloud_to_entity_ids.end())
                    throw std::invalid_argument("receive(IPAddress): attached cloud not found");
                attached_entity_id = it->second;
                break;
            }
            case Lansnoop::IPAddress::ATTACHED_TO_NOT_SET:
            default:
                throw std::invalid_argument(std::string("receive(IPAddress ")
                        + std::to_string(ipaddress.id())
                        + "): invalid attached_to");
        }

        const LocationComponent& location = components.get(attached_entity_id, components.location_components);
        float x = 2*rng() + scatter_factor * location.x;
        float y = 2*rng() + scatter_factor * location.y;

        int entity_id = generate_entity_id();
        std::string description("IP address ");
        description += bytes_to_ip_address(ipaddress.address());
        if (ipaddress.ns_name().size()) {
            description += " (";
            description += ipaddress.ns_name();
            description += ")";
        }
        components.description_components.push_back(DescriptionComponent(entity_id, description));

        std::vector<std::string> labels;
        if (ipaddress.ns_name().size())
            labels.push_back(ipaddress.ns_name());
        labels.push_back(bytes_to_ip_address(ipaddress.address()));
        components.label_components.push_back(LabelComponent(entity_id, labels));
        components.location_components.push_back(LocationComponent(entity_id, x, y, 1.0f));
        components.shape_components.push_back(ShapeComponent(entity_id, ShapeComponent::Shape::CYLINDER));
        components.fdg_vertex_components.push_back(FDGVertexComponent(entity_id));
        components.fdg_edge_components.push_back(FDGEdgeComponent(entity_id, attached_entity_id));
        components.interface_edge_components.push_back(InterfaceEdgeComponent(entity_id, attached_entity_id));
        this->ipaddress_to_entity_ids[ipaddress.id()] = entity_id;
    }

    //  Update to an existing IPAddrInfo instance.
    //
    else {
        int entity_id = ipaddress_to_entity_ids.at(ipaddress.id());
        int attached_entity_id;
        switch (ipaddress.attached_to_case()) {
            case Lansnoop::IPAddress::kInterfaceId: {
                auto it = this->interface_to_entity_ids.find(ipaddress.interface_id());
                if (it == this->interface_to_entity_ids.end())
                    throw std::invalid_argument("receive(IPAddress): attached interface not found");
                attached_entity_id = it->second;
                break;
            }
            case Lansnoop::IPAddress::kCloudId: {
                auto it = this->cloud_to_entity_ids.find(ipaddress.cloud_id());
                if (it == this->cloud_to_entity_ids.end())
                    throw std::invalid_argument("receive(IPAddress): attached cloud not found");
                attached_entity_id = it->second;
                break;
            }
            case Lansnoop::IPAddress::ATTACHED_TO_NOT_SET:
            default:
                throw std::invalid_argument(std::string("receive(IPAddress ")
                        + std::to_string(ipaddress.id())
                        + "): invalid attached_to");
        }
        FDGEdgeComponent& fdg_edge = components.get(entity_id, components.fdg_edge_components);
        fdg_edge.other_entity_id = attached_entity_id;
        InterfaceEdgeComponent& interface_edge = components.get(entity_id, components.interface_edge_components);
        interface_edge.other_entity_id = attached_entity_id;

        DescriptionComponent& d = components.get(entity_id, components.description_components);
        d.description = "IP address ";
        d.description += bytes_to_ip_address(ipaddress.address());
        if (ipaddress.ns_name().size()) {
            d.description += " (";
            d.description += ipaddress.ns_name();
            d.description += ")";
        }
        LabelComponent& l = components.get(entity_id, components.label_components);
        l.labels.clear();
        if (ipaddress.ns_name().size())
            l.labels.push_back(ipaddress.ns_name());
        l.labels.push_back(bytes_to_ip_address(ipaddress.address()));
    }
}


void NetworkModelSystem::receive(Components& components, const Lansnoop::Cloud& cloud)
{
    if (!cloud_to_entity_ids.count(cloud.id())) {
        int attached_entity_id;
        switch (cloud.attached_to_case()) {
            case Lansnoop::Cloud::kInterfaceId: {
                auto it = this->interface_to_entity_ids.find(cloud.interface_id());
                if (it == this->interface_to_entity_ids.end())
                    throw std::invalid_argument("receive(Cloud): attached interface not found");
                attached_entity_id = it->second;
                break;
            }
            case Lansnoop::Cloud::kCloudId: {
                auto it = this->cloud_to_entity_ids.find(cloud.cloud_id());
                if (it == this->cloud_to_entity_ids.end())
                    throw std::invalid_argument("receive(Cloud): attached cloud not found");
                attached_entity_id = it->second;
                break;
            }
            case Lansnoop::Cloud::ATTACHED_TO_NOT_SET:
            default:
                throw std::invalid_argument(std::string("receive(Cloud ")
                        + std::to_string(cloud.id())
                        + "): invalid attached_to");
        }

        const LocationComponent& location = components.get(attached_entity_id, components.location_components);
        float x = 2*rng() + scatter_factor * location.x;
        float y = 2*rng() + scatter_factor * location.y;

        int entity_id = generate_entity_id();
        components.description_components.push_back(DescriptionComponent(entity_id, cloud.description()));
        components.label_components.push_back(LabelComponent(entity_id, cloud.description()));
        components.location_components.push_back(LocationComponent(entity_id, x, y, 1.0f));
        // components.shape_components.push_back(ShapeComponent(entity_id, ShapeComponent::Shape::CYLINDER));
        components.fdg_vertex_components.push_back(FDGVertexComponent(entity_id));
        components.fdg_edge_components.push_back(FDGEdgeComponent(entity_id, attached_entity_id));
        components.interface_edge_components.push_back(InterfaceEdgeComponent(entity_id, attached_entity_id));
        this->cloud_to_entity_ids[cloud.id()] = entity_id;
    }
    //  TODO: handle cloud updates
}


void NetworkModelSystem::update(Components& components)
{
    Lansnoop::Event event;
    while (read_event_nb(in, event)) {
        switch (event.type_case()) {

            case Lansnoop::Event::kNetwork:
                receive(components, event.network());
                break;

            case Lansnoop::Event::kInterface:
                receive(components, event.interface());
                break;

            case Lansnoop::Event::kTraffic:
                receive(components, event.traffic());
                break;

            case Lansnoop::Event::kIpaddress:
                receive(components, event.ipaddress());
                break;

            case Lansnoop::Event::kCloud:
                receive(components, event.cloud());
                break;

            case Lansnoop::Event::TYPE_NOT_SET:
            // default:
                break;
        }
    }

    // components.describe_entities();
}


void NetworkModelSystem::open(const std::string& path)
{
    this->in.open(path, std::ifstream::binary);
    if (!in.good())
        throw std::invalid_argument("NetworkModelSystem: unable to open input path");
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
}
