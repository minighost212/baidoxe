#pragma once

// ============================================================================
//  obj_model.h - Wavefront OBJ / MTL model loader.
//
//  ObjModel owns the parsed geometry of an .obj file together with the diffuse
//  colours read from its referenced .mtl library. Each material group is stored
//  as an independent car::Mesh / GpuMesh pair so it can be submitted with its
//  own base colour through the existing single-uniform renderer.
//
//  The class follows the same RAII pattern as the rest of the renderer:
//  construct -> load() (CPU parse) -> upload() (GPU upload, needs a GL context)
//  -> render() -> destruction frees GPU resources.
// ============================================================================

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "car_model.h"
#include "renderer.h"

class ObjModel {
public:
    ObjModel() = default;
    ~ObjModel() { destroy(); }

    ObjModel(const ObjModel&) = delete;
    ObjModel& operator=(const ObjModel&) = delete;

    // Parse the .obj file (and the .mtl it references). Faces belonging to any
    // material named in excludeMaterials are skipped (useful to drop terrain
    // groups such as "ground" / "walk" and keep the procedural street).
    // Returns false if the file cannot be opened or no geometry was produced.
    bool load(const std::string& objPath,
              const std::vector<std::string>& excludeMaterials = {});

    // Upload every parsed group to the GPU. Requires an active OpenGL context
    // and a fully initialised Renderer (the basic shader program must exist).
    void upload();

    // Draw all groups with the supplied model transform. Each group is drawn
    // with its own diffuse colour read from the material library. Requires an
    // initialised Renderer (the basic shader program must be bound).
    void render(Renderer& renderer, const glm::mat4& model) const;

    void destroy();

    bool loaded() const { return loaded_; }
    bool uploaded() const { return uploaded_; }
    glm::vec3 boundingBoxMin() const { return bboxMin_; }
    glm::vec3 boundingBoxMax() const { return bboxMax_; }
    glm::vec3 boundingBoxSize() const { return bboxMax_ - bboxMin_; }
    std::size_t groupCount() const { return groups_.size(); }

private:
    struct Material {
        glm::vec3 diffuse{0.8f, 0.8f, 0.8f};
        bool defined = false;
    };

    struct Group {
        car::Mesh mesh;
        GpuMesh gpu;
        glm::vec3 color{0.8f, 0.8f, 0.8f};
        bool hasColor = false;

        Group() = default;
        Group(Group&&) = default;
        Group& operator=(Group&&) = default;
        Group(const Group&) = delete;
        Group& operator=(const Group&) = delete;
    };

    bool loadMaterials(const std::string& mtlPath);
    void beginGroup(const std::string& materialName);
    void addTriangle(unsigned i0, unsigned i1, unsigned i2);
    void computeNormals();
    bool isExcluded(const std::string& materialName) const;

    // Raw per-vertex positions (OBJ indices are 1-based into this vector).
    std::vector<glm::vec3> positions_;
    // Accumulated per-vertex normals (smooth shading), indexed like positions_.
    std::vector<glm::vec3> vertexNormals_;
    // Material name -> diffuse colour.
    std::unordered_map<std::string, Material> materials_;
    // Material names whose faces are dropped during parsing.
    std::vector<std::string> excludeMaterials_;
    // One mesh per material group.
    std::vector<std::unique_ptr<Group>> groups_;
    // Index of the group currently receiving faces (-1 when none active).
    int currentGroupIndex_ = -1;
    // True while the active material is in the exclude list.
    bool currentExcluded_ = false;

    glm::vec3 bboxMin_{0.0f};
    glm::vec3 bboxMax_{0.0f};
    bool bboxValid_ = false;

    bool loaded_ = false;
    bool uploaded_ = false;
};

inline bool ObjModel::loadMaterials(const std::string& mtlPath) {
    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        std::fprintf(stderr, "ObjModel: could not open material library '%s'\n",
                     mtlPath.c_str());
        return false;
    }

    std::string line;
    std::string currentName;
    while (std::getline(file, line)) {
        // Strip a trailing carriage return (Windows line endings).
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::istringstream ss(line);
        std::string token;
        if (!(ss >> token)) continue;

        if (token == "newmtl") {
            ss >> currentName;
            materials_[currentName] = Material{};
        } else if (token == "Kd" && !currentName.empty()) {
            float r = 0.8f, g = 0.8f, b = 0.8f;
            ss >> r >> g >> b;
            materials_[currentName].diffuse = glm::vec3(r, g, b);
            materials_[currentName].defined = true;
        }
        // Ka, Ks, Ns, d and texture maps are intentionally ignored: the basic
        // renderer only needs a single diffuse colour per group.
    }
    return true;
}

inline bool ObjModel::isExcluded(const std::string& materialName) const {
    for (const auto& name : excludeMaterials_) {
        if (name == materialName) return true;
    }
    return false;
}

inline void ObjModel::beginGroup(const std::string& materialName) {
    currentExcluded_ = isExcluded(materialName);
    if (currentExcluded_) {
        currentGroupIndex_ = -1;
        return;
    }
    auto group = std::make_unique<Group>();
    auto it = materials_.find(materialName);
    if (it != materials_.end() && it->second.defined) {
        group->color = it->second.diffuse;
        group->hasColor = true;
    }
    groups_.push_back(std::move(group));
    currentGroupIndex_ = static_cast<int>(groups_.size()) - 1;
}

inline void ObjModel::addTriangle(unsigned i0, unsigned i1, unsigned i2) {
    if (currentExcluded_) return;
    if (currentGroupIndex_ < 0) {
        // Faces declared before any usemtl get a default group.
        beginGroup("");
    }
    Group& group = *groups_[static_cast<std::size_t>(currentGroupIndex_)];
    car::Mesh& mesh = group.mesh;

    const unsigned base = static_cast<unsigned>(mesh.v.size());
    const std::array<unsigned, 3> indices = {i0, i1, i2};
    const glm::vec3& p0 = positions_[i0];
    const glm::vec3& p1 = positions_[i1];
    const glm::vec3& p2 = positions_[i2];
    const glm::vec3 faceNormal = glm::normalize(glm::cross(p1 - p0, p2 - p0));

    for (unsigned idx : indices) {
        car::Vertex v{};
        v.p = positions_[idx];
        v.n = faceNormal;
        v.uv = glm::vec2(0.0f);
        v.mat = 0.0f; // BODY channel -> renderer uses uColor for the group.
        mesh.v.push_back(v);
        // Accumulate the face normal into the shared-vertex normal for later
        // smoothing (keeps hard edges at material boundaries because each
        // group has its own vertex list).
        if (idx < vertexNormals_.size()) {
            vertexNormals_[idx] += faceNormal;
        }
    }
    mesh.i.push_back(base);
    mesh.i.push_back(base + 1);
    mesh.i.push_back(base + 2);
}

inline void ObjModel::computeNormals() {
    // Normalize the per-position accumulated normals. Groups already copied
    // face normals per vertex, so this is only useful for callers that want a
    // smoothed lookup; the mesh vertices retain flat face normals which gives
    // the stylised building a clean faceted look.
    for (auto& n : vertexNormals_) {
        const float len = glm::length(n);
        if (len > 1e-9f) n /= len;
    }
}

inline bool ObjModel::load(const std::string& objPath,
                           const std::vector<std::string>& excludeMaterials) {
    destroy();
    excludeMaterials_ = excludeMaterials;

    std::ifstream file(objPath);
    if (!file.is_open()) {
        std::fprintf(stderr, "ObjModel: could not open '%s'\n", objPath.c_str());
        return false;
    }

    // Resolve the .mtl path relative to the .obj directory.
    const std::size_t slash = objPath.find_last_of("/\\");
    const std::string directory =
        (slash == std::string::npos) ? std::string() : objPath.substr(0, slash + 1);

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::istringstream ss(line);
        std::string token;
        if (!(ss >> token)) continue;

        if (token == "mtllib") {
            std::string mtlName;
            ss >> mtlName;
            loadMaterials(directory + mtlName);
        } else if (token == "v") {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            ss >> x >> y >> z;
            const glm::vec3 p(x, y, z);
            positions_.push_back(p);
            vertexNormals_.emplace_back(0.0f);

        } else if (token == "usemtl") {
            std::string materialName;
            ss >> materialName;
            beginGroup(materialName);
        } else if (token == "f") {
            // Collect vertex indices. Supports "v", "v/vt", "v//vn" and
            // "v/vt/vn". Negative (relative) indices are handled as well.
            std::vector<unsigned> faceIndices;
            std::string field;
            while (ss >> field) {
                const std::size_t firstSlash = field.find('/');
                const std::string vStr =
                    (firstSlash == std::string::npos) ? field : field.substr(0, firstSlash);
                int vIdx = std::stoi(vStr);
                if (vIdx < 0) {
                    vIdx += static_cast<int>(positions_.size()) + 1;
                }
                faceIndices.push_back(static_cast<unsigned>(vIdx - 1));
            }
            // Fan-triangulate polygons with more than three vertices.
            for (std::size_t k = 1; k + 1 < faceIndices.size(); ++k) {
                addTriangle(faceIndices[0], faceIndices[k], faceIndices[k + 1]);
            }
        }
        // 'g', 'o', 'vt', 'vn', 's', 'l' and comments are intentionally ignored:
        // geometry is grouped by material, which is all the renderer needs.
    }

    computeNormals();

    loaded_ = !groups_.empty();
    if (!loaded_) {
        std::fprintf(stderr, "ObjModel: no geometry parsed from '%s'\n",
                     objPath.c_str());
    }
    return loaded_;
}

inline void ObjModel::upload() {
    if (!loaded_) return;
    for (auto& group : groups_) {
        group->gpu.upload(group->mesh);
    }
    uploaded_ = true;
}

inline void ObjModel::render(Renderer& renderer, const glm::mat4& model) const {
    if (!uploaded_) return;
    for (const auto& group : groups_) {
        if (group->gpu.indexCount() == 0) continue;
        // The basic shader multiplies the per-group diffuse colour by the
        // lighting term. Groups without an explicit Kd fall back to mid-grey.
        const glm::vec3 color = group->hasColor ? group->color : glm::vec3(0.7f);
        renderer.draw(group->gpu, model, color);
    }
}

inline void ObjModel::destroy() {
    for (auto& group : groups_) {
        group->gpu.destroy();
    }
    groups_.clear();
    positions_.clear();
    vertexNormals_.clear();
    materials_.clear();
    excludeMaterials_.clear();
    currentGroupIndex_ = -1;
    currentExcluded_ = false;
    bboxMin_ = glm::vec3(0.0f);
    bboxMax_ = glm::vec3(0.0f);
    bboxValid_ = false;
    loaded_ = false;
    uploaded_ = false;
}
