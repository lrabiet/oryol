//------------------------------------------------------------------------------
//  RawMeshLoader.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "RawMeshLoader.h"
#include "Render/Util/MeshBuilder.h"
#include "Render/Core/meshFactory.h"

namespace Oryol {
namespace Render {

OryolClassImpl(RawMeshLoader);

using namespace Core;
using namespace IO;

//------------------------------------------------------------------------------
bool
RawMeshLoader::Accepts(const mesh& mesh) const {
    // we don't support loading from files (yet?), so just return false here
    return false;
}

//------------------------------------------------------------------------------
void
RawMeshLoader::Load(mesh& mesh) const {
    // this loader doesn't support loading without data
    mesh.setState(Resource::State::Failed);
}

//------------------------------------------------------------------------------
bool
RawMeshLoader::Accepts(const mesh& mesh, const Ptr<Stream>& stream) const {
    o_assert(mesh.GetState() == Resource::State::Setup);

    // take a quick peek at the data to check the magic number
    if (stream->Open(OpenMode::ReadOnly)) {
        uint32 magic = 0;
        stream->Read(&magic, sizeof(magic));
        stream->Close();
        if (magic == 'ORAW') {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
void
RawMeshLoader::Load(mesh& mesh, const Ptr<Stream>& stream) const {
    o_assert(mesh.GetState() == Resource::State::Setup);
    o_assert(nullptr != this->mshFactory);
    
    // open stream and get pointer to contained data
    if (stream->Open(OpenMode::ReadOnly)) {
        const uint8* endPtr = nullptr;
        const uint8* ptr = stream->MapRead(&endPtr);
        o_assert(nullptr != ptr);
        
        // get pointers to chunks
        const MeshBuilder::Header* hdr = (const MeshBuilder::Header*) ptr;
        o_assert('ORAW' == hdr->magic);
        
        // setup vertex buffer attrs
        VertexBufferAttrs vbAttrs;
        vbAttrs.setNumVertices(hdr->numVertices);
        vbAttrs.setUsage(mesh.GetSetup().GetVertexUsage());
        VertexLayout layout;
        const MeshBuilder::HeaderVertexComponent* comp = (const MeshBuilder::HeaderVertexComponent*) &(hdr[1]);
        for (uint32 i = 0; i < hdr->numVertexComponents; i++, comp++) {
            layout.Add((VertexAttr::Code) comp->attr, (VertexFormat::Code) comp->format);
        }
        vbAttrs.setVertexLayout(layout);
        mesh.setVertexBufferAttrs(vbAttrs);
        
        // setup index buffer attrs
        IndexBufferAttrs ibAttrs;
        ibAttrs.setNumIndices(hdr->numIndices);
        ibAttrs.setIndexType((IndexType::Code)hdr->indexType);
        ibAttrs.setUsage(mesh.GetSetup().GetIndexUsage());
        mesh.setIndexBufferAttrs(ibAttrs);
        
        // setup primitive groups
        const MeshBuilder::HeaderPrimitiveGroup* prim = (const MeshBuilder::HeaderPrimitiveGroup*) comp;
        mesh.setNumPrimitiveGroups(hdr->numPrimitiveGroups);
        for (uint32 i = 0; i < hdr->numPrimitiveGroups; i++, prim++) {
            PrimitiveGroup primGroup((PrimitiveType::Code)prim->type, prim->baseElement, prim->numElements);
            mesh.setPrimitiveGroup(i, primGroup);
        }
        
        // setup vertex data
        uint8* vertices = (uint8*) prim;
        this->mshFactory->createVertexBuffer(vertices, hdr->verticesByteSize, mesh);
        
        // setup index data
        if (hdr->indexType != IndexType::None) {
            uint8* indices = vertices + hdr->verticesByteSize;
            this->mshFactory->createIndexBuffer(indices, hdr->indicesByteSize, mesh);
        }
        
        // finally setup the vertex layout
        this->mshFactory->createVertexLayout(mesh);
        
        // set mesh to valid, and return
        mesh.setState(Resource::State::Valid);
        
        stream->UnmapRead();
        stream->Close();
    }
    else {
        // this shouldn't happen
        o_error("RawMeshLoader::Load(): failed to open stream!\n");
        mesh.setState(Resource::State::Failed);
    }
}
    
} // namespace Render
} // namespace Oryol