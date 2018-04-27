// Copyright (c) 2017-2017 Agora.io, Inc.
// a module that communition with Lbes and Lbec with Singnaling

#include "signaling_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <utility>
using namespace agora_sdk_cpp;  // NOLINT

using std::string;
using std::vector;
using std::cout;
using std::endl;

uv_async_t async;

static const std::string APP_ID("3e75eb10860d4ee4826bdae1ae7c4121");  // NOLINT
static const std::string SIGNKEY_SIGNAL(
    "2b66ded51a9541cfb5f6319053b192bd");             // NOLINT

enum LoginStatus {
  LOGIN_STATUS_NOT_LOGIN,
  LOGIN_STATUS_LOGGING,
  LOGIN_STATUS_LOGGED,
};

static Nan::Callback loginSuccessCallback_;
static Nan::Callback loginFailCallback_;
static Nan::Callback logoutCallback_;
static Nan::Callback logCallback_;
static Nan::Callback InstantMessageCallback_;
static Nan::Callback BCCallback_;
static Nan::Persistent<v8::Function> constructor;

using namespace v8;
void SignalingClient::Init(Local<Object> exports) {
  Nan::HandleScope scope;

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("SignalingClient").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "StartSignaling", StartSignaling);
  Nan::SetPrototypeMethod(tpl, "SendInstantMessage", SendInstantMessage);
  Nan::SetPrototypeMethod(tpl, "SendBCCall", SendBCCall);
  Nan::SetPrototypeMethod(tpl, "SetLoginSuccessCallback",
                          SetLoginSuccessCallback);
  Nan::SetPrototypeMethod(tpl, "SetLoginFailCallback", SetLoginFailCallback);
  Nan::SetPrototypeMethod(tpl, "SetLogoutCallback", SetLogoutCallback);
  Nan::SetPrototypeMethod(tpl, "SetLogCallback", SetLogCallback);
  Nan::SetPrototypeMethod(tpl, "SetInstantMessageCallback",
                          SetInstantMessageCallback);
  Nan::SetPrototypeMethod(tpl, "SetBCCallback", SetBCCallback);

  constructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("SignalingClient").ToLocalChecked(),
               tpl->GetFunction());
}

SignalingClient::SignalingClient(const std::string &account, bool is_test_env)
    : account_(account), is_test_env_(is_test_env) {
  login_status_ = LOGIN_STATUS_NOT_LOGIN;
  signaling_ = nullptr;
}

SignalingClient::~SignalingClient() {
  // LbecQuit();
}

void OnMessage(uv_async_t *handle) {
  //fprintf(stdout, "OnMessage notified ********\n");
  SignalingClient *sig = static_cast<SignalingClient *>(handle->data);
  if (sig == nullptr) return;
  while (sig->queue_.size() != 0) {
    SignalingEventCallback f = sig->queue_.take();
    f();
  }
}

void SignalingClient::New(const Nan::FunctionCallbackInfo<Value> &args) {
  string account;
  if (args.IsConstructCall()) {
    // Invoked as constructor: `new SignalingClient(...)` string account;
    if (args[0]->IsString()) {
      String::Utf8Value str(args[0]->ToString());
      account = (const char *)(*str);
    } else {
      account = "test_account";
    }
    bool value = args[1]->IsBoolean() ? true : args[1]->BooleanValue();
    SignalingClient *obj = new SignalingClient(account, value);
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    // Invoked as plain function `SignalingClient(...)`, turn into construct
    // call.
    const int argc = 1;
    Local<Value> argv[argc] = {args[0]};
    Local<Function> cons = Nan::New<v8::Function>(constructor);
    args.GetReturnValue().Set(cons->NewInstance(argc, argv));
  }
  uv_async_init(uv_default_loop(), &async, OnMessage);
  // uv_async_init(uv_default_loop(), &async,
  // std::bind(&SignalingClient::OnMessage, this));
}

void SignalingClient::SendInstantMessage(
    const Nan::FunctionCallbackInfo<v8::Value> &args) {
  if (!args[0]->IsString() || !args[1]->IsString()) {
    Nan::ThrowTypeError("Wrong argument type!");
    return;
  }
  String::Utf8Value account_str(args[0]->ToString());
  string account = (const char *)(*account_str);
  String::Utf8Value msg_str(args[1]->ToString());
  string msg = (const char *)(*msg_str);
  Isolate *isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  SignalingClient *obj = ObjectWrap::Unwrap<SignalingClient>(args.Holder());
  obj->InstantMessage(account, msg);
}

void SignalingClient::SendBCCall(
    const Nan::FunctionCallbackInfo<v8::Value> &args) {
  if (args.Length() != 3 || !args[0]->IsString() || !args[1]->IsString() ||
      !args[2]->IsString()) {
    Nan::ThrowTypeError("Wrong argument type!");
    return;
  }
  String::Utf8Value str1(args[0]->ToString());
  string func = (const char *)(*str1);
  String::Utf8Value str2(args[1]->ToString());
  string call_id = (const char *)(*str2);
  String::Utf8Value str3(args[2]->ToString());
  string msg = (const char *)(*str3);
  Isolate *isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  SignalingClient *obj = ObjectWrap::Unwrap<SignalingClient>(args.Holder());
  obj->BCCall(func, call_id, msg);
}

void SignalingClient::StartSignaling(
    const Nan::FunctionCallbackInfo<v8::Value> &args) {
  Isolate *isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  SignalingClient *obj = ObjectWrap::Unwrap<SignalingClient>(args.Holder());
  obj->InitSignaling();
}

void SignalingClient::SetLoginSuccessCallback(
    const Nan::FunctionCallbackInfo<v8::Value> &args) {
  if (!args[0]->IsFunction()) {
    Nan::ThrowTypeError("Wrong argument type!");
    return;
  }
  v8::Local<v8::Function> cbFunc = v8::Local<v8::Function>::Cast(args[0]);
  loginSuccessCallback_.SetFunction(cbFunc);
}
void SignalingClient::SetLoginFailCallback(
    const Nan::FunctionCallbackInfo<v8::Value> &args) {
  if (!args[0]->IsFunction()) {
    Nan::ThrowTypeError("Wrong argument type!");
    return;
  }
  v8::Local<v8::Function> cbFunc = v8::Local<v8::Function>::Cast(args[0]);
  loginFailCallback_.SetFunction(cbFunc);
}
void SignalingClient::SetLogCallback(
    const Nan::FunctionCallbackInfo<v8::Value> &args) {
  if (!args[0]->IsFunction()) {
    Nan::ThrowTypeError("Wrong argument type!");
    return;
  }
  v8::Local<v8::Function> cbFunc = v8::Local<v8::Function>::Cast(args[0]);
  logCallback_.SetFunction(cbFunc);
}
void SignalingClient::SetLogoutCallback(
    const Nan::FunctionCallbackInfo<v8::Value> &args) {
  if (!args[0]->IsFunction()) {
    Nan::ThrowTypeError("Wrong argument type!");
    return;
  }
  v8::Local<v8::Function> cbFunc = v8::Local<v8::Function>::Cast(args[0]);
  logoutCallback_.SetFunction(cbFunc);
}

void SignalingClient::SetInstantMessageCallback(
    const Nan::FunctionCallbackInfo<v8::Value> &args) {
  if (!args[0]->IsFunction()) {
    Nan::ThrowTypeError("Wrong argument type!");
    return;
  }
  v8::Local<v8::Function> cbFunc = v8::Local<v8::Function>::Cast(args[0]);
  InstantMessageCallback_.SetFunction(cbFunc);
}
void SignalingClient::SetBCCallback(
    const Nan::FunctionCallbackInfo<v8::Value> &args) {
  if (!args[0]->IsFunction()) {
    Nan::ThrowTypeError("Wrong argument type!");
    return;
  }
  v8::Local<v8::Function> cbFunc = v8::Local<v8::Function>::Cast(args[0]);
  BCCallback_.SetFunction(cbFunc);
}

bool SignalingClient::InitSignaling() {
  if (signaling_ == nullptr) {
    signaling_ = getAgoraSDKInstanceCPP();
    signaling_->callbackSet(this);

    // Set Test-Env here if run in test envrionment
    if (is_test_env_) {
      is_test_env_ = true;
      std::string lbss("lbss");
      std::string lbs_host("125.88.159.176");
      signaling_->dbg(lbss.c_str(), lbss.size(), lbs_host.c_str(),
                      lbs_host.size());
    }
    signaling_thread_ = std::thread(&SignalingClient::RunSignalingThread, this);
  } else {
    cout << "already login signaling" << endl;
    return true;
  }

  if (signaling_ == nullptr) {
    cout << "create signaling handler failed, login failed" << endl;
    return false;
  }

  LoginSignaling();
  return true;
}

void SignalingClient::onLoginSuccess(uint32_t uid, int fd) {
  cout << "login signaling success, uid: " << uid << " fd: " << fd << endl;
  login_status_ = LOGIN_STATUS_LOGGED;
  queue_.put([=]() {
    if (!loginSuccessCallback_.IsEmpty()) {
      const int argc = 2;
      Nan::HandleScope scope;
      v8::Local<v8::Value> args[argc];
      args[0] = Nan::New(uid);
      args[1] = Nan::New(fd);
      loginSuccessCallback_.Call(argc, args);
    }
  });
  async.data = this;
  uv_async_send(&async);
}

void SignalingClient::onLogout(int ecode) {
  cout << "logout signaling, error code: " << ecode << endl;
  login_status_ = LOGIN_STATUS_NOT_LOGIN;
  LoginSignaling();
  queue_.put([=]() {
    if (!logoutCallback_.IsEmpty()) {
      const int argc = 1;
      Nan::HandleScope scope;
      v8::Local<v8::Value> args[argc];
      args[0] = Nan::New(ecode);
      logoutCallback_.Call(argc, args);
    }
  });
  async.data = this;
  uv_async_send(&async);
}

void SignalingClient::onLoginFailed(int ecode) {
  cout << "login signaling failed, error code: " << ecode << endl;
  login_status_ = LOGIN_STATUS_NOT_LOGIN;
  LoginSignaling();
  queue_.put([=]() {
    if (!loginFailCallback_.IsEmpty()) {
      const int argc = 1;
      Nan::HandleScope scope;
      v8::Local<v8::Value> args[argc];
      args[0] = Nan::New(ecode);
      loginFailCallback_.Call(argc, args);
    }
  });
  async.data = this;
  uv_async_send(&async);
}

void SignalingClient::onLog(const char *txt, size_t txt_size) {
  std::string log(txt, txt_size + 1);
  queue_.put([=]() {
    if (!logCallback_.IsEmpty()) {
      const int argc = 1;
      Nan::HandleScope scope;
      v8::Local<v8::Value> args[argc];
      args[0] = Nan::New<v8::String>(log).ToLocalChecked();
      // args[0] = Local<String>(txt, txt_size).ToLocalChecked();
      logCallback_.Call(argc, args);
    }
  });
  async.data = this;
  uv_async_send(&async);
}

void SignalingClient::onMessageInstantReceive(const char *account,
                                              size_t account_size, uint32_t uid,
                                              const char *msg,
                                              size_t msg_size) {
  std::string user_type(account, account_size);
  std::string message(msg, msg_size);
  cout << "on Instant message, from account: " << user_type << ", uid: " << uid
       << ", message: " << message << endl;
  queue_.put([=]() {
    if (!InstantMessageCallback_.IsEmpty()) {
      const int argc = 2;
      Nan::HandleScope scope;
      v8::Local<v8::Value> args[argc];
      args[0] = Nan::New<v8::String>(user_type).ToLocalChecked();
      args[1] = Nan::New<v8::String>(message).ToLocalChecked();
      InstantMessageCallback_.Call(argc, args);
    }
  });
  async.data = this;
  uv_async_send(&async);
}

void SignalingClient::onBCCall_result(const char *reason, size_t reason_size,
                                      const char *json_ret,
                                      size_t json_ret_size, const char *callID,
                                      size_t callID_size) {
  std::string call_id(callID, callID_size);
  std::string ret(reason, reason_size);
  std::string bc_ret(json_ret, json_ret_size);
  cout << "on BC Call, reason: " << ret << ", from call id: " << call_id
       << ", json result: " << bc_ret << endl;
  queue_.put([=]() {
    if (!BCCallback_.IsEmpty()) {
      const int argc = 2;
      Nan::HandleScope scope;
      v8::Local<v8::Value> args[argc];
      args[0] = Nan::New<v8::String>(call_id).ToLocalChecked();
      args[1] = Nan::New<v8::String>(bc_ret).ToLocalChecked();
      BCCallback_.Call(argc, args);
    }
  });
  async.data = this;
  uv_async_send(&async);
}

void SignalingClient::RunSignalingThread() { signaling_->start(); }

std::string SignalingClient::GenerateToken(const std::string &version,
                                           const std::string &app_id,
                                           const std::string &signkey,
                                           const std::string &account,
                                           uint32_t expired_ts) {
  //  return version + ":" + app_id + ":" + std::to_string(expired_ts) + ":" +
  //         singleton<crypto>::instance()->md5(account + app_id + signkey +
  //                                            std::to_string(expired_ts));
  return "asdfadfasdf";
}

void SignalingClient::LoginSignaling() {
  if (login_status_ != LOGIN_STATUS_NOT_LOGIN) {
    cout << "already login or logging" << endl;
    return;
  }

  login_status_ = LOGIN_STATUS_LOGGING;
  std::string token =
      GenerateToken("1", APP_ID, SIGNKEY_SIGNAL, account_, 1924790400);
  signaling_->login(APP_ID.c_str(), APP_ID.length(), account_.c_str(),
                    account_.length(), token.c_str(), token.length(), 0, "", 0);
  cout << "logining in with account: " << account_ << endl;
  ;
}

bool SignalingClient::CheckLoginStatus() {
  bool ret = (signaling_ != nullptr && login_status_ == LOGIN_STATUS_LOGGED);
  if (!ret) {
    cout << "signaling not login" << endl;
  }
  return ret;
}

void SignalingClient::BCCall(const std::string &func,
                             const std::string &call_id,
                             const std::string &join_str) {
  cout << "BCCall: call_id: " << call_id << " join_str: " << join_str << endl;
  signaling_->bc_call(func.c_str(), func.length(), join_str.c_str(),
                      join_str.length(), call_id.c_str(), call_id.length());
}
void SignalingClient::InstantMessage(const std::string &account,
                                     const std::string &msg) {
  cout << "InstantMessage: " << endl;
  signaling_->messageInstantSend(account.c_str(), account.size(), 0,
                                 msg.c_str(), msg.size(), "", 0);
}
