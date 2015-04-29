RGB Pong Clock
==============

Andrew Holmes @pongclock

Inspired by, and shamelessly derived from 
    Nick's LED Projects
https://123led.wordpress.com/about/

Videos of the clock in action:

https://vine.co/v/hwML6OJrBPw

https://vine.co/v/hgKWh1KzEU0

https://vine.co/v/hgKz5V0jrFn

Adapted for Spark Core by Paul Kourany, April 2015

Spark Core from here:
https://www.spark.io/

Uses an Adafruit 16x32 RGB matrix availble from here:
http://www.adafruit.com/product/420

This microphone:
http://www.adafruit.com/products/1063

RGBPongClock
------------
This is an adaptation of Andrew Holmes (mostly complete) RGBPongClock for Arduino
using the Spark Core.

The adaptation uses a Cloud Webhook to request a 7-day forecast from api.openweathermap.org
and, using the webhook's JSON parsing feature, receives a temperature and a weather condition
ID for each of the 7 days.  Without parsing, the return payload is typically about 2KB but
with parsing done by the webhook, the return payload is less than 100 bytes!

Some functions such as birthday, Halloween and Christmas display were removed in order for the
code to fit in the Core flash.  The clock setting code was removed since the Core syncs time to
the cloud.  The Spectrum clock was modified to use fix_fft(*) an 8-bit "lite" fft library with
less RAM and code requirements.  Finally, a Spark.function() was created to allow setting of
specific clock modes since the mode will rotate at 5 minute intervals (primarily for demo purposes)

I will adding a "off Cloud" demo mode using a jumper so that the unit can work at the Maker Faire
if no WiFi is available.  The mode will preset the time and weather conditions with sample data,
bypassing the need for WiFi.

(*) Google fix_fft to get more details on this library 

Webhook
-------
In order for RGBPongClock to get weather from api.openweathermap.org without doing all the
data parsing in code, a JSON parsing webhook is used:

```
{
"event": "weather_hook",
"url": "http://api.openweathermap.org/data/2.5/forecast/daily",
"requestType": "POST",
"headers": null,
"query": {
	"q": "Ottawa,ON",
	"mode": "json",
	"units": "metric",
	"cnt": 7
	},
"responseTemplate": "{{#list}}{{temp.day}}{{#weather}}~{{id}}~{{/weather}}{{/list}}",
"json": null,
"auth": null,
"mydevices": true
}
```

The webhook file is provided (weather.json)

CONFIGURE HOOK
--------------
The RGBPongClock.ino file contains a #define HOOK_NAME that defines the "event name" of
the webhook.  The name defined after the ```hook-response/``` part must also match the
```#define HOOK_PUB``` and the event defined in the webhook.  In the example, the event
is named ```weather_hook```.

In order to get the weather for your area, you will need to change the "q" and "units"
query parameters to match your city and temperature units.  For Fahrenheit, me "units"
parameter line can be removed entirely.

The JSON parsing template is defined by ```responseTemplate``` following the Mustache
stateless query format.

DON'T FORGET to create the webhook using Spark CLI:

```  spark webhook create weather.json```

  
OTHER LIBRARIES
---------------

RGBPongClock makes use of the following libraries:
```
  RGBMatrixPanel (including SparkIntervalTimer)
  Adafruit_GFX library
  fix_fft
```
