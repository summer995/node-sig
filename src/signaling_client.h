// Copyright (c) 2017-2017 Agora.io, Inc.
// a module that communition with Lbes and Lbec with Singnaling

#pragma once  // NOLINT(build/header_guard)

#include "agora_sig.h"

#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <nan.h>
#include <node.h>
#include <node_object_wrap.h>

#include "block_queue.h"

typedef std::function<void()> SignalingEventCallback;

class SignalingClient : public Nan::ObjectWrap,
                        public agora_sdk_cpp::ICallBack {
 public:
  static void Init(v8::Local<v8::Object> exports);
  static void New(const Nan::FunctionCallbackInfo<v8::Value> &info);
  static void StartSignaling(const Nan::FunctionCallbackInfo<v8::Value> &args);

  static void SendInstantMessage(
      const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void SendBCCall(const Nan::FunctionCallbackInfo<v8::Value> &args);

  static void SetLoginSuccessCallback(
      const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void SetLoginFailCallback(
      const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void SetLogCallback(const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void SetLogoutCallback(
      const Nan::FunctionCallbackInfo<v8::Value> &args);

  static void SetInstantMessageCallback(
      const Nan::FunctionCallbackInfo<v8::Value> &args);
  static void SetBCCallback(const Nan::FunctionCallbackInfo<v8::Value> &args);

 private:
  SignalingClient(const std::string &account, bool is_test_env);
  virtual ~SignalingClient();

  bool InitSignaling();
  void BCCall(const std::string &func, const std::string &call_id,
              const std::string &join_str);
  void InstantMessage(const std::string &account, const std::string &msg);

 public:
  /// Signaling callbacks
  virtual void onLoginSuccess(uint32_t uid, int fd);
  virtual void onLogout(int ecode);
  virtual void onLoginFailed(int ecode);
  virtual void onLog(char const *txt, size_t txt_size);
  // LBEC call or user call
  virtual void onMessageInstantReceive(char const *account, size_t account_size,
                                       uint32_t uid, char const *msg,
                                       size_t msg_size);

  // Result from LBEC
  virtual void onBCCall_result(char const *reason, size_t reason_size,
                               char const *json_ret, size_t json_ret_size,
                               char const *callID, size_t callID_size);

 private:
  void RunSignalingThread();

  std::string GenerateToken(const std::string &version,
                            const std::string &app_id,
                            const std::string &signkey,
                            const std::string &account, uint32_t expired_ts);

  void LoginSignaling();

  bool CheckLoginStatus();

  bool MessageFromLbes(const std::string &account, const std::string &message);

  bool MessageFromLbec(const std::string &message);

 public:
  BlockQueue<SignalingEventCallback> queue_;

 private:
  uint32_t login_status_;
  std::thread signaling_thread_;
  agora_sdk_cpp::IAgoraAPI *signaling_;
  std::string account_;
  bool is_test_env_;
};
