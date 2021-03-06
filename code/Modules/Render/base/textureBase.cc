//------------------------------------------------------------------------------
//  textureBase.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "textureBase.h"

namespace Oryol {
namespace Render {
    
using namespace Core;
using namespace IO;

//------------------------------------------------------------------------------
textureBase::textureBase() {
    // empty
}

//------------------------------------------------------------------------------
void
textureBase::clear() {
    this->ioRequest.Invalidate();
    this->textureAttrs = TextureAttrs();
}

//------------------------------------------------------------------------------
void
textureBase::setIORequest(const Ptr<IOProtocol::Get>& req) {
    this->ioRequest = req;
}

//------------------------------------------------------------------------------
void
textureBase::setTextureAttrs(const TextureAttrs& attrs) {
    this->textureAttrs = attrs;
}

} // namespace Render
} // namespace Oryol