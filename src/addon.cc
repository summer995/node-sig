#include <nan.h>
#include "signaling_client.h"

using namespace v8;

void InitAll(Local<Object> exports) {
  SignalingClient::Init(exports);
}

NODE_MODULE(addon, InitAll)
