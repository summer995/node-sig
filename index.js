var addon = require('./build/Release/skeppleton');
addon.SetMessageCallback(function(account, msg) {
  console.log(account);
  console.log(msg);
});

console.log(addon.StartThread());

var command = 0;

setInterval(function() 
{
  //addon.SendCommandToThread(command);
  //command++;
},1000);
