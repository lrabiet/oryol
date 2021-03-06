//------------------------------------------------------------------------------
//  IOFacade.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "IOFacade.h"
#include "IO/assignRegistry.h"
#include "Core/CoreFacade.h"

namespace Oryol {
namespace IO {

using namespace Core;
    
OryolGlobalSingletonImpl(IOFacade);

/// @todo: make numIOLanes configurable
const int32 IOFacade::numIOLanes = 4;

//------------------------------------------------------------------------------
IOFacade::IOFacade() {
    this->SingletonEnsureUnique();
    this->mainThreadId = std::this_thread::get_id();
    assignRegistry::CreateSingleton();
    schemeRegistry::CreateSingleton();
    this->requestRouter = ioRequestRouter::Create(IOFacade::numIOLanes);
    CoreFacade::Instance()->RunLoop()->Add(RunLoop::Callback("IO::IOFacade", 0, std::bind(&IOFacade::doWork, this)));
}

//------------------------------------------------------------------------------
IOFacade::~IOFacade() {
    o_assert(this->isMainThread());
    CoreFacade::Instance()->RunLoop()->Remove("IO::IOFacade");
    this->requestRouter = 0;
    schemeRegistry::DestroySingleton();
    assignRegistry::DestroySingleton();
}

//------------------------------------------------------------------------------
void
IOFacade::doWork() {
    o_assert(this->isMainThread());
    if (this->requestRouter.isValid()) {
        this->requestRouter->DoWork();
    }
}

//------------------------------------------------------------------------------
bool
IOFacade::isMainThread() {
    return std::this_thread::get_id() == this->mainThreadId;
}

//------------------------------------------------------------------------------
void
IOFacade::SetAssign(const String& assign, const String& path) {
    assignRegistry::Instance()->SetAssign(assign, path);
}

//------------------------------------------------------------------------------
bool
IOFacade::HasAssign(const String& assign) const {
    return assignRegistry::Instance()->HasAssign(assign);
}

//------------------------------------------------------------------------------
String
IOFacade::LookupAssign(const String& assign) const {
    return assignRegistry::Instance()->LookupAssign(assign);
}

//------------------------------------------------------------------------------
String
IOFacade::ResolveAssigns(const String& str) const {
    return assignRegistry::Instance()->ResolveAssigns(str);
}

//------------------------------------------------------------------------------
void
IOFacade::UnregisterFileSystem(const StringAtom& scheme) {
    schemeRegistry::Instance()->UnregisterFileSystem(scheme);
}

//------------------------------------------------------------------------------
bool
IOFacade::IsFileSystemRegistered(const StringAtom& scheme) const {
    return schemeRegistry::Instance()->IsFileSystemRegistered(scheme);
}

//------------------------------------------------------------------------------
Ptr<IOProtocol::Get>
IOFacade::LoadFile(const URL& url, int32 ioLane) {
    Ptr<IOProtocol::Get> ioReq = IOProtocol::Get::Create();
    ioReq->SetURL(url);
    ioReq->SetLane(ioLane);
    this->requestRouter->Put(ioReq);
    return ioReq;
}

//------------------------------------------------------------------------------
Ptr<IOProtocol::GetRange>
IOFacade::LoadFileRange(const URL& url, int32 startOffset, int32 endOffset, int32 ioLane) {
    Ptr<IOProtocol::GetRange> ioReq = IOProtocol::GetRange::Create();
    ioReq->SetURL(url);
    ioReq->SetLane(ioLane);
    ioReq->SetStartOffset(startOffset);
    ioReq->SetEndOffset(endOffset);
    this->requestRouter->Put(ioReq);
    return ioReq;
}

//------------------------------------------------------------------------------
void
IOFacade::AddPreloadFile(const URL& url, int32 ioLane) {
    /// @todo: implement AddPreloadFile()
}

//------------------------------------------------------------------------------
bool
IOFacade::IsPreloadingFinished() const {
    /// @todo: implement IsPreloadingFinished()
    return false;
}
    
} // namespace IO
} // namespace Oryol