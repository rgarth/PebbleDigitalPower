Pebble.addEventListener('showConfiguration', function(e) {
  /*var battery;
  var color;
  // Show config page
  if (localStorage.getItem('battery')) {
    battery = '&battery=' + localStorage.getItem('battery');
  } else { 
    battery = '&battery=true';
  }
  if (localStorage.getItem('color')) {
    color = localStorage.getItem('color');
  } else {
    color = "55FF00";
  }
  var URL = 'http://rgarth.github.io/PebbleDigitalPower/configuration.html?color=' + 
      color + battery;
  console.log('Configuration window opened. ' + URL);
  Pebble.openURL(URL);*/
});

Pebble.addEventListener('webviewclosed',
  function(e) {
    /*
    var configuration = JSON.parse(decodeURIComponent(e.response));
    var dictionary = {
      "KEY_BATTERY": configuration.battery,
      "KEY_COLOR": configuration.color
    };
    localStorage.setItem('battery', configuration.battery);
    localStorage.setItem('color', configuration.color);
    // Send to Pebble
    Pebble.sendAppMessage(dictionary,
      function(e) {
        console.log("Configuration sent to Pebble successfully!");
      },
      function(e) {
        console.log("Error sending configuration info to Pebble!");
      }
    );
  */}
); 
