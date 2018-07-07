var SignalingClient = require('./build/Release/signaling-client');

var signaling_client = new SignalingClient.SignalingClient("myaccount",true);

signaling_client.SetLoginSuccessCallback(function (uid, fd) {
  console.log("on LoginSuccess: uid: " + uid + ", fd: " + fd);
});
signaling_client.SetLoginFailCallback(function (ecode) {
  console.log("on LoginFail: " + ecode);
});
signaling_client.SetLogCallback(function (log) {
  //console.log("on Log: " + log);
});
signaling_client.SetLogoutCallback(function (ecode) {
  console.log("on Logout: " + ecode);
});
signaling_client.SetBCCallback(function (call_id, msg) {
  console.log("on BCCall: call_id: " + call_id + ", msg: " + msg);
});

signaling_client.SetInstantMessageCallback(function (account, msg) {
  console.log("on : InstantMessage: account" + account + ", msg: " + msg);
});

signaling_client.StartSignaling();

setInterval(function () {
  signaling_client.SendInstantMessage("test_account", "message");
  signaling_client.SendBCCall("JoinSignalingLbec", "test_call_id", "message");
}, 3 * 1000);
setTimeout(function () {
  console.log("timeout");
}, 1000 * 1000);

