#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <string>
#include <regex>
#include <stdexcept>

#include <fcntl.h>

#include <glad/glad.h>

#include "stb_image.h"

#include "EventSerialization.hpp"
#include "Entities.hpp"
#include "Components.hpp"

#include "NetworkModelSystem.hpp"


constexpr float scatter_factor = 2.0f;

namespace {
    struct TextureMatch {
        std::regex regex;
        unsigned int texture;
    };
};
static std::vector<TextureMatch> textures;
static unsigned int catchall_texture;


static unsigned int load_texture(const std::string& path)
{
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (!data)
        throw std::invalid_argument(std::string("failed loading ") + path);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    return texture;
}


static void load_texture(const std::string& path, const std::string& regex_s)
{
    std::regex regex(regex_s, std::regex_constants::icase);
    unsigned int texture = load_texture(path);
    textures.push_back(TextureMatch { regex, texture });
}


unsigned int select_interface_texture(const std::string& manufacturer)
{
    for (const TextureMatch& m : textures)
        if (std::regex_match(manufacturer, m.regex))
            return m.texture;

    return catchall_texture;
}


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


static bool is_rfc1918(const std::string& bytes)
{
    unsigned char o0 = bytes[0];
    unsigned char o1 = bytes[1];
    return bytes.size() == 4 && (
        o0 == 10                              //  10.0.0.0/8
        || (o0 == 172 && o1 >= 16 && o1 <=31) //  172.16.0.0/12
        || (o0 == 192 && o1 == 168)           //  192.168.0.0/16
    );
}


void NetworkModelSystem::receive(Components& components, const Lansnoop::Network& network)
{
    if (!network_to_entity_ids.count(network.id())) {
        int entity_id = generate_entity_id();
        std::string description("network ");
        description += std::to_string(network.id());
        components.description_components.push_back(DescriptionComponent(entity_id, description));
        components.label_components.push_back(LabelComponent(entity_id, std::string("network ") + std::to_string(network.id())));
        components.location_components.push_back(LocationComponent(entity_id, 2*rng(), 2*rng(), 1.0f));
        // components.shape_components.push_back(ShapeComponent(entity_id, ShapeComponent::Shape::CYLINDER));
        components.fdg_vertex_components.push_back(FDGVertexComponent(entity_id));
        this->network_to_entity_ids[network.id()] = entity_id;
    }
}


void NetworkModelSystem::receive(Components& components, const Lansnoop::Interface& interface)
{
    int network_entity_id = this->network_to_entity_ids.at(interface.network_id());

    if (!interface_to_entity_ids.count(interface.id())) {
        const LocationComponent& location = components.get(network_entity_id, components.location_components);
        float x = 2*rng() + scatter_factor * location.x;
        float y = 2*rng() + scatter_factor * location.y;

        int entity_id = generate_entity_id();
        std::string description("interface ");
        description += bytes_to_mac(interface.address());
        components.description_components.push_back(DescriptionComponent(entity_id, description));
        components.label_components.push_back(LabelComponent(entity_id, bytes_to_mac(interface.address())));
        components.location_components.push_back(LocationComponent(entity_id, x, y, 1.0f));
        glm::vec3 color(1.0, 0.5, 0.2);
        // components.shape_components.push_back(ShapeComponent(entity_id, ShapeComponent::Shape::BOX, color));
        components.fdg_vertex_components.push_back(FDGVertexComponent(entity_id));
        components.fdg_edge_components.push_back(FDGEdgeComponent(entity_id, network_entity_id, 10.f));
        components.interface_edge_components.push_back(InterfaceEdgeComponent(entity_id, network_entity_id));
        this->interface_to_entity_ids[interface.id()] = entity_id;

        unsigned int texture = select_interface_texture(interface.maker());
        components.textured_shape_components.push_back(TexturedShapeComponent(entity_id, texture));
    }
    else {
        //  Check for changes to the assignment of this interface to a different network.
        int entity_id = interface_to_entity_ids.at(interface.id());
        FDGEdgeComponent& attractor = components.get(entity_id, components.fdg_edge_components);
        if (attractor.other_entity_id != network_entity_id)
            attractor.other_entity_id = network_entity_id;
        InterfaceEdgeComponent& edge = components.get(entity_id, components.interface_edge_components);
        if (edge.other_entity_id != network_entity_id)
            edge.other_entity_id = network_entity_id;
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

        glm::vec3 color(0.5, 0.2, 1.0);
        if (is_rfc1918(ipaddress.address()))
            color = glm::vec3(0.2, 1.0, 0.5);

        std::vector<std::string> labels;
        if (ipaddress.ns_name().size())
            labels.push_back(ipaddress.ns_name());
        labels.push_back(bytes_to_ip_address(ipaddress.address()));
        components.label_components.push_back(LabelComponent(entity_id, labels));
        components.location_components.push_back(LocationComponent(entity_id, x, y, 1.0f));
        components.shape_components.push_back(ShapeComponent(entity_id, ShapeComponent::Shape::CYLINDER, color));
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
        std::vector<std::string> new_labels;
        if (ipaddress.ns_name().size())
            new_labels.push_back(ipaddress.ns_name());
        new_labels.push_back(bytes_to_ip_address(ipaddress.address()));
        if (l.labels != new_labels) {
            l.labels = new_labels;
            l.fade = 2.f;
        }
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


void NetworkModelSystem::init()
{
    const std::string path = "viewer/textures/";
    load_texture(path + "apple.png", "^Apple.*$");
    load_texture(path + "asus.jpeg", "^ASUSTek.*");
    load_texture(path + "check_point.jpeg", "^Check Point Software Technologies$");
    load_texture(path + "cisco.jpeg", "^CISCO.*$");
    load_texture(path + "dell.png", "^Dell.*");
    load_texture(path + "F5_Networks.jpeg", "^F5 Networks.*");
    load_texture(path + "fortinet.jpeg", "^Fortinet.*");
    load_texture(path + "google.png", ".*Google.*");
    load_texture(path + "hp.png", "^hp.*|Hewlett.Packard.*$");
    load_texture(path + "ibm.jpeg", "^IBM.*");
    load_texture(path + "intel.png", "^INTEL.*$");
    load_texture(path + "juniper-networks.jpeg", "^Juniper Networks.*");
    load_texture(path + "lg_electronics.jpeg", "^LG Electronics");
    load_texture(path + "meraki.jpeg", "^Meraki.*$");
    load_texture(path + "microsoft.jpeg", "microsoft");
    load_texture(path + "motorola-mobility.png", "^Motorola Mobility.*");
    load_texture(path + "murata.jpeg", "^Murata.*");
    load_texture(path + "netgear.png", "^NETGEAR$");
    load_texture(path + "paloalto.jpeg", "^Palo Alto Networks.*");
    load_texture(path + "pcs-systemtechnik.png", "^PCS Systemtechnik GmbH$");
    load_texture(path + "polycom.png", "^polycom.*$");
    load_texture(path + "raspberry-pi.png", "^Raspberry Pi Foundation$");
    load_texture(path + "riverbed.jpeg", "^Riverbed Technology.*");
    load_texture(path + "samsung.png", "^Samsung Electronics Co.,Ltd$");
    load_texture(path + "sonos.png", "^Sonos, Inc.$");
    load_texture(path + "vmware.jpeg", "^VMWare.*");
    load_texture(path + "withings.png", "^Withings$");
    load_texture(path + "xerox.png", ".*XEROX.*");

    //  Final catch-all matching anything:
    catchall_texture = load_texture(path + "default.png");

    //  TODO: make texture files relative to some command line parameter.
    //  TODO: move this configuration to a runtime loaded file.
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
